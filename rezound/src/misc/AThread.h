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
#ifndef __AThread_H__
#define __AThread_H__

#include "../../config/common.h"

/*
 * This is pthread and win32-thread wrapper that isn't very functional, but serves for my purposes.   
 *
 * The public interface looks like:
 *
 * WARNING! PITFALL! When a thread pointer is deleted or a stack thread object goes out of scope, the destructor of the derived class is called
 *   At this point the derived class's data members are destroyed and AThread's destructor is waiting on the thread to end.  If main() is still
 *   using those now-destroyed data-members, then a crash will likely follow.  The solution is to always arrange for the thread to have ended 
 *   before the derived class's destructor returns.  This can be done either explicitly before being to destroy the object, or as the first thing
 *   in the [most-]derived class's destructor.  This would be accomplished by flagging the thread to end and then calling wait() to wait for it 
 *   to finish.
 *
 * Also, no means of terminating a thread exists.  This is because terminating a thread in mid-execution can result in half-way modified data, 
 * locked resources never getting unlocked and such.  Rather, it is best to design the thread execution path to check often for requests for the 
 * thread to end, and then it to end gracefully.
 *

	class AThread
	{
	public:
		AThread();
		virtual ~AThread();

		void start(size_t stackSize= a platform specific default);

		// returns true if the thread ended (or was already not running)
		// returns false on timeout
		bool wait(int timeout_ms=-1);

		static void yield();

		// gets a uniq thread identifier for the currently running thread
		static void *getCurrentThreadID();
	
		// returns a pointer to the AThread object for the currently running thread or NULL if AThread didn't create the thread
		static AThread *getCurrentThread();

		bool isRunning() const;

		void *getThreadID() const;

		// This method is used to set thread-specific data with the given key to the given value.
		// The value object must be allocated on the heap and will be cleaned up by the AThread system.
		// If the value assocated with the key needs to be cleaned up sooner then call setSpecific(key, NULL) or
		// just delete the CThreadSpecificObject pointer whose dtor will do the same.
		// If 'persistent' is true, then the value will not be cleared out with clearThreadSpecific().
		// NOTE: A created CThreadSpecificObject must be associated with no more than 1 key value.
		// NOTE: The actual object given is what is associated for the life-time of that data-value.  No copies of the object will/can be made.
		// NOTE: The key value 0 is reserved.
		static void setSpecific(unsigned key, CThreadSpecificObject *value, bool persistent = false);

		// returns the thread-specific data object previously set by setSpecific() with the same key
		// if no value is found with the given key, then NULL will be returned.
		static CThreadSpecificObject *getSpecific(unsigned key);

		// cleans up all non-persistent thread specific data
		// If needed, this can be called before the thread actually ends to reset most thread specific information, 
		// but a complete cleanup of all thread specific data will be performed when the thread ends.
		static void clearThreadSpecific();

		// NOTE: no thread specific data is preserved between subsequent starts() of the same AThread object (not 
		//       even values flagged as persistent)


		// sets the name of the thread which is a thread specific data.  
		static void setThreadName(const std::string &threadName);

		// gets the name of the thread; returns "" if no name has been set
		static const std::string &getThreadName();


		// These methods are used to set / determine if the thread has been requested to quit
		// The isCancelled() method returns a reference to the bool flag in case it needs to be passed down to non-AThread-aware code
		void setCancelled(bool cancelled);
		const bool &isCancelled() const;

	protected:
		// derive and implement this method
		virtual void main()=0;
	};
 */

#include "CMutex.h"
#include <string>

// In order to use thread specific data, a sub-class of this object should be used.
// When thread specific data is fetched, an CThreadSpecificObject pointer is returned 
// which should be cast to the specific sub-class type to obtain custom data values.
// The purpose of the class is so that the sub-class's destructor can do any custom cleanup
class CThreadSpecificObject
{
public:
	CThreadSpecificObject();

	// NOTE: behavior is undefined if thread specific data is
	// set/fetched during or after the destructor of AThread has run
	virtual ~CThreadSpecificObject();

private:
	// non copiable
	explicit CThreadSpecificObject(const CThreadSpecificObject &src);
	CThreadSpecificObject &operator=(const CThreadSpecificObject &rhs);

	friend class AThread;
	unsigned mKey;
	bool mPersistent;
};


// private type
#if defined(__GNUC__)
#	include <unordered_map>
	typedef std::unordered_map<unsigned, CThreadSpecificObject *> ThreadSpecificMap_t;
#else
# 	include <map>
	typedef std::map<unsigned, CThreadSpecificObject *> ThreadSpecificMap_t;
#endif


#ifdef _WIN32

	// *** WIN32 implementation ***

	#define ATHREAD_DEFAULT_STACK_SIZE 0

	class AThread
	{
	public:
		AThread();
		virtual ~AThread();

		void start(size_t stackSize=ATHREAD_DEFAULT_STACK_SIZE);

		bool wait(int timeout_ms=-1) const;

		static void yield();
		static void *getCurrentThreadID();
		static AThread *getCurrentThread();
		static void sleep(const int secs=0, const int msecs=0);

		bool isRunning() const;

		void *getThreadID() const;
		void *getHandle() const;

		static void setSpecific(unsigned key, CThreadSpecificObject *value, bool persistent = false);
		static CThreadSpecificObject *getSpecific(unsigned key);
		static void clearThreadSpecific() throw();

		static void setThreadName(const std::string &threadName);
		static const std::string &getThreadName();

		void setCancelled(bool cancelled) { mCancelled = cancelled; }
		const bool& isCancelled() const { return mCancelled; }
		static const bool& isCurrentThreadCancelled() { return getCurrentThread()->isCancelled(); }

		// this should only be set if you are abandoning the thread
		// and understand the implications (eg. all thread vars must be local)
		void setDeleteOnExit(bool del) { mDeleteOnExit = del; }

	protected:
		// just derive and implement this method
		// don't make this pure virtual, since it causes issues with exceptions and destructors!
		virtual void main(){}

	private:
		friend class CRawThreadHelper;

		ThreadSpecificMap_t mThreadSpecificMap;
		std::string mThreadName;

		void *mThreadHandle; 
		unsigned mThreadID; 
		bool mRunning;
		bool mCancelled;
		bool mDeleteOnExit;

		static unsigned __stdcall AThreadStart(void *temp);
		static void clearAllThreadSpecific(bool clearPersistent) throw();
	};

