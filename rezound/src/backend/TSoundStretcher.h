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
		srcLength(_srcLength),
		toLength(_toLength),
		pos(0)
	{
	}

// ??? when I would copy 1 sample from a lower sample rate and paste into a higher this seemed to return bogus

	const sample_t getSample()
	{
		// ??? temporary simple implementation
		return(src[(sample_pos_t)(((sample_fpos_t)(pos++)*(sample_fpos_t)srcLength/(sample_fpos_t)toLength)+srcOffset)]);
	}

private:
	const src_type src;
	const sample_pos_t srcOffset;
	const sample_pos_t srcLength;
	const sample_pos_t toLength;

	sample_pos_t pos;

};

#endif
