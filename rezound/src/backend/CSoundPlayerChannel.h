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

#ifndef __CSoundPlayerChannel_H__
#define __CSoundPlayerChannel_H__


#include "../../config/common.h"

class CSoundPlayerChannel;

#include <cc++/thread.h>

#include "CTrigger.h"
#include "CEnvelope.h"

#include "CSound_defs.h"
class ASoundPlayer;

// ??? perhaps these should change to float if sample_t is floating point .. perhap define them where sample_t is defined
#define MAX_VOLUME_SCALAR 32767
#define MAX_VOLUME_SCALAR_DIV 32768

static CEnvelope __defaultEnvelope;

class CSoundPlayerChannel
{
public:

	explicit CSoundPlayerChannel(const CSoundPlayerChannel &src);
	virtual ~CSoundPlayerChannel();

	CSound *getSound() const;
	CSound * const sound;

	CSoundPlayerChannel &operator=(const CSoundPlayerChannel &src);

	void play(bool _playLooped=false,bool _playSelectionOnly=false,CEnvelope &_envelope=__defaultEnvelope);
	void pause();
	void stop();
	void kill();

	bool isPlaying() const;
	bool isPaused() const;
	bool isPlayingSelectionOnly() const;
	bool isPlayingLooped() const;

	void setPlaySpeed(float _playSpeed);

	// really play position
	sample_pos_t getPosition() const { return (sample_pos_t)playPosition; }
	void setPosition(sample_pos_t newPosition);

	sample_pos_t getStartPosition() const { return startPosition; }
	void setStartPosition(sample_pos_t newPosition);

	sample_pos_t getStopPosition() const { return stopPosition; }
	void setStopPosition(sample_pos_t newPosition);

	void setMute(unsigned channel,bool mute);
	bool getMute(unsigned channel) const;

	// value = 0.0 to 1.0
	void setVolume(float value);
	// value = -1.0(left) to 1.0(right)
	void setPan(float value);

	void addOnPlayTrigger(TriggerFunc triggerFunc,void *data);
	void removeOnPlayTrigger(TriggerFunc triggerFunc,void *data);
	void addOnPauseTrigger(TriggerFunc triggerFunc,void *data);
	void removeOnPauseTrigger(TriggerFunc triggerFunc,void *data);

private:

	friend class ASoundPlayer;

	// - called by ASoundPlayer
	// - nChannels is the number of channels buffer represents (i.e 1 mono, 2 stereo, etc)
	// - bufferSize is in sample frames
	void mixOntoBuffer(const unsigned nChannels,sample_t * const buffer,const size_t bufferSize);

	void lock() const;
	void unlock() const;

	mutable ost::Mutex mutex;
	ASoundPlayer *player;
	CSoundPlayerChannel(ASoundPlayer *_player,CSound *_sound);

	// Playing Status and Play Positions
	volatile bool playing,paused,playLooped,playSelectionOnly;
	volatile sample_fpos_t playPosition;
	float playSpeed;
	volatile sample_pos_t startPosition,stopPosition;

	// Envelope stuff
	CEnvelope envelope;

	// Mute
	bool muted[MAX_CHANNELS];

	// Volume and Pan
	void calcVolumeScalars();
	volatile float volume,pan;
	volatile int leftVolumeScalar,rightVolumeScalar; // ??? there somehow needs to be volume scalars for each channel on the output device (for now, I just map left and right to every other channel)

	// Which output route to use
	unsigned outputRoute;

	CTrigger playTrigger,pauseTrigger;

	void deinit();
	void init();

	void createInitialOutputRoute();
	void getOutputRouteParams(unsigned route,unsigned channel,unsigned &outputDevice,unsigned &outputDeviceChannel);

};

#endif
