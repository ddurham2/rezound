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

#ifndef __DSP_FlangeEffect_H__
#define __DSP_FlangeEffect_H__

#include "../../config/common.h"

#include "../CSound_defs.h"
#include "../ALFO.h"

#include "Delay.h"

/* --- CFlangeEffect ------------------------------------
 *
 * - This DSP block produces a flange effect based on the parameters given to 
 *   the constructor.  
 *
 * - To use this DSP block, simply construct it with the parameters and call processSample()
 *   to given and get each successive sample.
 *
 * - Here is a diagram that shows how this block works:
	From http://www.harmony-central.com/Effects/Articles/Flanging/ (slightly modified)

                   feedback gain
                         /|
                --------< |<------- 
               |         \|        |
               |                   |
               |        -----      |
               |       | LFO |     |
               |        -----      |
               V          |        |    wet gain
  input       ---      -------     |       |\         ---    output
    -------->| + |--->| Delay |----------->| >------>| + |------>
         |    ---      -------             |/         ---
	 |                                             ^
         |                              dry gain       |
         |                                 |\          |
          -------------------------------->| >---------
                                           |/   
 *
 */
class CDSPFlangeEffect
{
public:
	// delayTime is in samples
	// LFODepth is in samples (the amount of samples delayed beyond delayTime when the LFO is at its max, 1.0)
	// The LFO object must have a range of [0,1] and nothing greater
	CDSPFlangeEffect(unsigned _delayTime,float _wetGain,float _dryGain,ALFO *_LFO,unsigned _LFODepth,float _feedback) :
		delay((unsigned)(_delayTime+_LFODepth+1)),
		delayTime(_delayTime),

		wetGain(_wetGain),
		dryGain(_dryGain),

		LFO(_LFO),

		LFODepth(_LFODepth),

		feedback(_feedback)
	{
		// ??? I could validate the parameters
	}

	virtual ~CDSPFlangeEffect()
	{
	}

	const mix_sample_t processSample(const mix_sample_t inputSample)
	{
		// calculate the delay time in samples from the LFO
		const float _delayTime=(delayTime+(LFODepth*LFO->nextValue()));

		// read a sample from the delay
		const mix_sample_t delayedSample=delay.getSample(_delayTime);

		// write to delay
		delay.putSample((mix_sample_t)(inputSample+(delayedSample*feedback)));

		return((mix_sample_t)((inputSample*dryGain)+(delayedSample*wetGain)));
	}

private:
	TDSPDelay<mix_sample_t> delay;
	const float delayTime;
	const float wetGain;
	const float dryGain;
	ALFO * const LFO;
	const float LFODepth;
	const float feedback;
};

#endif
