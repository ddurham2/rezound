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
#ifndef __CRWMutex_H__
#define __CRWMutex_H__

#include "../../config/common.h"

/*
 * This is a multi-platform rwlock wrapper.
 *
 * The public interface looks like:

	class CRWMutex
	{
	public:
		CRWMutex();
		virtual ~CRWMutex() throw();

		// read-locks can be obtained recursively with other read-locks, but not with other write-locks
		void readLock();
		bool tryReadLock();
		size_t getReadLockCount() const;

		// write-locks can be obtained recursively with other write-locks, but not with other read-locks
		void writeLock();
		bool tryWriteLock();
		bool isLockedForWrite() const;

		void unlock();
	};

 *
 *
 * A utility class exists for dealing with CRWMutexes
 * The public interface looks like:

	class CRWMutexLocker
	{
	public:
		enum LockTypes
		{
			ltReader,
			ltWriter
		};

		// construct a locker object with no mutex assigned
		CRWMutexLocker();

		// construct a locker object with the given mutex.. the mutex is locked/try-locked on construction
		CRWMutexLocker(CRWMutex &_m,LockTypes lockType,bool block=true);

		// the destructor unlocks the mutex iff it was locked
		virtual ~CRWMutexLocker() throw();

		// returned whether the mutex lock was obtained by this object (sometimes false if a try-lock occurred)
		bool didLock() const;

		// unlocks the currently assigned mutex if it was locked, assigns a new mutex to this locked and locks/try-locks the new mutex
		// the locked status is returned
		bool reassign(CRWMutex &_m,LockTypes lockType,bool block=true);

		// unlocks the currently assigned mutex if it was locked and leaves the locker object with no assigned mutex
		void unassign();
	};

 */

// a skeleton stub for extenting functionality
class ARWMutex
{
public:

	ARWMutex() {}
	virtual ~ARWMutex() throw() {}

	virtual void readLock() {}
	virtual bool tryReadLock() { return false; }

	virtual void writeLock() {}
	virtual bool tryWriteLock() { return false; }

	virtual void unlock() {}
};

#ifdef _WIN32
	// *** WIN32 implementation ***
	// based on code in the PTypes library: http://www.melikyan.com/ptypes/

	#include "CMutex.h"

	class CRWMutex : public ARWMutex
	{
	public:

		CRWMutex();
		virtual ~CRWMutex() throw();

		void readLock();
		bool tryReadLock();
		size_t getReadLockCount() const { return readcnt+1; }

		void writeLock();
		bool tryWriteLock();
		bool isLockedForWrite() const { return writecnt>0; }

		void unlock();

	private:
		void *reading; // ??? would make this HANDLE, but really don't think I need to include a 30k line header file for it
		void *finished; // ??? would make this HANDLE, but really don't think I need to include a 30k line header file for it
		int readcnt;
		int writecnt;

		CMutex wrMutex;
	};



#else
	// *** posix implementation ***

	#include <pthread.h>
	#include "CAtomicCounter.h"

	class CRWMutex : public ARWMutex
	{
	public:

		CRWMutex();
		virtual ~CRWMutex() throw();

		void readLock();
		bool tryReadLock();
		size_t getReadLockCount() const { return readLockCount.value(); }

		void writeLock();
		bool tryWriteLock();
		bool isLockedForWrite() const { return writeLockCount.value()>0; }

		void unlock();

	private:
		pthread_rwlock_t rwlock;

		// even though only one write lock can be aquired, it can be locked more than once 
		// if the thread currently holding the write-lock locks again (which is allowed)
		mutable CAtomicCounter writeLockCount; 
		void *writeLockOwner;

		mutable CAtomicCounter readLockCount;

	};

#endif


/* 
 * This class simply locks the given mutex on construct and unlocks on destruction
 * it is useful to use where a lock should be obtained, then released on return or
 * exception... when an object of this class goes out of scope, the lock will be 
 * released
 *
 * The construct can optionally take a second parameter that says not to wait when 
 * locking, but to do a trylock.  If this method is used, then it is necessary to
 * call didLock() to see if a lock was obtained.
 */
// TODO: dispatcher performance testing was ~2% faster if these methods were made inline
class CRWMutexLocker
{
public:
	enum LockTypes
	{
		ltReader,
		ltWriter
	};

	// construct a locker object with no mutex assigned
	CRWMutexLocker();

	// construct a locker object with the given mutex.. the mutex is locked/try-locked on construction
	CRWMutexLocker(CRWMutex &_m,LockTypes lockType,bool block=true);

	// the destructor unlocks the mutex iff it was locked
	virtual ~CRWMutexLocker() throw();

	// returned whether the mutex lock was obtained by this object (sometimes false if a try-lock occurred)
	bool didLock() const { return locked; }

	// unlocks the currently assigned mutex if it was locked, assigns a new mutex to this locked and locks/try-locks the new mutex
	// the locked status is returned
	bool reassign(CRWMutex &_m,LockTypes lockType,bool block=true);

	// unlocks the currently assigned mutex if it was locked and leaves the locker object with no assigned mutex
	void unassign();

private:
	CRWMutex *m;
	bool locked;
};




#endif