#else
	// *** posix implementation ***

	#include <pthread.h>
	#include "CConditionVariable.h"

	#define ATHREAD_DEFAULT_STACK_SIZE (4 * 1024 * 1024)

	class AThread
	{
	public:
		AThread();
		virtual ~AThread();

		void start(size_t stackSize=ATHREAD_DEFAULT_STACK_SIZE);

		bool wait(int timeout_ms=-1) const;

		static void yield();
		static void *getCurrentThreadID();
		static AThread *getCurrentThread();
		static void sleep(const int secs=0, const int msecs=0);

		bool isRunning() const;

		void *getThreadID() const;

		static void setSpecific(unsigned key, CThreadSpecificObject *value, bool persistent = false);
		static CThreadSpecificObject *getSpecific(unsigned key);
		static void clearThreadSpecific() throw();

		static void setThreadName(const std::string &threadName);
		static const std::string &getThreadName();

		void setCancelled(bool cancelled) { mCancelled = cancelled; }
		const bool& isCancelled() const { return mCancelled; }
		static const bool& isCurrentThreadCancelled() { return getCurrentThread()->isCancelled(); }

		// this method arranges for AThread::alarmHandler() to be called within the thread that called AThread::alarm()
		// after the given number of seconds.  The subclass should override alarmHandler() and implement the application
		// specific code.
		//
		// The implementation sets the process's signal handler for SIGALRM, therefore the application should not alter
		// the signal handler else the behavior will be unpredictable.  Also, do NOT use ::alarm() in the same process
		// that uses AThread::alarm()
		//
		// PITFALL:
		// In the case the a AThread subclass is going to use an alarm for different reason at different times.  It is 
		// probably undesirable to call AThread::alarm(n) where n>0 without having called AThread::alarm(0) first.  
		// The problem is that if the subclass's implementation of alarmHandler() uses any state information out of the
		// thread object, without a call to alarm(0) it will be ambiguous as to which call to alarm() this is a timeout for.
		//
		// For example:
		//
		// class foo : public AThread {
		// protected:
		//    void alarmHandler() {
		//       if(f==1)
		//         // CASE A
		//       else if(f==2)
		//         // CASE B
		//    }
		//
		//    void main() {
		//       ...
		//       f=1;
		//       alarm(3);
		//       ...
		//       alarm(0);
		//       f=2;
		//       alarm(10);
		//       ...
		//    }
		// }
		//
		// In the above class, if alarm(0) in main() were not in place, then when alarmHandler() is called, it is possible that f will equal 2 but was a timeout for the call to alarm(3)
		//
		// alarm(0) is automatically called when main() has returned normally or abnormally.  But it is still necessary to ensure that AThread::alarmHandler() will not be called and use
		// something that is destroyed when main returns.
		//
		// calling alarm(0) from the destructor is probably not desired since it would clear the alarm() in the thread that's calling the destroying the object (NOT clearing the alarm
		// for that object's system thread)
		//
		static void alarm(int seconds);

		// this should only be set if you are abandoning the thread
		// and understand the implications (eg. all thread vars must be local)
		void setDeleteOnExit(bool del) { mDeleteOnExit = del; }

	protected:
		// just derive and implement this method
		// don't make this pure virtual, since it causes issues with exceptions and destructors!
		virtual void main() {}

		// this may be overridden to handle the receipt of an alarm scheduled using AThread::alarm()
		virtual void alarmHandler() throw() {}

	private:
		friend class CRawThreadHelper;

		ThreadSpecificMap_t mThreadSpecificMap;
		std::string mThreadName;

		pthread_t mThreadID;
		bool mRunning;
		bool mCancelled;
		bool mDeleteOnExit;

		CMutex mStartSyncMut;
		CConditionVariable mStartSyncCond;
		CMutex mRunningLock;
		// The first two are used to synchronize the locking of the third, so it
		// can be locked reliably when the thread starts, and unlocked when it
		// finishes.  (It would be easier if mRunningLock could be locked by the
		// parent, it needs to be unlocked by the child.  But the locker and 
		// unlocker need to be the same thread.)

		static void clearAllThreadSpecific(bool clearPersistent) throw();

		static void *AThreadStart(void *temp);
		static void SIGALRM_handler(int sig);
	};

#endif

// All threads that are not created using AThread must create a CRawThreadHelper object
// inside of the thread.  The helper object is responsible for initializing thread-specific
// data. Its constructor initializes the data and its destructor de-initializes the data.
// Therefore, the object should be destructed right before the thread terminates.
class CRawThreadHelper
{
public:
	CRawThreadHelper();
	virtual ~CRawThreadHelper();
private:
	AThread *mDummyThread;
};

#endif // __AThread_H__

