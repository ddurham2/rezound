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
#include "CRWMutex.h"

#include <stdexcept>
#include <string>
#include <stdio.h>
using namespace std;

#ifdef _WIN32
	// *** WIN32 implementation ***
	// based on code in the PTypes library: http://www.melikyan.com/ptypes/

	#include <windows.h>

	#ifndef __func__
	#define __func__ __FUNCTION__
	#endif

	static inline int __fastcall pincrement(int *i)
	{
		 return InterlockedIncrement((LPLONG)i);
	}

	static inline int __fastcall pdecrement(int *i)
	{
		 return InterlockedDecrement((LPLONG)i);
	}

	CRWMutex::CRWMutex() :
		readcnt(-1),
		writecnt(0)
	{
		reading = CreateEvent(0, TRUE, FALSE, 0);
		if(reading==0)
			throw runtime_error(string(__func__)+" -- error creating reading event");

		finished = CreateEvent(0, FALSE, TRUE, 0);
		if(finished==0)
			throw runtime_error(string(__func__)+" -- error creating finished event");
	}

	CRWMutex::~CRWMutex() throw()
	{
		CloseHandle(reading);
		CloseHandle(finished);
	}

	// read lock
	void CRWMutex::readLock()
	{
		if (pincrement(&readcnt) == 0) 
		{ 
			WaitForSingleObject(finished, INFINITE);
			SetEvent(reading);
		}
		WaitForSingleObject(reading, INFINITE);
	}

	bool CRWMutex::tryReadLock()
	{
		throw runtime_error(string(__func__)+" -- unimplemented");
	/*
		// this implementation is based on readLock()'s but with 0 length timeouts

		While Implementating this, I was unsure of correctness..
		It basically does zero length timeouts on wait functions and undoes whatever has been done if the wait failes
		What I'm unsure of is in the inner-most case, it does a ResetEvent(reading) to undo the SetEvent it did above
		but since there's interdependancies on the signaled-state of 'reading' with other methods, I wasn't sure if that's
		proper to do without a better understanding of how the entire implementation is supposed to work.  I initially 
		started adding comments to explain how it worked, but got hung up on eactly how readLock() work in relation to 
		the unlock method.. a closer look might reveal the details, but I need to move on porting.. expecting everything
		else to work since it's based on 3rd party code.

		if (pincrement(&readcnt) == 0) 
		{ 
			if(WaitForSingleObject(finished, 0)==0)
			{
				SetEvent(reading);
				
				if(WaitForSingleObject(reading, 0)==0)
					return true;
				else
				{
					pdecrement(&readcnt)
					ResetEvent(reading);
					return false;
				}

			}
			else
				pdecrement(&readcnt);
		}
		else
		{
			if(WaitForSingleObject(reading, 0)==0)
				return true;
			else
				pdecrement(&readcnt);
		}

		return false;
	*/
	}


	void CRWMutex::writeLock()
	{
		wrMutex.lock();
		if (writecnt == 0) 
			WaitForSingleObject(finished, INFINITE);

		writecnt++;
	}

	bool CRWMutex::tryWriteLock()
	{
		// this implementation is based on writeLock()'s but with 0 length timeouts
		if(wrMutex.trylock())
		{
			if(WaitForSingleObject(finished, 0)==0)
			{
				writecnt++;
				return true;
			}
			else
				wrMutex.unlock();
		}
		return false;
	}

	void CRWMutex::unlock()
	{
		if (writecnt != 0) 
		{
			writecnt--;
			if (writecnt == 0) SetEvent(finished);
			wrMutex.unlock();
		} 
		else if (pdecrement(&readcnt) < 0) 
		{
			ResetEvent(reading);
			SetEvent(finished);
		} 
	}



