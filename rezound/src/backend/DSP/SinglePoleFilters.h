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

#ifndef __DSP_SinglePoleFilters_H__
#define __DSP_SinglePoleFilters_H__

#include "../../config/common.h"

#include <math.h>

/* --- CDSPSinglePoleLowpassFilter ----------------------
 *	Based on designs in Chapter 19 of "The Scientist 
 *	and Engineer's Guide to Digital Signal Processing"
 *
 *	http://www.dspguide.com/
 *
 *	Basically a simple RC lowpass filter
 *
 *	The frist template parameter specifies the type of the input samples (and is thus 
 *	the type also of the output, the return value of processSample() ).  And The second
 *	template parameter specifies the type of the coefficients used in the calculations.
 */
// ??? an version that uses an LFO would be nice.. and pretty easy... just recalc a0 and b1 from the LFO each time
// ??? making prevSample variables might speed up computation since it wouldn't be converting back and forth float<==>integer
// ??? Equation 19-6 in the book shows 5 coefficients for a 4 stage lowpass filter as if DSP block had been done 4 times in series
template<class sample_t,class coefficient_t=float> class TDSPSinglePoleLowpassFilter
{
public:
	// the cut-off frequency is expressed in a fraction of the sampling rate [0,0.5]
	TDSPSinglePoleLowpassFilter(const coefficient_t cutoffFreqFraction) :
		b1(exp(-2.0*M_PI*cutoffFreqFraction)),
		a0(1.0-b1),
		prevSample(0)
	{
	}

	const sample_t processSample(const sample_t inputSample)
	{
		return(prevSample=(sample_t)(a0*inputSample+b1*prevSample));
	}

private:
	coefficient_t b1,a0;
	sample_t prevSample; // 1 sample delay basically
};





/* --- CDSPSinglePoleHighpassFilter ----------------------
 *	Based on designs in Chapter 19 of "The Scientist 
 *	and Engineer's Guide to Digital Signal Processing"
 *
 *	http://www.dspguide.com/
 *
 *	Basically a simple RC highpass filter
 *
 *	The frist template parameter specifies the type of the input samples (and is thus 
 *	the type also of the output, the return value of processSample() ).  And The second
 *	template parameter specifies the type of the coefficients used in the calculations.
 */
// ??? an version that uses an LFO would be nice.. and pretty easy... just recalc a0, a1 and b1 ... from the LFO each time
// ??? making prevSample variables might speed up computation since it wouldn't be converting back and forth float<==>integer
template<class sample_t,class coefficient_t=float> class TDSPSinglePoleHighpassFilter
{
public:
	// the cut-off frequency is expressed in a fraction of the sampling rate [0,0.5]
	TDSPSinglePoleHighpassFilter(const coefficient_t cutoffFreqFraction) :
		b1(exp(-2.0*M_PI*cutoffFreqFraction)),
		a0((1.0+b1)/2),
		a1(-(1.0+b1)/2),
		prevInputSample(0),
		prevOutputSample(0)
	{
	}

	const sample_t processSample(const sample_t inputSample)
	{
		prevOutputSample=(sample_t)(a0*inputSample+a1*prevInputSample+b1*prevOutputSample);
		prevInputSample=inputSample;
		return(prevOutputSample);
	}

private:
	coefficient_t b1,a0,a1;
	sample_t prevInputSample; // 1 sample delay basically
	sample_t prevOutputSample; // 1 sample delay basically
};





/* --- CDSPBandpassFilter --------------------------------
 *	Based on designs in Chapter 19 of "The Scientist 
 *	and Engineer's Guide to Digital Signal Processing"
 *
 *	http://www.dspguide.com/
 *
 *	The frist template parameter specifies the type of the input samples (and is thus 
 *	the type also of the output, the return value of processSample() ).  And The second
 *	template parameter specifies the type of the coefficients used in the calculations.
 *
 *	***NOTE*** -- this is coded exactly the same as TDSPNotchFilter except in the calculation of a1 and a2
 *		??? perhaps use inheritance
 */
