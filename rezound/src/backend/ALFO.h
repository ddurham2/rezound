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

#ifndef __ALFO_H__
#define __ALFO_H__

#include "../../config/common.h"


/* --- ALFO ---------------------
 *  - This is an abstract class used to generate an LFO value
 *  - DSP algorithms can be designed to use one of these instead of a hard coded function or constant value
 *  - The derived LFO class simply overrides the nextValue method and DSP algorithms call it not needing to know the LFO function
 *  	- Sometimes, however,  it is not possible to write the algorithm using an LFO and not knowing the LFO function because
 *  	  some algorithms may need to determine things from the integral or derivative of the function.  I could have each 
 *  	  LFO implementation to supply these related functions too, but as of now, I haven't needed it.
 */
class ALFO
{
public:
	virtual ~ALFO()
	{
	}

	virtual const float nextValue()=0;
	virtual const float getValue(sample_pos_t time) const=0; // time is in samples, that is not seconds or ms

protected:

	ALFO()
	{
	}

};



// --- some LFO implementations ------

#include "unit_conv.h"
#include <math.h>

/*
 * - LFO with simply a constant value
 */
class CConstantLFO : public ALFO
{
public:
	CConstantLFO(float _value) :
		value(_value)
	{
	}

	virtual ~CConstantLFO()
	{
	}

	const float nextValue()
	{
		return(value);
	}

	const float getValue(const sample_pos_t time) const
	{
		return(value);
	}

private:
	const float value;
	
};



/*
 * - Sine shaped LFO with a range of [-1,1] and a frequenct and initial value specified by the constructor's arguments
 * - Note: I use sinf which is (from glibc) a faster float version of sine rather than double percision
 * 	- This function should be available on other platforms because it is mentioned in the ANSI C99 specification
 */
class CSinLFO : public ALFO
{
public:
	// frequency is in hertz
	// initialAngle is in desgrees
	// sampleRate is given to know how often nextValue will be called per cycle
	CSinLFO(float _frequency,float initialAngle,unsigned sampleRate) :
			// convert from herz to scalar to mul with counter
		frequency((1.0/((float)sampleRate/_frequency))*(2.0*M_PI)),
		initial(degrees_to_radians(initialAngle)/frequency),

			// initialize the counter to return the initial angle
		counter(initial)
	{
	}

	virtual ~CSinLFO()
	{
	}

	const float nextValue()
	{
		return(sinf((counter++)*frequency));
	}

	const float getValue(const sample_pos_t time) const
	{
		return(sinf((time+initial)*frequency));
	}

protected:
	float frequency;
	float initial;
	// ??? I probably do need to worry about counter wrap around
		// perhaps I could know a threshold when to fmod the counter
	float counter;
};




/* 
 * - Same as CSinLFO except it has a range of [0,1] (but it still has the same shape as sine)
 */
class CPosSinLFO : public CSinLFO
{
public:
	CPosSinLFO(float frequency,float initialAngle,unsigned sampleRate) :
		CSinLFO(frequency,initialAngle,sampleRate)
	{
	}

	virtual ~CPosSinLFO()
	{
	}

	const float nextValue()
	{
		return((sinf((counter++)*frequency)+1.0)/2.0);
	}

	const float getValue(const sample_pos_t time) const
	{
		return((sinf((time+initial)*frequency)+1.0)/2.0);
	}

};




#endif
