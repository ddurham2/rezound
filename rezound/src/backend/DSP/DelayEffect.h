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

#ifndef __DSP_DelayEffect_H__
#define __DSP_DelayEffect_H__

#include "../../config/common.h"

#include "../CSound_defs.h"

#include "Delay.h"


/* --- CDSPDelayEffect ----------------------------
 *
 * - This DSP block is used to create a delay effect or echoes.  It is different
 *   than the CDSPDelay block in that this one is a full fledged effect more than
 *   just a DSP component block.
 *
 * - The constructor is given the number of taps, tap times, tap gains, and tap feed backs
 * - To use this DSP block, simply construct it with the parameters and call processSample()
 *   to given and get each successive sample.
 * - The following is a diagram showing how this block works
	Based somewhat from http://www.harmony-central.com/Effects/Articles/Delay/
 
                             ---             ---             ---                
              --------------| + |<----------| + |<----------| + |<-- ... <------------
             |               ---             ---             ---                      |
             |                ^               ^               ^                       | 
             |         tap 1  |        tap 2  |        tap 3  |                tap N  | 
             |       feedback/_\     feedback/_\     feedback/_\             feedback/_\
             v                |               |               |                       |
  input     ---     -------   |     -------   |     -------   |             -------   |
    ------>| + |-->| Delay |--+--->| Delay |--+--->| Delay |--+----> ... ->| Delay |--+
       |    ---     -------   |     -------   |     -------   |             -------   |
       |                     _|_             _|_             _|_                     _|_
       |                tap 1\_/        tap 2\_/        tap 3\_/     ...        tap N\_/
       |                 gain |          gain |          gain |                  gain |
       |                      v               v               v                       v
       |                     ---             ---             ---                     ---     output
        ------------------->| + |---------->| + |---------->| + |--> ... ---------->| + |------>
                             ---             ---             ---                     ---  

 *
 */
class CDSPDelayEffect
{
public:
		// ??? convert these to vectors of floats and also change the way action parameters are passed
		// actually convert tapTimes to a vector of unsigned
	CDSPDelayEffect(size_t _tapCount,float *_tapTimes,float *_tapGains,float *_tapFeedbacks) :
		tapCount(_tapCount),

		tapTimes(_tapTimes),
		tapGains(_tapGains),
		tapFeedbacks(_tapFeedbacks),

		delays(new TDSPDelay<mix_sample_t>[_tapCount])
	{
		// perhaps fill with data from the past not zero for initial delay time number of samples
		for(size_t t=0;t<tapCount;t++)
			delays[t].setDelayTime((unsigned)tapTimes[t]);
	}

	virtual ~CDSPDelayEffect()
	{
		delete [] delays;
	}

	const mix_sample_t processSample(const mix_sample_t inputSample)
	{
		mix_sample_t fedbackSample=inputSample;
		mix_sample_t outputSample=inputSample;
		
		int k=tapCount-1;

		// start at the last tap and propogate backwards (in the diagram)
		const mix_sample_t s=delays[k].getSample();
		outputSample+=(mix_sample_t)(s*tapGains[k]);
		fedbackSample+=(mix_sample_t)(s*tapFeedbacks[k]);
		k--;
		
		for(;k>=0;k--)
		{
			const mix_sample_t s=delays[k].getSample();
			outputSample+=(mix_sample_t)(s*tapGains[k]);
			fedbackSample+=(mix_sample_t)(s*tapFeedbacks[k]);
			delays[k+1].putSample(s);
		}

		// write to delay 
		delays[0].putSample(fedbackSample);

		return(outputSample);
	}

private:
	const size_t tapCount;

	const float *tapTimes; // in samples
	const float *tapGains; // ??? change to unsigned
	const float *tapFeedbacks;

	TDSPDelay<mix_sample_t> *delays;
};

#endif
