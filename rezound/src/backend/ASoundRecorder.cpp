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
#include "unit_conv.h"

#include <algorithm>

#define PREALLOC_SECONDS 1

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

void ASoundRecorder::start(const double _startThreshold,const sample_pos_t maxDuration)
{
	CMutexLocker l(mutex);
	if(!started)
	{
		if(_startThreshold>0)
			startThreshold=-1; // no threshold asked for
		else
			startThreshold=dBFS_to_amp(_startThreshold); // translate dBFS into an actual sample value

		started=true;
		if(maxDuration!=NIL_SAMPLE_POS && (MAX_LENGTH-writePos)>maxDuration)
			stopPosition=writePos+maxDuration;
		else
			stopPosition=NIL_SAMPLE_POS;
	}
}

void ASoundRecorder::stop()
{
	CMutexLocker l(mutex);
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

}

void ASoundRecorder::redo(const sample_pos_t maxDuration)
{
	// redo only resets the start position but doesn't cleanup extra space added
	// because we will need space right back again... So, the done() method removes
	// any extra allocated space beyond what was recorded that doesn't need to be
	// there

	CMutexLocker l(mutex);
	// clear all cues that were added during the last record
	addedCues.clear();

	writePos=origLength;
	prealloced=sound->getLength()-origLength;

	if(maxDuration!=NIL_SAMPLE_POS && (MAX_LENGTH-writePos)>maxDuration)
		stopPosition=writePos+maxDuration;
	else
		stopPosition=NIL_SAMPLE_POS;
}

void ASoundRecorder::done()
{
	// this method removes extra space in the sound beyond what was recorded since the last start()/stop()
	if(started)
		throw string(__func__)+" -- done should not be called while the recorder is start()-ed";

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

	for(unsigned i=0;i<MAX_CHANNELS;i++)
	{
		DCOffset[i]=0;
		DCOffsetSum[i]=0;
		DCOffsetCompensation[i]=0;
	}
	DCOffsetCount=0;
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


void ASoundRecorder::onData(sample_t *samples,const size_t _sampleFramesRecorded)
{
/* just to see the data if I need to
	{
	FILE *f=fopen("test.raw","a");
	fwrite(samples,1,_sampleFramesRecorded*2,f);
	fclose(f);
	}
*/

	size_t sampleFramesRecorded=_sampleFramesRecorded;
	CMutexLocker l(mutex);
	try
	{
		const unsigned channelCount=sound->getChannelCount();

		// modify samples by the DC Offset compensation
		for(unsigned i=0;i<channelCount;i++)
		{
			if(DCOffsetCompensation[i]!=0)
			{
				sample_t *_samples=samples+i;
				const mix_sample_t DCOffsetCompensation=this->DCOffsetCompensation[i];
				for(size_t t=0;t<sampleFramesRecorded;t++)
				{
					*_samples=ClipSample(*_samples+DCOffsetCompensation);
					_samples+=channelCount;
				}
			}
		}

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

		// calculate the DC offset of data being recorded
		for(unsigned i=0;i<channelCount;i++)
		{
			const sample_t *_samples=samples+i;
			double &DCOffsetSum=this->DCOffsetSum[i];
			for(size_t t=0;t<sampleFramesRecorded;t++)
			{
				DCOffsetSum+=*_samples;
				// next sample in interleaved format
				_samples+=channelCount;
			}
		}
		DCOffsetCount+=sampleFramesRecorded;

		if(DCOffsetCount>=(sound->getSampleRate()*5))
		{ // at least 5 second has been sampled, so record this as the current DCOffset and start over
			for(unsigned i=0;i<channelCount;i++)
			{
				DCOffset[i]=(sample_t)(DCOffsetSum[i]/DCOffsetCount);
				DCOffsetSum[i]=0;
			}
			DCOffsetCount=0;
		}


		statusTrigger.trip();

		if(started)
		{
			// if a startThreshold has been set, then ignore data even if started if no data is above the threshold
			if(startThreshold>=0)
			{
				for(unsigned i=0;i<channelCount;i++)
				{
					const sample_t *_samples=samples+i;
					for(size_t t=0;t<sampleFramesRecorded;t++)
					{
						sample_t s=*_samples;
						if(s<0)
							s=-s;
						if(s>=startThreshold)
						{
							// threshold met, now turn off the threshold check
							startThreshold=-1;

							// ignore data in chunk before the first sample that met the treshold
							samples+=(t*channelCount);
							sampleFramesRecorded-=t;

							goto goAheadAndSave; // using 'goto', because 'break' can't go past two loops
						}
						_samples+=channelCount;
					}

				}
				return;
			}

			goAheadAndSave:

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
				stop();
				return;
			}
		}
	}
	catch(...)
	{
		throw;
	}
}