// ??? an version that uses LFO2 would be nice.. and pretty easy... just recalc a0 and b1 ... from the LFOs each time
// ??? making prevSample variables might speed up computation since it wouldn't be converting back and forth float<==>integer
template<class sample_t,class coefficient_t=float> class TDSPBandpassFilter
{
public:
	// the band frequency is expressed in a fraction of the sampling rate [0,0.5] as is the band width
	TDSPBandpassFilter(const coefficient_t bandFreqFraction,const coefficient_t bandWidthFraction) :
		R(1.0-3.0*bandWidthFraction),
		K((1.0-2.0*R*cos(2.0*M_PI*bandFreqFraction)+R*R)/(2.0-2.0*cos(2.0*M_PI*bandFreqFraction))),
		a0(1.0-K),
		a1(2.0*(K-R)*cos(2.0*M_PI*bandFreqFraction)),
		a2(R*R-K),
		b1(2.0*R*cos(2.0*M_PI*bandFreqFraction)),
		b2(-(R*R)),
		prevInputSample1(0),
		prevInputSample2(0),
		prevOutputSample1(0),
		prevOutputSample2(0)
	{
	}

	const sample_t processSample(const sample_t inputSample)
	{
		const sample_t outputSample=(sample_t)(a0*inputSample+a1*prevInputSample1+a2*prevInputSample2+b1*prevOutputSample1+b2*prevOutputSample2);

		prevOutputSample2=prevOutputSample1;
		prevOutputSample1=outputSample;

		prevInputSample2=prevInputSample1;
		prevInputSample1=inputSample;

		return(outputSample);
	}

private:
	coefficient_t R,K;
	coefficient_t a0,a1,a2,b1,b2;
	sample_t prevInputSample1,prevInputSample2; // 2 sample delay basically
	sample_t prevOutputSample1,prevOutputSample2; // 2 sample delay basically
};





/* --- CDSPNotchFilter -----------------------------------
 *	Based on designs in Chapter 19 of "The Scientist 
 *	and Engineer's Guide to Digital Signal Processing"
 *
 *	http://www.dspguide.com/
 *
 *	The frist template parameter specifies the type of the input samples (and is thus 
 *	the type also of the output, the return value of processSample() ).  And The second
 *	template parameter specifies the type of the coefficients used in the calculations.
 *
 *	***NOTE*** -- this is coded exactly the same as TDSPBandpassFilter except in the calculation of a1 and a2
 *		??? perhaps use inheritance
 */
// ??? an version that uses LFO2 would be nice.. and pretty easy... just recalc a0 and b1 ... from the LFOs each time
// ??? making prevSample variables might speed up computation since it wouldn't be converting back and forth float<==>integer
template<class sample_t,class coefficient_t=float> class TDSPNotchFilter
{
public:
	// the band frequency is expressed in a fraction of the sampling rate [0,0.5] as is the band width
	TDSPNotchFilter(const coefficient_t bandFreqFraction,const coefficient_t bandWidthFraction) :
		R(1.0-3.0*bandWidthFraction),
		K((1.0-2.0*R*cos(2.0*M_PI*bandFreqFraction)+R*R)/(2.0-2.0*cos(2.0*M_PI*bandFreqFraction))),
		a0(K),
		a1(-2.0*K*cos(2.0*M_PI*bandFreqFraction)),
		a2(K),
		b1(2.0*R*cos(2.0*M_PI*bandFreqFraction)),
		b2(-(R*R)),
		prevInputSample1(0),
		prevInputSample2(0),
		prevOutputSample1(0),
		prevOutputSample2(0)
	{
	}

	const sample_t processSample(const sample_t inputSample)
	{
		const sample_t outputSample=(sample_t)(a0*inputSample+a1*prevInputSample1+a2*prevInputSample2+b1*prevOutputSample1+b2*prevOutputSample2);

		prevOutputSample2=prevOutputSample1;
		prevOutputSample1=outputSample;

		prevInputSample2=prevInputSample1;
		prevInputSample1=inputSample;

		return(outputSample);
	}

private:
	coefficient_t R,K;
	coefficient_t a0,a1,a2,b1,b2;
	sample_t prevInputSample1,prevInputSample2; // 2 sample delay basically
	sample_t prevOutputSample1,prevOutputSample2; // 2 sample delay basically
};

#endif
