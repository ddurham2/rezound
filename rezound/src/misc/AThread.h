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

/*
 * This is a quick and dirty pthread wrapper that isn't very 
 * functional, but serves for my purposes.  I wrote it because 
 * I once used commonc++ as a cross-platform threading API, but 
 * it was a bit cumbersome and configuring for it was crazy 
 * when going between 1.x and 2.x of it.
 */

#include <pthread.h>

#include <sched.h>	// for sched_yield()
#include <stdio.h>	// for fprintf()
#include <string.h>	// for memset()
#include <stdlib.h>	// for abort()

#include <stdexcept>
#include <string>

class AThread
{
public:
	AThread() : 
		threadID(0x7fffffff),
		running(false)
	{
	}

	virtual ~AThread()
	{
		wait();
	}

	void start(size_t stackSize=4*1024*1024)
	{
		if (isRunning())
			throw(runtime_error(string(__func__)+" -- thread is already started"));

		pthread_attr_t attr;
		pthread_attr_init(&attr);

		pthread_attr_setstacksize(&attr,stackSize);

/* ??? 
	Here I attempted to lower the chances of a drop in the 
	sound because of the playing thread not getting scheduled
	enough.  However, I tried several varieties of values and
	none really seemed to make any difference.  I really think 
	the drops in sound are due my not buffering very much data
	because I want the play position to represent somewhat 
	where in the sound is acually coming out of the speakers.
	I need to somehow be able to buffer more data, but request
	the play position of what is being played rather than what
	I have buffered.

		struct sched_param schedParam;
		memset(&schedParam,0,sizeof(schedParam));
		schedParam.sched_priority=1;
		pthread_attr_setschedparam(&attr,&schedParam);
*/

		running=true;

		if(pthread_create(&threadID,&attr,AThreadStart,(void *)this)!=0)
		{
			running=false;
			throw(runtime_error(string(__func__)+" -- cannot create thread"));
		}
	}

	void wait()
	{
		if(threadID==0x7fffffff)
			return;

		void *status;
		pthread_join(threadID,&status);
		threadID=0x7fffffff;
	}

	static void yield()
	{
		sched_yield();
	}

	bool isRunning() const
	{
		return(running);
	}

protected:
	// just derive an implement this method
	virtual void main()=0;

private:
	pthread_t threadID;
	bool running;

	static void *AThreadStart(void *temp)
	{
		AThread *thread=(AThread *)temp;
		try
		{
			thread->main();
			thread->running=false;
		}
		catch(exception &e)
		{
			thread->running=false;
			fprintf(stderr,"exception was thrown within thread -- ID: 0x%x;\nmessage: %s\naborting\n",(unsigned)thread->threadID,e.what());fflush(stderr);
			abort();
		}
		catch(...)
		{
			thread->running=false;
			fprintf(stderr,"unhandled exception was thrown within thread -- ID: 0x%x; aborting\n",(unsigned)thread->threadID);fflush(stderr);
			abort();
		}
		return(NULL);
	}

};


#endif // __AThread_H__

