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

#ifndef __CConditionVariable_H__
#define __CConditionVariable_H__

#include "../../config/common.h"

/*
 * This is a quick and dirty conditional variable wrapper. See AThread.h for more details
 */

#include <pthread.h>

#include <errno.h>	// for EBUSY
#include <string.h>	// for strerror()

#include <stdexcept>
#include <string>

#include "CMutex.h"

/*
	Example use:

	CMutex m;
	CConditionalVariable c;

	// --- code that waits for condition to be true -----

	m.lock();
	while(desired condition IS NOT true)
		c.wait(m);

	// now condition IS true
		...
	m.unlock();

	...


	// --- code that causes condition to be true ------

	m.lock();
	... something that causes condition to be true ...
	c.broadcast();
	m.unlock();

	...



	Now, if it can be known that only two threads will ever 
	be dealing with the condition, c.signal() can be used 
	to be more efficient.  In doubt, use c.broadcast()
*/

class CConditionVariable
{
public:

	CConditionVariable()
	{
		pthread_cond_init(&cond,NULL);
	}

	virtual ~CConditionVariable()
	{
		const int ret=pthread_cond_destroy(&cond);
		if(ret)
			throw(runtime_error(string(__func__)+" -- error destroying conditional variable -- "+strerror(ret))); // may not care tho
	}

	void wait(CMutex &mutex)
	{
		const int ret=pthread_cond_wait(&cond,&mutex.mutex);
		if(ret)
			throw(runtime_error(string(__func__)+" -- error waiting -- "+strerror(ret)));
	}

/*
	void wait(int timeout,CMutex &mutex)
	{
		const int ret=pthread_cond_timewait(&cond,&mutex.mutex,...);
		if(ret)
			throw(runtime_error(string(__func__)+" -- error waiting -- "+strerror(ret)));
	}
*/

	void signal()
	{
		const int ret=pthread_cond_signal(&cond);
		if(ret)
			throw(runtime_error(string(__func__)+" -- error signaling -- "+strerror(ret)));
	}

	void broadcast()
	{
		const int ret=pthread_cond_broadcast(&cond);
		if(ret)
			throw(runtime_error(string(__func__)+" -- error broadcasting -- "+strerror(ret)));
	}

private:
	pthread_cond_t cond;

};


#endif
