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

#ifndef __DSP_SoundStretcher_H__
#define __DSP_SoundStretcher_H__

#include "../../config/common.h"

#include "../CSound_defs.h"

template<class src_type> class TSoundStretcher
{
public:


	/*
	 * src is the data to read
	 * srcOffset is the first sample that this class will look at
  	 * srcLength is the length of samples beyond the srcOffset that should be read
	 * toLength is the amount of times that getSample() will be called
	 * frameSize can be passed if the src is actually interlaced data of more than 1 channel of audio
	 * frameOffset should be passed when frameSize is given other than one to say which channel in the interlaced data should be processed
	 * paddedForInterpolation can be passed as true to tell TSoundStretcher not to worry about overrunning the src index since the caller has padded at the end for possible sample interpolation (currently only 1 extra sample frame would ever be needed)
	 */
	TSoundStretcher(const src_type &_src,const sample_fpos_t _srcOffset,const sample_fpos_t _srcLength,const sample_fpos_t _toLength,unsigned _frameSize=1,unsigned _frameOffset=0,bool paddedEndForInterpolation=false) :
		src(_src),
		srcOffset(_srcOffset),
		srcLength((_srcLength>0 && !paddedEndForInterpolation) ? _srcLength-1 : _srcLength),
		toLength(_toLength),
		frameSize(_frameSize),
		frameOffset(_frameOffset),
		step((int)floor(_srcLength/_toLength)),

		pos(0)
	{
		if(frameSize==0)
			throw(runtime_error(string(__func__)+" -- frameSize is 0"));
		if(frameOffset>=frameSize)
			throw(runtime_error(string(__func__)+" -- frameOffset is >= frameSize: "+istring(frameOffset)+">="+istring(frameSize)));

		// determine which actual implementation we can use based on the given parameters
		// use less expensive scaling methods when possible, else use 2-point sample interpolation
		if(srcLength==floor(srcLength) && toLength==floor(toLength))
		{ // both srcLength and toLength are integers
			if(srcOffset==floor(srcOffset) && _srcLength==_toLength)
			{ // srcOffset is an integer and the scaling is 1.0
				pos=(long)floor(srcOffset);
				if(frameSize==1)
					_getSample= &TSoundStretcher<src_type>::copy_frameSize1_GetSample;
				else
					_getSample= &TSoundStretcher<src_type>::copy_frameSizeN_GetSample;
				_getCurrentSrcPosition= &TSoundStretcher<src_type>::copy_GetCurrentSrcPosition;
			}
			else if(srcOffset==floor(srcOffset) && ((long)floor(_srcLength)%(long)floor(_toLength))==0)
			{ // srcOffset is an integer and srcLength is an integral multiple of toLength
				pos=(long)floor(srcOffset);
				if(frameSize==1)
					_getSample= &TSoundStretcher<src_type>::step_frameSize1_GetSample;
				else
					_getSample= &TSoundStretcher<src_type>::step_frameSizeN_GetSample;
				_getCurrentSrcPosition= &TSoundStretcher<src_type>::step_GetCurrentSrcPosition;
			}
			else if(_srcLength<=1)
			{
				if(frameSize==1)
					_getSample= &TSoundStretcher<src_type>::scale_frameSize1_GetSample;
				else
					_getSample= &TSoundStretcher<src_type>::scale_frameSizeN_GetSample;
				_getCurrentSrcPosition= &TSoundStretcher<src_type>::scale_GetCurrentSrcPosition;
			}
			else
			{
				if(frameSize==1)
					_getSample= &TSoundStretcher<src_type>::linearInterpolate_frameSize1_GetSample;
				else
					_getSample= &TSoundStretcher<src_type>::linearInterpolate_frameSizeN_GetSample;
				_getCurrentSrcPosition= &TSoundStretcher<src_type>::linearInterpolate_GetCurrentSrcPosition;
			}
		}
		else
		{
			if(frameSize==1)
				_getSample= &TSoundStretcher<src_type>::linearInterpolate_frameSize1_GetSample;
			else
				_getSample= &TSoundStretcher<src_type>::linearInterpolate_frameSizeN_GetSample;
			_getCurrentSrcPosition= &TSoundStretcher<src_type>::linearInterpolate_GetCurrentSrcPosition;
		}
	}

	const sample_t getSample() { return (this->*_getSample)(); }

	// useful for the next time you create a TSoundStretcher if TSoundStretcher is being used multiple times for chunk divided data
	const sample_fpos_t getCurrentSrcPosition() const { return (this->*_getCurrentSrcPosition)(); }

private:
	const src_type src;
	const sample_fpos_t srcOffset;
	const sample_fpos_t srcLength;
	const sample_fpos_t toLength;
	const sample_pos_t frameSize;
	const sample_pos_t frameOffset;
	const sample_pos_t step;

	sample_pos_t pos;

	// --- copy ----------------------------
	const sample_t copy_frameSize1_GetSample()
	{
		return(src[pos++]);
	}
	const sample_t copy_frameSizeN_GetSample()
	{
		return(src[(pos++)*frameSize+frameOffset]);
	}
	const sample_fpos_t copy_GetCurrentSrcPosition() const
	{
		return pos;
	}


	// --- step ----------------------------
	const sample_t step_frameSize1_GetSample()
	{
		const sample_t s=src[pos];
		pos+=step;
		return(s);
	}
	const sample_t step_frameSizeN_GetSample()
	{
		const sample_t s=src[pos*frameSize+frameOffset];
		pos+=step;
		return(s);
	}
	const sample_fpos_t step_GetCurrentSrcPosition() const
	{
		return pos;
	}


	// --- scale ---------------------------
	const sample_t scale_frameSize1_GetSample()
	{ // only used for border condition cases
		return(src[(sample_pos_t)(((sample_fpos_t)(pos++)/toLength*(srcLength+1.0))+srcOffset)]);
	}
	const sample_t scale_frameSizeN_GetSample()
	{ // only used for border condition cases
		return(src[((sample_pos_t)(((sample_fpos_t)(pos++)/toLength*(srcLength+1.0))+srcOffset))*frameSize+frameOffset]);
	}
	const sample_fpos_t scale_GetCurrentSrcPosition() const
	{
		return ((sample_fpos_t)(pos)/toLength*(srcLength+1.0))+srcOffset;
	}


	// --- linearInterpolate ---------------
	const sample_t linearInterpolate_frameSize1_GetSample()
	{
		const sample_fpos_t fPos=(((sample_fpos_t)(pos++))/toLength*(srcLength))+srcOffset;
		const sample_pos_t iPos=(sample_pos_t)fPos;
		const float p2=fPos-iPos;
		const float p1=1.0-p2;
		return((sample_t)(p1*src[iPos]+p2*src[iPos+1]));
	}
	const sample_t linearInterpolate_frameSizeN_GetSample()
	{
		const sample_fpos_t fPos=(((sample_fpos_t)(pos++))/toLength*(srcLength))+srcOffset;
		const sample_pos_t iPos=(sample_pos_t)fPos;
		const float p2=fPos-iPos;
		const float p1=1.0-p2;
		const sample_pos_t pos=iPos*frameSize+frameOffset;
		return((sample_t)(p1*src[pos]+p2*src[pos+frameSize]));
	}
	const sample_fpos_t linearInterpolate_GetCurrentSrcPosition() const
	{
		return (((sample_fpos_t)pos)/toLength*(srcLength))+srcOffset;
	}




	typedef const sample_t (TSoundStretcher<src_type>::*getSample_t)();
	getSample_t _getSample;

	typedef const sample_fpos_t (TSoundStretcher<src_type>::*getCurrentSrcPosition_t)() const;
	getCurrentSrcPosition_t _getCurrentSrcPosition;
};

#endif
