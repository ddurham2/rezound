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
#include "AThread.h"

#include <stdexcept>
#include <string>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
using namespace std;

#include "clocks.h"
#ifdef GOOGLE_PROFILE
#include <google/profiler.h>
#endif
#ifdef __APPLE__
#import <Foundation/Foundation.h>
//#include "ScopedAutoReleasePool.h"
#endif

#if defined(_WIN32)
	// *** WIN32 implementation ***
	#include <windows.h>
	#include <process.h>
	#include <crtdbg.h>
	#include "istring"

	#ifndef __func__
	#	define __func__	__FUNCTION__
	#endif

	#define THREAD_NONE 0


	// initializes gTSKey (if it hasn't already been) used by get/setCurrentThreadObjectTS()
	static DWORD gTSKey;
	static void initTSKey()
	{
		// allocate the ts-key for TLS data if we haven't already done so
		static bool TSKeyIsSet = false;
		if(!TSKeyIsSet) {
			TSKeyIsSet = true;
			gTSKey = TlsAlloc();
		}
	}

	// associates the given AThread* with the currently running thread so that getCurrentThreadInstance() can later return it
	// must be called at the beginning of a thread's execution and again at the end of a thread given NULL
	static void setCurrentThreadObjectTS(AThread *t)
	{
		TlsSetValue(gTSKey, t);
	}

	// Returns the AThread* for the currently running thread.  The process is aborted
	// if thread specific data has not been properly initialized using either AThread or CRawThreadHelper.
	static AThread *getCurrentThreadObjectTS() // rename to getCurrentThreadInstance()
	{
		AThread *thread = (AThread *)TlsGetValue(gTSKey);
		if (!thread) {
			// If this happens, then a thread was created and is trying to use thread specific data
			// without having first initialized the thread specific data.  Thread specific data 
			// must be initialized using CRawThreadHelper when AThread is not used to spawn the thread.
			assert(("Attempted to access thread specific data for a non AThread", false));

			MessageBox(
				0,
				itstring("Attempted to access thread specific data for a non AThread").c_str(),
				itstring("Error").c_str(),
				MB_OK);
			abort();

		}
		return thread;
	}


	AThread::AThread()
		: mThreadID(0)
		, mThreadHandle(THREAD_NONE)
		, mRunning(false)
		, mCancelled(false)
		, mDeleteOnExit(false)
	{
	}

	AThread::~AThread()
	{
		// TODO consider throwing an exception if it's still isRunning() so that we force applications to make sure that the thread is dead when it gets here

		// just in case the derived class destructor didn't do this
		setCancelled(true);

		wait();

		if (mDeleteOnExit) {
			// This is safe because if mDeleteOnExit is true then we are
			// being destructed from our own thread.
			clearAllThreadSpecific(true);
		}

		if (mThreadHandle != THREAD_NONE) {
			CloseHandle(mThreadHandle);
		}
	}

	void AThread::start(size_t stackSize)
	{
		initTSKey();

		if (isRunning()) {
			throw(runtime_error(string(__func__)+" -- thread is already started"));
		}

		// this would happen if the thread were waited on and then restarted
		if (mThreadHandle != THREAD_NONE) {
			CloseHandle(mThreadHandle);
		}

		mCancelled = false;

		// NOTE: if you're getting undefined _beginthreadex symbols, then you need to change your code-generation in the project properties to multi-threaded
		mThreadHandle = (void*)_beginthreadex(NULL, (unsigned int)stackSize, (unsigned (__stdcall*)(void *))AThreadStart, (void*)this, 0, (unsigned*)&mThreadID);
		if (0 == mThreadHandle) { 
			// error
			mRunning = false;
			mThreadID = 0;
			mThreadHandle = THREAD_NONE;
			throw(runtime_error(string(__func__)+" -- cannot create thread"));
		}

		mRunning = true;
	}

	bool AThread::wait(int timeout_ms) const
	{
		// always fail waiting on ourselves.
		if (getCurrentThreadID() == getThreadID()) return false;
		bool ret = false;

		if ((mThreadHandle != THREAD_NONE) && (WAIT_TIMEOUT != WaitForSingleObject(const_cast<void*>(mThreadHandle), (timeout_ms < 0 ? INFINITE : timeout_ms) ))) {
			ret = true;
		}

		return ret;
	}

	void AThread::yield()
	{
		sleep();
	}

	void AThread::sleep(const int secs, const int msecs)
	{
		clocks::sleep(secs,msecs);
	}

	void *AThread::getCurrentThreadID()
	{
		return (void*)::GetCurrentThreadId();
	}

	bool AThread::isRunning() const
	{
		return mRunning;
	}

	void *AThread::getThreadID() const {
		return (void*)(mThreadHandle == THREAD_NONE ? 0 : mThreadID);
	}

	void *AThread::getHandle() const {
		return mThreadHandle;
	}

	unsigned __stdcall AThread::AThreadStart(void *temp)
	{
		AThread *thread = (AThread*)temp;
		setCurrentThreadObjectTS(thread);

		try {
			#ifdef GOOGLE_PROFILE
			ProfilerRegisterThread();
			#endif
			thread->main();
#ifdef _DEBUG
			// handy for debugging... not necessary for release
			fprintf(stderr, "thread ended 0x%x  %d\n",thread->mThreadID,thread->mThreadID);
#endif
		} catch(exception &e) {
			fprintf(stderr "exception was thrown within thread -- %s\n",e.what());

			// It is important that all threads properly catch their own exceptions!
			// Please make sure you trap this!!!
			// We only abort in debug builds so the app doesn't crash in release mode.
			assert(("UNHANDLED EXCEPTION IN THREAD, PLEASE CATCH ALL EXCEPTIONS",false));
		}
		// we always want other unhandled exceptions to crash so debugger or crashdump is triggered
		//catch(...) {
		//	fprintf(stderr "unhandled exception was thrown within thread\n");
		//}

		thread->mRunning = false;
		
		if(thread->mDeleteOnExit){
			// Let the thread clear its own thread specific data so that
			// the thread can guarantee that it is still a valid object
			// when clearAllThreadSpecific() tries to access it.
			delete thread;
		} else {
			// The thread did not clear its own thread specific data, so
			// we must do it here.
			clearAllThreadSpecific(true);
		}

		setCurrentThreadObjectTS(0);

		return 0;
	}

