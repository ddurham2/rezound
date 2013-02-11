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
#include "CMutex.h"
#include <stdio.h>
#include <assert.h>

#ifdef _WIN32
	// *** WIN32 implementation ***

	#include <stdexcept>
	#include <string>
	#include "istring"
	using namespace std;

	#ifndef __func__
	#define __func__ __FUNCTION__
	#endif

	#include <windows.h>
	#include "DLLFunction.h"
	#include "Singleton.h"

	namespace {
		class DLL : public Singleton<DLL> {
		public:
			typedef BOOL (WINAPI *InitializeCriticalSectionAndSpinCount_t)(LPCRITICAL_SECTION lpCriticalSection,DWORD dwSpinCount);
			DLLFunction<InitializeCriticalSectionAndSpinCount_t> InitializeCriticalSectionAndSpinCount;
		private:
			friend class Singleton<DLL>;
			DLL()
				: InitializeCriticalSectionAndSpinCount(_T("kernel32.dll"),"InitializeCriticalSectionAndSpinCount")
			{}
		};
	}

	/* if recursive is true, then the same thread can safely lock the mutex even if it already has a lock on it */
	CMutex::CMutex(bool recursive,const char *name) 
	{
		// recursive is the default behavior on win32, so IOW, we can't ever dead-lock ourself if we wanted to.. but other implementations still may be more efficient with the mutex not being recursive
		mutex=CreateMutex(NULL,FALSE,name?itstring(name).c_str():NULL);
	}

	CMutex::~CMutex() throw()
	{
		CloseHandle(mutex);
	}

	bool CMutex::trylock(int timeout_ms)
	{
		int ret=1;

		if(timeout_ms<0){
			timeout_ms=INFINITE;
		}


		/*
		   In case you're wondering if this WaitForSingleObject operation is reference counted, it is:
				http://msdn2.microsoft.com/en-us/library/ms682411.aspx
				Quote:
					The thread that owns a mutex can specify the same mutex in repeated wait function calls 
					without blocking its execution. Typically, you would not wait repeatedly for the same 
					mutex, but this mechanism prevents a thread from deadlocking itself while waiting for a 
					mutex that it already owns. However, to release its ownership, the thread must call 
					ReleaseMutex once for each time that the mutex satisfied a wait.
		 */

		// since WAIT_OBJECT_0 == 0L, we can just return whatever we get
		ret=WaitForSingleObject(mutex,timeout_ms);

		if(WAIT_ABANDONED==ret){
			// this special case actually does grant the mutex ownership, 
			// so change the return value to ignore this case
			ret=0;
		}

		//if(ret && WAIT_TIMEOUT!=ret)
		//	throw runtime_error(string(__FUNCTION__)+" -- error aquiring lock -- "+strerror(ret));

		return ret==0;
	}

	void CMutex::unlock()
	{
		ReleaseMutex(mutex);
	}

	//***********************************************************************************
	// "Critical Sections" have been said to be more efficient in win32 and often involve
	// just one hardware instruction rather than a system call in the case of mutexes...
	// ...thus, the birth of CFastMutex!

	CFastMutex::CFastMutex()
		: m_ownerthread(0)
		, m_lockcount(0)
	{
		m_CriticalSection=new CRITICAL_SECTION;
		if(DLL::instance().InitializeCriticalSectionAndSpinCount){
			DLL::instance().InitializeCriticalSectionAndSpinCount((LPCRITICAL_SECTION)m_CriticalSection,4000);
		} else {
			InitializeCriticalSection((LPCRITICAL_SECTION)m_CriticalSection);
		}
	}
	
	CFastMutex::~CFastMutex() throw(){
		unlock();
		DeleteCriticalSection((LPCRITICAL_SECTION)m_CriticalSection);
		delete m_CriticalSection;
		m_CriticalSection=NULL;
	}

	bool CFastMutex::trylock(int timeout_ms){
		assert(("trylock called with timeout",timeout_ms<0));
		assert(("no critical section",m_CriticalSection!=0));

		if (m_ownerthread==GetCurrentThreadId()) { 
			// Recursive lock
			m_lockcount++; 
			return true; 
		}

#if defined(_DEBUG) && 0 // alter to enable deadlock detection
		int count = 0;
		while(m_ownerthread!=NULL && count<50){
			Sleep(100);
			count++;
		}
		if(m_ownerthread!=NULL){
			itstring msg = itstring("Deadlock detected in thread ") + itstring(istring(GetCurrentThreadId())) + itstring(" blocked by thread ") + itstring(istring(m_ownerthread));
			MessageBox(NULL,msg,_T("CFastMutex::trylock"),MB_ICONSTOP|MB_OK|MB_TOPMOST);
			DebugBreak();
		}
#endif

		EnterCriticalSection((LPCRITICAL_SECTION)m_CriticalSection);
		m_lockcount++;

		// guarenteed exclusive access since we entered a critical section
		// save the threadid that created the mutex so we can check for errors on unlock
		m_ownerthread=GetCurrentThreadId();

		return true;
	}
	
	void CFastMutex::unlock(){
		if(m_lockcount > 0){
			m_lockcount--;
			if (m_lockcount == 0) {
				assert(("no critical section",m_CriticalSection!=0));

				// MSDN says: If a thread calls LeaveCriticalSection when it does not have ownership of 
				// the specified critical section object, an error occurs that may cause another 
				// thread using EnterCriticalSection to wait indefinitely.
				assert(("thread trespassing on unlock",GetCurrentThreadId()==m_ownerthread));

				// guarenteed exclusive access since we haven't left our critical section
				m_ownerthread=NULL;

				LeaveCriticalSection((LPCRITICAL_SECTION)m_CriticalSection);
			}
		}
	}
