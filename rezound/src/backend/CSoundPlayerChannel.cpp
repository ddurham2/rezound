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

#include "CSoundPlayerChannel.h"

#include <math.h>

#include <stdexcept>
#include <string>

#include <istring>

/* TODO
 * - Provisions have been made in here for supporting multiple simultaneous output devices, 
 *   however, ASoundPlayer doesn't haven't this notion yet.  When it does, it should pass
 *   mixOntoChannels multiple buffers, one for each device, and multiple nChannel values.
 * - I must do it this way so that all the envelope and play positioning can be done knowing
 *   the last iteration's results so it can save the new values in the data members.
 *
 * - Well, I made the provisions for assigning routes, but I did it in such a way that each
 *   channel in a sound file gets assigned to only 1 device and channel.  This sounds fine, 
 *   except, a loaded mono sound can only play out of one speaker... Perhaps I could handle
 *   them separately or alter the route defining structure to be more of a list of destiations
 *   instead of 1 value.
 */

CSoundPlayerChannel::CSoundPlayerChannel(ASoundPlayer *_player,ASound *_sound) :
	sound(_sound),
	player(_player),
	playing(false),
	playSpeed(1.0)
{
	init();
	createInitialOutputRoute();
}

CSoundPlayerChannel::CSoundPlayerChannel(const CSoundPlayerChannel &src) :
	sound(src.sound),
	player(src.player),
	playing(false),
	playSpeed(1.0)
{
	throw(runtime_error(string(__func__)+" -- should this ever be called???"));
	//init();
	//copyData(src);
}

CSoundPlayerChannel::~CSoundPlayerChannel()
{
	deinit();
}

ASound *CSoundPlayerChannel::getSound() const
{
	return(sound);
}

CSoundPlayerChannel &CSoundPlayerChannel::operator=(const CSoundPlayerChannel &src)
{
	throw(runtime_error(string(__func__)+" -- should this ever be called???"));
/*
	if(&player!=&(src.player))
	{
		deinit();
		player=src.player;
		init();
		copyData(src);
	}
	return(*this);
*/
}

void CSoundPlayerChannel::init()
{
	player->addSoundPlayerChannel(this);
	playing=false;
	envelope.init();
	playPosition=0;
	volume=1.0;
	pan=0.0;
	calcVolumeScalars();
	startPosition=stopPosition=0;
	outputRoute=0;

	for(size_t t=0;t<MAX_CHANNELS;t++)
		muted[t]=false;
}

void CSoundPlayerChannel::deinit()
{
	kill();
	player->removeSoundPlayerChannel(this);
}

/*
void CSoundPlayerChannel::copyData(const CSoundPlayerChannel &src)
{
	// copy necessary data values only
}
*/

void CSoundPlayerChannel::play(bool _playLooped,bool _playSelectionOnly,CEnvelope &_envelope)
{
	// mutex???
	if(!playing)
	{
		playLooped=_playLooped;
		playSelectionOnly=_playSelectionOnly;
		if(playSelectionOnly)
			playPosition=startPosition;
		else
			playPosition=0;
	
		envelope=_envelope;

		// Setup envelope
		envelope.releasing=false;
		envelope.attacking=envelope.attackFactor!=0;
		envelope.envelopeCount=0;

		paused=false;
		playing=true;
		playTrigger.trip();
		pauseTrigger.trip();
	}
}

void CSoundPlayerChannel::pause()
{
	if(playing)
	{
		paused=!paused;
		pauseTrigger.trip();
	}
}

void CSoundPlayerChannel::stop()
{
	// if MUTEXes are added here, then address it in ASoundPlayer::mixChannels, when a loop point is reached and not supposed to loop, but are supposed to stop
	if(playing)
	{
		lock();
		try
		{
			if(!paused)
			{
				if(!envelope.attacking) // If it's not attacking
					envelope.envelopeCount=envelope.releaseFactor;
				else // it is attacking
				{
					float ratio=(float)envelope.attackFactor/envelope.releaseFactor;
					// Don't worry, ratio cannot be zero, because it wouldn't be attacking if attackFactor were 0
					envelope.envelopeCount=(int)((float)envelope.envelopeCount/ratio);
					envelope.attacking=false;
				}
				envelope.releasing=true;
			}
			else
			{
				kill();
			}
			unlock();
		}
		catch(...)
		{
			unlock();
			throw;
		}
	}
}

void CSoundPlayerChannel::kill()
{
	if(playing)
	{
		playing=false;
		paused=false;
		try
		{
			playTrigger.trip();
			pauseTrigger.trip();
		}
		catch(...)
		{
			// do anything here? (need to ignore or transfer when thrown from mixChannels)
			throw;
		}
	}
}

