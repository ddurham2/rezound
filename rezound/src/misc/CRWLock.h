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

#ifndef __CRWLock_H__
#define __CRWLock_H__

#include "../../config/common.h"

/*
 * This is a quick and dirty rwlock wrapper. See AThread.h for more details
 */

#include <pthread.h>

#include <string.h>	// for strerror()
#include <errno.h>	// for EBUSY

#include <stdexcept>
#include <string>

// ??? this should be called CRWMutex (shouldn't it?)
class CRWLock
{
public:

	CRWLock() :
		isWriteLocked(0),
		readLockCount(0)
	{
		const int ret=pthread_rwlock_init(&rwlock,NULL);
		if(ret)
			throw(runtime_error(string(__func__)+" -- error creating pthread rwlock -- "+strerror(ret)));
	}

	virtual ~CRWLock()
	{
		const int ret=pthread_rwlock_destroy(&rwlock);
		if(ret)
			throw(runtime_error(string(__func__)+" -- error destroying pthread rwlock -- "+strerror(ret))); // may not care tho
	}

	// read lock
	void readLock()
	{
		const int ret=pthread_rwlock_rdlock(&rwlock);
		if(ret)
			throw(runtime_error(string(__func__)+" -- error aquiring read lock -- "+strerror(ret)));
		readLockCount++;
	}

	bool tryReadLock()
	{
		int ret=pthread_rwlock_tryrdlock(&rwlock);
		if(ret!=EBUSY && ret!=0)
			throw(runtime_error(string(__func__)+" -- error doing try read lock -- "+strerror(ret)));
		if(ret==0)
			readLockCount++;
		return(ret==0);
	}

	size_t getReadLockCount() const
	{
		return(readLockCount);
	}


	// write lock
	void writeLock()
	{
		const int ret=pthread_rwlock_wrlock(&rwlock);
		if(ret)
			throw(runtime_error(string(__func__)+" -- error aquiring write lock -- "+strerror(ret)));
		isWriteLocked++;
	}

	bool tryWriteLock()
	{
		const int ret=pthread_rwlock_trywrlock(&rwlock);
		if(ret!=EBUSY && ret!=0)
			throw(runtime_error(string(__func__)+" -- error doing try write lock -- "+strerror(ret)));
		if(ret==0)
			isWriteLocked++;
		return(ret==0);
	}

	bool isLockedForWrite() const
	{
		return(isWriteLocked>0);
	}


	// unlock
	void unlock()
	{
		const int ret=pthread_rwlock_unlock(&rwlock);
		if(ret)
			throw(runtime_error(string(__func__)+" -- error unlocking -- "+strerror(ret)));
		if(isWriteLocked>0)
			isWriteLocked--;
		else
			readLockCount--;
	}


private:
	pthread_rwlock_t rwlock;
	int isWriteLocked; // even though only one write lock can be aquired, it can be locked more than once if the thread currently holding the write-lock locks again (which is allowed)
	int readLockCount;

};


/* 
 * This class simply locks the given mutex on construct and unlocks on destruction
 * it is useful to use where a lock should be obtained, then released on return or
 * exception... when an object of this class goes out of scope, the lock will be 
 * released
 */
class CRWMutexLocker
{
public:
	enum LockTypes
	{
		ltReader,
		ltWriter
	};

	CRWMutexLocker(CRWLock &_m,const LockTypes lockType) :
		m(_m)
	{
		if(lockType==ltReader)
			m.readLock();
		else
			m.writeLock();
	}

	virtual ~CRWMutexLocker()
	{
		m.unlock();
	}

private:
	CRWLock &m;
};




#endif

