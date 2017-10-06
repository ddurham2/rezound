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

#include <CMutex.h>
#include <CConditionVariable.h>
#include <AThread.h>
#include <TMemoryPipe.h>
#include <TAutoBuffer.h>

#include "CTrigger.h"
#include "CSound_defs.h"
class ASoundPlayer;

class CSoundPlayerChannel
{
public:
	virtual ~CSoundPlayerChannel();

	CSound *getSound() const;

	enum LoopTypes
	{
		ltLoopNone,		// don't loop
		ltLoopNormal,		// loop either the whole sound or the selection (depending on playSelectionOnly passed to play())
		ltLoopSkipMost,		// loop normally except skip most of the middle and put a short gap there instead
		ltLoopGapBeforeRepeat	// loop normally except put a short gap after the end point so it's clear where the loop point is
	};

	// the position arg is only used iff playLooped and playSelectionOnly are both false
	void play(sample_pos_t position=0,LoopTypes _playLooped=ltLoopNone,bool _playSelectionOnly=false);
	void pause();
	void stop();

	bool isPlaying() const;
	bool isPaused() const;
	bool isPlayingSelectionOnly() const;
	bool isPlayingLooped() const;

	void setSeekSpeed(float _seekSpeed);
	float getSeekSpeed() const;

	// really is the "play position"
	sample_pos_t getPosition() const { return playing ? playPosition : 0; }
	void setPosition(sample_pos_t newPosition);

	sample_pos_t getStartPosition() const { return startPosition; }
	void setStartPosition(sample_pos_t newPosition);

	sample_pos_t getStopPosition() const { return stopPosition; }
	void setStopPosition(sample_pos_t newPosition);

	void setMute(unsigned channel,bool mute);
	bool getMute(unsigned channel) const;

	void addOnPlayTrigger(TriggerFunc triggerFunc,void *data);
	void removeOnPlayTrigger(TriggerFunc triggerFunc,void *data);
	void addOnPauseTrigger(TriggerFunc triggerFunc,void *data);
	void removeOnPauseTrigger(TriggerFunc triggerFunc,void *data);

	const vector<int16_t> getOutputRoutes() const;

	// pass an empty vector if this is not to restore, but possibly recreate the output routes
	void updateAfterEdit(const vector<int16_t> &restoreOutputRoutes);

private: /* for ASoundPlayer only */
	friend class ASoundPlayer;
	CSoundPlayerChannel(ASoundPlayer *_player,CSound *_sound);

	// - called by ASoundPlayer
	// - nChannels is the number of channels buffer represents (i.e 1 mono, 2 stereo, etc)
	// - bufferSize is in sample frames
	void mixOntoBuffer(const unsigned nChannels,sample_t * const buffer,const size_t bufferSize);

	CSound * const sound;

	volatile mutable bool somethingWantsToClearThePrebufferQueue;
				// ??? perhaps everywhere that I set the prebufferPosition I also write/clear the pipe
	mutable CMutex prebufferPositionMutex;	// restricts critical sections that write to the prebuffer queue
	mutable CMutex prebufferReadingMutex;	// restricts critical sections that read from the prebuffer queue
	sample_pos_t prebufferPosition;
	bool prebufferChunk();

	void playingHasEnded();

	class CPrebufferThread : public AThread
	{
	public:
		CPrebufferThread(CSoundPlayerChannel *parent);
		void main();

		CSoundPlayerChannel *parent;
		bool kill;
		CConditionVariable waitForPlay;
		mutable CMutex waitForPlayMutex;
	} prebufferThread;

	friend class CPrebufferThread;


	// two fifos for prebuffering audio and play positions
	sample_pos_t framesConsumedFromAudioPipe;	// keeps track of the amount of data that has been consumed from the audio pipe since so we know when to read another prebuffered play position.  This value gets reset whenever the prebuffering starts over
	TMemoryPipe<sample_t> prebufferedAudioPipe;
	struct RChunkPosition
	{
		sample_pos_t position;
		bool produceGapSignal;
	};
	TMemoryPipe<RChunkPosition> prebufferedPositionsPipe;



	sample_t prevLast2Frames[2][MAX_CHANNELS]; // ??? for JACK, this needs to be locked from swapping
	sample_fpos_t prevLastFrameOffset; // ??? for JACK, this needs to be locked from swapping

	unsigned prepared_channelCount;
	unsigned prepared_sampleRate;

	sample_pos_t gapSignalPosition;
	sample_pos_t gapSignalLength; // in frames
	TAutoBuffer<sample_t> gapSignalBuffer;
	void createGapSignal();

	ASoundPlayer *player;

	// Playing Status and Play Positions
	volatile bool prebuffering,playing,paused,playSelectionOnly;
	volatile LoopTypes loopType;
	bool lastBufferWasGapSignal; // true if the last buffer that was processed in mixOntoBuffer had its isGap flag turned on (if this is the case, then I have to handle setting the play position in the setSeekSpeed() method a little different)
	sample_pos_t playPosition;
	float seekSpeed;
	float playSpeedForMixer;
	int playSpeedForPrebuffering;
	volatile sample_pos_t startPosition,stopPosition;

	// Mute
	bool muted[MAX_CHANNELS];

	// Which output route to use
	unsigned outputRoute;

	CTrigger playTrigger,pauseTrigger;

	void deinit();

	// by examining the data currently in queue, this returns the most likely position of the oldest frame in the prebuffered pipe.  It is best to call this method with the prebufferReadingMutex locked
	sample_pos_t estimateOldestPrebufferedPosition(float origSeekSpeed,sample_pos_t origStartPosition,sample_pos_t origStopPosition);
	void estimateLowestAndHighestPrebufferedPosition(sample_pos_t &lowestPosition,sample_pos_t &highestPosition,float origSeekSpeed,sample_pos_t origStartPosition,sample_pos_t origStopPosition);

	// clears the prebuffered data and [attempts to] sets the prebuffer position to the position that produced the oldest sample that was in the prebuffer
	void unprebuffer(float origSeekSpeed,sample_pos_t origStartPosition,sample_pos_t origStopPosition,sample_pos_t setNewPosition=MAX_LENGTH);

	void createInitialOutputRoute();
	const vector<bool> getOutputRoute(unsigned deviceIndex,unsigned audioChannel) const;
};

#endif
