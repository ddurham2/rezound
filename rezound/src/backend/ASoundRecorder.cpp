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


ASoundRecorder::ASoundRecorder() :
	sound(NULL)
{
}

ASoundRecorder::~ASoundRecorder()
{
}

void ASoundRecorder::onInit(ASound *_sound)
{
	started=false;
	sound=_sound;
	prealloced=0;
}

void ASoundRecorder::onStart()
{
	origLength=sound->getLength();
	writePos=origLength;
	started=true;
}

void ASoundRecorder::onData(const sample_t *samples,const size_t sampleFramesRecorded)
{
	// need to find peak value for each channel ???
	// and give public methods to get the most recent peak value per channel
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

		const unsigned channelCount=sound->getChannelCount();
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

void ASoundRecorder::onStop(bool error)
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
	}
	catch(...)
	{
		sound->unlockForResize();
		throw;
	}
}

void ASoundRecorder::onRedo()
{
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

unsigned ASoundRecorder::getChannelCount() const
{
	return(sound->getChannelCount());
}

unsigned ASoundRecorder::getSampleRate() const
{
	return(sound->getSampleRate());
}

void ASoundRecorder::setPeakReadTrigger(TriggerFunc triggerFunc,void *data)
{
	peakReadTrigger.set(triggerFunc,data);
}