#else
	// *** posix implementation ***
	#define THREAD_NONE		((pthread_t)0x7fffffff)

	#include <sched.h>	// for sched_yield()
	#include <stdio.h>	// for fprintf()
	#include <stdlib.h>	// for abort()
	#include <string.h>
	#include <errno.h>

	/* for signal stuff only */
	#include <signal.h>
	#include <map>
	using namespace std;

	#include "CConditionVariable.h"
	#include "istring"
	#include "clocks.h"


	// initializes gTSKey (if it hasn't already been) used by get/setCurrentThreadObjectTS()
	static pthread_key_t gTSKey;
	static void initTSKey()
	{
		static bool TSKeyIsSet = false;
		if(!TSKeyIsSet) {
			TSKeyIsSet = true;
			pthread_key_create(&gTSKey, 0);
		}
	}

	// associates the given AThread* with the currently running thread so that getCurrentThreadInstance() can later return it
	// must be called at the beginning of a thread's execution and again at the end of a thread given NULL
	static void setCurrentThreadObjectTS(AThread *t)
	{
		pthread_setspecific(gTSKey, t);
	}

	// returns the AThread* for the currently running thread or NULL if this thread wasn't created with AThread
	static AThread *getCurrentThreadObjectTS() // rename to getCurrentThreadInstance()
	{
		return (AThread *)pthread_getspecific(gTSKey);
	}


	/* --------------------- */

	AThread::AThread() 
		: mThreadID(THREAD_NONE)
		, mRunning(false)
		, mDeleteOnExit(false)
	{
	}

	AThread::~AThread()
	{
		// TODO consider throwing an exception if it's still isRunning() so that we force applications to make sure that the thread is dead when it gets here

		// just in case the derived class destructor didn't do this
		setCancelled(true);

		wait();

		if (mDeleteOnExit) {
			// This is safe because if mDeleteOnExit is true then we are
			// being destructed from our own thread.
			clearAllThreadSpecific(true);
		}
	}

	void AThread::start(size_t stackSize)
	{
		initTSKey();
		CMutexLocker ml(mStartSyncMut);

		if(isRunning()) {
			throw(runtime_error(string(__func__)+" -- thread is already started"));
		}

		pthread_attr_t attr;
		pthread_attr_init(&attr);

		pthread_attr_setstacksize(&attr,stackSize);

		mRunning = true;
		mCancelled = false;

		if (pthread_create(&mThreadID, &attr, &AThread::AThreadStart, (void*)this) != 0) {
			mRunning = false;
			mThreadID=THREAD_NONE;
			int err = errno;
			throw(runtime_error(string(__func__)+" -- cannot create thread: ("+istring(err)+") "+strerror(err)));
		}

		// The creating thread waits until the child has locked mRunningLock.  That
		// should keep anyone from calling wait before that happens.
		mStartSyncCond.wait(mStartSyncMut);
	}

	bool AThread::wait(int timeout_ms) const
	{
		if (!const_cast<AThread*>(this)->mRunningLock.trylock(timeout_ms)) {
			return false;
		}

		bool okay = true;
		
		if (mThreadID != THREAD_NONE) {
			okay = (pthread_join(mThreadID, NULL) == 0);
			if(okay) {
				const_cast<AThread*>(this)->mThreadID = THREAD_NONE;
			}
		}
		
		const_cast<AThread*>(this)->mRunningLock.unlock();
		return okay;
	}

	void AThread::yield()
	{
		sched_yield();
	}

	void AThread::sleep(const int secs, const int msecs)
	{
		clocks::sleep(secs, msecs);
	}

	void *AThread::getCurrentThreadID()
	{
		return (void*)pthread_self();
	}

	bool AThread::isRunning() const
	{
		return mRunning;
	}

	void *AThread::getThreadID() const {
		return (void*)mThreadID;
	}

	void *AThread::AThreadStart(void* temp)
	{
		AThread *thread = (AThread*)temp;
		setCurrentThreadObjectTS(thread);

		try {
			// Get the mRunningLock locked, and release the creator thread.
			thread->mStartSyncMut.lock();
			thread->mRunningLock.lock();
			thread->mStartSyncCond.signal();
			thread->mStartSyncMut.unlock();

			#ifdef GOOGLE_PROFILE
			ProfilerRegisterThread();
			#endif
#ifdef __APPLE__
			assert([NSThread isMultiThreaded] == YES);
			
			#if defined(__USING_ARC__)
			@autoreleasepool{
				thread->main();
			}
			#else
			ScopedAutoReleasePool pool;
			thread->main();
			#endif
#else
			thread->main();
#endif
		} catch(exception &e) {
			fprintf(stderr,"exception was thrown within thread -- ID: %s:(%lu);\nmessage: %s\naborting\n", getThreadName().c_str(), (unsigned long)thread->mThreadID, e.what());fflush(stderr);

			// we only abort all the time with linux code, Mac can continue without crashing
			assert(((void)"UNHANDLED EXCEPTION IN THREAD, ALL EXCEPTIONS MUST BE CAUGHT",false));
#if !defined(__APPLE__)
			abort();
		} catch(...) { 
			// linux will abort, Mac will not so it will crash with unhandled exception and generate a crash report
			fprintf(stderr,"unhandled exception was thrown within thread -- ID: %s:(%x); aborting\n", getThreadName().c_str(), (unsigned)thread->mThreadID);fflush(stderr);
			
			// we only abort all the time with linux code
			abort();
#endif
		}

		thread->mRunning = false;
		try { AThread::alarm(0); } catch(...) { /* oh well */ }

		// wake up waiting callers to AThread::wait()
		thread->mRunningLock.unlock();

		if(thread->mDeleteOnExit){
			// Let the thread clear its own thread specific data so that
			// the thread can guarantee that it is still a valid object
			// when clearAllThreadSpecific() tries to access it.
			delete thread;
		} else {
			// The thread did not clear its own thread specific data, so
			// we must do it here.
			clearAllThreadSpecific(true);
		}

		setCurrentThreadObjectTS(0);

		return NULL;
	}



	// --- stuff for using AThread::alarm()
	/*
	 * This works by having a thread mRunning (upon at least one call to AThread::alarm()) that is responsible
	 * for sending a SIGALARM signal to particular threads that have scheduled and alarm with the 
	 * AThread::alarm() function.  The worker thread efficiently wakes up to send the signal.
	 *
	 * The sighandler for SIGALRM calls AThread::alarmHandler() always.
	 */
	
	class CAlarmerThread : public AThread
	{
	public:
		CAlarmerThread()
			: kill(false)
		{
		}

		virtual ~CAlarmerThread() throw()
		{
			try {
				kill = true;
				c.signal();
				wait();
			} catch(exception &e) {
				/* oh well  it's static anyway */
			}
		}

		void main()
		{
			try {
				while (!kill) {
					CMutexLocker ml(m);

					// calculate the minimum time we need to wait for all scheduled alarms
					clocks::msec_t min_timeout = 60 * 1000; // wake up in 60 seconds to start out with
					{
						clocks::msec_t currentTime = clocks::fixed_msec();
						for(map<void*, clocks::msec_t>::const_iterator i = alarms.begin(); i != alarms.end(); i++) {
							clocks::msec_t timeout = i->second - currentTime;
							if (timeout < min_timeout) {
								min_timeout = timeout;
							}
						}
					}

					if (min_timeout > 0) {
						c.wait(m, min_timeout);
					}

					if (kill) {
						break;
					}

					// check if any threads need to be signaled
					{
						clocks::msec_t currentTime = clocks::fixed_msec();
restart_check:
						for(map<void*, clocks::msec_t>::iterator i = alarms.begin(); i != alarms.end(); i++) {
							clocks::msec_t timeout=i->second-currentTime;
							if (timeout <= 0) {
								pthread_kill((pthread_t)i->first, SIGALRM);
								alarms.erase(i);
								// since map::erase() doesn't return the next iterator, we have to just restart the loop
								goto restart_check;
							}
						}
					}
				}
			} catch(exception &e) {
				fprintf(stderr, "exception in alarmer thread -- %s\n", e.what());
				fflush(stderr);
			}
		}

	private:
		friend void AThread::alarm(int seconds);

		bool kill;
		CMutex m;
		CConditionVariable c;
		map<void*, clocks::msec_t> alarms;

	} gAlarmerThread;

	// NOTE: that on linux the sig handler set up for SIGALRM is shared across all threads, so all threads that
	//    are going to be using AThread::alarm() must all use the same handler function set by calling 
	//    AThread::set_SIGALRM_handler(void (*sighandler)(int)).  This is why set_SIGALRM_handler() is static.
	//    It's very likely that an application should call AThread::set_SIGARLM_handler() only once (i.e. not set
	//    per thread.. because it can't be).
	void AThread::SIGALRM_handler(int sig)
	{
		// this is invoked IN the thread that is receiving the alarm
		if(AThread *that = getCurrentThreadObjectTS()) {
			that->alarmHandler();
		}
	}

	void AThread::alarm(int seconds)
	{
		if (seconds < 0) {
			return;
		}

		if (!gAlarmerThread.isRunning()) {
			if (seconds == 0) {
				return; // if it was never mRunning, then no need to cancel
			}

			// arrange for ::SIGALARM_handler() to be called whenever SIGALRM is sent to this process
			// it is supposed to be raised only by gAlarmerThread declared above.
			struct sigaction act;
			memset(&act, 0, sizeof(act));
			act.sa_handler = AThread::SIGALRM_handler;
			sigaction(SIGALRM, &act, NULL);

			gAlarmerThread.start();
		}

		CMutexLocker ml(gAlarmerThread.m);

		if (seconds>0) { 
			// schedule an alarm
			clocks::msec_t currentTime = clocks::fixed_msec();
			gAlarmerThread.alarms[getCurrentThreadID()] = currentTime + (seconds * 1000LL);
			gAlarmerThread.c.signal();
		} else { 
			// cancel any set alarm
			map<void*, clocks::msec_t>::iterator i = gAlarmerThread.alarms.find(getCurrentThreadID());
			if (i != gAlarmerThread.alarms.end()) {
				gAlarmerThread.alarms.erase(i);
			}
		}
	}

