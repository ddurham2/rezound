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

#ifndef NO_POSIX_RWLOCKS

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
			throw runtime_error(string(__func__)+" -- error creating pthread rwlock -- "+strerror(ret));
	}

	virtual ~CRWLock()
	{
		const int ret=pthread_rwlock_destroy(&rwlock);
		if(ret)
			throw runtime_error(string(__func__)+" -- error destroying pthread rwlock -- "+strerror(ret)); // may not care tho
	}

	// read lock
	void readLock()
	{
		const int ret=pthread_rwlock_rdlock(&rwlock);
		if(ret)
			throw runtime_error(string(__func__)+" -- error aquiring read lock -- "+strerror(ret));
		readLockCount++;
	}

	bool tryReadLock()
	{
		int ret=pthread_rwlock_tryrdlock(&rwlock);
		if(ret!=EBUSY && ret!=0)
			throw runtime_error(string(__func__)+" -- error doing try read lock -- "+strerror(ret));
		if(ret==0)
			readLockCount++;
		return ret==0;
	}

	size_t getReadLockCount() const
	{
		return readLockCount;
	}


	// write lock
	void writeLock()
	{
		const int ret=pthread_rwlock_wrlock(&rwlock);
		if(ret)
			throw runtime_error(string(__func__)+" -- error aquiring write lock -- "+strerror(ret));
		isWriteLocked++;
	}

	bool tryWriteLock()
	{
		const int ret=pthread_rwlock_trywrlock(&rwlock);
		if(ret!=EBUSY && ret!=0)
			throw runtime_error(string(__func__)+" -- error doing try write lock -- "+strerror(ret));
		if(ret==0)
			isWriteLocked++;
		return ret==0;
	}

	bool isLockedForWrite() const
	{
		return isWriteLocked>0;
	}


	// unlock
	void unlock()
	{
		const int ret=pthread_rwlock_unlock(&rwlock);
		if(ret)
			throw runtime_error(string(__func__)+" -- error unlocking -- "+strerror(ret));
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

#else // NEED TO IMPLEMENT OUR OWN RWLOCK since posix rwlocks are not available


#include "CMutex.h"
#include <stdio.h>
#include <execinfo.h>

/*
 * the configure script is ready to allow libPTL (on netbsd) as long as it contains rwlocks
 * in which cause wouldn't need to implement my own class.  I tried implementing my own class
 * which worked a little bit, but was fairly unreliable.  I'm not sure if my used of rwlocks is
 * invalid or if my implementation is.  But somehow a writelock seems to hang around starving all
 * readers.   But for now I would just like to get 0.9beta out the door and not worry about the
 * netbsd platform (which probably almost no one would be using to run ReZound)
 */
#error Need a thread library with rwlocks (or implement a proper rwlock class here)

#if 0 // doesn't work for some reason 

/* badly starves writers */
class CRWLock
{
public:
	CRWLock() :
		activeReaders(0),
		writeLocks(0)
	{
	}

	virtual ~CRWLock()
	{
	}

	// read lock
	void readLock()
	{
		print_backtrace("readLock");
		readerMutex.lock();
		if(activeReaders==0)
			writerMutex.lock();
		activeReaders++;
		readerMutex.unlock();
	}

	bool tryReadLock()
	{
		/*
		// same as readLock() but uses trylock() everywhere that readLock() uses lock()
		if(readerMutex.trylock())
		{
			if(activeReaders==0)
			{
				if(writerMutex.trylock())
				{
					readerMutex.unlock();
					return true;
				}
			}
			activeReaders++;
			readerMutex.unlock();
		}
		return false;
		*/
		print_backtrace("tryReadLock");
		readLock();
		return true;
	}

	size_t getReadLockCount() const
	{
		return activeReaders;
	}


	// write lock
	void writeLock()
	{
		print_backtrace("writeLock");
		writerMutex.lock();
		writeLocks++;
	}

	bool tryWriteLock()
	{
		print_backtrace("tryWriteLock");
		//return writerMutex.trylock();
		writeLock();
		return true;
	}

	bool isLockedForWrite() const
	{
		return writerMutex.isLocked() && activeReaders<=0;
	}

#if ENABLE_JACK
	#warning NOT JACK SAFE WITHOUT pthread_rwlocks
#endif

	// unlock (try lock doesn't work well because unlocking from a trylock can wait for blocked readers!)
	void unlock()
	{
		print_backtrace("unlock");
		if(isLockedForWrite())
			writeUnlock();
		else if(getReadLockCount()>0)
			readUnlock();
	}

private:
	CMutex readerMutex,writerMutex;
	int activeReaders;
	int writeLocks;

	void readUnlock()
	{
		print_backtrace("readUnlock");
		readerMutex.lock();
		if(activeReaders>0)
		{
			activeReaders--;
			if(activeReaders<=0)
				writerMutex.unlock();
		}
		readerMutex.unlock();
	}

	void writeUnlock()
	{
		print_backtrace("writeUnlock");
		writeLocks--;
		writerMutex.unlock();
	}

	void print_backtrace(const char *s)
	{
		void *array[25];
		char **strings;
		int size=backtrace(array,25);
		strings=backtrace_symbols(array,size);
		printf("%s ===================================================\n",s);
		printf("readLocks: %d writeLocks: %d\n",activeReaders,writeLocks);
		for(int t=0;t<size;t++)
		{
			for(int k=0;k<t;k++)
				printf(" ");
			printf("%s\n",strings[t]);
		}
		fflush(stdout);
		free(strings);
	}
};
#endif

/* my sorry attempt at a quick-and-dirty implementation (didn't work correctly)
class CRWLock
{
public:
	CRWLock() :
		activeReaders(0),
		activeWriters(0)
	{
	}

	virtual ~CRWLock()
	{
	}

	// read lock
	void readLock()
	{
		mutexThatMakesReadersWaitOnWriters.lock();
		mutex.lock();
		if(activeReaders<=0)
			writerMutex.lock();
		activeReaders++;
		mutex.unlock();
		mutexThatMakesReadersWaitOnWriters.unlock();
	}

	bool tryReadLock()
	{
		// same as readLock() but uses trylock() everywhere that readLock() uses lock()
		if(mutexThatMakesReadersWaitOnWriters.trylock())
		{
			if(mutex.trylock())
			{
				if(activeReaders<=0)
				{
					if(writerMutex.trylock())
					{
						activeReaders++;
						mutex.unlock();
						mutexThatMakesReadersWaitOnWriters.unlock();
						return true;
					}
					else
					{
						mutex.unlock();
						mutexThatMakesReadersWaitOnWriters.unlock();
					}
				}
			}
			else
				mutexThatMakesReadersWaitOnWriters.unlock();
		}
		return false;
	}

	size_t getReadLockCount() const
	{
		return activeReaders;
	}


	// write lock
	void writeLock()
	{
		mutexThatMakesReadersWaitOnWriters.lock();
		writerMutex.lock();
		activeWriters++;
		mutexThatMakesReadersWaitOnWriters.unlock();
	}

	bool tryWriteLock()
	{
		if(mutexThatMakesReadersWaitOnWriters.trylock())
		{
			if(writerMutex.trylock())
			{
				activeWriters++;
				mutexThatMakesReadersWaitOnWriters.unlock();
				return true;
			}
			else
				mutexThatMakesReadersWaitOnWriters.unlock();
		}
		return false;
	}

	bool isLockedForWrite() const
	{
		return activeWriters>0;
	}


	// unlock
	void unlock()
	{
		mutex.lock();
		if(isLockedForWrite())
			writeUnlock();
		else if(getReadLockCount()>0)
			readUnlock();
		mutex.unlock();
	}

private:
	CMutex mutex,writerMutex,mutexThatMakesReadersWaitOnWriters;
	int activeReaders;
	int activeWriters;

	void readUnlock()
	{
		//mutex.lock();
		if(activeReaders>0)
		{
			activeReaders--;
			if(activeReaders<=0)
				writerMutex.unlock();
		}
		//mutex.unlock();
	}

	void writeUnlock()
	{
		//mutex.lock();
		if(activeWriters>0)
		{
			activeWriters--;
			writerMutex.unlock();
		}
		//mutex.unlock();
	}


};
*/

#if 0 // had problems working correctly
// Implementation based on: http://www.cs.kent.edu/~walker/classes/os.f00/lectures/L15.pdf
class CRWLock
{
public:
	CRWLock() :
		waitingReaders(0),
		activeReaders(0),
		waitingWriters(0),
		activeWriters(0)
	{
	}

	virtual ~CRWLock()
	{
		mutex.lock();
		okToRead.unlock();
		okToWrite.unlock();
		mutex.unlock();
	}

	// read lock
	void readLock()
	{
		mutex.lock();
		if((activeWriters+waitingWriters)>0)
			waitingWriters++;
		else 
		{
			okToRead.unlock();
			activeReaders++;
		}
		mutex.unlock();
		okToRead.lock();
	}

	bool tryReadLock()
	{
		/*??? bogus race-condition implementation */
		if(activeWriters<=0)
		{
			readLock();
			return true;
		}
		else
			return false;
	}

	size_t getReadLockCount() const
	{
		return activeReaders;
	}


	// write lock
	void writeLock()
	{
		mutex.lock();
		if(activeWriters+activeReaders>0)
			waitingWriters++;
		else
		{
			okToWrite.unlock();
			activeWriters++;
		}
		mutex.unlock();
		okToWrite.lock();
	}

	bool tryWriteLock()
	{
		/*??? bogus race-condition implementation */
		if(activeReaders<=0)
		{
			writeLock();
			return true;
		}
		else
			return false;
	}

	bool isLockedForWrite() const
	{
		return activeWriters>0;
	}


	// unlock
	void unlock()
	{
		mutex.lock();
		if(isLockedForWrite())
			writeUnlock();
		else if(getReadLockCount()>0)
			readUnlock();
		mutex.unlock();
	}

private:
	CMutex mutex,okToRead,okToWrite;
	int waitingReaders;	// waiting readers
	int activeReaders;	// active readers
	int waitingWriters;	// waiting writers
	int activeWriters; 	// active writers

	void readUnlock()
	{
		//mutex.lock();
		if(activeReaders>0)
		{
			activeReaders--;
			if(activeReaders==0 && waitingWriters>0)
			{
				okToWrite.unlock();
				activeWriters++;
				waitingWriters--;
			}
		}
		//mutex.unlock();
	}

	void writeUnlock()
	{
		//mutex.lock();
		if(activeWriters>0)
		{
			activeWriters--;
			if(waitingWriters>0)
			{
				okToWrite.unlock();
				activeWriters++;
				waitingWriters--;
			}
			else if(waitingWriters>0)
			{
				okToRead.unlock();
				activeReaders++;
				waitingWriters--;
			}
		}
		//mutex.unlock();
	}

};
#endif

#endif // NO_POSIX_RWLOCKS


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

