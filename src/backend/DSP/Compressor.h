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

#ifndef __DSP_Compressor_H__
#define __DSP_Compressor_H__

#include "../../config/common.h"

#include <algorithm>

#include "../CSound_defs.h"

#include "LevelDetector.h"


/* --- CDSPCompressor ----------------------------------------
 *
 * - This DSP block begins to change the gain from 1.0 to a different value based 
 *   on the ratio on the sound when the level of the level signal rises above the given 
 *   'threshold'.  The rate at which it begins to change this gain is given by the 
 *   'attackTime' and the rate at which it begins to return to a gain of 1.0 once 
 *   the level is again below the threshold is given by 'releaseTime'.  The level 
 *   is detected with a moving average (performed by CDSPRMSLevelDetector) the width
 *   of the moving window is given by 'windowTime'
 *
 * - To use simply construct the block with the desired parameters and repeatedly call 
 *   processSample() which returns the 'inputSample' that was given but adjusted by the 
 *   calculated gain which acts according to the changes in level for successive 
 *   'levelSample' values.  These two different values allows one to create a cross-
 *   compressor having one signal adjusted by the level of another
 *
 * - Note: it may be desirable to initialize the level detector within the algorithm
 *   by calling initSample() for 'windowTime' samples before calling processSample() 
 *   and using its return value.  This is because we don't really know the level before 
 *   enough sample have been observed.
 *
 * Info reference: http://www.harmony-central.com/Effects/Articles/Compression/
 * 		   and newsgroup: comp.dsp  my thread, 'Compressor Question #2'
 *
 * Formula for the gain when the level is above the threshold: (Thanks to amolitor's notes)
 * 	
 * R is the ratio in XdB:YdB -- for an X dB increase in the input there should be only a Y dB increase in the ouput
 * D is the measured level in dB
 * T is the threshold in dB
 *
 * g = -((R-1)*(D-T))/R   that is g is the output change in dB given D in dB
 *
 * Now to avoid so much math per sample I need to convert this to the unites of my actual audio data (sample values):
 *
 * We want to find g as a gain scalar instead of dB change.
 *
 * Using the formula for dB to gain scalar:   scalar=10^(dB/20)
 *
 * g = 10 ^ (-((R-1)*(D-T))/(20*R))
 *
 * Now we can avoid the negation of the exponent: 
 *
 * g = .1 ^ (((R-1)*(D-T))/(20*R))
 *
 * Now we want to have D as a sample value and not a value in dB.  We will call the measured level in sample value units L instead of D in dB
 *
 * The formula for converting a sample value to dB (dBFS actually)
 *
 * D = 20*log10(abs(S)/MAX)    (where MAX is the maximum sample value)
 *
 * substituting D in our gain calulation:
 *
 * g = .1 ^ (((R-1)*((20*log10(abs(S)/MAX))-T))/(20*R))
 *
 * since R is constant we can define
 *
 * Q = 20*(R-1) / (20*R)
 *
 * and simplify the gain calulation to:
 *
 * g = .1 ^ (Q*log10(abs(L)/MAX)-(T/20))
 *
 * since T is constant we can define
 *
 * K = T/20
 *
 * and again simplify the gain calculation to:
 *
 * g = .1 ^ (Q*log10(abs(L)/MAX)-K)
 *
 * and this is as simple as I can get it
 *
 */
class CDSPCompressor
{
public:
	// all times are in samples
	CDSPCompressor(unsigned _windowTime,sample_t _threshold,float _compressionRatio,unsigned _attackTime,unsigned _releaseTime) :
		windowTime(_windowTime),
		threshold(_threshold),
		compressionRatio(_compressionRatio),

		// have to take into account the ratio to be reached for the calculation 
		// of the envelope velocity (value to add/sub to bouncingRatio each sample frame)
		attackVelocity((_compressionRatio-1.0)/_attackTime),
		releaseVelocity((_compressionRatio-1.0)/_releaseTime),

		bouncingRatio(1.0),
		levelDetector(windowTime)
	{
		// ??? verify parameters?
	}

	virtual ~CDSPCompressor()
	{
	}

	// using this is optional -- it is used to initialize the level detector -- see Note above
	void initSample(const mix_sample_t s)
	{
		levelDetector.readLevel(s);
	}

	const mix_sample_t processSample(const mix_sample_t inputSample,const mix_sample_t levelSample)
	{
		const mix_sample_t level=levelDetector.readLevel(levelSample);
		
		// ??? I need to allow compression ratio to also be less than 1 to act as an exciter
		if(level>=threshold && bouncingRatio!=compressionRatio)
			bouncingRatio= min(bouncingRatio+attackVelocity,compressionRatio);
		else if(level<threshold && bouncingRatio!=1.0)
			bouncingRatio= max(bouncingRatio-releaseVelocity,1.0f);

		// attempt to smooth out the way the attack sounds
		//const float _bouncingRatio=unitRange_to_otherRange_linear(unitRange_to_unitRange_squared(otherRange_to_unitRange_linear(bouncingRatio,1.0,compressionRatio)),1.0,compressionRatio);
		#define _bouncingRatio bouncingRatio

		if(_bouncingRatio>1.0)
			return (mix_sample_t)(inputSample * pow((double)threshold/(double)level,(_bouncingRatio-1.0)/_bouncingRatio) );
		else
			return inputSample;
		
	}

	// this works like processSample except that it uses all the channels in levelInputFrame to determine the signal level and adjust all the samples in the frame according to that singular calculated level
	// its output is a modification of the inputFrame parameter
	void processSampleFrame(mix_sample_t *inputFrame,mix_sample_t *levelInputFrame,const unsigned frameSize)
	{
		// determine the maximum sample value in the frame to use as the level detector's input
		mix_sample_t levelSample=levelInputFrame[0];
		if(levelSample<0) levelSample=-levelSample;
		for(unsigned t=1;t<frameSize;t++)
		{
			mix_sample_t l=levelInputFrame[t];
			if(l<0) l=-l;
			levelSample=max(levelSample,l);
		}
			
		const mix_sample_t level=levelDetector.readLevel(levelSample);
		
		// ??? I need to allow compression ratio to also be less than 1 to act as an exciter
		if(level>=threshold && bouncingRatio!=compressionRatio)
			bouncingRatio= min(bouncingRatio+attackVelocity,compressionRatio);
		else if(level<threshold && bouncingRatio!=1.0f)
			bouncingRatio= max(bouncingRatio-releaseVelocity,1.0f);

		if(bouncingRatio>1.0f)
		{
			float g=pow((float)threshold/(float)level,(bouncingRatio-1.0f)/bouncingRatio);
			//printf("%f=(%d/%d)**(%f/%f)\n",g,threshold,level,bouncingRatio-1.0,bouncingRatio);
			if(g>1.0f)
				g=1.0f;
			for(unsigned t=0;t<frameSize;t++)
				inputFrame[t]=(mix_sample_t)(inputFrame[t]*g);
		}
	}


	const unsigned getWindowTime() const
	{
		return windowTime; // the value this was constructed with
	}


private:
	const unsigned windowTime;
	const mix_sample_t threshold;
	const float compressionRatio;
	const float attackVelocity;
	const float releaseVelocity;

	// this is the compressionRatio affected by the attack and 
	// release times as the level goes above and below the threshold
	float bouncingRatio;

	CDSPRMSLevelDetector levelDetector;
};

#endif