#else
	// *** posix implementation ***

	#include <pthread.h>

	#include <stdexcept>
	#include <string>
	using namespace std;

	#include <errno.h>	// for EBUSY
	#include <string.h>	// for strerror()
	#include <unistd.h>	// for usleep()
	//#include <time.h>	// for clock_gettime()

	#include "clocks.h"

	/* if recursive is true, then the same thread can safely lock the mutex even if it already has a lock on it */
	CMutex::CMutex(bool recursive,const char *name) :
		mutex(new pthread_mutex_t)
	{
		if(name!=NULL)
			throw runtime_error(string(__func__)+" -- globally named mutexes are not implemented on this platform");
		if(recursive)
		{
			//pthread_mutex_t _mutex=PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
			pthread_mutexattr_t attr;
			pthread_mutexattr_init(&attr);
			pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);

			pthread_mutex_init((pthread_mutex_t *)mutex,&attr);

			pthread_mutexattr_destroy(&attr);
		}
		else
			pthread_mutex_init((pthread_mutex_t *)mutex,NULL);

		/* these are a little slower, but safer.. needs to be a runtime option probably
		// make it an error to lock it twice in the same thread
		pthread_mutex_t _mutex=PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;
		mutex=_mutex;
		*/
	}

	CMutex::~CMutex() throw()
	{
		const int ret=pthread_mutex_destroy((pthread_mutex_t *)mutex);
		delete (pthread_mutex_t *)mutex;
		if(ret)
			//throw runtime_error(string(__func__)+" -- error destroying mutex -- "+strerror(ret)); // may not care tho
			fprintf(stderr, "%s -- error destroying mutex -- %s\n",__func__,strerror(ret));
	}

	// returns true if the lock was successful else false 
	bool CMutex::trylock(int timeout_ms) 
	{
		int ret=1;
		if(timeout_ms<0)
		{
			ret=pthread_mutex_lock((pthread_mutex_t *)mutex);
			if(ret)
				throw runtime_error(string(__func__)+" -- error aquiring lock -- "+strerror(ret));
		}
		else if(timeout_ms==0)
		{
			ret=pthread_mutex_trylock((pthread_mutex_t *)mutex);
			if(ret && ret!=EBUSY)
				throw runtime_error(string(__func__)+" -- error doing try lock -- "+strerror(ret));
		}
		else //if(timeout_ms>0)
		{
#ifdef __APPLE__
			/* 
				On OS X there is no implementation of pthread_mutex_timedlock()
				I attempted to implement it.  Looking at the implementation of pthread_mutex_lock() in 
					http://darwinsource.opendarwin.org/10.4.6.ppc/Libc-391.2.5/pthreads/pthread_mutex.c
				the logic was straigh forward and it looked as tho the semaphore_wait_signal() and pthread_wait()
				calls on locking the 'sem' value into semaphre_timedwait_signal() and semaphore_timedwait() calls.
	
				However, after going to much trouble to do this, the two functions, new_sem_from_pool() and 
				restore_sem_to_pool() are in private symbol tables in libc.dynlib and are not accessible when linking.

			 	Below is a VERY poor-man's and extremely starvation-prone implementation of trylock-with-timeout.
				It simply periodically tries to lock (with no timeout) until the timeout expires or the lock is obtained.

				There are 2 alternatives to this stupid approach when this implementation just will not work:
				On OS X: copy the pthread_mutex.c and pthread_cond.c (and other supporting snippets) and use them as a 
				   starting point in an alternate implementation of mutexes but with a pthread_mutex_timedlock() 
				   (condition variables are included because they have to deal with the internals of a mutex)
				For all platforms: implement mutex using a condition variable.  Apparently pthread_cond_timedwait() is 
				   more implemented on other platforms.  An implementation using condition variables would go something like:
					- use a normal pthreads mutex to serialize the lock and unlock methods
					- when locking, if the our "mutex" is unlocked, just say we locked it.. 
					- track who the owner is for  recursive mutexes
					- an unlock with others waiting is just a signal to anyone waiting on a lock, or setting the flag if others aren't waiting
					- I would guess that the condition variable is fair about who it signals when it wakes a waiter up
					- a lock without anyone else waiting is a simple set of a flag
				If I go with the second implementation I would probably just #ifdef out the current implementation in case 
				another platform later doesn't implement pthread_cond_timedwait()
			 */

			#warning this is a very poor and starvation-prone implementation of trylock-with-timeout.  Do not use it if starvation is a potential problem.  See this source file for alternatives when necessary.

			const time_t base=time(NULL);

			unsigned long expireTime=(clocks::fixed_msec()-((clocks::msec_t)base*1000))+timeout_ms;
			
			unsigned long currentTime;
			do {
				if(trylock(0))
					return true;

				if(timeout_ms<50)
				{ // just try once again after the full timeout and that's it
					usleep(timeout_ms*1000);
					return trylock(0);
				}
				else
					usleep(50*1000); // sleep for 50ms
				
				currentTime=(clocks::fixed_msec()-((clocks::msec_t)base*1000));
			} while(currentTime<expireTime);

			return false;
			
#else
			struct timespec abs_timeout;

	#ifdef __linux
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

			// add our desired duration to the current time-of-day
			abs_timeout.tv_sec+=(timeout_ms/1000);
			abs_timeout.tv_nsec+=(timeout_ms%1000)*1000*1000;
			abs_timeout.tv_sec+=abs_timeout.tv_nsec/1000000000;
			abs_timeout.tv_nsec%=1000000000;
			
			ret=pthread_mutex_timedlock((pthread_mutex_t *)mutex,&abs_timeout);
			if(ret && ret!=ETIMEDOUT)
				throw runtime_error(string(__func__)+" -- error aquiring lock -- "+strerror(ret));
#endif
		}
		return ret==0;
	}

	void CMutex::unlock()
	{
		const int ret=pthread_mutex_unlock((pthread_mutex_t *)mutex);
		if(ret)
			throw runtime_error(string(__func__)+" -- error unlocking mutex -- "+strerror(ret));
	}



#endif

CMutexLocker::CMutexLocker() :
	m(NULL)
{
}

CMutexLocker::CMutexLocker(AMutex &_m) :
	m(NULL)
{
	reassign(_m,-1);
}

CMutexLocker::CMutexLocker(AMutex &_m,int timeout_ms) :
	m(NULL)
{
	reassign(_m,timeout_ms);
}

CMutexLocker::~CMutexLocker() throw()
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
bool CMutexLocker::reassign(AMutex &_m,int timeout_ms)
{
	unassign();

	// if you get a crash here, it means you have invoked a null class instance
	// and this is just where it is showing up... check your call stack!

	if(_m.trylock(timeout_ms))
	{
		m=&_m;
		return true;
	}
	return false;
}

// unlocks the currently assigned mutex if it was locked and leaves the locker object with no assigned mutex
void CMutexLocker::unassign()
{
	if(m)
	{
		m->unlock();
	}
	m=NULL;
}

CMutexUnlocker::~CMutexUnlocker() throw ()
{
	try
	{
		lock(); 
	}
	catch(exception &e)
	{
		fprintf(stderr, "%s -- exception -- %s\n",__func__,e.what());
	}
}