#endif


CRawThreadHelper::CRawThreadHelper()
{
	initTSKey();
	mDummyThread = new AThread;
	setCurrentThreadObjectTS(mDummyThread);
}

CRawThreadHelper::~CRawThreadHelper()
{
	AThread::clearAllThreadSpecific(true);
	delete mDummyThread;
	setCurrentThreadObjectTS(0);
}

// this function and the instance variable below is to allow thread specific data to be set up for the main thread
int setupMainThreadPlaceHolder()
{
	// not deleting this on purpose
	new CRawThreadHelper();

	srand((unsigned)clocks::time_msec());
	AThread::setThreadName("main");
	return 0;
}
static int gMainInitDummy = setupMainThreadPlaceHolder();

void AThread::setSpecific(unsigned key, CThreadSpecificObject *value, bool persistent)
{
	AThread *that = getCurrentThreadObjectTS();
	
	if(value) {
		value->mKey = key;
		value->mPersistent = persistent;
	}

	ThreadSpecificMap_t::iterator i = that->mThreadSpecificMap.find(key);
	if(i == that->mThreadSpecificMap.end()) {
		// nothing yet exists with this key .. add it

		if(value) {
			that->mThreadSpecificMap[key] = value;
		}

	} else {
		// something already exists with this key .. replace it

		if(i->second) {
			if(i->second->mKey > 0) {
				// we're not being called from CThreadSpecificObject::dtor()
				
				
				// prevent CThreadSpecificObject::dtor() from recurring on this method
				i->second->mKey = 0;
				delete i->second;
			}
		}
		if(value) {
			i->second = value;
		} else {
			that->mThreadSpecificMap.erase(i);
		}
	}
}

