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

#ifndef __DSP_Delay_H__
#define __DSP_Delay_H__

#include "../../config/common.h"

/* --- TDSPDelay --------------------------------
 *	- This class is uses a circular buffer to delay the input values by a certain delay time
 *	- There are several ways of retrieving samples out of the delay line.
 *		- Some to pull from the end of the delay, and some to pull from the middle of the delay
 * 
 *	- all delay time parameters are in samples not seconds or milliseconds
 *	
 *	- This delay line has a method getSample(unsigned delayTime) in which delayTime be passed 
 *	  0 to maxDelayTime-1.  Where 0 returns the most recent sample given to putSample and given
 *	  maxDelayTime-1 it returns the oldest sample in the delay line.
 *		- Likewise, the getSample(float delayTime) works in a similar way, but it can approximate
 *		  the sample that should appear at the given fractional delayTime.  NOTE: this method
 *		  doesn't work well if maxDelayTime is 1 since it only has one sample to work with.
 *
 *      - To properly use getSample/putSample, getSample should be called first, then putSample should
 *	  be called after.  This is because of the simplest case of delaying by 1 sample, there is one
 *	  element in the buffer.  getSample returns the oldest value (the previous sample given to 
 *	  putSample, and putSample is then called to update that one element with the new sample.
 *
 *      - the template parameter is the type of data to be delayed
 */
template<class sample_t> class TDSPDelay
{
public:
	TDSPDelay(const unsigned _maxDelayTime=1) :
		buffer(NULL),
		maxDelayTime(0),

		putPos(0),
		getPos(0)
	{
		setDelayTime(_maxDelayTime);
	}

	void setDelayTime(unsigned _maxDelayTime)
	{
		if(_maxDelayTime<1)
			_maxDelayTime=1;

		maxDelayTime=_maxDelayTime;

		if(buffer!=NULL)
			delete [] buffer;

		buffer=new sample_t[maxDelayTime]; // expecting an exception on error rather than checking for NULL
		clear();
	}

	virtual ~TDSPDelay()
	{
		if(buffer!=NULL)
			delete [] buffer;
	}

	void clear()
	{
		memset(buffer,0,maxDelayTime*sizeof(*buffer));
		getPos=2*maxDelayTime; // I don't start at zero because getSample(...) subtracts from the positions
		putPos=getPos-1;
	}

	// give an input sample, returns the sample delayed by the constructed delay time
	sample_t processSample(const sample_t input)
	{
		sample_t output=getSample();
		putSample(input);
		return(output);
	}

	void putSample(const sample_t s)
	{
		buffer[(++putPos)%maxDelayTime]=s;
	}

	const sample_t getSample()
	{
		const sample_t s=buffer[(getPos++)%maxDelayTime];
		return(s);
	}

	const sample_t getSample(const unsigned delayTime)
	{
		const sample_t s=buffer[(putPos-delayTime)%maxDelayTime];
		return(s);
	}

	const sample_t getSample(const float delayTime)
	{
		const float fReadPos=putPos-delayTime;
		const unsigned iReadPos=(unsigned)fReadPos; // floored

		const float p2=fReadPos-iReadPos;
		const float p1=1.0-p2;

		const sample_t s=(sample_t)(p1*buffer[(iReadPos)%maxDelayTime] + p2*buffer[(iReadPos+1)%maxDelayTime]);
		return(s);
	}


private:
	sample_t *buffer;
	unsigned maxDelayTime;
	size_t putPos,getPos;
};

#endif
