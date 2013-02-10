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
#ifndef __CConditionVariable_H__
#define __CConditionVariable_H__

#include "../../config/common.h"

/*
 * This is a multi-platform condition variable wrapper.
 *
 * Its public interface looks like:
 
	class CConditionVariable
	{
	public:
		CConditionVariable();
		virtual ~CConditionVariable() throw();

		// returns false if it timed out, else true
		// NOTE: if timeout is >= 0, and wait() returns false, there's no guarantee that the condition-predicate is false, so it may need to be checked
		bool wait(CMutex &mutex,int timeout=-1);

		void signal();		// slightly more efficient if there is only going to be a single waiter
		void broadcast();	// use if there are multiple waiters
	};

 
	---------------------------------------------------------------------------

	Example use:

	CMutex m;
	CConditionVariable c;

	// --- code that waits for condition to be true -----

	m.lock();
	while(desired condition IS NOT true) 
		c.wait(m); // the thread will wake up every time the condition variable is signaled..

	// now condition IS true
		...
	m.unlock();

	...


	// --- code that causes condition to be true ------

	m.lock();
	... something that causes condition to be true ...
	c.broadcast(); (or c.signal())
	m.unlock();

	...



	Now, if it can be known that only one thread will ever 
	be waiting on the condition, c.signal() can be used 
	to be more efficient.  In doubt, use c.broadcast()
	broadcast() wakes up all the threads waiting

	Also, a waiting thread is not supposed to wake up until
	a signaler *unlocks* the mutex.  Hence signal() can
	be called before or after the condition is actually true
	as long as it's done while the mutex is locked.
*/

#include "CMutex.h"

#if defined(_WIN32)
	// *** WIN32 implementation ***

	// doing this to avoid including windows.h
	//   must use this below because CRITICAL_SECTION is a typedef and it causes internal compiler errors
	struct _RTL_CRITICAL_SECTION;

	class CConditionVariable
	{
	public:

		CConditionVariable();	
		~CConditionVariable() throw();

		// returns false if it timed out, else true
		// NOTE: if timeout is >= 0, and wait() returns false, there's no guarantee that the condition-predicate is false, so it may need to be checked
		bool wait(CMutex &mutex,int timeout_ms=-1);

		void signal();

		void broadcast();

	private:

		enum {
			SIGNAL = 0,
			BROADCAST = 1,
			MAX_EVENTS = 2
		};

		// Count of the number of waiters.
		unsigned waiters_count_;

		// Serialize access to <waiters_count_>.
		_RTL_CRITICAL_SECTION *waiters_count_lock_;

		// Signal and broadcast event HANDLEs.
		void* events_[MAX_EVENTS]; // ??? would make this HANDLE, but really don't think I need to include a huge windows.h header file for a void * typedef

	};

#else
	// *** posix implementation ***

	#include <pthread.h>
	
	class CConditionVariable
	{
	public:

		CConditionVariable();
		virtual ~CConditionVariable() throw();

		// returns false if it timed out, else true
		// NOTE: if timeout is >= 0, and wait() returns false, there's no guarantee that the condition-predicate is false, so it may need to be checked
		bool wait(CMutex &mutex,int timeout=-1);

		void signal();

		void broadcast();

	private:
		pthread_cond_t cond;

	};

#endif

#endif
