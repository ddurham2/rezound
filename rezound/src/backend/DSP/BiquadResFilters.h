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

#ifndef __DSP_BiquadResFilters_H__
#define __DSP_BiquadResFilters_H__

#include "../../config/common.h"

#include <math.h>

/* --- CDSPBiquadResFilter ----------------------
 *	Based on designs in Section 6 of "Game Programming 
 *	Gems 3"
 *
 *	The first template parameter specifies the type of the input samples (and is thus 
 *	the type also of the output, the return value of processSample() ).  And The second
 *	template parameter specifies the type of the coefficients used in the calculations.
 */
// ??? an version that uses an LFO would be nice.. and pretty easy... just recalc coefficients from the LFO each time
template<class sample_t,class coefficient_t> class TDSPBiquadResFilter
{
public:
	const sample_t processSample(const sample_t inputSample)
	{
		const coefficient_t ouputSample=a0*inputSample + a1*prevInputSample1 + a2*prevInputSample2 - b1*prevOutputSample1 - b2*prevOutputSample2;
		prevOutputSample2=prevOutputSample1;
		prevOutputSample1=ouputSample;

		prevInputSample2=prevInputSample1;
		prevInputSample1=inputSample;

		return((sample_t)ouputSample);
	}

protected:
	const coefficient_t omega,sin_omega,cos_omega,alpha,scalar;
	coefficient_t a0,a1,a2,b1,b2;

	// the cut-off frequency is expressed in a fraction of the sampling rate [0,0.5]
	TDSPBiquadResFilter(const coefficient_t cutoffFreqFraction,const coefficient_t resonance) :
		omega(2.0*M_PI*cutoffFreqFraction),
		sin_omega(sin(omega)),
		cos_omega(cos(omega)),
		alpha(sin_omega/(2.0*resonance)),
		scalar(1.0/(1.0+alpha)),

		prevInputSample1(0),
		prevInputSample2(0),
		prevOutputSample1(0),
		prevOutputSample2(0)
	{
	}

private:
	coefficient_t prevInputSample1;
	coefficient_t prevInputSample2;
	coefficient_t prevOutputSample1;
	coefficient_t prevOutputSample2;
};



template<class sample_t,class coefficient_t=float> class TDSPBiquadResLowpassFilter : public TDSPBiquadResFilter<sample_t,coefficient_t>
{
public:

	// the cut-off frequency is expressed in a fraction of the sampling rate [0,0.5]
	TDSPBiquadResLowpassFilter(const coefficient_t cutoffFreqFraction,const coefficient_t resonance) :
		TDSPBiquadResFilter<sample_t,coefficient_t>(cutoffFreqFraction,resonance)
	{
		this->a0=0.5*(1.0-this->cos_omega)*this->scalar;
		this->a1=(1.0-this->cos_omega)*this->scalar;
		this->a2=this->a0;
		this->b1=-2.0*this->cos_omega*this->scalar;
		this->b2=(1.0-this->alpha)*this->scalar;
	}

};

template<class sample_t,class coefficient_t=float> class TDSPBiquadResHighpassFilter : public TDSPBiquadResFilter<sample_t,coefficient_t>
{
public:

	// the cut-off frequency is expressed in a fraction of the sampling rate [0,0.5]
	TDSPBiquadResHighpassFilter(const coefficient_t cutoffFreqFraction,const coefficient_t resonance) :
		TDSPBiquadResFilter<sample_t,coefficient_t>(cutoffFreqFraction,resonance)
	{
		this->a0=0.5*(1.0+this->cos_omega)*this->scalar;
		this->a1=-(1.0+this->cos_omega)*this->scalar;
		this->a2=this->a0;
		this->b1=-2.0*this->cos_omega*this->scalar;
		this->b2=(1.0-this->alpha)*this->scalar;
	}

};

template<class sample_t,class coefficient_t=float> class TDSPBiquadResBandpassFilter : public TDSPBiquadResFilter<sample_t,coefficient_t>
{
public:

	// the cut-off frequency is expressed in a fraction of the sampling rate [0,0.5]
	TDSPBiquadResBandpassFilter(const coefficient_t cutoffFreqFraction,const coefficient_t resonance) :
		TDSPBiquadResFilter<sample_t,coefficient_t>(cutoffFreqFraction,resonance)
	{
		this->a0=this->alpha*this->scalar;
		this->a1=0.0;
		this->a2=-this->a0;
		this->b1=-2.0*this->cos_omega*this->scalar;
		this->b2=(1.0-this->alpha)*this->scalar;
	}

};

#endif
