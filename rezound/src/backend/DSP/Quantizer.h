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

#ifndef __DSP_Quantizer_H__
#define __DSP_Quantizer_H__

#include "../../config/common.h"

#include <math.h>

#include <stdexcept>

#include <istring>

/* --- TDSPQuantizer ------------------------------------
 *	This class is a DSP block to quantize the number of sample levels.
 *
 *	The parameter, quantumCount, is the number of levels to have above zero.
 *	This is thus produce the same number below zero.  And zero is considered
 *	a level as well.  So There the actual output is (quantumCount*2)+1 possible
 *	distinct sample values.
 *
 *	// ??? I could specialize this template for integer and float types separately to improve performance
 */
template<class sample_t,int maxSample> class TDSPQuantizer
{
public:
	TDSPQuantizer(const unsigned _quantumCount) :
		quantumCount(_quantumCount),
		fQuantumCount(_quantumCount),
		s(maxSample/fQuantumCount)
	{
		if(_quantumCount<1)
			throw(runtime_error(string(__func__)+" -- invalid quantumCount: "+istring(quantumCount)));
	}

	virtual ~TDSPQuantizer()
	{
	}

	const sample_t processSample(const sample_t input) const
	{
		return (sample_t)(floorf((float)input/(float)maxSample*quantumCount)*s);
	}

private:
	const float quantumCount;
	const float fQuantumCount;
	const float s;
};

#endif
