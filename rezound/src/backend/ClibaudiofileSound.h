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

#ifndef __ClibaudiofileSound_H__
#define __ClibaudiofileSound_H__


#include "../../config/common.h"

#include "ASound.h"

class ClibaudiofileSound : public ASound
{
public:

	ClibaudiofileSound();
	ClibaudiofileSound(const string &_filename,const unsigned _sampleRate,const unsigned _channelCount,const sample_pos_t _size);

	virtual void loadSound(const string filename);
	virtual void saveSound(const string filename);

	// actually opens the file to determine the format
	static bool supportsFormat(const string filename);
	// only returns true if the given extension is one that it handles
	static bool handlesExtension(const string extension);

protected:

private:

};

#endif

