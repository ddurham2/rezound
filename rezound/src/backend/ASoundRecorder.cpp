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

#include "ASoundRecorder.h"

#include "AStatusComm.h"

#include <algorithm>

#define PREALLOC_SECONDS 5

ASoundRecorder::ASoundRecorder() :
	clipCount(0),
	sound(NULL)
{
	for(unsigned i=0;i<MAX_CHANNELS;i++)
		lastPeakValues[i]=0.0;
}

ASoundRecorder::~ASoundRecorder()
{
}

void ASoundRecorder::start(const sample_pos_t maxDuration)
{
	mutex.EnterMutex();
	try
	{
		if(!started)
		{
			started=true;
			if(maxDuration!=NIL_SAMPLE_POS && (MAX_LENGTH-writePos)>maxDuration)
				stopPosition=writePos+maxDuration;
			else
				stopPosition=NIL_SAMPLE_POS;
		}

		mutex.LeaveMutex();
	}
	catch(...)
	{
		mutex.LeaveMutex();
		throw;
	}
}

void ASoundRecorder::stop()
{
	mutex.EnterMutex();
	try
	{
		if(started)
		{
			started=false;

			// remove extra prealloceded space
			sound->lockForResize();
			try
			{
				if(prealloced>0)
					// remove extra space
					sound->removeSpace(sound->getLength()-prealloced,prealloced);
				prealloced=0;

				sound->unlockForResize();
			}
			catch(...)
			{
				sound->unlockForResize();
				throw;
			}
		}

		mutex.LeaveMutex();
	}
	catch(...)
	{
		mutex.LeaveMutex();
		throw;
	}
}

void ASoundRecorder::redo(const sample_pos_t maxDuration)
{
	// redo only resets the start position but doesn't cleanup extra space added
	// because we will need space right back again... So, the done() method removes
	// any extra allocated space beyond what was recorded that doesn't need to be
	// there

	mutex.EnterMutex();
	try
	{
		// clear all cues that were added during the last record
		addedCues.clear();

		writePos=origLength;
		prealloced=sound->getLength()-origLength;

		if(maxDuration!=NIL_SAMPLE_POS && (MAX_LENGTH-writePos)>maxDuration)
			stopPosition=writePos+maxDuration;
		else
			stopPosition=NIL_SAMPLE_POS;

		mutex.LeaveMutex();
	}
	catch(...)
	{
		mutex.LeaveMutex();
		throw;
	}
}

void ASoundRecorder::done()
{
	// this method removes extra space in the sound beyond what was recorded since the last start()/stop()
	if(started)
		throw(string(__func__)+" -- done should not be called while the recorder is start()-ed");

	sound->lockForResize();
	try
	{
		if(sound->getLength()>writePos)
			// remove extra space
			sound->removeSpace(writePos,sound->getLength()-writePos);

		else // since TPoolFile::insertSpace didn't do it all along, we need to do it now (but removeSpace would do it)
			sound->poolFile.backupSAT();

		try
		{
			for(size_t t=0;t<addedCues.size();t++)
				sound->addCue(addedCues[t].name,addedCues[t].time,addedCues[t].isAnchored);
		}
		catch(exception &e)
		{	
			Error(e.what());
		}
		

		sound->unlockForResize();
	}
	catch(...)
	{
		sound->unlockForResize();
		sound=NULL;
		throw;
	}

}


void ASoundRecorder::initialize(CSound *_sound)
{
	started=false;
	sound=_sound;
	clipCount=0;

	/*
	// insert 15 mins worth of space (for testing as if recording has been going for 15 mins)
	sound->lockForResize();
	sound->addSpace(0,44100*15*60,true);
	sound->unlockForResize();
	*/

	prealloced=0;
	origLength=sound->getLength();
	writePos=origLength; // ??? - 1?
}


void ASoundRecorder::deinitialize()
{
	if(sound!=NULL)
	{
		if(started)
			stop();

		done();
		sound=NULL;
		clipCount=0;
	}
}