CThreadSpecificObject *AThread::getSpecific(unsigned key)
{
	AThread *that = getCurrentThreadObjectTS();
	ThreadSpecificMap_t::const_iterator i = that->mThreadSpecificMap.find(key);
	if(i != that->mThreadSpecificMap.end()) {
		return i->second;
	}

	return NULL;
}

void AThread::clearThreadSpecific() throw()
{
	clearAllThreadSpecific(false);
}

void AThread::clearAllThreadSpecific(bool clearPersistent) throw()
{
	if(clearPersistent) {
		// do this so that we'll clear the thread-name for the debugger as well since setThreadName() pokes the runtime debug information
		setThreadName("");
	}

	AThread *that = getCurrentThreadObjectTS();

	// clear non-persistent
	//  (we do this first because if some non-persistent's dtor accesses a persistent object, we are sure not to have destroyed it yet)
	for(ThreadSpecificMap_t::iterator i = that->mThreadSpecificMap.begin(); i != that->mThreadSpecificMap.end();) {
		// either clear everything or else just items that aren't persistent
		if(i->second && !i->second->mPersistent) {
			try {
				i->second->mKey = 0; // prevent dtor from recurring to setSpecific()
				delete i->second;
			} catch(...) {
				/* oh well */
			}

			ThreadSpecificMap_t::iterator e(i);
			++i;
			that->mThreadSpecificMap.erase(e);
		} else {
			++i;
		}
	}

	// now clear persistent (if requested)
	if(clearPersistent) {
		for(ThreadSpecificMap_t::iterator i = that->mThreadSpecificMap.begin(); i != that->mThreadSpecificMap.end();) {
			if(i->second && i->second->mPersistent) {
				try {
					i->second->mKey = 0; // prevent dtor from recurring to setSpecific()
					delete i->second;
				} catch(...) {
					/* oh well */
				}

				ThreadSpecificMap_t::iterator e(i);
				++i;
				that->mThreadSpecificMap.erase(e);
			} else {
				++i;
			}
		}
	}
}


