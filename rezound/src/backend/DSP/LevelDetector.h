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

#ifndef __DSP_LevelDetector_H__
#define __DSP_LevelDetector_H__

#include "../../config/common.h"

#include <math.h>

#include "../CSound_defs.h"

#include "Delay.h"

/* --- CDSPRMSLevelDetector --------------------------------------
 *
 *	- This class can be used to determine the level or volume of an audio source.
 *	- It uses a progressing window of past samples to calculate an RMS (root-mean-square which is sqrt((s1^2 + s2^2 + ... Sn^2)/N) ) versus using a normal average or 'mean'
 *	- The windowTime given at construction is in samples not seconds
 *	- It is often necessary to initialize the object with 'windowTime' samples before even using the value from readLevel()
 *		- To do this, simply call readLevel() for the first 'windowTime' samples in the audio stream
 */
class CDSPRMSLevelDetector
{
public:
	CDSPRMSLevelDetector(const unsigned _windowTime) :
		window(_windowTime),
		sumOfSquaredSamples(0.0),
		windowTime(_windowTime),
		iWindowTime(_windowTime)
	{
		// ??? based on windowTotals type and sample_t's type there is a maximum time I should impose on windowTime
	}

	virtual ~CDSPRMSLevelDetector()
	{
	}

	// every time you read the level, you have to supply the next sample in the audio stream
		// ??? I could avoid using sqrt by saying that I return the 'power' and not the 'level' and I would then have to square the value I compare this return value with, instead
	const mix_sample_t readLevel(mix_sample_t newSample)
	{
		const mix_sample_t currentAmplitude=(mix_sample_t)sqrt(sumOfSquaredSamples/windowTime);

		// remove oldest sample from running statistic
		sumOfSquaredSamples-=window.getSample();

		// add new sample to running statistic
		const mix_sample_t temp=newSample*newSample;
		sumOfSquaredSamples+=(temp);
		window.putSample(temp);
		
		return(currentAmplitude);
	}

	const unsigned getWindowTime() const
	{
		return(iWindowTime); // the value this was constructed with
	}

private:

	TDSPDelay<mix_sample_t> window; // used to know a past set of samples
	double sumOfSquaredSamples;
	const double windowTime;
	const unsigned iWindowTime;
};

#endif
