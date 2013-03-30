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
 *    sample_fpos_t should be floating point, but big enough to hold all values of sample_pos_t
 */
#ifdef ENABLE_LARGEFILE

	// 64 bit
	typedef int64_t 	sample_pos_t;	// integer sample position
	typedef long double 	sample_fpos_t;	// floating-point sample position

	#define MAX_LENGTH (0x7fffffffffffffffLL-(1024LL*1024LL))

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

	// 32 bit
	typedef int32_t 	sample_pos_t;	// integer sample count
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

#endif

static const sample_pos_t NIL_SAMPLE_POS=~((sample_pos_t)0);


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

struct int24_t // int24_t -> unsigned char[3]
{
	uint8_t data[3]; 

	enum {			// indexes into data[] for the low, middle and high bytes
#ifdef WORDS_BIGENDIAN
		lo=2,
		mid=1,
		hi=0
#else
		lo=0,
		mid=1,
		hi=2
#endif
	};

	inline void set(const int_fast32_t v)
	{
		data[lo]=v&0xff;
		data[mid]=(v>>8)&0xff;
		data[hi]=(v>>16)&0xff;
	}

	inline int32_t get() const
	{
		if(data[hi]&0x80) // sign extension
			return (int_fast32_t)data[lo] | ((int_fast32_t)data[mid]<<8) | ((int_fast32_t)data[hi]<<16) | (0xff<<24);
		else
			return (int_fast32_t)data[lo] | ((int_fast32_t)data[mid]<<8) | ((int_fast32_t)data[hi]<<16);
	}
} 
#ifdef __GNUC__
	__attribute__ ((packed)) 
#endif
;

#include <math.h>

// It is necessary to call this template function with the template arguments because the
// compiler cannot infer the specialization to used since not all the template parameters
// types are used in the function's parameter list.
//
// ??? Some of the implementation may one day need conditional expressions to handle the 
// possibility that the negative range is one sample value larger than the positive range
	/* just going to have to get a linker error now with gcc>=3.4 */
template<typename from_sample_t,typename to_sample_t> static inline const to_sample_t convert_sample(const register from_sample_t sample) ;//{ sample_type_conversion_for_this_combination_unimplemented; }

// int8_t -> ...
	// int8_t -> int8_t
	template<> STATIC_TPL inline const int8_t convert_sample<int8_t,int8_t>(const register int8_t sample) { return sample; }

	// int8_t -> int16_t
	template<> STATIC_TPL inline const int16_t convert_sample<int8_t,int16_t>(const register int8_t sample) { return sample*256; }

	// int8_t -> int24_t
	//template<> static inline const int24_t convert_sample<int8_t,int24_t>(const register int8_t sample) { }

	// int8_t -> int32_t
	//template<> static inline const int32_t convert_sample<int8_t,int32_t>(const register int8_t sample) { }

	// int8_t -> float
	template<> STATIC_TPL inline const float convert_sample<int8_t,float>(const register int8_t sample) { return ((float)sample)/127.0f; }

	// int8_t -> double
	//template<> static inline const double convert_sample<int8_t,double>(const register int8_t sample) { }


// int16_t -> ...
	// int16_t -> int8_t
	template<> STATIC_TPL inline const int8_t convert_sample<int16_t,int8_t>(const register int16_t sample) { return sample/256; }

	// int16_t -> int16_t
	template<> STATIC_TPL inline const int16_t convert_sample<int16_t,int16_t>(const register int16_t sample) { return sample; }

	// int16_t -> int24_t
	template<> STATIC_TPL inline const int24_t convert_sample<int16_t,int24_t>(const register int16_t sample) { int24_t r; r.set(sample*256); return r; }

	// int16_t -> int32_t
	template<> STATIC_TPL inline const int32_t convert_sample<int16_t,int32_t>(const register int16_t sample) { return sample*65536; }

	// int16_t -> float
	template<> STATIC_TPL inline const float convert_sample<int16_t,float>(const register int16_t sample) { return (float)sample/32767.0f; }

	// int16_t -> double
	template<> STATIC_TPL inline const double convert_sample<int16_t,double>(const register int16_t sample) { return (double)sample/32767.0; }


