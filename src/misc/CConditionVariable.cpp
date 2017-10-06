/* 
 * Copyright (C) 2002 - David W. Durham
 * 
 * This file is part of ReZound, an audio editing application.
 * 
 * ReZound is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 * 
 * ReZound is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */
#include "CConditionVariable.h"

#include <stdexcept>
#include <string>
#include <stdio.h>
using namespace std;	

#if defined(__APPLE__)
#	include "clocks.h"
#endif

#ifdef _WIN32
	// *** WIN32 implementation ***

	/*
		This implementation is based on section 3.2 of http://www.cs.wustl.edu/~schmidt/win32-cv-1.html
	*/

	#include <windows.h>

	CConditionVariable::CConditionVariable() :
		waiters_count_lock_(new CRITICAL_SECTION)
	{
		waiters_count_=0;
		InitializeCriticalSection(waiters_count_lock_);

		// Create an auto-reset event.
		events_[SIGNAL] = CreateEvent (NULL,  // no security
			FALSE, // auto-reset event
			FALSE, // non-signaled initially
			NULL); // unnamed

		// Create a manual-reset event.
		events_[BROADCAST] = CreateEvent (NULL,  // no security
			TRUE,  // manual-reset
			FALSE, // non-signaled initially
			NULL); // unnamed

	}

	CConditionVariable::~CConditionVariable() throw()
	{
		CloseHandle(events_[SIGNAL]);
		CloseHandle(events_[BROADCAST]);
		DeleteCriticalSection(waiters_count_lock_);

		delete waiters_count_lock_;
	}

	bool CConditionVariable::wait(CMutex &mutex,int timeout_ms)
	{
		// Avoid race conditions.
		EnterCriticalSection (waiters_count_lock_);
		waiters_count_++;
		LeaveCriticalSection (waiters_count_lock_);

		// It's ok to release the <external_mutex> here since Win32
		// manual-reset events maintain state when used with
		// <SetEvent>.  This avoids the "lost wakeup" bug...
		//LeaveCriticalSection (external_mutex);
		mutex.unlock();

		// Wait for either event to become signaled due to <pthread_cond_signal>
		// being called or <pthread_cond_broadcast> being called.
		int result = WaitForMultipleObjects (2, events_, FALSE, (timeout_ms<0 ? INFINITE : timeout_ms) );

		EnterCriticalSection (waiters_count_lock_);
		waiters_count_--;
		int last_waiter = result == WAIT_OBJECT_0 + BROADCAST && waiters_count_ == 0;
		LeaveCriticalSection (waiters_count_lock_);

		// Some thread called <pthread_cond_broadcast>.
		if (last_waiter)
		{
			// We're the last waiter to be notified or to stop waiting, so
			// reset the manual event. 
			ResetEvent (events_[BROADCAST]); 
		}

		// Reacquire the <external_mutex>.
		//EnterCriticalSection (external_mutex, INFINITE);
		mutex.lock();

		return result!=WAIT_TIMEOUT;
	}

	void CConditionVariable::signal()
	{
		// Avoid race conditions.
		EnterCriticalSection (waiters_count_lock_);
		int have_waiters = waiters_count_ > 0;
		LeaveCriticalSection (waiters_count_lock_);

		if (have_waiters)
		SetEvent (events_[SIGNAL]);
	}

	void CConditionVariable::broadcast()
	{
		// Avoid race conditions.
		EnterCriticalSection (waiters_count_lock_);
		int have_waiters = waiters_count_ > 0;
		LeaveCriticalSection (waiters_count_lock_);

		if (have_waiters)
			SetEvent (events_[BROADCAST]);
	}

#else
	// *** posix implementation ***
	
	#include <errno.h>	// for EBUSY
	#include <string.h>	// for strerror()
	

	#ifdef __APPLE__
	#include <sys/types.h>
	#include <sys/timeb.h>
	#endif
	
	CConditionVariable::CConditionVariable()
	{
		pthread_cond_init(&cond,NULL);
	}

	CConditionVariable::~CConditionVariable() throw()
	{
		const int ret=pthread_cond_destroy(&cond);
		if(ret)
			//throw runtime_error(string(__func__)+" -- error destroying conditional variable -- "+strerror(ret)); // may not care tho
			fprintf(stderr, "%s -- error destroying conditional variable -- %s\n",__func__,strerror(ret));
	}

	/*
	void CConditionVariable::wait(CMutex &mutex)
	{
		const int ret=pthread_cond_wait(&cond,(pthread_mutex_t *)mutex.mutex);
		if(ret)
			throw runtime_error(string(__func__)+" -- error waiting -- "+strerror(ret));
	}
	*/

	bool CConditionVariable::wait(CMutex &mutex,int timeout)
	{
		if(timeout<0)
		{
			const int ret=pthread_cond_wait(&cond,(pthread_mutex_t *)mutex.mutex);
			if(ret)
				throw runtime_error(string(__func__)+" -- error waiting -- "+strerror(ret));
			return true;
		}
		else
		{
			struct timespec abs_timeout;

		#if !defined(__APPLE__)
			/* getting the abs_timeout this way is the most precise, but requires linking against librt */
			clock_gettime(CLOCK_REALTIME,&abs_timeout);
		#else
			/* getting the time this way is less precise that above, but is more compatible */
			{ 
			clocks::msec_t current=clocks::time_msec();
			abs_timeout.tv_sec=current/1000;
			abs_timeout.tv_nsec=(current%1000)*1000*1000;
			}
		#endif

			abs_timeout.tv_sec+=(timeout/1000);
			abs_timeout.tv_nsec+=(timeout%1000)*1000*1000;
			abs_timeout.tv_sec+=abs_timeout.tv_nsec/1000000000;
			abs_timeout.tv_nsec%=1000000000;
			const int ret=pthread_cond_timedwait(&cond,(pthread_mutex_t *)mutex.mutex,&abs_timeout);
			if(ret && ret!=ETIMEDOUT) /* ??? also could get interrupted with a signal ??? EINTR */
				throw runtime_error(string(__func__)+" -- error waiting -- "+strerror(ret));
			return ret==0;
		}
	}

	void CConditionVariable::signal()
	{
		const int ret=pthread_cond_signal(&cond);
		if(ret)
			throw runtime_error(string(__func__)+" -- error signaling -- "+strerror(ret));
	}

	void CConditionVariable::broadcast()
	{
		const int ret=pthread_cond_broadcast(&cond);
		if(ret)
			throw runtime_error(string(__func__)+" -- error broadcasting -- "+strerror(ret));
	}

#endif
