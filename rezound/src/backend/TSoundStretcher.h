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

#ifndef __TSoundStretcher_H__
#define __TSoundStretcher_H__

#include "CSound_defs.h"


template<class src_type> class TSoundStretcher
{
public:


	/*
	 * src is the data to read
	 * srcOffset is the first sample that this class will look at
  	 * srcLength is the length of samples beyond the srcOffset that should be read
	 * toLength is the amount of times that getSample() will be called
	 */
	TSoundStretcher(const src_type &_src,const sample_pos_t _srcOffset,const sample_pos_t _srcLength,const sample_pos_t _toLength) :
		src(_src),
		srcOffset(_srcOffset),
		srcLength(_srcLength>0 ? _srcLength-1 : _srcLength),
		toLength(_toLength),
		step(_srcLength/_toLength),

		pos(0)
	{
		if(_srcLength==_toLength)
		{
			pos=srcOffset;
			_getSample= &TSoundStretcher<src_type>::copy_GetSample;
		}
		else if((_srcLength%_toLength)==0)
		{
			pos=srcOffset;
			_getSample= &TSoundStretcher<src_type>::step_GetSample;
		}
		else if(_srcLength<=1)
			_getSample= &TSoundStretcher<src_type>::scale_GetSample;
		else
			_getSample= &TSoundStretcher<src_type>::linearInterpolate_GetSample;
	}

	const sample_t getSample() { return (this->*_getSample)(); }


private:
	const src_type src;
	const sample_pos_t srcOffset;
	const sample_fpos_t srcLength;
	const sample_fpos_t toLength;
	const sample_pos_t step;

	sample_pos_t pos;

	const sample_t copy_GetSample()
	{
		return(src[pos++]);
	}

	const sample_t step_GetSample()
	{
		const sample_t s=src[pos];
		pos+=step;
		return(s);
	}

	const sample_t scale_GetSample()
	{ // only used for border condition cases
		return(src[(sample_pos_t)(((sample_fpos_t)(pos++)/toLength*(srcLength+1.0))+srcOffset)]);
	}

	const sample_t linearInterpolate_GetSample()
	{
		const sample_fpos_t fPos=((sample_fpos_t)(pos++))/toLength*srcLength;
		const sample_pos_t iPos=(sample_pos_t)fPos;
		const float p2=fPos-iPos;
		const float p1=1.0-p2;
		return((sample_t)(p1*src[srcOffset+iPos]+p2*src[srcOffset+iPos+1]));
	}

	typedef const sample_t (TSoundStretcher<src_type>::*getSample_t)();
	getSample_t _getSample;

};

#endif
