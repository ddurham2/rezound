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
#include <stdexcept>
#include <algorithm>
#include <istring>
#include "CSound_defs.h"

// volume
static inline const double scalar_to_dB(const double scalar) { return 20.0*log10(scalar); }
static inline const double dB_to_scalar(const double dB) { return pow(10.0,dB/20.0); }

static inline const mix_sample_t percent_to_amp(const double percent) { return (mix_sample_t)(percent*MAX_SAMPLE/100.0); }
									// ??? if I use this function should I abs the amp value?
static inline const double amp_to_percent(const mix_sample_t amp) { return (double)amp*100.0/(double)MAX_SAMPLE; }



template<class type> static inline const type dBFS_to_amp(const double dBFS,const type maxSample) { return (type)(maxSample*pow(10.0,dBFS/20.0)); }
static inline const mix_sample_t dBFS_to_amp(const double dBFS) { return (mix_sample_t)(MAX_SAMPLE*pow(10.0,dBFS/20.0)); }

template<class type> static inline const double amp_to_dBFS(const type amp,const type maxSample) { return 20.0*log10(fabs((double)amp/maxSample)); }
static inline const double amp_to_dBFS(const mix_sample_t amp) { return 20.0*log10(fabs((double)amp/MAX_SAMPLE)); }


// rate


// times
static inline const sample_pos_t ms_to_samples(const sample_fpos_t ms,const unsigned sampleRate) { return (sample_pos_t)sample_fpos_floor(((sample_fpos_t)sampleRate*ms/1000.0)+0.5); }
static inline const sample_pos_t s_to_samples(const sample_fpos_t s,const unsigned sampleRate) { return (sample_pos_t)sample_fpos_floor(((sample_fpos_t)sampleRate*s)+0.5); }
static inline const sample_fpos_t samples_to_ms(const sample_pos_t samples,const unsigned sampleRate) { return (sample_fpos_t)samples/(sample_fpos_t)sampleRate*1000.0; }
static inline const sample_fpos_t samples_to_s(const sample_pos_t samples,const unsigned sampleRate) { return (sample_fpos_t)samples/(sample_fpos_t)sampleRate; }

// sTime is in seconds
static inline const string seconds_to_string(const sample_fpos_t sTime,int secondsDecimalPlaces=0,bool includeUnits=false)
{
	string time;

	if(sTime>=3600)
	{ // make it HH:MM:SS.sss
		const int hours=(int)(sTime/3600);
		const int mins=(int)((sTime-(hours*3600))/60);
		const double secs=sTime-((hours*3600)+(mins*60));

		time=istring(hours,2,true)+":"+istring(mins,2,true)+":"+istring(secs,(secondsDecimalPlaces>0 ? 3+secondsDecimalPlaces : 2),secondsDecimalPlaces,true);
	}
	else
	{ // make it MM:SS.sss
		int mins=(int)(sTime/60);
		double secs=sTime-(mins*60);

		/* 
		 * if it's going to render (because of rounding in istring) as 3:60.000 
		 * then make that 4:00.000 which would happen if the seconds had come out 
		 * to 59.995 or more so that's (60 - .005) which is (60 - 5/(10^deciplaces))
		 * (this probably needs to be done slimiarly in the HH:MM:SS.sss case too)
		 */
		if(secs >= 60.0-(5.0/pow(10.0,secondsDecimalPlaces)))
		{
			mins++;
			secs=0;
		}

		time=istring(mins,2,true)+":"+istring(secs,(secondsDecimalPlaces>0 ? 3+secondsDecimalPlaces : 2),secondsDecimalPlaces,true);
	}

	if(includeUnits)
		return(time+"s");
	else
		return(time);
}


// angles
static inline const double degrees_to_radians(const double degrees) { return degrees*(2.0*M_PI)/360.0; }
static inline const double radians_to_degrees(const double radians) { return radians*360.0/(2.0*M_PI); }


// frequency
static inline const double freq_to_fraction(const double frequency,const unsigned sampleRate,bool throwOnError=false) { if(throwOnError && (frequency<0.0 || frequency>sampleRate/2)) {throw runtime_error(string(__func__)+" -- frequency out of range, "+istring(frequency)+", for sample rate of, "+istring(sampleRate)); } return max(0.0,min(0.5,frequency/(double)sampleRate)); }
static inline const double fraction_to_freq(const double fraction,const unsigned sampleRate,bool throwOnError=false) { if(throwOnError && (fraction<0.0 || fraction>0.5)) { throw runtime_error(string(__func__)+" -- fraction out of range, "+istring(fraction)); } return max(0.0,min(sampleRate/2.0,fraction*(double)sampleRate)); }

/* no longer used because I wanted to have evenly spaced octaves rather than going precisely from 0 to SR/2 
	// given x [0,1] this returns a frequency in Hz except curved for human interface use on the frontend
static inline const double unitRange_to_curvedFreq(const double x,const unsigned sampleRate) { return pow(x,3.0)*(sampleRate/2.0); }
static inline const double curvedFreq_to_unitRange(const double freq,const unsigned sampleRate) { return pow(2.0*freq/sampleRate,1.0/3.0); }
*/

	// maps a given (real) octave number [0,number of octaves] to a frequency in Hz where the octave 0 returns the given baseFrequency
static inline const double octave_to_freq(const double octave,const double baseFrequency) { return baseFrequency*pow(2.0,octave); }
static inline const double freq_to_octave(const double freq,const double baseFrequency) { return log(freq/baseFrequency)/log(2.0); }


// range conversions -- generally for user interface convenience

// [0,1] -> [0,1] patterned after f(x)=x^2
static inline const double unitRange_to_unitRange_squared(const double x) { return  pow(x,2.0); }
static inline const double unitRange_to_unitRange_unsquared(const double x) { return  sqrt(x); } // inverse of previous

	// NOTE: this is a little different than ..._squared ??? I'm not sure if it's working right it seems to be recip-sym
// [0,1] -> [0,1] with more accuracy near 0.5 patterned after f(x)=x^3
static inline const double unitRange_to_unitRange_cubed(const double x) { return (pow(2.0*x-1.0,3.0)+1.0)/2.0; }
static inline const double unitRange_to_unitRange_uncubed(const double x) { return (cbrt(2.0*x-1.0)+1.0)/2.0; } // inverse of previous

// [0,1] -> [1/a,a] with exponential behavior .. recipsym means reciprocally symetric as the range of the function is reciprocally symetric around 1
static inline const double unitRange_to_recipsymRange_exp(const double x,const double a) { return pow(a,2.0*x-1.0); }
static inline const double recipsymRange_to_unitRange_exp(const double x,const double a) { return 0.5*log(x)/log(a)+0.5; } // inverse of previous

// [0,1] -> [a,b] with linear behavior
static inline const double unitRange_to_otherRange_linear(const double x,const double a,const double b) { return a+((b-a)*x); }
static inline const double otherRange_to_unitRange_linear(const double x,const double a,const double b) { return (x-a)/(b-a); } // inverse of previous

#endif
