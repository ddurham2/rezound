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

#include "ASoundTranslator.h"
#include "CSound.h"
#include "AStatusComm.h"

#include <stdexcept>

vector<const ASoundTranslator *> ASoundTranslator::registeredTranslators;

ASoundTranslator::ASoundTranslator()
{
}

ASoundTranslator::~ASoundTranslator()
{
}

bool ASoundTranslator::loadSound(const string filename,CSound *sound) const
{
	sound->lockForResize();
	try
	{
		// use working file if it exists
		if(!sound->createFromWorkingPoolFileIfExists(filename))
		{
			if(!onLoadSound(filename,sound))
			{
				sound->unlockForResize();
				sound->closeSound(); // this removes the working poolfile
				return false; // cancelled
			}

			sound->setIsModified(false);
		}

		// ??? could check various other things
		if(!sound->poolFile.isOpen())
			throw(runtime_error(string(__func__)+" -- internal error -- no pool file was open after loading file"));

		sound->unlockForResize();
	}
	catch(...)
	{
		sound->unlockForResize();
		throw;
	}
	return true;
}

bool ASoundTranslator::saveSound(const string filename,const CSound *sound) const
{
	return saveSound(filename,sound,0,sound->getLength());
}

bool ASoundTranslator::saveSound(const string filename,const CSound *sound,const sample_pos_t saveStart,const sample_pos_t saveLength) const
{
	sound->lockSize();
	try
	{
		/*
		 * ??? A nice feature that would be good now that saving a file can be cancelled
		 * is for onSaveSound to be passed two files.. one to display in error msgs and on dialogs and such
		 * and another to actually write to.. then this code would do a move after everything was successful.
		 * The only problem might be running out of space... Perhaps if space is low, or there isn't
		 * enough for raw PCM size on that directory, then I should prompt the user
		 *
		 * Another solution is to rename the current file that exists and then do the same.. when the 
		 * save is successful, delete the original, or if the save is cancelled, then rename the original
		 * back again.
		 *
		 * ESPECIALLY since saving a file can be cancelled thus killing the original
		 */

		if(saveLength>sound->getLength() || (sound->getLength()-saveLength)<saveStart)
			throw runtime_error(string(__func__)+" -- invalid saveStart and saveLength range: from "+istring(saveStart)+" for "+istring(saveLength));

		bool ret=onSaveSound(filename,sound,saveStart,saveLength);
		sound->unlockSize();
		return ret;
	}
	catch(...)
	{
		sound->unlockSize();
		throw;
	}
}

// --- static methods --------------------------------------------

/* just a thought:
	This method could be given some abstract stream class pointer instead 
	of a filename which could access a file or a network URL.   Then the 
	translators would also have to be changed to read from that stream 
	instead of the file, and libaudiofile would at this point in time have 
	trouble doing that.
*/
#include <CPath.h>
const ASoundTranslator *ASoundTranslator::findTranslator(const string filename,bool isRaw)
{
	if(registeredTranslators.size()<=0)
		buildRegisteredTranslatorsVector();

	if(isRaw)
	{
		for(size_t t=0;t<ASoundTranslator::registeredTranslators.size();t++)
		{
			if(ASoundTranslator::registeredTranslators[t]->handlesRaw())
				return(ASoundTranslator::registeredTranslators[t]);
		}
	}

	CPath path(filename);
	if(path.exists() && path.isRegularFile())
	{ // try to determine from the contents of the file
		for(size_t t=0;t<ASoundTranslator::registeredTranslators.size();t++)
		{
			if(ASoundTranslator::registeredTranslators[t]->supportsFormat(filename))
				return(ASoundTranslator::registeredTranslators[t]);
		}
	}

	// file doesn't exist or no supported signature, so attempt to determine the translater based on the file extension
	const string extension=istring(CPath(filename).extension()).lower();
	for(size_t t=0;t<ASoundTranslator::registeredTranslators.size();t++)
	{
		if(ASoundTranslator::registeredTranslators[t]->handlesExtension(extension,filename))
			return(ASoundTranslator::registeredTranslators[t]);
	}

	// find the raw translator and ask the user if they want to use it
	for(size_t t=0;t<ASoundTranslator::registeredTranslators.size();t++)
	{
		if(ASoundTranslator::registeredTranslators[t]->handlesRaw())
		{
			if(Question("No handler found to support the format for "+filename+"\nWould you like to use a raw format?",yesnoQues)==yesAns)
				return(ASoundTranslator::registeredTranslators[t]);
			else
				break;
		}
	}
	throw(runtime_error(string(__func__)+" -- unhandled format/extension for the filename '"+filename+"'"));
}

const vector<const ASoundTranslator *> ASoundTranslator::getTranslators()
{
	if(registeredTranslators.size()<=0)
		buildRegisteredTranslatorsVector();
	return registeredTranslators;
}

const vector<string> ASoundTranslator::getFlatFormatList()
{
	vector<string> v;
	
	for(size_t t=0;t<registeredTranslators.size();t++)
	{
		const vector<string> formatNames=registeredTranslators[t]->getFormatNames();
		const vector<vector<string> > formatFileMasks=registeredTranslators[t]->getFormatFileMasks();
		for(size_t i=0;i<formatNames.size();i++)
		{
			for(size_t k=0;k<formatFileMasks[i].size();k++)
			{
				// only return formats with masks that are in the form: "*.ext" and trim off the *. in front
				// this is because existing code used to use this when only extensions were returned by
				// ASoundTranslator objects and not a generic filemask.  If I need to accomodate things
				// furthur later, then I will.
				if(formatFileMasks[i][k].substr(0,2)=="*.")
					v.push_back(formatFileMasks[i][k].substr(2)+" ["+formatNames[i]+"]");
			}
		}
	}

	return v;
}


#include "CrezSoundTranslator.h"
#include "ClibvorbisSoundTranslator.h"
#include "ClibaudiofileSoundTranslator.h"
#include "CMIDISDSSoundTranslator.h"
#include "ClameSoundTranslator.h"
#include "CvoxSoundTranslator.h"
#include "CrawSoundTranslator.h"
#include "Cold_rezSoundTranslator.h"
void ASoundTranslator::buildRegisteredTranslatorsVector()
{
	registeredTranslators.clear();

	static const CrezSoundTranslator rezSoundTranslator;
	registeredTranslators.push_back(&rezSoundTranslator);

#ifdef HAVE_LIBVORBIS
	static const ClibvorbisSoundTranslator libvorbisSoundTranslator;
	registeredTranslators.push_back(&libvorbisSoundTranslator);
#endif

#ifdef HAVE_LIBAUDIOFILE
	static const ClibaudiofileSoundTranslator libaudiofileSoundTranslator;
	registeredTranslators.push_back(&libaudiofileSoundTranslator);

	static const CrawSoundTranslator rawSoundTranslator;
	registeredTranslators.push_back(&rawSoundTranslator);
#endif

	static const CMIDISDSSoundTranslator MIDISDSSoundTranslator;
	registeredTranslators.push_back(&MIDISDSSoundTranslator);

	if(ClameSoundTranslator::checkForApp())
	{
		static const ClameSoundTranslator lameSoundTranslator;
		registeredTranslators.push_back(&lameSoundTranslator);
	}

	if(CvoxSoundTranslator::checkForApp())
	{
		static const CvoxSoundTranslator voxSoundTranslator;
		registeredTranslators.push_back(&voxSoundTranslator);
	}

	static const Cold_rezSoundTranslator old_rezSoundTranslator;
	registeredTranslators.push_back(&old_rezSoundTranslator);
}

