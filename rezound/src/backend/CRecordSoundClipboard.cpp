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
#include "AFrontendHooks.h"

#include "ASoundRecorder.h"
#include "ASoundPlayer.h" // to be able to call aboutToRecord and doneRecording

typedef TPoolAccesser<sample_t,CSound::PoolFile_t > CClipboardPoolAccesser;

CRecordSoundClipboard::CRecordSoundClipboard(const string description,const string _workingFilename,ASoundPlayer *_soundPlayer) :
	ASoundClipboard(description),
	workingFilename(_workingFilename),
	workingFile(NULL),
	soundPlayer(_soundPlayer),

	tempAudioPoolKey(0)
{
}

CRecordSoundClipboard::~CRecordSoundClipboard()
{
	delete workingFile;workingFile=NULL;
}

void CRecordSoundClipboard::copyFrom(const CSound *sound,const bool whichChannels[MAX_CHANNELS],sample_pos_t start,sample_pos_t length)
{
	throw runtime_error(string(__func__)+" -- this clipboard is read-only");
}

bool CRecordSoundClipboard::isReadOnly() const
{
	return true;
}


#include "CJACKSoundPlayer.h" // just for CJACKSoundPlayer::hack_sampleRate

bool CRecordSoundClipboard::prepareForCopyTo()
{
	// --- possibly cleanup from previous ----
	if(!isEmpty())
	{
		VAnswer ans=Question("There is already data on this recording clipboard.  Do you want to record something new?",cancelQues);
		if(ans==noAns)
			return true;
		else if(ans==cancelAns)
			return false;
		//else go ahead and record
	}

	clearWhichChannels();
	delete workingFile;workingFile=NULL;
	// ---------------------------------------


	ASoundRecorder *recorder=NULL;
	soundPlayer->aboutToRecord();
	try
	{
		string dummyFilename="";
		bool dummyRaw=false;
		unsigned channelCount;
		unsigned sampleRate;
		sample_pos_t dummyLength;

#ifdef ENABLE_JACK
		sampleRate=CJACKSoundPlayer::hack_sampleRate;
		if(!gFrontendHooks->promptForNewSoundParameters(dummyFilename,dummyRaw,true,channelCount,false,sampleRate,true,dummyLength,true))
		{
			soundPlayer->doneRecording();
			return false;
		}
#else
		if(!gFrontendHooks->promptForNewSoundParameters(dummyFilename,dummyRaw,true,channelCount,false,sampleRate,false,dummyLength,true))
		{
			soundPlayer->doneRecording();
			return false;
		}
#endif

		remove(workingFilename.c_str());
		workingFile=new CSound(workingFilename,sampleRate,channelCount,1); // at least 1 sample is manditory
		this->sampleRate=sampleRate;

		recorder=ASoundRecorder::createInitializedSoundRecorder(workingFile);

		if(!gFrontendHooks->promptForRecord(recorder))
		{
			delete recorder; recorder=NULL;
			delete workingFile;workingFile=NULL;
			soundPlayer->doneRecording();
			return false;
		}

		// recorded for each channel
		for(size_t t=0;t<channelCount;t++)
			whichChannels[t]=true;

		// ??? temporary until CSound can have zero lenth
		workingFile->lockForResize(); try { workingFile->removeSpace(0,1); workingFile->unlockForResize(); } catch(...) { workingFile->unlockForResize(); throw; }

		delete recorder; recorder=NULL;
		soundPlayer->doneRecording();
	}
	catch(...)
	{
		delete workingFile;workingFile=NULL;
		delete recorder; recorder=NULL;
		soundPlayer->doneRecording();
		throw;
	}

	return true;
}

void CRecordSoundClipboard::copyTo(CSound *sound,unsigned destChannel,unsigned srcChannel,sample_pos_t start,sample_pos_t length,MixMethods mixMethod,SourceFitTypes fitSrc,bool invalidatePeakData)
{
	if(isEmpty())
		throw runtime_error(string(__func__)+" -- this clipboard has not been recorded to yet");

	if(destChannel>sound->getChannelCount())
		throw runtime_error(string(__func__)+" -- destChannel (+"+istring(destChannel)+") out of range ("+istring(sound->getChannelCount())+")");
	if(srcChannel>workingFile->getChannelCount())
		throw runtime_error(string(__func__)+" -- srcChannel (+"+istring(srcChannel)+") out of range ("+istring(workingFile->getChannelCount())+")");
	if(!whichChannels[srcChannel])
		throw runtime_error(string(__func__)+" -- data does not exist in clipboard for srcChannel (+"+istring(srcChannel)+")");
	if(length>getLength(sound->getSampleRate()) && fitSrc==sftNone)
		throw runtime_error(string(__func__)+" -- length ("+istring(length)+")is greater than the amount of data in the clipboard ("+istring(getLength(sound->getSampleRate())+")"));


	// ??? I perhaps need to copy the cues also... as long as the names don't conflict

	const CRezPoolAccesser src=workingFile->getAudio(srcChannel);

	sound->mixSound(destChannel,start,src,0,sampleRate,length,mixMethod,fitSrc,invalidatePeakData,true);
}

sample_pos_t CRecordSoundClipboard::getLength(unsigned _sampleRate) const
{
	if(workingFile==NULL || workingFile->getLength()<=1)
		return 0;
	else
			// ??? probably want to divide first
		return (sample_pos_t)((sample_fpos_t)workingFile->getLength()*(sample_fpos_t)_sampleRate/(sample_fpos_t)workingFile->getSampleRate());
}

bool CRecordSoundClipboard::isEmpty() const
{
	return workingFile==NULL || (workingFile->getLength()<=1 && tempAudioPoolKey==0);
}

void CRecordSoundClipboard::temporarilyShortenLength(unsigned sampleRate,sample_pos_t changeTo)
{
	if(workingFile==NULL)
		return;

	workingFile->lockForResize();
	try
	{
		if(changeTo>getLength(sampleRate))
			throw runtime_error(string(__func__)+" -- changeTo is greater than the current length");
		if(changeTo==getLength(sampleRate))
			return;

		sample_pos_t newLength=(sample_pos_t)sample_fpos_floor((sample_fpos_t)changeTo/(sample_fpos_t)workingFile->getSampleRate()*(sample_fpos_t)sampleRate);
		origLength=workingFile->getLength();

		tempAudioPoolKey=workingFile->moveDataToTemp(whichChannels,newLength,workingFile->getLength()-newLength);

		workingFile->unlockForResize();
	}
	catch(...)
	{
		workingFile->unlockForResize();
		throw;
	}
	
}

void CRecordSoundClipboard::undoTemporaryShortenLength()
{
	if(workingFile==NULL)
		return;

	workingFile->lockForResize();
	try
	{
		if(tempAudioPoolKey!=0)
			workingFile->moveDataFromTemp(whichChannels,tempAudioPoolKey,workingFile->getLength(),origLength-workingFile->getLength());
		workingFile->unlockForResize();
	}
	catch(...)
	{
		workingFile->unlockForResize();
		throw;
	}
	tempAudioPoolKey=0;
}

unsigned CRecordSoundClipboard::getSampleRate() const
{
	return workingFile==NULL ? 0 : workingFile->getSampleRate();
}

unsigned CRecordSoundClipboard::getChannelCount() const
{
	return workingFile==NULL ? 0 : workingFile->getChannelCount();
}