unsigned ASoundRecorder::getChannelCount() const
{
	return sound->getChannelCount();
}

unsigned ASoundRecorder::getSampleRate() const
{
	return sound->getSampleRate();
}

bool ASoundRecorder::isStarted() const
{
	return started;
}

bool ASoundRecorder::isWaitingForThreshold() const
{
	return started && startThreshold>=0;
}

string ASoundRecorder::getRecordedLengthS() const
{
	return sound->getTimePosition(writePos-origLength);
}

const sample_pos_t ASoundRecorder::getRecordedLength() const
{
	return writePos-origLength;
}

string ASoundRecorder::getRecordedSizeS() const
{
	return sound->getAudioDataSize(writePos-origLength);
}

string ASoundRecorder::getRecordLimitS() const
{
	if(stopPosition!=NIL_SAMPLE_POS)
		return sound->getTimePosition(stopPosition-origLength);
	else
		return "memory";
}

bool ASoundRecorder::cueNameExists(const string name) const
{
	for(size_t t=0;t<addedCues.size();t++)
	{
		if(strcmp(addedCues[t].name,name.c_str())==0)
			return true;
	}
	return false;
}

void ASoundRecorder::addCueNow(const string name,bool isAnchored)
{
	if(sound->containsCue(name) || cueNameExists(istring(name).truncate(MAX_SOUND_CUE_NAME_LENGTH)))
		throw runtime_error(string(__func__)+_(" -- a cue already exist with the name")+": '"+name+"'");

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
	return p;
}

sample_t ASoundRecorder::getDCOffset(unsigned channel) const
{
	return DCOffset[channel];
}

void ASoundRecorder::compensateForDCOffset()
{
	// subtract current DC Offset output current DC Offset compensation 
	for(unsigned t=0;t<MAX_CHANNELS;t++)
		DCOffsetCompensation[t]+= -DCOffset[t];
}

void ASoundRecorder::setStatusTrigger(TriggerFunc triggerFunc,void *data)
{
	statusTrigger.set(triggerFunc,data);
}

void ASoundRecorder::removeStatusTrigger(TriggerFunc triggerFunc,void *data)
{
	statusTrigger.unset(triggerFunc,data);
}



// ------------------------

#include "COSSSoundRecorder.h"
#include "CPortAudioSoundRecorder.h"
#include "CJACKSoundRecorder.h"

#include "AStatusComm.h"

ASoundRecorder *ASoundRecorder::createInitializedSoundRecorder(CSound *sound)
{
	ASoundRecorder *soundRecorder=NULL;

#if defined(ENABLE_PORTAUDIO)
	soundRecorder=new CPortAudioSoundRecorder() ;
#elif defined(ENABLE_JACK)
	soundRecorder=new CJACKSoundRecorder();
#elif defined(ENABLE_OSS)
	soundRecorder=new COSSSoundRecorder();
#endif

	try
	{
		soundRecorder->initialize(sound);
		return soundRecorder;
	}
	catch(exception &e)
	{
		delete soundRecorder;

#if !defined(ENABLE_PORAUDIO) && !defined(ENABLE_JACK)
		// OSS was the only defined method
		throw;
#else
		// OSS was not the original method chosen at configure time so now fall back to using OSS if it wasn't disabled
	#ifdef ENABLE_OSS
		fprintf(stderr,"%s\n",(string(e.what())+"\nAttempting to fall back to using OSS for audio input.").c_str());
		//Warning(string(e.what())+"\nAttempting to fall back to using OSS for audio input.");

		// try OSS
		soundRecorder=new COSSSoundRecorder();
		try
		{
			soundRecorder->initialize(sound);
			return soundRecorder;
		}
		catch(exception &e)
		{ // now really give up
			delete soundRecorder;
			throw runtime_error(string("Error occurred after trying to fall back to OSS\n")+e.what());
		}
	#else
		throw;
	#endif
#endif
	}

}