bool CSoundPlayerChannel::isPlaying() const
{
	return(playing);
}

bool CSoundPlayerChannel::isPaused() const
{
	return(paused);
}

bool CSoundPlayerChannel::isPlayingSelectionOnly() const
{
	return(playing && playSelectionOnly);
}

bool CSoundPlayerChannel::isPlayingLooped() const
{
	return(playing && playLooped);
}

void CSoundPlayerChannel::setPlaySpeed(float _playSpeed)
{
	//printf("setting play speed to: %f\n",_playSpeed);
	playSpeed=_playSpeed;
	if(playSpeed<-100.0)
		playSpeed=-100.0;
	else if(playSpeed>100.0)
		playSpeed=100.0;

	// make sure the play speed is never zero (??? I guess... so it won't completly stall in the sound player?)
	if(fabs(playSpeed)<0.001)
		playSpeed=playSpeed<0 ? -0.001 : 0.001;
}

void CSoundPlayerChannel::setPosition(sample_pos_t newPosition)
{
	lock(); // we lock when we set the variable because playPosition is modified there too in mixOntoBuffer
	try
	{
		if(newPosition>sound->getLength())
			throw(runtime_error(string(__func__)+" -- newPosition parameter out of bounds: "+istring(newPosition)));
		playPosition=newPosition;

		unlock();
	}
	catch(...)
	{
		unlock();
		throw;
	}
}


void CSoundPlayerChannel::setStartPosition(sample_pos_t newPosition)
{
	lock();
	try
	{
		if(newPosition>sound->getLength())
			throw(runtime_error(string(__func__)+" -- newPosition parameter out of bounds: "+istring(newPosition)));
		if(stopPosition<newPosition)
			stopPosition=newPosition;
		startPosition=newPosition;

		if(playing && playSelectionOnly && playPosition<startPosition)
			playPosition=startPosition;

		unlock();
	}
	catch(...)
	{
		unlock();
		throw;
	}
}

void CSoundPlayerChannel::setStopPosition(sample_pos_t newPosition)
{
	lock();
	try
	{
		if(newPosition>sound->getLength())
			throw(runtime_error(string(__func__)+" -- newPosition parameter out of bounds: "+istring(newPosition)));
		if(startPosition>newPosition)
			startPosition=newPosition;
		stopPosition=newPosition;

		if(playing && playSelectionOnly && playPosition>stopPosition)
			playPosition=stopPosition;

		unlock();
	}
	catch(...)
	{
		unlock();
		throw;
	}
}

void CSoundPlayerChannel::lock() const
{
	mutex.EnterMutex();
}

void CSoundPlayerChannel::unlock() const
{
	mutex.LeaveMutex();
}

void CSoundPlayerChannel::addOnPlayTrigger(TriggerFunc triggerFunc,void *data)
{
	playTrigger.set(triggerFunc,data);
}

void CSoundPlayerChannel::removeOnPlayTrigger(TriggerFunc triggerFunc,void *data)
{
	playTrigger.unset(triggerFunc,data);
}

void CSoundPlayerChannel::addOnPauseTrigger(TriggerFunc triggerFunc,void *data)
{
	pauseTrigger.set(triggerFunc,data);
}

void CSoundPlayerChannel::removeOnPauseTrigger(TriggerFunc triggerFunc,void *data)
{
	pauseTrigger.unset(triggerFunc,data);
}

void CSoundPlayerChannel::setMute(unsigned channel,bool mute)
{
	if(channel>=MAX_CHANNELS)
		throw(runtime_error(string(__func__)+" -- channel parameter is out of range: "+istring(channel)));
	muted[channel]=mute;
}

bool CSoundPlayerChannel::getMute(unsigned channel) const
{
	if(channel>=MAX_CHANNELS)
		throw(runtime_error(string(__func__)+" -- channel parameter is out of range: "+istring(channel)));
	return(muted[channel]);
}

void CSoundPlayerChannel::setVolume(float value)
{
	if(value<0 || value>1.0)
		throw(runtime_error(string(__func__)+" -- value parameter must be from 0 to 1.0 : "+istring(value,1,5)));

	volume=value;
	calcVolumeScalars();
}

void CSoundPlayerChannel::setPan(float value)
{
	if(value<-1.0 || value>1.0)
		throw(runtime_error(string(__func__)+" -- value parameter must be from -1.0 to 1.0 : "+istring(value,1,5)));

	pan=value;
	calcVolumeScalars();
}

