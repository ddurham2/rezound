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

//??? I should do an experiement to see how much smaller, if any, an application got by un-inlining this class and giving it a cpp file

/*
 * This is a multi-platform mutex wrapper. (pthreads and win32)
 *
 * There are no flags keeping up with the state of the mutex simply because used with a 
 * CConditionVariable the state information gets off since it unlocks the internal mutex 
 * without having a chance to change the flags
 * One way around this (if I need the flags back) would be to have CConditionVariable 
 * have it's own internal mutex and it doesn't use this class
 *
 * The class's public interface looks like:

	class CMutex
	{
	public:
		CMutex(bool recursive=true,const char *name=NULL) ;
		virtual ~CMutex() throw();

		void lock();

		// returns true if the lock was successful else false 
		bool trylock(int timeout_ms=0) ;

		void unlock();
	};

 *
 *
 * Additionally, there are 2 utility classes for dealing with CMutex objects
 * Their public interfaces look like:

	class CMutexLocker
	{
	public:
		// construct a locker object with no mutex assigned
		CMutexLocker();

		// construct a locker object with the given mutex.. the mutex is locked/try-locked on construction
		CMutexLocker(CMutex &_m,int timeout_ms=-1);

		// the destructor unlocks the mutex iff it was locked
		virtual ~CMutexLocker() throw();

		// returned whether the mutex lock was obtained by this object (sometimes false if a try-lock occurred)
		bool didLock() const;

		// unlocks the currently assigned mutex if it was locked, assigns a new mutex to this locked and locks/try-locks the new mutex
		// the locked status is returned
		bool reassign(CMutex &_m,int timeout_ms=-1);

		// unlocks the currently assigned mutex if it was locked and leaves the locker object with no assigned mutex
		void unassign();
	};

	// It SIMPLY unlock()s on ctor and lock()s on dtor
	class CMutexUnlocker
	{
	public:
		CMutexUnlocker(CMutex &_m);
		~CMutexUnlocker() throw();
	};

 */

class AMutex
{
public:
	AMutex(){}
	virtual ~AMutex() throw() {}

	void lock() { trylock(-1); }

	// returns true if the lock was successful else false 
	virtual bool trylock(int timeout_ms=0/*in milliseconds*/) { 
		// had troubles with pure virtual here 
		(void)timeout_ms; // for unreferenced formal parameter warning
		return false; 
	} 

	virtual void unlock() {} // had troubles with pure virtual here
};


#ifdef _WIN32
	// *** WIN32 implementation ***

	#include <stddef.h>

	class CMutex : public AMutex
	{
	public:

		/* if recursive is true, then the same thread can safely lock the mutex even if it already has a lock on it */
		CMutex(bool recursive=true,const char *name=NULL) ;
		virtual ~CMutex() throw() ;

		void lock() { trylock(-1); }

		// returns true if the lock was successful else false 
		bool trylock(int timeout_ms=0/*in milliseconds*/); 

		void unlock();

	private:
		friend class CConditionVariable;
		void *mutex; // ??? would make this HANDLE, but really don't think I need to include a 30k line header file for it

	};

	// this class only implements infinite trylock, and so can be used as an optimization when full blocking is all that is needed
	class CFastMutex : public AMutex
	{
	public:
		CFastMutex();
		virtual ~CFastMutex() throw() ;

		bool trylock(int timeout_ms=0/*in milliseconds*/); // note: timeout_ms value is ignored
		void unlock();

	private:
		void*	m_CriticalSection; // actually LPCRITICAL_SECTION, but we're not including windows.h
		int		m_ownerthread;
		int		m_lockcount;
	};

#else
	// *** posix implementation ***

	//#include <stddef.h>

	class CMutex : public AMutex
	{
	public:

		/* if recursive is true, then the same thread can safely lock the mutex even if it already has a lock on it */
		CMutex(bool recursive=true,const char *name=0);
		virtual ~CMutex() throw() ;

		void lock() { trylock(-1); }

		// returns true if the lock was successful else false 
		bool trylock(int timeout_ms=0/*in milliseconds*/);

		void unlock();

	private:
		friend class CConditionVariable;
		void *mutex;
	};


	// dummy implementation... unless someone knows a quicker implementation?
	#define CFastMutex CMutex

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
class CMutexLocker
{
public:
	// construct a locker object with no mutex assigned
	CMutexLocker();

	// construct a locker object with the given mutex.. the mutex is locked/try-locked on construction
	CMutexLocker(AMutex &_m,int timeout_ms=-1);

	// the destructor unlocks the mutex iff it was locked
	virtual ~CMutexLocker() throw() ;

	// returned whether the mutex lock was obtained by this object (sometimes false if a try-lock occurred)
	bool didLock() const { return m!=0; }

	// unlocks the currently assigned mutex if it was locked, assigns a new mutex to this locked and locks/try-locks the new mutex
	// the locked status is returned
	bool reassign(AMutex &_m,int timeout_ms=-1);

	// unlocks the currently assigned mutex if it was locked and leaves the locker object with no assigned mutex
	void unassign();

	// returns a pointer to the current mutex, or NULL if there isn't one
	AMutex *getMutex() const { return m; }

private:
	AMutex *m;
};

// This is a seemingly strange needed class, but it is sometimes needed to respect mutex locking order when dealing with multiple mutexes
//
// It SIMPLY unlock()s on ctor and lock()s on dtor
class CMutexUnlocker
{
public:
	CMutexUnlocker(AMutex &_m) : locked(false), m(_m) { m.unlock(); }
	~CMutexUnlocker() throw ();
	
	void lock() { if(!locked) { m.lock(); locked=true; } }
private:
	CMutexUnlocker& operator=( const CMutexUnlocker & ) { return *this; } // explicitly not supported

	bool locked;
	AMutex &m;
};


#endif

