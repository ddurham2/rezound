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

#include "CRecordSoundClipboard.h"

#include <stdio.h> // for remove

#include <stdexcept>

#include <istring>

#include "AStatusComm.h"
#include "COSSSoundRecorder.h"
#include "AFrontendHooks.h"

typedef TPoolAccesser<sample_t,CSound::PoolFile_t > CClipboardPoolAccesser;

CRecordSoundClipboard::CRecordSoundClipboard(const string description,const string _workingFilename) :
	ASoundClipboard(description),
	workingFilename(_workingFilename),
	workingFile(NULL)
{
}

CRecordSoundClipboard::~CRecordSoundClipboard()
{
	delete workingFile;workingFile=NULL;
}

void CRecordSoundClipboard::copyFrom(const CSound *sound,const bool whichChannels[MAX_CHANNELS],sample_pos_t start,sample_pos_t length)
{
	throw(runtime_error(string(__func__)+" -- this clipboard is read-only"));
}

bool CRecordSoundClipboard::isReadOnly() const
{
	return(true);
}



bool CRecordSoundClipboard::prepareForCopyTo()
{
	if(!isEmpty())
	{
		VAnswer ans=Question("There is already data on this recording clipboard.  Do you want to record something new?",cancelQues);
		if(ans==noAns)
			return(true);
		else if(ans==cancelAns)
			return(false);
		//else go ahead and record
	}

	clearWhichChannels();
	delete workingFile;workingFile=NULL;

	// need to somehow choose an implementation ??? perhaps we should be constructed with an ASoundRecorder class
	COSSSoundRecorder recorder;

	try
	{
		unsigned channelCount;
		unsigned sampleRate;
		if(!gFrontendHooks->promptForNewSoundParameters(channelCount,sampleRate))
			return(false);

		remove(workingFilename.c_str());
		workingFile=new CSound(workingFilename,sampleRate,channelCount,1); // at least 1 sample is manditory
		this->sampleRate=sampleRate;

		recorder.initialize(workingFile);

		if(!gFrontendHooks->promptForRecord(&recorder))
		{
			recorder.deinitialize();
			delete workingFile;workingFile=NULL;
			return(false);
		}

		// recorded for each channel
		for(size_t t=0;t<channelCount;t++)
			whichChannels[t]=true;

		// ??? temporary until CSound can have zero lenth
		workingFile->lockForResize(); try { workingFile->removeSpace(0,1); workingFile->unlockForResize(); } catch(...) { workingFile->unlockForResize(); throw; }

		recorder.deinitialize();
	}
	catch(...)
	{
		delete workingFile;workingFile=NULL;
		recorder.deinitialize();
		throw;
	}

	return(true);
}

void CRecordSoundClipboard::copyTo(CSound *sound,unsigned destChannel,unsigned srcChannel,sample_pos_t start,sample_pos_t length,MixMethods mixMethod,bool invalidatePeakData)
{
	if(isEmpty())
		throw(runtime_error(string(__func__)+" -- this clipboard has not been recorded to yet"));

	if(destChannel>sound->getChannelCount())
		throw(runtime_error(string(__func__)+" -- destChannel (+"+istring(destChannel)+") out of range ("+istring(sound->getChannelCount())+")"));
	if(srcChannel>workingFile->getChannelCount())
		throw(runtime_error(string(__func__)+" -- srcChannel (+"+istring(srcChannel)+") out of range ("+istring(workingFile->getChannelCount())+")"));
	if(!whichChannels[srcChannel])
		throw(runtime_error(string(__func__)+" -- data does not exist in clipboard for srcChannel (+"+istring(srcChannel)+")"));
	if(length>getLength(sound->getSampleRate()))
		throw(runtime_error(string(__func__)+" -- length ("+istring(length)+")is greater than the amount of data in the clipboard ("+istring(getLength(sound->getSampleRate())+")")));


	// ??? I perhaps need to copy the cues also... as long as the names don't conflict

	const CRezPoolAccesser src=workingFile->getAudio(srcChannel);

	// ??? would need to handle sampleRate conversion... SHOULD: put this functionality in mixSound so everyone could benefit from it
	sound->mixSound(destChannel,start,src,0,sampleRate,length,mixMethod,invalidatePeakData);
}

sample_pos_t CRecordSoundClipboard::getLength(unsigned _sampleRate) const
{
	if(workingFile==NULL || workingFile->getLength()<=1)
		return(0);
	else
			// ??? probably want to divide first
		return((sample_pos_t)((sample_fpos_t)workingFile->getLength()*(sample_fpos_t)_sampleRate/(sample_fpos_t)workingFile->getSampleRate()));
}

bool CRecordSoundClipboard::isEmpty() const
{
	return(workingFile==NULL || workingFile->getLength()<=1);
}

