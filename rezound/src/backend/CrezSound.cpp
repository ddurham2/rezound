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

#include "CrezSound.h"

#include <stdexcept>

CrezSound::CrezSound() :
	ASound()
{
}

CrezSound::CrezSound(const string &_filename,const unsigned _sampleRate,const unsigned _channelCount,const sample_pos_t _size) :
	ASound(_filename,_sampleRate,_channelCount,_size)
{
#if 0
	sampleRate=_sampleRate;
	channelCount=_channelCount;
	size=_size;

/* done in createWorkingPoolFile
	if(channelCount>=MAX_CHANNELS)
		throw(runtime_error(string(__func__)+" -- invalid number of channels: "+istring(channelCount)));
*/
/*
	if(_size>MAX_LENGTH)
		throw(runtime_error(string(__func__)+" -- size is greater than MAX_LENGTH"));
*/

	const string tempFilename=_filename;

	try
	{
		// get a real temp file name
		remove(tempFilename.c_str());

		createWorkingPoolFile(tempFilename);
		filename=tempFilename;
	}
	catch(...)
	{
		try 
		{
			poolFile.closeFile(false,true);
		} catch(...) {}
		//remove(tempFilename);
		throw;
	}

	matchUpChannelLengths(NIL_SAMPLE_POS);
#endif
}

void CrezSound::loadSound(const string _filename)
{
	lockForResize();
	try
	{
		if(!createFromWorkingPoolFileIfExists(_filename))
		{
			ASound::PoolFile_t originalPoolFile(REZOUND_POOLFILE_BLOCKSIZE,REZOUND_POOLFILE_SIGNATURE);

			originalPoolFile.openFile(_filename,false);
			// ??? needs a status bar ??? perhaps you could pass a function object to it
			originalPoolFile.copyToFile(getWorkingFilename(_filename));
			originalPoolFile.closeFile(false,false);

			if(!createFromWorkingPoolFileIfExists(_filename,false))
				throw(runtime_error(string(__func__)+" -- internal error -- bad data in file or TPoolFile::copyToFile failed"));

			// undo the side-effect that createFromWrokingPoolFile set isModified to true
			_isModified=false;

		}
		unlockForResize();
	}
	catch(...)
	{
		unlockForResize();
		try
		{
			poolFile.closeFile(false,false);
		} catch(...) {}
		throw;
	}

	filename=_filename;
}

void CrezSound::saveSound(const string filename)
{
	lockSize();
	try
	{
		poolFile.flushData();
		poolFile.copyToFile(filename);
		unlockSize();
	}
	catch(...)
	{
		unlockSize();
		throw;
	}
}

