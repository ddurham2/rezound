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

#ifdef HAVE_LIBAUDIOFILE

#include "CSound.h"

#include <stdexcept>

#include "AFrontendHooks.h"
#include "AStatusComm.h"

CrawSoundTranslator::CrawSoundTranslator()
{
}

CrawSoundTranslator::~CrawSoundTranslator()
{
}

bool CrawSoundTranslator::onLoadSound(const string filename,CSound *sound) const
{
	AFfilesetup setup=afNewFileSetup();
	try
	{
		AFrontendHooks::RawParameters parameters;
		if(!gFrontendHooks->promptForRawParameters(parameters))
			return(false);

		afInitFileFormat(setup,AF_FILE_RAWDATA);

		afInitChannels(setup,AF_DEFAULT_TRACK,parameters.channelCount);

		afInitRate(setup,AF_DEFAULT_TRACK,parameters.sampleRate);

		switch(parameters.sampleFormat)
		{
		case AFrontendHooks::RawParameters::f8BitSignedPCM:
			afInitSampleFormat(setup,AF_DEFAULT_TRACK,AF_SAMPFMT_TWOSCOMP,8);
			break;
		case AFrontendHooks::RawParameters::f8BitUnsignedPCM:
			afInitSampleFormat(setup,AF_DEFAULT_TRACK,AF_SAMPFMT_UNSIGNED,8);
			break;
		case AFrontendHooks::RawParameters::f16BitSignedPCM:
			afInitSampleFormat(setup,AF_DEFAULT_TRACK,AF_SAMPFMT_TWOSCOMP,16);
			break;
		case AFrontendHooks::RawParameters::f16BitUnsignedPCM:
			afInitSampleFormat(setup,AF_DEFAULT_TRACK,AF_SAMPFMT_UNSIGNED,16);
			break;
		case AFrontendHooks::RawParameters::f24BitSignedPCM:
			afInitSampleFormat(setup,AF_DEFAULT_TRACK,AF_SAMPFMT_TWOSCOMP,24);
			break;
		case AFrontendHooks::RawParameters::f24BitUnsignedPCM:
			afInitSampleFormat(setup,AF_DEFAULT_TRACK,AF_SAMPFMT_UNSIGNED,24);
			break;
		case AFrontendHooks::RawParameters::f32BitSignedPCM:
			afInitSampleFormat(setup,AF_DEFAULT_TRACK,AF_SAMPFMT_TWOSCOMP,32);
			break;
		case AFrontendHooks::RawParameters::f32BitUnsignedPCM:
			afInitSampleFormat(setup,AF_DEFAULT_TRACK,AF_SAMPFMT_UNSIGNED,32);
			break;
		case AFrontendHooks::RawParameters::f32BitFloatPCM:
			afInitSampleFormat(setup,AF_DEFAULT_TRACK,AF_SAMPFMT_FLOAT,32);
			break;
		case AFrontendHooks::RawParameters::f64BitFloatPCM:
			afInitSampleFormat(setup,AF_DEFAULT_TRACK,AF_SAMPFMT_DOUBLE,64);
			break;
		default:
			throw runtime_error(string(__func__)+" -- unhandled parameters.sampleFormat: "+istring(parameters.sampleFormat));
		}

		afInitCompression(setup,AF_DEFAULT_TRACK,AF_COMPRESSION_NONE);		// ??? should be a parameter, quite a few are supported by libaudiofile (at least it says)
			//afInitInitCompressionParams(setup,AF_DEFAULT_TRACK, ... ); 		// ??? sould depend on the compression type... should interface with the frontend somehow

		afInitByteOrder(setup,AF_DEFAULT_TRACK,(parameters.endian==AFrontendHooks::RawParameters::eBigEndian) ? AF_BYTEORDER_BIGENDIAN : AF_BYTEORDER_LITTLEENDIAN);

#if (LIBAUDIOFILE_MAJOR_VERSION*10000+LIBAUDIOFILE_MINOR_VERSION*100+LIBAUDIOFILE_MICRO_VERSION) >= /*000204*/204
		afInitDataOffset(setup,AF_DEFAULT_TRACK,parameters.dataOffset);

		if(parameters.dataLength>0)
			afInitFrameCount(setup,AF_DEFAULT_TRACK,parameters.dataLength);
#else
		if(parameters.dataOffset>0 || parameters.dataLength>0)
			Warning("cannot set data offset or data length when loading raw because libaudiofile is less than version 0.2.4 -- upgrade libaudiofile (perhaps even to cvs) if you need this functionality");
#endif

		const bool ret=loadSoundGivenSetup(filename,sound,setup);

		afFreeFileSetup(setup);

		return ret;
	}
	catch(...)
	{
		afFreeFileSetup(setup);
		throw;
	}
}

bool CrawSoundTranslator::onSaveSound(const string filename,CSound *sound) const
{
	throw(runtime_error(string(__func__)+" -- unimplemented -- I need to create a frontend to allow the user to choose the export parameters"));
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

#endif // HAVE_LIBAUDIOFILE