void CSoundPlayerChannel::calcVolumeScalars()
{
	if(pan<0)
	{    // to the left
		leftVolumeScalar=(int)(volume*(float)MAX_VOLUME_SCALAR);
		rightVolumeScalar=(int)(-pan*volume*(float)MAX_VOLUME_SCALAR);
	}
	else if(pan>0)
	{    // to the right
		leftVolumeScalar=(int)(pan*volume*(float)MAX_VOLUME_SCALAR);
		rightVolumeScalar=(int)(volume*(float)MAX_VOLUME_SCALAR);
	}
	else
	{    // in the middle
		leftVolumeScalar=rightVolumeScalar=(int)(volume*(float)MAX_VOLUME_SCALAR);
	}
}

/* ??? need to rewrite this to support N channels of PCM data, no l or r buffers... */
void CSoundPlayerChannel::mixOntoBuffer(const unsigned nChannels,sample_t * const oBuffer,const size_t oBufferLength)
{
	if(!playing || (paused && playSpeed==1.0))
		return;

	lock();

	const ASound &sound=*this->sound;

	if(!sound.trylockSize())
	{ // some action is probably taking place
		unlock();
		return;
	}

	try
	{
		sample_pos_t pos1,pos2;
		const bool loop=playLooped;

		const size_t deviceIndex=0; // ??? would loop through all devices later

		if(playSelectionOnly)
		{
			// pos1 is selectStart; pos2 is selectStop
			pos1=startPosition;
			pos2=stopPosition;
		}
		else
		{
			// Pos1 is 0; Pos2 is Length-1
			pos1=0;
			pos2=sound.getLength()-1;
		}

		// use a local variable instead of looking up the data member each time
		register sample_fpos_t playPosition=this->playPosition;

		// assure that playPosition is in range
		if(playPosition<0.0)
			playPosition=0.0;
		else if(playPosition>pos2)
			playPosition=pos2;

		// ??? eventually need to use linear interpolation with the fractional sample position, but I just let it alias for now.... perhaps the hardware could support it in some way?
		// ??? however, for now, if the sample rate is a multiple of the opened sample rate, linear interpolation will matter much less (i.e. 44100, 22050, 11025, etc)

		// heed the sampling rate of the sound also when adjusting the play position (??? only have device 0 for now)
		const sample_fpos_t tPlaySpeed=playSpeed  *  (((sample_fpos_t)sound.getSampleRate())/((sample_fpos_t)player->devices[deviceIndex].sampleRate));
		const sample_fpos_t origPlayPosition=playPosition;

		#define MOVE_PLAY_POSITION								\
			if(tPlaySpeed>0.0)								\
			{										\
				playPosition+=tPlaySpeed;						\
				if(playPosition>pos2)							\
				{									\
					if(loop)							\
						playPosition=pos1;					\
					else								\
					{								\
						kill();							\
						break;							\
					}								\
				}									\
			}										\
			else										\
			{										\
				if(playPosition>=-tPlaySpeed)						\
					playPosition+=tPlaySpeed;					\
				else									\
				{									\
					playPosition=0;							\
					break;								\
				}									\
			}

		#define SETUP_BUFFERS_AND_POSITIONS							\
			playPosition=origPlayPosition;							\
													\
			const bool isMuted=muted[i];							\
													\
			unsigned outputDevice,outputDeviceChannel;					\
			getOutputRouteParams(outputRoute,i,outputDevice,outputDeviceChannel);		\
													\
			outputDeviceChannel%=nChannels;							\
			register sample_t *ooBuffer=oBuffer+outputDeviceChannel;			\
													\
			const CRezPoolAccesser &srcData=sound.getAudio(i);				\
													\
			volatile int &volumeScalar= ((i%player->devices[deviceIndex].channelCount)==0) ? this->leftVolumeScalar : this->rightVolumeScalar; /*not used in every instance*/



		if(!envelope.attacking && !envelope.releasing)
		{    // the envelope is not active
			if(leftVolumeScalar==MAX_VOLUME_SCALAR && rightVolumeScalar==MAX_VOLUME_SCALAR)
			{    // don't do extra calc for volumes and pans
				for(unsigned i=0;i<sound.getChannelCount();i++)
				{
					SETUP_BUFFERS_AND_POSITIONS
					for(size_t t=0;t<oBufferLength;t+=nChannels)
					{
						if(!isMuted)
							ooBuffer[t]=ClipSample(ooBuffer[t]+srcData[(sample_pos_t)playPosition]);

						MOVE_PLAY_POSITION
					}
				}
			}
			else // Uses an Amplitude
			{
				for(unsigned i=0;i<sound.getChannelCount();i++)
				{
					SETUP_BUFFERS_AND_POSITIONS
					for(size_t t=0;t<oBufferLength;t+=nChannels)
					{
						if(!isMuted)
							ooBuffer[t]=ClipSample(ooBuffer[t]+(((mix_sample_t)srcData[(sample_pos_t)playPosition]*volumeScalar)/MAX_VOLUME_SCALAR_DIV) );

						MOVE_PLAY_POSITION
					}
				}
			}
		}
		else // UseEnvelope
		{
			CEnvelope envelope=this->envelope;
			const CEnvelope origEnvelope=envelope;

			for(unsigned i=0;i<sound.getChannelCount();i++)
			{
				SETUP_BUFFERS_AND_POSITIONS

				// restore the envelope to the original each time
				envelope=origEnvelope;

				register int envelopeCount=envelope.envelopeCount;
				if(envelope.attacking)
				{
					const int attackFactor=envelope.attackFactor;
					for(size_t t=0;t<oBufferLength;t+=nChannels)
					{
						if(envelopeCount>=attackFactor)
							envelope.attacking=false;
						else
							envelopeCount++;

						// I a >> 8 because an int*int (volumeScalar*envelopeCount) Yields a 64bit number, which means I can't have amplitude=32767 AND attackFactor>65535.  attackFactor=65535 is usually only over a second or so
						register const int envAmplitude=(volumeScalar*(envelopeCount>>8)/attackFactor)<<8;

						if(!isMuted)
							ooBuffer[t]=ClipSample(ooBuffer[t]+(((mix_sample_t)srcData[(sample_pos_t)playPosition]*envAmplitude)/MAX_VOLUME_SCALAR_DIV) );

						MOVE_PLAY_POSITION
					}
				}
				else // Releasing
				{
					const int releaseFactor=envelope.releaseFactor;
					for(size_t t=0;t<oBufferLength;t+=nChannels)
					{
						if(envelopeCount<=0)
						{
							envelope.releasing=false;
							kill();
							break;
						}
						else
							envelopeCount--;

						register const int envAmplitude=(volumeScalar*(envelopeCount>>8)/releaseFactor)<<8;

						if(!isMuted)
							ooBuffer[t]=ClipSample(ooBuffer[t]+(((mix_sample_t)srcData[(sample_pos_t)playPosition]*envAmplitude)/MAX_VOLUME_SCALAR_DIV) );

						MOVE_PLAY_POSITION
					}
				}
			}

			// now write the the last iteration's results back to the data memeber
			this->envelope=envelope;
		}

		// now write the the last iteration's results back to the data memeber
		this->playPosition=(sample_pos_t)playPosition;

		sound.unlockSize();
		unlock();
	}
	catch(...)
	{
		sound.unlockSize();
		unlock();
		throw;
	}
}