static void setThreadNameInDebugger(const char *threadName)
{
#if defined(_DEBUG) && defined(_WIN32)
	// This code fragment tells the VisualStudio debugger the name of the current thread
	// See: http://msdn2.microsoft.com/en-us/library/xcb2z8hs(VS.80).aspx
	#define MS_VC_EXCEPTION 0x406D1388

	#pragma pack(push,8)
	typedef struct tagTHREADNAME_INFO
	{
		DWORD dwType; // Must be 0x1000.
		LPCSTR szName; // Pointer to name (in user addr space).
		DWORD dwThreadID; // Thread ID (-1=caller thread).
		DWORD dwFlags; // Reserved for future use, must be zero.
	} THREADNAME_INFO;
	#pragma pack(pop)

	Sleep(10);
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = threadName;
	info.dwThreadID = -1;
	info.dwFlags = 0;

	__try
	{
		RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{}
#endif
}

void AThread::setThreadName(const string &threadName)
{
	if(AThread *t = getCurrentThreadObjectTS()) {
		t->mThreadName = threadName;
	}
	setThreadNameInDebugger(threadName.c_str());
}

static const string gEmptyThreadName;
const string &AThread::getThreadName()
{
	const AThread * const t = getCurrentThreadObjectTS();
	return t ? t->mThreadName : gEmptyThreadName;
}

AThread *AThread::getCurrentThread()
{
	return getCurrentThreadObjectTS();
}

// ======================================================

CThreadSpecificObject::CThreadSpecificObject()
	: mKey(0)
{
}

CThreadSpecificObject::~CThreadSpecificObject()
{
	if(mKey > 0) {
		// make sure we're not in the list any longer
		try {
			unsigned tmp = mKey;

			// indicate to setSpecific not to delete this object
			mKey = 0;
			AThread::setSpecific(tmp, 0);
		} catch(...) {
			/* oh well */
		}
	}
}

