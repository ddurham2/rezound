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
	CDSPRMSLevelDetector(const unsigned _windowTime=1) :
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

	const mix_sample_t readCurrentLevel() const
	{
		// ??? I could avoid using sqrt by saying that I return the 'power' and not the 'level' and I would then have to square the value I compare this return value with, instead
		return (mix_sample_t)sqrt(sumOfSquaredSamples/windowTime);
	}

	void updateLevel(mix_sample_t newSample)
	{
		// remove oldest sample from running statistic
		sumOfSquaredSamples-=window.getSample();

		// add new sample to running statistic
		const mix_sample_t temp=newSample*newSample;
		sumOfSquaredSamples+=temp;
		window.putSample(temp);
	}

	// every time you read the level, you have to supply the next sample in the audio stream
	const mix_sample_t readLevel(mix_sample_t newSample)
	{
		const mix_sample_t currentAmplitude=readCurrentLevel();
		updateLevel(newSample);
		return currentAmplitude;
	}


	const unsigned getWindowTime() const
	{
		return iWindowTime; // the value this was constructed with
	}

	// will invalidate any previous information collected about the moving RMS
	void setWindowTime(unsigned _windowTime) // in samples
	{
		window.setDelayTime(_windowTime);
		windowTime=_windowTime;
		iWindowTime=_windowTime;

		sumOfSquaredSamples=0;
		window.clear();
	}

private:

	TDSPDelay<mix_sample_t> window; // used to know a past set of samples
	double sumOfSquaredSamples;
	double windowTime;
	unsigned iWindowTime;
};


/* --- CDSPPeakLevelDetector --------------------------------------
 *
 *	- This class can be used to determine the level or volume of an audio source.
 *	- It finds the sample with the maximum absolute value within the past window-time samples
 *	- The windowTime given at construction is in samples not seconds
 *	- It is often necessary to initialize the object with 'windowTime' samples before even using the value from readLevel()
 *		- To do this, simply call readLevel() for the first 'windowTime' samples in the audio stream
 */
#include <map>
class CDSPPeakLevelDetector
{
public:
	CDSPPeakLevelDetector(const unsigned _windowTime) :
		window(_windowTime),
		windowTime(_windowTime)
	{
		// initialize the history and the histogram
		for(size_t t=0;t<windowTime;t++)
			window.putSample(0);
		histogram[0]=windowTime;
	}

	virtual ~CDSPPeakLevelDetector()
	{
	}

	// every time you read the level, you have to supply the next sample in the audio stream
	const sample_t readLevel(const sample_t _newSample)
	{
		// same abs value of the new sample
		const sample_t newSample= _newSample<0 ? -_newSample : _newSample;

		// get the oldest sample from the delay window
		const sample_t oldestSample=window.getSample();

		// add the new sample into the delay window
		window.putSample(newSample);
	
		// remove the oldest sample from the histogram
		map<sample_t,size_t>::iterator i1=histogram.find(oldestSample);
		if(i1->second<=1)
			histogram.erase(i1);
		else
			i1->second--;

		// add the new sample to the hisogram
		map<sample_t,size_t>::iterator i2=histogram.find(newSample);
		if(i2!=histogram.end())
			i2->second++;
		else
			histogram[newSample]=1;
		
		// return the largest value in the histogram
		return histogram.rbegin()->first;
	}

	const unsigned getWindowTime() const
	{
		return windowTime; // the value this was constructed with
	}

private:
	TDSPDelay<sample_t> window; // used to know a past set of samples
	map<sample_t,size_t> histogram;
	const unsigned windowTime;
};

#endif