#else
	// *** posix implementation ***

	#include <string.h>	// for strerror()
	#include <errno.h>	// for EBUSY
	#include "AThread.h"

	CRWMutex::CRWMutex()
		: writeLockOwner(NULL)
	{
		const int ret=pthread_rwlock_init(&rwlock,NULL);
		if(ret)
			throw runtime_error(string(__func__)+" -- error creating pthread rwlock -- "+strerror(ret));
	}

	CRWMutex::~CRWMutex() throw()
	{
		const int ret=pthread_rwlock_destroy(&rwlock);
		if(ret)
			//throw runtime_error(string(__func__)+" -- error destroying pthread rwlock -- "+strerror(ret)); // may not care tho
			fprintf(stderr, "%s -- error destroying pthread rwlock -- %s\n",__func__,strerror(ret));
	}

	void CRWMutex::readLock()
	{
		const int ret=pthread_rwlock_rdlock(&rwlock);
		if(ret)
			throw runtime_error(string(__func__)+" -- error aquiring read lock -- "+strerror(ret));
		readLockCount.inc();
	}

	bool CRWMutex::tryReadLock()
	{
		int ret=pthread_rwlock_tryrdlock(&rwlock);
		if(ret==EBUSY)
			return false;
		if(ret!=0)
			throw runtime_error(string(__func__)+" -- error doing try read lock -- "+strerror(ret));

		readLockCount.inc();
		return true;
	}

	void CRWMutex::writeLock()
	{
		// handle recursive write-locks (this check does not contain race conditions because they're true iff we already have an exclusive lock)
		if(writeLockOwner==AThread::getCurrentThreadID())
		{
			writeLockCount.inc();
			return;
		}

		const int ret=pthread_rwlock_wrlock(&rwlock);
		if(ret)
			throw runtime_error(string(__func__)+" -- error aquiring write lock -- "+strerror(ret));
		writeLockOwner=AThread::getCurrentThreadID();
		writeLockCount.inc(); // = 1
	}

	bool CRWMutex::tryWriteLock()
	{
		// handle recursive write-locks (this check does not contain race conditions because they're true iff we already have an exclusive lock)
		if(writeLockOwner==AThread::getCurrentThreadID())
		{
			writeLockCount.inc();
			return true;
		}

		const int ret=pthread_rwlock_trywrlock(&rwlock);
		if(ret==EBUSY)
			return false;

		if(ret!=0)
			throw runtime_error(string(__func__)+" -- error doing try write lock -- "+strerror(ret));

		writeLockOwner=AThread::getCurrentThreadID();
		writeLockCount.inc(); // = 1
		return true;
	}

	void CRWMutex::unlock()
	{
		// handle recursive write-locks (this check of flags does not contain race conditions because they're true iff we already have an exclusive lock)
		if(writeLockOwner==AThread::getCurrentThreadID())
		{ // we currently have the write lock
			writeLockCount.dec();
			if(writeLockCount.value()<=0) {
				writeLockOwner=0;
				const int ret=pthread_rwlock_unlock(&rwlock);
				if(ret)
					throw runtime_error(string(__func__)+" -- error unlocking -- "+strerror(ret));
			}
		}
		else
		{ // apparently we have a read lock
			const int ret=pthread_rwlock_unlock(&rwlock);
			if(ret)
				throw runtime_error(string(__func__)+" -- error unlocking -- "+strerror(ret));
			readLockCount.dec();
		}
	}


#endif


// construct a locker object with no mutex assigned
CRWMutexLocker::CRWMutexLocker() :
	m(NULL),
	locked(false)
{
}

// construct a locker object with the given mutex.. the mutex is locked/try-locked on construction
CRWMutexLocker::CRWMutexLocker(CRWMutex &_m,LockTypes lockType,bool block) :
	m(NULL),
	locked(false)
{
	reassign(_m,lockType,block);
}

// the destructor unlocks the mutex iff it was locked
CRWMutexLocker::~CRWMutexLocker() throw()
{
	try
	{
		unassign();
	}
	catch(exception &e)
	{
		fprintf(stderr, "%s -- exception -- %s\n",__func__,e.what());
	}
}

// unlocks the currently assigned mutex if it was locked, assigns a new mutex to this locked and locks/try-locks the new mutex
// the locked status is returned
bool CRWMutexLocker::reassign(CRWMutex &_m,LockTypes lockType,bool block)
{
	unassign();

	m=&_m;
	if(block)
	{
		if(lockType==ltReader)
			m->readLock();
		else // if(lockType==ltWriter)
			m->writeLock();
		locked=true;
		return true;
	}
	else
	{
// TODO: We can't use tryReadLock until it is implemented for Win32
//		if(lockType==ltReader)
//			return locked=m->tryReadLock();
//		else // if(lockType==ltWriter)
			return locked=m->tryWriteLock();
	}
}

// unlocks the currently assigned mutex if it was locked and leaves the locker object with no assigned mutex
void CRWMutexLocker::unassign()
{
	if(m && locked)
	{
		m->unlock();
		locked=false;
	}
	m=NULL;
}

