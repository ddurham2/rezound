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

	#define MAX_LENGTH (0x7fffffff-1024)

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

	#define MAX_LENGTH (0x7fffffffffffffff-1024)

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
//	- might deal with endian-ness too

// audio type specifications
#if 1 // 16 bit PCM

	#define MAX_SAMPLE ((sample_t)32767)
	#define MIN_SAMPLE ((sample_t)-MAX_SAMPLE) // these should stay symetric so I don't have to handle the possilbility of them being off center in the frontend rendering
	typedef int16_t sample_t;
	typedef int_fast32_t mix_sample_t; // this needs to hold at least a value of MAX_SAMPLE squared

#elif 0 // 32 bit floating point PCM (??? UNTESTED!)
	#define MAX_SAMPLE ((float)1.0)
	#define MIN_SAMPLE ((float)-MAX_SAMPLE) // these should stay symetric so I don't have to handle the possilbility of them being off center in the frontend rendering
	typedef float sample_t;
	typedef float mix_sample_t;

#else
	#error please enable one section above
#endif


static mix_sample_t _SSS;
#define ClipSample(s) ((sample_t)(_SSS=((mix_sample_t)(s)), _SSS>MAX_SAMPLE ? MAX_SAMPLE : ( _SSS<MIN_SAMPLE ? MIN_SAMPLE : _SSS ) ) )


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
	//sftChangeSpeed // without changing pitch (??? future)
};

#endif
