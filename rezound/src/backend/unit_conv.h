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

#ifndef __unit_conv_h__
#define __unit_conv_h__


#include "../../config/common.h"

/*
 * Unit conversion functions
 */

#include <math.h>
#include "CSound_defs.h"

// volume
static const double scalar_to_dB(const double scalar) { return(10.0*log10(scalar)); }
static const double dB_to_scalar(const double dB) { return(pow(10.0,dB/10.0)); }

static const mix_sample_t percent_to_amp(const double percent) { return((mix_sample_t)floor((percent*MAX_SAMPLE/100.0)+0.5)); }
									// ??? if I use this function should I abs the amp value?
static const double amp_to_percent(const mix_sample_t amp) { return((double)amp*100.0/(double)MAX_SAMPLE); }


// rate


// times
static const sample_pos_t ms_to_samples(const sample_fpos_t ms,const unsigned sampleRate) { return((sample_pos_t)sample_fpos_floor(((sample_fpos_t)sampleRate*ms/1000.0)+0.5)); }
static const sample_pos_t s_to_samples(const sample_fpos_t s,const unsigned sampleRate) { return((sample_pos_t)sample_fpos_floor(((sample_fpos_t)sampleRate*s)+0.5)); }
static const sample_fpos_t samples_to_ms(const sample_pos_t samples,const unsigned sampleRate) { return((sample_fpos_t)samples/(sample_fpos_t)sampleRate*1000.0); }
static const sample_fpos_t samples_to_s(const sample_pos_t samples,const unsigned sampleRate) { return((sample_fpos_t)samples/(sample_fpos_t)sampleRate); }


// angles
static const double degrees_to_radians(const double degrees) { return(degrees*(2.0*M_PI)/360.0); }
static const double radians_to_degrees(const double radians) { return(radians*360.0/(2.0*M_PI)); }



// range conversions -- generally for user interface convenience

// [0,1] -> [0,1] with more accuracy near 0.5 patterned after f(x)=x^3
static const double unitRange_to_unitRange_cubed(const double x) { return( (pow(2.0*x-1.0,3.0)+1.0)/2.0 ); }
static const double unitRange_to_unitRange_uncubed(const double x) { return( (cbrt(2.0*x-1.0)+1.0)/2.0 ); } // inverse of previous

// [0,1] -> [1/a,a] with exponential behavior 
static const double unitRange_to_bipolarRange_exp(const double x,const double a) { return( pow(a,2.0*x-1.0) ); }
static const double bipolarRange_to_unitRange_exp(const double x,const double a) { return( 0.5*log(x)/log(a)+0.5 ); } // inverse of previous

// [0,1] -> [a,b] with linear behavior
static const double unitRange_to_otherRange_linear(const double x,const double a,const double b) { return( a+((b-a)*x)); }
static const double otherRange_to_unitRange_linear(const double x,const double a,const double b) { return( (x-a)/(b-a) ); } // inverse of previous

#endif
