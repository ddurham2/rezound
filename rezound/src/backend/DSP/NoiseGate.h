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

#ifndef __DSP_NoiseGate_H__
#define __DSP_NoiseGate_H__

#include "../../config/common.h"

#include <algorithm>

#include "../CSound_defs.h"

#include "LevelDetector.h"

/* --- CDSPNoiseGate -----------------------------------------
 *
 * - This DSP block begins the mute the sound when the level of the audio drops
 *   below the given 'threshold'.  The rate at which it begins to mute the data is
 *   given by the 'gainAttackTime' and the rate at which it begins to come out of the
 *   mute once the level is again above the threshold is given by 'gainReleaseTime'.
 *   The level is detected with a moving average (performed by CDSPRMSLevelDetector) 
 *   the width of the moving window is given by 'windowTime'
 *
 * - To use simply construct the block with the desired parameters and repeatedly call 
 *   processSample() which returns the sample that was given but adjusted by the 
 *   calculated gain.
 *
 * - Note: it may be desirable to initialize the level detector within the algorithm
 *   by calling initSample() for 'windowTime' samples before calling processSample() 
 *   and using its return value.  This is because we don't really know the level before 
 *   enough sample have been observed.
 *
 * Info reference: http://www.harmony-central.com/Effects/Articles/Expansion/
 *
 */
class CDSPNoiseGate
{
public:
	// all times are in samples
	CDSPNoiseGate(unsigned _windowTime,sample_t _threshold,unsigned _gainAttackTime,unsigned _gainReleaseTime) :
		windowTime(_windowTime),
		threshold(_threshold),
		gainAttackVelocity(1.0/(float)_gainAttackTime),
		gainReleaseVelocity(1.0/(float)_gainReleaseTime),
	
		gain(1.0),
		levelDetector(windowTime)
	{
		// ??? verify parameters?
	}

	virtual ~CDSPNoiseGate()
	{
	}

	// using this is optional -- it is used to initialize the level detector -- see Note above
	void initSample(const mix_sample_t s)
	{
		levelDetector.readLevel(s);
	}

	const mix_sample_t processSample(const mix_sample_t s)
	{
		const mix_sample_t level=levelDetector.readLevel(s);
		
		if(level<=threshold && gain>0.0)
			gain= max(gain-gainAttackVelocity,(float)0.0);
		else if(level>threshold && gain<1.0)
			gain= min(gain+gainReleaseVelocity,(float)1.0);

			// ??? should be able to make this 3 return statements in the 2 cases+else above
		return(gain!=1.0 ? (mix_sample_t)(s*gain) : s);
	}


	// can be used to reset the internal gain if desired
	void resetGain(const float _gain=1.0)
	{
		gain=_gain;
	}

	const unsigned getWindowTime() const
	{
		return(windowTime); // the value this was constructed with
	}


private:
	const unsigned windowTime;
	const mix_sample_t threshold;
	const float gainAttackVelocity;
	const float gainReleaseVelocity;

	float gain;
	CDSPRMSLevelDetector levelDetector;
};


#endif