// int24_t -> ...
	// int24_t -> int8_t
	//template<> static inline const int8_t convert_sample<int24_t,int8_t>(const int24_t sample) { }

	// int24_t -> int16_t
	template<> STATIC_TPL inline const int16_t convert_sample<int24_t,int16_t>(const int24_t sample) { return sample.get()>>8; }

	// int24_t -> int24_t
	template<> STATIC_TPL inline const int24_t convert_sample<int24_t,int24_t>(const int24_t sample) { return sample; }

	// int24_t -> int32_t
	//template<> static inline const int32_t convert_sample<int24_t,int32_t>(const int24_t sample) { }

	// int24_t -> float
	template<> STATIC_TPL inline const float convert_sample<int24_t,float>(const int24_t sample) { return sample.get()/8388607.0f; }

	// int24_t -> double
	//template<> static inline const double convert_sample<int24_t,double>(const int24_t sample) { }


// int32_t -> ...
	// int32_t -> int8_t
	//template<> static inline const int8_t convert_sample<int32_t,int8_t>(const register int32_t sample) { }

	// int32_t -> int16_t
	template<> STATIC_TPL inline const int16_t convert_sample<int32_t,int16_t>(const register int32_t sample) { return sample/65536; }

	// int32_t -> int24_t
	//template<> static inline const int24_t convert_sample<int32_t,int24_t>(const register int32_t sample) { }

	// int32_t -> int32_t
	template<> STATIC_TPL inline const int32_t convert_sample<int32_t,int32_t>(const register int32_t sample) { return sample; }

	// int32_t -> float
	template<> STATIC_TPL inline const float convert_sample<int32_t,float>(const register int32_t sample) { return ((double)sample)/2147483647.0; }

	// int32_t -> double
	//template<> static inline const double  convert_sample<int32_t,double >(const register int32_t sample) { }


// float -> ...
	// float -> int8_t
	template<> STATIC_TPL inline const int8_t convert_sample<float,int8_t>(const register float sample) { return (int8_t)floorf((sample>1.0f ? 1.0f : (sample<-1.0f ? -1.0f : sample))*127.0f); }

	// float -> int16_t
	template<> STATIC_TPL inline const int16_t convert_sample<float,int16_t>(const register float sample) { return (int16_t)floorf((sample>1.0f ? 1.0f : (sample<-1.0f ? -1.0f : sample))*32767.0f); }

	// float -> int24_t
	template<> STATIC_TPL inline const int24_t convert_sample<float,int24_t>(const register float sample) { int24_t r; r.set((int_fast32_t)floorf((sample>1.0f ? 1.0f : (sample<-1.0f ? -1.0f : sample))*8388607.0f)); return r; }

	// float -> int32_t
	template<> STATIC_TPL inline const int32_t convert_sample<float,int32_t>(const register float sample) { return (int32_t) floor((double)(sample>1.0f ? 1.0f : (sample<-1.0f ? -1.0f : sample)) * 2147483647.0); }

	// float -> float
	template<> STATIC_TPL inline const float convert_sample<float,float>(const register float sample) { return sample; }

	// float -> double
	template<> STATIC_TPL inline const double convert_sample<float,double>(const register float sample) { return (double)sample; }


// double -> ...
	// double -> int8_t
	//template<> static inline const int8_t convert_sample<double,int8_t>(const register double sample) { }

	// double -> int16_t
	template<> STATIC_TPL inline const int16_t convert_sample<double,int16_t>(const register double sample) { return (int16_t)floor((sample>1.0 ? 1.0 : (sample<-1.0 ? -1.0 : sample))*32767.0); }

	// double -> int24_t
	//template<> static inline const int24_t convert_sample<double,int24_t>(const register double sample) { }

	// double -> int32_t
	//template<> static inline const int32_t convert_sample<double,int32_t>(const register double sample) { }

	// double -> float
	template<> STATIC_TPL inline const float convert_sample<double,float>(const register double sample) { return (float)sample; }

	// double -> double
	template<> STATIC_TPL inline const double convert_sample<double,double>(const register double sample) { return sample; }







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

// used in .rez files
enum AudioEncodingTypes
{
	// let 0 be invalid
	aetPCMSigned16BitInteger=1,
	aetPCM32BitFloat=2,
	aetPCMSigned8BitInteger=3,
	aetPCMSigned24BitInteger=4,
	aetPCMSigned32BitInteger=5
};

enum Endians
{
	eLittleEndian=0,
	eBigEndian=1
};


#endif