// ??? I could fix this by making a 2 element union, where one element is a count of how many of the other element is about to follow... this way I wouldn't have the 32 channel limit
#define MAX_ROUTE_CHANNELS 32 // ??? we'll have a problem if a loaded sound even can have more than 32 channels.  
struct _ROutputRoute
{
	uint32_t outputDevice;
	uint32_t outputDeviceChannel;
};
typedef _ROutputRoute ROutputRoute[MAX_ROUTE_CHANNELS];

void CSoundPlayerChannel::createInitialOutputRoute()
{
	TPoolAccesser<ROutputRoute,ASound::PoolFile_t> a=sound->getGeneralDataAccesser<ROutputRoute>("OutputRoutes");

	const size_t outputDeviceIndex=0;
	
	if(a.getSize()<=0)
	{
		a.append(1);

		// assign the channesl in the sound to the device's channels in a round-robin fasion
		for(size_t t=0;t<MAX_ROUTE_CHANNELS;t++)
		{
			a[0][t].outputDevice=outputDeviceIndex;
			a[0][t].outputDeviceChannel=t%(player->devices[outputDeviceIndex].channelCount);
		}
	}
}

void CSoundPlayerChannel::getOutputRouteParams(unsigned route,unsigned channel,unsigned &outputDevice,unsigned &outputDeviceChannel)
{
	if(channel>=MAX_ROUTE_CHANNELS)
		throw(runtime_error(string(__func__)+" -- channel out of bounds: "+istring(channel)));

	// ??? make this const again if I ever move away from the typedef [32] thing
	/*const*/ TPoolAccesser<ROutputRoute,ASound::PoolFile_t> a=sound->getGeneralDataAccesser<ROutputRoute>("OutputRoutes");
	if(route>=a.getSize())
		route=0; // if out of bounds, use default

	outputDevice=a[route][channel].outputDevice;
	outputDeviceChannel=a[route][channel].outputDeviceChannel;
}

