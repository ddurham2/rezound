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

ASoundRecorder::ASoundRecorder() :
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
	origLength=sound->getLength();
	writePos=origLength;
	started=true;
}

void ASoundRecorder::redo()
{
	// do I want to clear out the buffers already recorded??? Cause it may cause a hiccup in the input containig some of the already buffered data
	// there may be a way to do this with OSS
	sound->lockForResize();
	try
	{
		if(sound->getLength()>origLength)
			sound->removeSpace(origLength,sound->getLength()-origLength);
		writePos=origLength;
	}
	catch(...)
	{
		sound->unlockForResize();
		throw;
	}
}


void ASoundRecorder::initialize(ASound *_sound)
{
	started=false;
	sound=_sound;
	prealloced=0;
}


void ASoundRecorder::deinitialize()
{
	if(sound!=NULL)
	{
		started=false;
		sound->lockForResize();
		try
		{
			// ??? on error should I remove all recorded space back to the origLength?
			if(prealloced>0)
				// remove extra space
				sound->removeSpace(sound->getLength()-prealloced,prealloced);

			sound->unlockForResize();
			sound=NULL;
		}
		catch(...)
		{
			sound->unlockForResize();
			sound=NULL;
			throw;
		}
	}
}


void ASoundRecorder::onData(const sample_t *samples,const size_t sampleFramesRecorded)
{
	const unsigned channelCount=sound->getChannelCount();

	for(unsigned i=0;i<channelCount;i++)
	{
		sample_t maxSample=0;
		const sample_t *_samples=samples+i;
		for(size_t t=0;t<sampleFramesRecorded;t++)
		{
			sample_t s=*_samples;
			if(s<0)
				s=-s; // only use positive values
			maxSample=max(maxSample,s);
			_samples+=channelCount;
		}
		lastPeakValues[i]=(float)maxSample/(float)MAX_SAMPLE;
	}
	peakReadTrigger.trip();

	if(started)
	{
		// we preallocate space in the sound in 5 second chunks
		if(prealloced<sampleFramesRecorded)
		{
			sound->lockForResize();
			try
			{
				while(prealloced<sampleFramesRecorded)
				{
					sound->addSpace(sound->getLength(),5*sound->getSampleRate(),false);
					prealloced+=(5*sound->getSampleRate());
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
}

unsigned ASoundRecorder::getChannelCount() const
{
	return(sound->getChannelCount());
}

unsigned ASoundRecorder::getSampleRate() const
{
	return(sound->getSampleRate());
}

float ASoundRecorder::getLastPeakValue(unsigned channel) const
{
	return(lastPeakValues[channel]);
}

void ASoundRecorder::setPeakReadTrigger(TriggerFunc triggerFunc,void *data)
{
	peakReadTrigger.set(triggerFunc,data);
}

void ASoundRecorder::removePeakReadTrigger(TriggerFunc triggerFunc,void *data)
{
	peakReadTrigger.unset(triggerFunc,data);
}

