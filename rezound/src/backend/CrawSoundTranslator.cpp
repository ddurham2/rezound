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

#include "CrawSoundTranslator.h"
#include "CSound.h"

#include <stdexcept>

/*
	This code still needs help in that it needs
	to know the parameters of the sound.  This is 
	a problem that I will evenutally have to 
	solve because other Translator objects need to
	know parameters when saving, i.e. compress 
	parameters.  So when I determine a solution to
	this problem in general, I'll have load and save
	be able to receive these parameters.
*/

CrawSoundTranslator::CrawSoundTranslator()
{
}

CrawSoundTranslator::~CrawSoundTranslator()
{
}

void CrawSoundTranslator::onLoadSound(const string filename,CSound *sound) const
{
	AFfilesetup setup=afNewFileSetup();
	try
	{
		// ??? I should be able to do this now with frontend hooks
		afInitFileFormat(setup,AF_FILE_RAWDATA);
		afInitByteOrder(setup,AF_DEFAULT_TRACK,AF_BYTEORDER_LITTLEENDIAN); 	// ??? should be a parameter
		afInitChannels(setup,AF_DEFAULT_TRACK,2);				// ??? should be a parameter
		afInitSampleFormat(setup,AF_DEFAULT_TRACK,AF_SAMPFMT_TWOSCOMP,16);	// ??? should be a parameter
		afInitRate(setup,AF_DEFAULT_TRACK,44100);				// ??? should be a parameter need to make sure that this doesn't do any conversion, all it should do is get returned by afGetRate in loadSoundGivenSetup 
		afInitCompression(setup,AF_DEFAULT_TRACK,AF_COMPRESSION_NONE);		// ??? should be a parameter, quite a few are supported by libaudiofile (at least it says)
			//afInitInitCompressionParams(setup,AF_DEFAULT_TRACK, ... ); 		// ??? sould depend on the compression type... should interface with the frontend somehow
				// ??? looks like a bug in libaudiofile... when I made this something non-zero it still reported that there were filesize/framesize number of frams to read which shouldn't be the case, it should subtract the data-offset from the filesize when determining how many samples should be available
		afInitDataOffset(setup,AF_DEFAULT_TRACK,0);				// ??? should be a parameter
		//afInitFrameCount(setup,AF_DEFAULT_TRACK, 12345 ); 			// ??? should be a parameter, I assume this can be used to limit the amount of data to read from the file

		loadSoundGivenSetup(filename,sound,setup);

		afFreeFileSetup(setup);
	}
	catch(...)
	{
		afFreeFileSetup(setup);
		throw;
	}
}

void CrawSoundTranslator::onSaveSound(const string filename,CSound *sound) const
{
	throw(runtime_error(string(__func__)+" -- unimplemented"));
}


bool CrawSoundTranslator::handlesExtension(const string extension) const
{
	return(extension=="raw");
}

bool CrawSoundTranslator::supportsFormat(const string filename) const
{
	return(false); // to keep it from catching all formats
}

const vector<string> CrawSoundTranslator::getFormatNames() const
{
	vector<string> names;

	names.push_back("Raw PCM Data");

	return(names);
}

const vector<vector<string> > CrawSoundTranslator::getFormatExtensions() const
{
	vector<vector<string> > list;

	vector<string> extensions;
	extensions.push_back("raw");
	list.push_back(extensions);

	return(list);
}
