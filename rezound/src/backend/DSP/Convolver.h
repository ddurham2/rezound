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

#ifndef __DSP_Convolver_H__
#define __DSP_Convolver_H__

#include "../../config/common.h"

#include <stdexcept>

#include <istring>

#include "Delay.h"


/* --- TDSPConvolver ------------------------------------
 * 
 *	This class is a DSP block to do a sample by sample convolution of the given 
 *	array of coefficients with the input given by the repeated calls to processSample()
 *
 *	The frist template parameter specifies the type of the input samples (and is thus 
 *	the type also of the output, the return value of processSample() ).  And The second
 *	template parameter specifies the type of the coefficients.
 */
template<class sample_t,class coefficient_t> class TDSPConvolver
{
public:
	TDSPConvolver(const coefficient_t _coefficients[],size_t _coefficientCount) :
		coefficients(_coefficients),
		coefficientCount(_coefficientCount),
		coefficientCountSub1(_coefficientCount-1),
		delay(_coefficientCount)
	{
		if(coefficientCount<1)
			throw(runtime_error(string(__func__)+" -- invalid coefficientCount: "+istring(coefficientCount)));
	}

	virtual ~TDSPConvolver()
	{
	}

	const sample_t processSample(const sample_t input)
	{
		coefficient_t output=input*coefficients[0];
		for(unsigned t=coefficientCountSub1;t>0;t--)
			output+=delay.getSample(t)*coefficients[t];

		delay.putSample((coefficient_t)input);

		return((sample_t)output);
	}

private:
	const coefficient_t *coefficients; // aka, the impluse response
	const unsigned coefficientCount;
	const unsigned coefficientCountSub1;

	TDSPDelay<coefficient_t> delay;
};

#endif
