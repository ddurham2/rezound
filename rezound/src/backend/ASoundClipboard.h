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

#ifndef __ASoundClipboard_H__
#define __ASoundClipboard_H__

#include "../../config/common.h"

class ASoundClipboard;

#include "CSound_defs.h"

class ASoundClipboard
{
public:
	virtual ~ASoundClipboard();

		// read data out of the CSound object and put onto the clipboard
	virtual void copyFrom(const CSound *sound,const sample_pos_t start,const sample_pos_t length)=0;

		// write data in to the CSound object from existing clipboard data
	virtual void copyTo(CSound *sound,const sample_pos_t start,const sample_pos_t length,MixMethods mixMethod)=0;

		// returns the amount of audio on the clipboard when resamled at the given sampleRate
	virtual sample_pos_t getLength(unsigned sampleRate) const=0;

		// returns true if there is no data on the clipboard
	virtual bool isEmpty() const=0;

		// returns true if copyFrom can be called
	virtual bool isReadOnly() const=0;

protected:
	ASoundClipboard();

};

#endif