void ASoundRecorder::onData(const sample_t *samples,const size_t _sampleFramesRecorded)
{
	size_t sampleFramesRecorded=_sampleFramesRecorded;
	mutex.EnterMutex();
	try
	{
		const unsigned channelCount=sound->getChannelCount();

		// give realtime peak data updates
		for(unsigned i=0;i<channelCount;i++)
		{
			mix_sample_t maxSample=(mix_sample_t)(lastPeakValues[i]*MAX_SAMPLE);
			const sample_t *_samples=samples+i;
			for(size_t t=0;t<sampleFramesRecorded;t++)
			{
				mix_sample_t s=*_samples;

				if(s>=MAX_SAMPLE || s<=MIN_SAMPLE)
					clipCount++;

				if(s<0)
					s=-s; // only use positive values

				maxSample=max(maxSample,s);

				// next sample in interleaved format
				_samples+=channelCount;
			}
			lastPeakValues[i]=(float)maxSample/(float)MAX_SAMPLE;
		}
		statusTrigger.trip();

		if(started)
		{
			// we preallocate space in the sound in PREALLOC_SECONDS second chunks
			if(prealloced<sampleFramesRecorded)
			{
				sound->lockForResize();
				try
				{
					while(prealloced<sampleFramesRecorded)
					{
						sound->addSpace(sound->getLength(),PREALLOC_SECONDS*sound->getSampleRate(),false);
						prealloced+=(PREALLOC_SECONDS*sound->getSampleRate());
					}
					sound->unlockForResize();
				}
				catch(...)
				{
					sound->unlockForResize();
					throw;
				}
			}

			if(stopPosition!=NIL_SAMPLE_POS && (this->writePos+sampleFramesRecorded)>stopPosition)
				sampleFramesRecorded=stopPosition-this->writePos;

			sample_pos_t writePos=0;
			for(unsigned i=0;i<channelCount;i++)
			{
				CRezPoolAccesser a=sound->getAudio(i);
				const sample_t *_samples=samples+i;
				writePos=this->writePos;
				for(size_t t=0;t<sampleFramesRecorded;t++)
				{
					a[writePos++]=*_samples;
					_samples+=channelCount;
				}

			}

			prealloced-=sampleFramesRecorded;
			this->writePos=writePos;

			if(stopPosition!=NIL_SAMPLE_POS && writePos>=stopPosition)
			{
				mutex.LeaveMutex();
				stop();
				return;
			}
		}
		mutex.LeaveMutex();
	}
	catch(...)
	{
		mutex.LeaveMutex();
		throw;
	}
}

unsigned ASoundRecorder::getChannelCount() const
{
	return(sound->getChannelCount());
}

unsigned ASoundRecorder::getSampleRate() const
{
	return(sound->getSampleRate());
}

bool ASoundRecorder::isStarted() const
{
	return(started);
}

string ASoundRecorder::getRecordedLengthS() const
{
	return(sound->getTimePosition(writePos-origLength));
}

const sample_pos_t ASoundRecorder::getRecordedLength() const
{
	return(writePos-origLength);
}

string ASoundRecorder::getRecordedSizeS() const
{
	return(sound->getAudioDataSize(writePos-origLength));
}

string ASoundRecorder::getRecordLimitS() const
{
	if(stopPosition!=NIL_SAMPLE_POS)
		return(sound->getTimePosition(stopPosition-origLength));
	else
		return("memory");
}

bool ASoundRecorder::cueNameExists(const string name) const
{
	for(size_t t=0;t<addedCues.size();t++)
	{
		if(strcmp(addedCues[t].name,name.c_str())==0)
			return(true);
	}
	return(false);
}

void ASoundRecorder::addCueNow(const string name,bool isAnchored)
{
	if(sound->containsCue(name) || cueNameExists(istring(name).truncate(MAX_SOUND_CUE_NAME_LENGTH)))
		throw(runtime_error(string(__func__)+" -- a cue already exist with the name: '"+name+"'"));

	// this actually adds the cue at the last 20th of a second or so
	// depending on the how often the derived class is invoking onData
	addedCues.push_back(CSound::RCue(istring(name).truncate(MAX_SOUND_CUE_NAME_LENGTH).c_str(),origLength+writePos,isAnchored));
}

/*
 * The get-and-reset idea ensures that we get the peak (maximum) value since
 * the last time we called this method.  That way if a timer is only calling
 * this every .25 seconds and the peak is recalculated every .1 seconds the
 * recalculation doesn't overwrite the value, it just uses the existing peak
 * value which is reset each time we ask for it.
 */
float ASoundRecorder::getAndResetLastPeakValue(unsigned channel)
{
	float p=lastPeakValues[channel];
	lastPeakValues[channel]=0.0;
	return(p);
}

void ASoundRecorder::setStatusTrigger(TriggerFunc triggerFunc,void *data)
{
	statusTrigger.set(triggerFunc,data);
}

void ASoundRecorder::removeStatusTrigger(TriggerFunc triggerFunc,void *data)
{
	statusTrigger.unset(triggerFunc,data);
}


