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

void ASoundRecorder::start()
{
	mutex.EnterMutex();
	try
	{
		if(!started)
		{
			started=true;

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

void ASoundRecorder::redo()
{
	// redo only resets the start position but doesn't cleanup extra space added
	// because we will need space right back again... So, the done() method removes
	// any extra allocated space beyond what was recorded that doesn't need to be
	// there

	mutex.EnterMutex();
	try
	{

		// in OSS, do I want to clear out the buffers already recorded??? Cause it may cause a hiccup in the input containig some of the already buffered data
		// 	I could do non-blocking reads while info.fragments is >0 or something
		// there may be a way to do this with OSS
		writePos=origLength;
		prealloced=sound->getLength()-origLength;

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

		sound->unlockForResize();
	}
	catch(...)
	{
		sound->unlockForResize();
		sound=NULL;
		throw;
	}

	
}


void ASoundRecorder::initialize(ASound *_sound)
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


void ASoundRecorder::onData(const sample_t *samples,const size_t sampleFramesRecorded)
{
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

string ASoundRecorder::getRecordedLength() const
{
	return(sound->getTimePosition(writePos-origLength));
}

string ASoundRecorder::getRecordedSize() const
{
	return(sound->getAudioDataSize(writePos-origLength));
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


