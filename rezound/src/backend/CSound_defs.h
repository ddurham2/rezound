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

#ifndef __CSound_defs_H__
#define __CSound_defs_H__

#include "../../config/common.h"

class CSound;

#include <stdint.h>

	// ??? rename to something more specific to audio, PCM, or rezound
	// create a MAX_LENGTH that I check for in the methods... I've made it, but I don't think I use it yet much
	// right now I made this lower only because I haven't made the frontend dialogs not show unnecessary check boxes and such...so the dialogs were huge even when many channels were not in use
	// if this is changed, also consider the array size of gJACKOutputPortNames and gJACKInputPortNames in settings.h/cpp (although it should be big enough)
#define MAX_CHANNELS 8



/* 
 * audio size specifications
 *
 *    sample_pos_t should always be unsigned since I use this assumption and never check for < 0 in the code
 *    sample_fpos_t should abe floating point, but big enough to hold all values of sample_pos_t
 */
#if 1 // 32 bit
	typedef uint32_t 	sample_pos_t;	// integral sample count
	typedef double		sample_fpos_t;	// floating-point sample count

	#define MAX_LENGTH (0x7fffffff-(1024*1024))

	#include <math.h>
	#define sample_fpos_floor(a)	(floor(a))
	#define sample_fpos_ceil(a)	(ceil(a))
	#define sample_fpos_round(a)	(nearbyint(a))
	#define sample_fpos_log(a)	(log(a))
	#define sample_fpos_exp(a)	(exp(a))
	#define sample_fpos_fabs(a)	(fabs(a))
	#define sample_fpos_sin(a)	(sin(a))
	#define sample_fpos_pow(a,b)	(pow(a,b))

#elif 0 // 64 bit
	typedef uint64_t 	sample_pos_t;	// integral sample position
	typedef long double 	sample_fpos_t;	// floating-point sample position

	#define MAX_LENGTH (0x7fffffffffffffff-(1024*1024))

	#include <math.h>
	#define sample_fpos_floor(a)	(floorl(a))
	#define sample_fpos_ceil(a)	(ceill(a))
	#define sample_fpos_round(a)	(nearbyintl(a))
	#define sample_fpos_log(a)	(logl(a))
	#define sample_fpos_exp(a)	(expl(a))
	#define sample_fpos_fabs(a)	(fabsl(a))
	#define sample_fpos_sin(a)	(sinl(a))
	#define sample_fpos_pow(a,b)	(powl(a,b))

#else
	#error please enable one section above
#endif

static const sample_pos_t NIL_SAMPLE_POS=~((sample_pos_t)0);



// ??? probably should add conversion macros which convert to and from several types of formats to and from the native format 
//	- this would help in importing and exporting audio data
//	- might deal with endian-ness to


// audio type specifications
#if defined(SAMPLE_TYPE_S16) // 16 bit PCM

	#define MAX_SAMPLE ((sample_t)32767)
	#define MIN_SAMPLE ((sample_t)-MAX_SAMPLE) // these should stay symetric so I don't have to handle the possibility of them being off center in the frontend rendering
	typedef int16_t sample_t;
	typedef int_fast32_t mix_sample_t; // this needs to hold at least a value of MAX_SAMPLE squared

	static mix_sample_t _SSS;
	#define ClipSample(s) ((sample_t)(_SSS=((mix_sample_t)(s)), _SSS>MAX_SAMPLE ? MAX_SAMPLE : ( _SSS<MIN_SAMPLE ? MIN_SAMPLE : _SSS ) ) )

#elif defined(SAMPLE_TYPE_FLOAT) // 32 bit floating point PCM
	#define MAX_SAMPLE (1.0f)
	#define MIN_SAMPLE (-MAX_SAMPLE) // these should stay symetric so I don't have to handle the possibility of them being off center in the frontend rendering
	typedef float sample_t;
	typedef float mix_sample_t;

	// empty implementation so that samples are not truncated until playback
	#define ClipSample(s) (s)

#else
	#error no SAMPLE_TYPE_xxx defined -- was supposed to be defined from the configure script
#endif



/***************************/
/* sample type conversions */
/***************************/

// ??? how would I handle 24bit? since I think it'd be represented with an int32_t;
// perhaps also have a bits integer template parameter that gets passed in

// It is necessary to call this template function with the template arguments because the
// compiler cannot infer the specialization to used since not all the template parameters
// types are used in the function's parameter list.

template<typename from_sample_t,typename to_sample_t> static inline const to_sample_t convert_sample(const register from_sample_t sample) { sample_type_conversion_for_this_combination_unimplemented; }

// int16_t -> int16_t
template<> static inline const int16_t convert_sample<int16_t,int16_t>(const register int16_t sample) { return sample; }

// int16_t -> float
template<> static inline const float convert_sample<int16_t,float>(const register int16_t sample) { return (float)sample/32767.0f; }

// int16_t -> double
template<> static inline const double convert_sample<int16_t,double>(const register int16_t sample) { return (double)sample/32767.0; }


// float -> int16_t
template<> static inline const int16_t convert_sample<float,int16_t>(const register float sample) { const int s=(int)(sample*32767.0f); return s>=32767 ? 32767 : (s<=-32767 ? -32767 : s); }

// float -> float
template<> static inline const float convert_sample<float,float>(const register float sample) { return sample; }

// float -> double
template<> static inline const double convert_sample<float,double>(const register float sample) { return (double)sample; }


// double -> int16_t
template<> static inline const int16_t convert_sample<double,int16_t>(const register double sample) { const int s=(int)(sample*32767.0); return s>=32767 ? 32767 : (s<=-32767 ? -32767 : s); }

// double -> float
template<> static inline const float convert_sample<double,float>(const register double sample) { return (float)sample; }

// double -> double
template<> static inline const double convert_sample<double,double>(const register double sample) { return sample; }

/**************************/







// used in CSound::mixSound()
enum MixMethods
{
    mmOverwrite,
    mmAdd,
    mmSubtract,
    mmMultiply,
    mmAverage 
};

// used in CSound::mixSound()
enum SourceFitTypes
{
	sftNone,
	sftChangeRate,
	sftChangeTempo
};

#endif
