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

#ifndef __CMutex_H__
#define __CMutex_H__

#include "../../config/common.h"

/*
 * This is a quick and dirty mutex wrapper. See AThread.h for more details
 */

#include <pthread.h>

#include <errno.h>	// for EBUSY
#include <string.h>	// for strerror()

#include <stdexcept>
#include <string>

class CMutex
{
public:

	CMutex() :
		locked(0)
	{
		pthread_mutex_init(&mutex,NULL);
	}

	virtual ~CMutex()
	{
		const int ret=pthread_mutex_destroy(&mutex);
		if(ret)
			throw std::runtime_error(std::string(__func__)+" -- error destroying mutex -- "+strerror(ret)); // may not care tho
	}

	void lock()
	{
		const int ret=pthread_mutex_lock(&mutex);
		if(ret)
			throw std::runtime_error(std::string(__func__)+" -- error aquiring lock -- "+strerror(ret));
		locked++;
	}

	bool trylock() // returns true if the lock was successful else false
	{
		const int ret=pthread_mutex_trylock(&mutex);
		if(ret!=EBUSY && ret!=0)
			throw std::runtime_error(std::string(__func__)+" -- error doing try lock -- "+strerror(ret));
		if(ret==0)
			locked++;
		return ret==0;
	}

	void unlock()
	{
		if(locked>0)
			locked--;
		const int ret=pthread_mutex_unlock(&mutex);
		if(ret)
			throw std::runtime_error(std::string(__func__)+" -- error unlocking mutex -- "+strerror(ret));
	}

	int isLocked() const
	{
		return locked;
	}

private:
	friend class CConditionVariable;
	pthread_mutex_t mutex;
	int locked;

};

/* 
 * This class simply locks the given mutex on construct and unlocks on destruction
 * it is useful to use where a lock should be obtained, then released on return or
 * exception... when an object of this class goes out of scope, the lock will be 
 * released
 *
 * The constructor can optionally take a second parameter that says not to wait when 
 * locking, but to do a trylock.  If this method is used, then it is necessary to
 * call didLock() to see if a lock was obtained.
 */
class CMutexLocker
{
public:
	CMutexLocker(CMutex &_m,bool block=true) :
		m(_m)
	{
		if(block)
		{
			m.lock();
			locked=true;
		}
		else
		{
			locked=m.trylock();
		}

	}

	virtual ~CMutexLocker()
	{
		if(locked)
			m.unlock();
	}

	bool didLock() const
	{
		return locked;
	}

private:
	CMutex &m;
	bool locked;
};


/*
 * This class does a try-lock on the mutex at construction.  It is then necessary
 * to check the didLock() method's return value and continue if it's true else
 * do not continue into the critical section.  If a lock was obtained then it 
 * will release the lock when the object destructs.
 */
class CMutexTryLocker : private CMutexLocker
{
public:
	CMutexTryLocker(CMutex &m) :
		CMutexLocker(m,false)
	{
	}

	virtual ~CMutexTryLocker()
	{
	}

	CMutexLocker::didLock;
};


#endif

