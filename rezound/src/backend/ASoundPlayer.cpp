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

#include "ASoundPlayer.h"

#include <stdexcept>

#include "CSound.h"
#include "CSoundPlayerChannel.h"

#include "settings.h"

#include "unit_conv.h"

ASoundPlayer::ASoundPlayer()
{
}

ASoundPlayer::~ASoundPlayer()
{
}

void ASoundPlayer::initialize()
{
	for(unsigned t=0;t<MAX_CHANNELS;t++)
	{
		maxRMSLevels[t]=peakLevels[t]=0;
		resetMaxRMSLevels[t]=resetPeakLevels[t]=false;
	}

	// only handling the first device ???
	for(unsigned t=0;t<devices[0].channelCount;t++)
		RMSLevelDetectors[t].setWindowTime(ms_to_samples(gMeterRMSWindowTime,devices[0].sampleRate));
}

void ASoundPlayer::deinitialize()
{
	stopAll();
}

CSoundPlayerChannel *ASoundPlayer::newSoundPlayerChannel(CSound *sound)
{
	return(new CSoundPlayerChannel(this,sound));
}

void ASoundPlayer::addSoundPlayerChannel(CSoundPlayerChannel *soundPlayerChannel)
{
	if(!soundPlayerChannels.insert(soundPlayerChannel).second)
		throw(runtime_error(string(__func__)+" -- sound player channel already in list"));
}

void ASoundPlayer::removeSoundPlayerChannel(CSoundPlayerChannel *soundPlayerChannel)
{
	soundPlayerChannel->stop();
	
	set<CSoundPlayerChannel *>::const_iterator i=soundPlayerChannels.find(soundPlayerChannel);
	if(i!=soundPlayerChannels.end())
		soundPlayerChannels.erase(i);
}

void ASoundPlayer::mixSoundPlayerChannels(const unsigned nChannels,sample_t * const buffer,const size_t bufferSize)
{
	memset(buffer,0,bufferSize*sizeof(*buffer)*nChannels);

	// ??? it might be nice that if no sound player channel object is playing that this method would not return
	// so that the caller wouldn't eat any CPU time doing anything with the silence returned

	for(set<CSoundPlayerChannel *>::iterator i=soundPlayerChannels.begin();i!=soundPlayerChannels.end();i++)
		(*i)->mixOntoBuffer(nChannels,buffer,bufferSize);


	// calculate the peak levels and max RMS levels for this chunk
	for(unsigned i=0;i<nChannels;i++)
	{
		size_t p=i;
		sample_t peak=resetPeakLevels[i] ? 0 : peakLevels[i];
		resetPeakLevels[i]=false;
		sample_t maxRMSLevel=resetMaxRMSLevels[i] ? 0 : maxRMSLevels[i];
		resetMaxRMSLevels[i]=false;
		for(size_t t=0;t<bufferSize;t++)
		{
			// m = max(m,abs(buffer[p]);
			sample_t s=buffer[p];

			s= s<0 ? -s : s; // s=abs(s)

			// peak=max(peak,s)
			if(peak<s)
				peak=s;

			// update the RMS level detectors
			sample_t RMSLevel=RMSLevelDetectors[i].readLevel(s);

			// RMSLevel=max(maxRMSLevel,RMSLevel)
			if(maxRMSLevel<RMSLevel)
				maxRMSLevel=RMSLevel;

			p+=nChannels;
		}
	
		maxRMSLevels[i]=maxRMSLevel;
		peakLevels[i]=peak;
	}
	for(unsigned i=nChannels;i<MAX_CHANNELS;i++)
		peakLevels[i]=0;
}

void ASoundPlayer::stopAll()
{
	for(set<CSoundPlayerChannel *>::iterator i=soundPlayerChannels.begin();i!=soundPlayerChannels.end();i++)
		(*i)->stop();
}

const sample_t ASoundPlayer::getRMSLevel(unsigned channel) const
{
	//return RMSLevelDetectors[channel].readCurrentLevel();
	const sample_t r=maxRMSLevels[channel];
	resetMaxRMSLevels[channel]=true;
	return r;
}

const sample_t ASoundPlayer::getPeakLevel(unsigned channel) const
{
	const sample_t p=peakLevels[channel];
	resetPeakLevels[channel]=true;
	return p;
}


