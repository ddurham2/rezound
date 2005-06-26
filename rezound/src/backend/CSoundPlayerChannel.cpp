/* vim:nowrap
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
#include <algorithm>
#include <utility>

#include <istring>

#include "CSound.h"
#include "ASoundPlayer.h"
#include "DSP/TSoundStretcher.h"
#include "settings.h"

	// ??? I need to make the FRAMES_PER_CHUNK value depend on the sample rate (and put a min on it) so that sound of a lower sample rate dont have a playposition with less resolution
	//   make the macro just resolve to a data-member which has been calculated from the sampleRate .. it's almost never used in a stack array's size
	// I might want to do something different like 1/4th of a second.. need to test on slower machines and put a load on the machines
#define CHUNKS_TO_PREBUFFER ((sample_pos_t)44) // ~1 second @ 44100 (??? probably need to base this of some information obtained from ASoundPlayer at construction)
#define FRAMES_PER_CHUNK ((sample_pos_t)1024) // a play position is written to the prebufferedPositionsPipe every FRAMES_PER_CHUNK sample frames

// this is special value queued in positions queue indicating to produce a gap signal before reading more data from the audio queue
//#define GAP_SIGNAL_INDICATOR (MAX_LENGTH+1) 


/* TODO
 * - Provisions have been made in here for supporting multiple simultaneous output devices, 
 *   however, ASoundPlayer doesn't haven't this notion yet.  When it does, it should pass
 *   mixOntoChannels multiple buffers, one for each device, and multiple nChannel values.
 * - I must do it this way so that all the play positioning can be done knowing
 *   the last iteration's results so it can save the new values in the data members.
 *   (synced positions among devices)
 *
 * - See AudioIO for more info about how this class fits into the whole picture
 *
 * - If I ever do have TSoundStretcher support more than 2 point sample interpolation then I 
 *   should make mixOntoBuffer support saving N interpolation samples instead of a fixed 1 sample.
 *
 * - Several places I clear the pipe, set the mutex, then clear it again; this is not such a good thing... 
 *   I have to clear the pipe once, then lock the mutex, then clear it again because the first clear clears 
 *   it, then an instance of prebufferChunk() might have been waiting while writing, then the second clear 
 *   clears that one.  This is not good mutex practice, I don't know if it will work correctly on multi-cpu 
 *   machines; I probably need to think this situation out more. ???
 *
 * == SINCE THE SECOND RE-WRITE ==
 *
 * - Currently for the 'play gap before loop' feature there is a blip of the beginning of the selection that
 *   is audible.  The prebuffer() lessens the blip by setting the rest of the chunk to be prebuffered to zeroes
 *   but the blip is still present because the mixOntoChannel() method will have already read the data from the
 *   audio queue before it reads the produceGapSignal flag and it can't put the audio data back nor hold it for
 *   a time while it produces the gap.  This could be rememdied perhaps by peeking at the audio data queue before
 *   reading or producing an extra chunk of silence. (And it's the same issue with skipping most of the middle)
 * 
 * - If it would be feasible to use libsamplerate then I probably should use it (or just make TSoundStretcher
 *   use it and make the object a data-member so that I wouldn't have to keep up with previous frames for 
 *   interpolation and so forth.. it should simplify a few sections of code and eliminate some of the
 *   data-members and calculations I'm doing to keep up with that mess.  Plus, libsamplerate supposedly sounds
 *   better.
 *
 * - I might be able to benifit by adding parameters to TMemoryPipe::read that says whether to block for audio 
 *   and another that was to wait for other reads to finish
 *
 * - One issue still remains with FOX in that seeking backwards with the keyboard (by holding '1' down) causes 
 *   hiccups because the key-press/release event is repeating and there's no way at the present to distinguish a 
 *   real keypress from a software generated one.  This repeated call to setSeekSpeed is causing it to clear and 
 *   re-calculate the prebufferd data, perhaps I can detect the situation in CSoundPlayerChannel and avoid 
 *   recalculating anything.
 */

CSoundPlayerChannel::CSoundPlayerChannel(ASoundPlayer *_player,CSound *_sound) :
	sound(_sound),
	prebufferPosition(0),
	prebufferThread(this),

	framesConsumedFromAudioPipe(0),

	prebufferedAudioPipe(1),
	prebufferedPositionsPipe(1),
	
	prepared_channelCount(0),
	prepared_sampleRate(0),

	gapSignalPosition(0),
	gapSignalLength(0),
	gapSignalBuffer(0),

	player(_player),

	playing(false),
	seekSpeed(1.0),
	playSpeedForMixer(1.0),
	playSpeedForPrebuffering(0)
{
	init();
}

CSoundPlayerChannel::~CSoundPlayerChannel()
{
	deinit();
}

CSound *CSoundPlayerChannel::getSound() const
{
	return sound;
}

void CSoundPlayerChannel::init()
{
	player->addSoundPlayerChannel(this);


	prebuffering=false;
	playing=false;
	lastBufferWasGapSignal=false;
	playPosition=0;

	// start out with something selected
	startPosition=sound->getLength()/2-sound->getLength()/4;
	stopPosition=sound->getLength()/2+sound->getLength()/4;

	outputRoute=0;

	for(size_t t=0;t<MAX_CHANNELS;t++)
		muted[t]=false;

	updateAfterEdit(vector<int16_t>());

	prebufferThread.kill=false;
	prebufferThread.start();
}

void CSoundPlayerChannel::deinit()
{
	{
		CMutexLocker l(prebufferThread.waitForPlayMutex);
		prebufferThread.kill=true;
		stop();
		prebufferThread.waitForPlay.signal(); // so the prebuffering thread will for sure catch kill==true
	}

	prebufferThread.wait();
	prebufferedAudioPipe.closeWrite();
	prebufferedPositionsPipe.closeWrite();

	player->removeSoundPlayerChannel(this);
}

void CSoundPlayerChannel::play(sample_pos_t position,LoopTypes _loopType,bool _playSelectionOnly)
{
	if(!player->isInitialized())
		throw runtime_error(string(__func__)+" -- the sound player is not initialized");

	if(playing)
		stop();

	// zero out previous frame for interpolation
	for(size_t t=0;t<MAX_CHANNELS;t++)
		prevLast2Frames[0][t]=prevLast2Frames[1][t]=0;
	prevLastFrameOffset=0.0;

	if(_loopType==ltLoopNone && !_playSelectionOnly)
	{ // use position
		if(position>=sound->getLength())
			return;
		loopType=ltLoopNone;
		playSelectionOnly=false;
		lastBufferWasGapSignal=false;
		prebufferPosition=playPosition=position;
	}
	else
	{ // ignore position and use the two flags
		loopType=_loopType;
		playSelectionOnly=_playSelectionOnly;
		lastBufferWasGapSignal=false;
		if(playSelectionOnly)
			prebufferPosition=playPosition=startPosition;
		else
			prebufferPosition=playPosition=0;
	}

		// there should be no readers or writers currently waiting on these pipes at this point
	prebufferedAudioPipe.clear();
	prebufferedPositionsPipe.clear();
	somethingWantsToClearThePrebufferQueue=false;

	gapSignalPosition=gapSignalLength;

	framesConsumedFromAudioPipe=0;
	prebuffering=true;
	playing=true;
	paused=false;
	playTrigger.trip();
	pauseTrigger.trip();

	// prime the fifos with a couple of chunks of data /* maybe just one would do ??? */
	if(!prebufferChunk() && !prebufferChunk())
	{
		CMutexLocker l(prebufferThread.waitForPlayMutex);
		prebufferThread.waitForPlay.signal(); // take the prebuffer thread out of its wait-state
	}
}

void CSoundPlayerChannel::pause()
{
	if(playing)
	{
		// would do, but I don't think it's absolutely necessary and it does cause unpausing to lock up
		//CMutexLocker(prebufferThread.waitForPlayMutex);

		paused=!paused;
		pauseTrigger.trip();
		prebufferThread.waitForPlay.signal(); // take the prebuffer thread out of its wait-state
		
	}
}

void CSoundPlayerChannel::stop()
{
	if(playing)
	{
		prebuffering=false;
		playing=false;
		paused=false;

		// clear currently prebufferedData so that any blocked write in prebufferChunk() finishes
		prebufferedAudioPipe.clear();
		prebufferedPositionsPipe.clear();

		// no need to set the somethingWantsToClearThePrebufferQueue because we've set prebuffering to false already which will cause it to not prebuffer any more data
	
		CMutexLocker l(prebufferPositionMutex);

		// remove any more prebuffered data that might have been just waiting to be written into the pipe
		prebufferedAudioPipe.clear();
		prebufferedPositionsPipe.clear();
	}
}

void CSoundPlayerChannel::playingHasEnded()
{
	if(playing)
	{
		playing=false;
		paused=false;
		playTrigger.trip();
		pauseTrigger.trip();
	}
}

bool CSoundPlayerChannel::isPlaying() const
{
	return playing;
}

bool CSoundPlayerChannel::isPaused() const
{
	return paused;
}

bool CSoundPlayerChannel::isPlayingSelectionOnly() const
{
	return playing && playSelectionOnly;
}

bool CSoundPlayerChannel::isPlayingLooped() const
{
	return playing && loopType!=ltLoopNone;
}

// it is best to have the prebufferReadingMutex locked before this method is called so that no data is being consumed
sample_pos_t CSoundPlayerChannel::estimateOldestPrebufferedPosition(float origSeekSpeed,sample_pos_t origStartPosition,sample_pos_t origStopPosition)
{
	// calculate the number of samples that that have been consumed of the oldest prebuffered chunk (because we might not be consuming entire chunks when tPlaySpeed isn't 1.0)
	const int m=prebufferedAudioPipe.getSize()/prepared_channelCount;
	const int n=prebufferedPositionsPipe.getSize();
	const int f=FRAMES_PER_CHUNK;
	const int framesConsumedOfOldestChunk=((n*f)-m);
	//printf("framesConsumedOfOldestChunk: %d  %d %d %d\n",framesConsumedOfOldestChunk,m,n,f);

	sample_pos_t oldestPrebufferedPosition=0;
	if(prebufferedPositionsPipe.getSize()>1)
	{
		// read the position of the oldest prebuffered chunk (not counting the fact that some of it may have already been consumed)
		RChunkPosition pos;
		int r=prebufferedPositionsPipe.peek(&pos,1,false);
		sample_pos_t oldestPrebufferedChunkPosition=pos.position;
		if(r!=1)
			oldestPrebufferedChunkPosition=playPosition;

		// now adjust the position of the oldestPrebufferedChunk into the oldestPrebufferedPosition which is the position of the oldest frame that is currently in the prebuffering queue
		if(origSeekSpeed>0.0f)
		{
			sample_pos_t pos1,pos2;
			if(playSelectionOnly)
			{
				pos1=origStartPosition;
				pos2=origStopPosition;
			}
			else
			{
				pos1=0;
				pos2=sound->getLength()-1;
			}

			switch(loopType)
			{
			case ltLoopNone:
				oldestPrebufferedPosition=oldestPrebufferedChunkPosition+framesConsumedOfOldestChunk;
				break;
			case ltLoopNormal:
			case ltLoopGapBeforeRepeat:
			case ltLoopSkipMost: // handling this specially would be nice, but I'm not going to worry about it right now
				//printf("oldestPrebufferedChunkPosition: %d pos1: %d\n",oldestPrebufferedChunkPosition,pos1);
				if(pos1<oldestPrebufferedChunkPosition)
					oldestPrebufferedPosition=pos1+(((oldestPrebufferedChunkPosition-pos1)+framesConsumedOfOldestChunk)%(pos2-pos1+1));
				else
					oldestPrebufferedPosition=playPosition;
				break;
			default:
				throw runtime_error(string(__func__)+" -- internal error -- unhandled loop type");
			}

			if(oldestPrebufferedPosition>pos2)
				oldestPrebufferedPosition=pos2;

		}
		else
		{
			// when playSpeed plays in reverse I don't regard loop points or selection positions
			if(oldestPrebufferedChunkPosition>(sample_pos_t)framesConsumedOfOldestChunk)
				oldestPrebufferedPosition=oldestPrebufferedChunkPosition-framesConsumedOfOldestChunk;
			else
				oldestPrebufferedPosition=0;
		}
	}
	else
		oldestPrebufferedPosition=playPosition;

	return oldestPrebufferedPosition;
}

// ??? could be estimated better
void CSoundPlayerChannel::estimateLowestAndHighestPrebufferedPosition(sample_pos_t &lowestPosition,sample_pos_t &highestPosition,float origSeekSpeed,sample_pos_t origStartPosition,sample_pos_t origStopPosition)
{
	CMutexLocker l(prebufferReadingMutex); // make sure mixOntoBuffer isn't running
	TAutoBuffer<RChunkPosition> positions(prebufferedPositionsPipe.getSize());
	int read=prebufferedPositionsPipe.peek(positions,positions.getSize(),false);

	lowestPosition=MAX_LENGTH;
	highestPosition=0;
	for(int t=0;t<read;t++)
	{
		if(positions[t].position<MAX_LENGTH) // ignore indicator values
		{
			lowestPosition=min(lowestPosition,positions[t].position);
			highestPosition=max(highestPosition,positions[t].position);
		}
	}

	highestPosition+=FRAMES_PER_CHUNK-1; // because the position pipe holds positions to the first frame written per chunk
}

void CSoundPlayerChannel::unprebuffer(float origSeekSpeed,sample_pos_t origStartPosition,sample_pos_t origStopPosition,sample_pos_t setNewPosition)
{
	if(!playing)
		return;

	// block mixOntoBuffer() from running (so we're not consuming anything from the prebuffered queue)
	CMutexLocker readLocker(prebufferReadingMutex); 

	// set this flag to cause prebufferChunk() to yield instead of do its thing
	somethingWantsToClearThePrebufferQueue=true;	

	const sample_pos_t oldestPrebufferedPosition=setNewPosition==MAX_LENGTH ? estimateOldestPrebufferedPosition(origSeekSpeed,origStartPosition,origStopPosition) : setNewPosition;

	// unblock any blocked write() currently in prebufferPosition()
	prebufferedAudioPipe.clear();
	prebufferedPositionsPipe.clear();

	// block prebufferChunk from running
	CMutexLocker writeLocker(prebufferPositionMutex); 
	somethingWantsToClearThePrebufferQueue=false; // now we can unset this


	// now clear any more prebuffered data that was created between the first clear and getting of the mutex lock
	framesConsumedFromAudioPipe=0;
	prebufferedAudioPipe.clear();
	prebufferedPositionsPipe.clear();


	// and set the prebuffer position to estimated position of the oldest prebuffered position so it will start prebuffering there
	playPosition=prebufferPosition=oldestPrebufferedPosition; 

	// in case it got set to false in prebufferChunk() just before obtaining the lock in here
	if(playing && !prebuffering)
	{
		prebuffering=true;	
		prebufferThread.waitForPlay.signal();
	}
}

void CSoundPlayerChannel::setSeekSpeed(float _seekSpeed)
{
	if(seekSpeed==_seekSpeed)
		return; // bail if not changing

	/*
	printf("%s %f\n",__func__,_seekSpeed);
	if(_seekSpeed==1.0)
		_seekSpeed=_seekSpeed;
	*/
	const float origSeekSpeed=seekSpeed;

	seekSpeed=_seekSpeed;
	if(seekSpeed<-100.0)
		seekSpeed=-100.0;
	else if(seekSpeed>100.0)
		seekSpeed=100.0;

	// make sure the play speed is never zero (so it won't completly stall in the progress)
	if(fabs(seekSpeed)<0.001)
		seekSpeed=seekSpeed<0 ? -0.001 : 0.001;

	/*
	 * When play speed is more than 5.0 then I want to make preBufferChunk()
	 * start skipping more data so that we don't have to read as fast off disk
	 */
	if(fabs(seekSpeed)>5.0)
	{
		// since playSpeedForPrebuffering just causes skips in the data, we only take the integer portion of the seekSpeed
		playSpeedForPrebuffering=(int)floor(seekSpeed);

		// let the mixer play the chunks at a speeds only between [1.0, 5.0), so here we're skipping data instead of mixing faster
		playSpeedForMixer=1;
	}
	else
	{
		playSpeedForPrebuffering=0;
		playSpeedForMixer=seekSpeed;
	}

	if(
	   // clear prebuffered data if it changes while seekSpeed is above 5x since <5 does the speed when reading the prebuffered data, or clear it when the seekSpeed just fell below 5 since we've prebuffered data-being-skipped
	   (fabs(seekSpeed)>5.0 || (fabs(origSeekSpeed)>5.0 && fabs(seekSpeed)<=5.0))
	   ||
	   // invalidate prebuffered data since the signs of the new and old play speeds where different (which means direction changed)
	   (seekSpeed*origSeekSpeed)<0.0)
	{ 
		unprebuffer(origSeekSpeed,getStartPosition(),getStopPosition());
		prebufferChunk(); // prime the pipe immediately
	}

	
	// if the channel was paused, but we just set the play speed to something not 1.0, then start playing again
	if(paused && seekSpeed!=1.0 && origSeekSpeed==1.0)
	{
		CMutexLocker l(prebufferThread.waitForPlayMutex);
		prebufferThread.waitForPlay.signal();
	}
}

float CSoundPlayerChannel::getSeekSpeed() const
{
	return seekSpeed;
}

void CSoundPlayerChannel::setPosition(sample_pos_t newPosition)
{
	if(newPosition>sound->getLength())
		throw runtime_error(string(__func__)+" -- newPosition parameter out of bounds: "+istring(newPosition));

	unprebuffer(seekSpeed,startPosition,stopPosition,newPosition);
}


void CSoundPlayerChannel::setStartPosition(sample_pos_t newPosition)
{
	if(newPosition>sound->getLength())
		throw runtime_error(string(__func__)+" -- newPosition parameter out of bounds: "+istring(newPosition));

	const sample_pos_t origStartPosition=startPosition;
	const sample_pos_t origStopPosition=stopPosition;

	if(stopPosition<newPosition)
		stopPosition=newPosition;
	startPosition=newPosition;

	// invalidate prebuffered data if necessary
	if(playing && playSelectionOnly)
	{
		sample_pos_t lowestPosition,highestPosition;
		estimateLowestAndHighestPrebufferedPosition(lowestPosition,highestPosition,seekSpeed,origStartPosition,origStopPosition);

		//static int g=0;
		if(startPosition>origStartPosition && startPosition>lowestPosition)
		{ // start position moved rightward and new start position is beyond the beginning of the prebuffered window
			//printf("clear from start change 1 (%d)\n",g++);
			unprebuffer(seekSpeed,origStartPosition,origStopPosition,startPosition);
			prebufferChunk();
		}
		else if(
			startPosition<origStartPosition && 
			origStartPosition>=(lowestPosition-min(lowestPosition,FRAMES_PER_CHUNK))
		)
		{ // start position moved leftward and the old loop window was shorter than the prebuffered window
			//printf("clear from start change 2 (%d)\n",g++);
			unprebuffer(seekSpeed,origStartPosition,origStopPosition);
			prebufferChunk();
		}
	}
}

void CSoundPlayerChannel::setStopPosition(sample_pos_t newPosition)
{
	if(newPosition>sound->getLength())
		throw runtime_error(string(__func__)+" -- newPosition parameter out of bounds: "+istring(newPosition));

	const sample_pos_t origStartPosition=startPosition;
	const sample_pos_t origStopPosition=stopPosition;

	if(startPosition>newPosition)
		startPosition=newPosition;
	stopPosition=newPosition;

	// invalidate prebuffered data if necessary
	if(playing && playSelectionOnly)
	{
		sample_pos_t lowestPosition,highestPosition;
		estimateLowestAndHighestPrebufferedPosition(lowestPosition,highestPosition,seekSpeed,origStartPosition,origStopPosition);

		//printf("lo: %d hi: %d orig stop: %d new stop: %d\n",lowestPosition,highestPosition,origStopPosition,stopPosition);
		//static int g=0;
		if(stopPosition<origStopPosition && stopPosition<=highestPosition)
		{ // stop position moved leftward and new position moves into or preceeds the currently prebuffered window
			//printf("clear from stop change 1 (%d)\n",g++);
			unprebuffer(seekSpeed,origStartPosition,origStopPosition);
			prebufferChunk();
		}
		else if(
			stopPosition>origStopPosition && 
			origStopPosition-min(origStopPosition,FRAMES_PER_CHUNK*CHUNKS_TO_PREBUFFER)<=highestPosition
		)
		{ // stop position moved rightward and old position is before or within the currently prebuffered window (so a repeat at old loop point is prebuffered)
			//printf("clear from stop change 2 (%d)\n",g++);
			unprebuffer(seekSpeed,origStartPosition,origStopPosition);
			prebufferChunk();
		}
	}
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
		throw runtime_error(string(__func__)+" -- channel parameter is out of range: "+istring(channel));
	muted[channel]=mute;
}

bool CSoundPlayerChannel::getMute(unsigned channel) const
{
	if(channel>=MAX_CHANNELS)
		throw runtime_error(string(__func__)+" -- channel parameter is out of range: "+istring(channel));
	return muted[channel];
}


/* ??? perhaps pass a parameter that says whether to assign or mix onto oBuffer if it wouldn't bugger up the algorithm too much */
void CSoundPlayerChannel::mixOntoBuffer(const unsigned nChannels,sample_t * const _oBuffer,const size_t _oBufferLength)
{
	if(paused && seekSpeed==1.0/*not seeking*/)
		return;

	// protecting from any method that also reads/clears the prebuffering queue
	CMutexTryLocker l(prebufferReadingMutex); // ??? the mutex needs to be locked in RAM (perhaps just the whole *this object if possible)
	if(!l.didLock())
		return;

#if 0 // printout of prebuffered status
	printf("\r                                                                         \r");
	for(int t=0;t<prebufferedPositionsPipe.getSize();t++)
		printf("=");
	fflush(stdout);
#endif

	if(prebufferedAudioPipe.getSize()<=0)
	{
		// here the pipe has emptied and we're no longer prebuffering so go ahead and shut down the playing state
		if(!prebuffering)
			playingHasEnded(); /* ??? Eeek.. this could affect JACK .. does trip() call the function directly or schedule it to happen?  does the event do anything indeterminate .. moreover the Trigger object needs to be locked in RAM (would be if whole object was locked)*/
		return;
	}
	
	const size_t deviceIndex=0; // ??? would loop through all devices later (probably actually in a more inner loop than here)

	// heed the sampling rate of the sound also when adjusting the play position (??? only have device 0 for now)
	const sample_fpos_t tPlaySpeed= fabs(playSpeedForMixer)  *  (((sample_fpos_t)prepared_sampleRate)/((sample_fpos_t)player->devices[deviceIndex].sampleRate));

	const unsigned channelCount=prepared_channelCount;

	#define READ_SIZE ((size_t)4096)
	sample_t _readBuffer[(READ_SIZE+2)*MAX_CHANNELS]; /* ??? for JACK, needs to be locked from swapping out .. is stack, so can that happen?  perhaps this needs to point to a TAutoBuffer declared in the object that is locked from being swapped out*/
	sample_t *readBuffer=_readBuffer+(2*MAX_CHANNELS); // make readBuffer have two valid frames worth of space before the pointer for interpolation purposes

	sample_t *oBuffer=_oBuffer;
	int outputBufferLength=_oBufferLength;

	while(outputBufferLength>0 && !(paused && seekSpeed==1.0))
	{
		// later code will populate this with the value that will be saved for 
		// the next iteration of this loop or the next call to prebufferChunk()
		sample_fpos_t lastFrameOffset; 

		// maximum that we would want to read give the play speed and output buffer length
		const int maxFramesToRead=(int)sample_fpos_ceil(outputBufferLength*tPlaySpeed-(1.0-prevLastFrameOffset));	

		// this should be set by either of the two cases below
		sample_pos_t framesRead;

		if(gapSignalPosition<gapSignalLength)
		{ // produce data from gap signal

			if(loopType==ltLoopSkipMost)
			{ // make the play position appear to skip over the middle
				const sample_pos_t skipMiddleMargin=(sample_pos_t)(gSkipMiddleMarginSeconds*prepared_sampleRate);
				const sample_fpos_t pos1=startPosition+skipMiddleMargin;
				const sample_fpos_t pos2=stopPosition-skipMiddleMargin;
				if(seekSpeed>0)
					playPosition=(sample_pos_t)(pos1+(((pos2-pos1)*gapSignalPosition)/gapSignalLength));
				else
					playPosition=(sample_pos_t)(pos2+(((pos1-pos2)*gapSignalPosition)/gapSignalLength));
			}

			framesRead=min(gapSignalLength-gapSignalPosition,
					min((sample_pos_t)READ_SIZE, (sample_pos_t)maxFramesToRead)
				);
			memcpy(readBuffer,gapSignalBuffer+(gapSignalPosition*channelCount),framesRead*channelCount*sizeof(sample_t));
			gapSignalPosition+=framesRead;
		}
		else
		{ // read data from audio pipe
			//                    maxFramesToRead could be 0 at very low play speeds
			const int samplesRead=maxFramesToRead>0 ? prebufferedAudioPipe.read(readBuffer,min(READ_SIZE,(size_t)(maxFramesToRead*channelCount)),false) : 0;
			if(samplesRead<=0)
				break; // no data available

			framesRead=samplesRead/channelCount;

			// update playPosition if necessary
			{ 
				const sample_pos_t lastChunkBoundryPassed=(framesConsumedFromAudioPipe/FRAMES_PER_CHUNK)*FRAMES_PER_CHUNK;
				const sample_pos_t chunkBoundriesConsumed=((framesConsumedFromAudioPipe+framesRead)-lastChunkBoundryPassed)/FRAMES_PER_CHUNK;
				for(sample_pos_t t=0;t<chunkBoundriesConsumed;t++)
				{
					RChunkPosition pos;
					prebufferedPositionsPipe.read(&pos,1,false); // don't block
					if(pos.produceGapSignal)
						gapSignalPosition=0;
					playPosition=pos.position;
				}
			}
			framesConsumedFromAudioPipe+=framesRead;

			// don't allow framesConsumedFromAudioPipe to forever grow (only need the remainder)
			framesConsumedFromAudioPipe%=FRAMES_PER_CHUNK;
		}
				

		// - calculate the output length that we could produce from the number of samples 
		//   read (and counting using the previously saved interpolation samples)
		// - need to min with outputBufferLength because at very very slow speeds even the interpolation 
		//   samples could produce many many output samples and this calculation would return 
		//   the number of samples they could produce
		const sample_pos_t outputLengthToUse=(sample_pos_t)min((sample_fpos_t)outputBufferLength,sample_fpos_ceil((framesRead+(1.0-prevLastFrameOffset))/tPlaySpeed) );

		//printf("outputBufferLength: %d framesRead: %d speed: %f outputLengthToUse: %d\n",outputBufferLength,framesRead,tPlaySpeed,outputLengthToUse);

		bool didOutput=false;
		const size_t _outputLengthToUse=outputLengthToUse*nChannels;
		for(unsigned i=0;i<channelCount;i++)
		{
			if(!muted[i])  // ??? this memory needs to be locked for JACK's sake
			{
				// populate for interpolation
				readBuffer[-(2*channelCount)+i]=prevLast2Frames[0][i]; 
				readBuffer[-   channelCount +i]=prevLast2Frames[1][i];
				const sample_t * const rreadBuffer=readBuffer-(2*channelCount);

				const vector<bool> outputRouting=getOutputRoute(0,i); // ??? this needs to be put into a data member of memory NOT TO BE SWAPPED for jack's sake and it needs to be updated in updateAfterEdit()

				for(size_t outputDeviceChannel=0;outputDeviceChannel<outputRouting.size();outputDeviceChannel++)
				{
					if(outputRouting[outputDeviceChannel])
					{
						sample_t *ooBuffer=oBuffer+outputDeviceChannel;

						if(tPlaySpeed==1.0)
						{ // simple 1.0 speed copy
							register size_t p=i;
							for(size_t t=0;t<_outputLengthToUse;t+=nChannels)
							{
								ooBuffer[t]=ClipSample((mix_sample_t)ooBuffer[t]+rreadBuffer[p]);
								p+=channelCount;
							}

							lastFrameOffset=1.0;
						}
						else
						{
							TSoundStretcher<const sample_t *> stretcher(rreadBuffer,prevLastFrameOffset,framesRead/*+(1.0-prevLastFrameOffset)*/,(sample_fpos_t)outputLengthToUse,channelCount,i,true);
							for(size_t t=0;t<_outputLengthToUse;t+=nChannels)
								ooBuffer[t]=ClipSample((mix_sample_t)ooBuffer[t]+stretcher.getSample());

							// ??? may need to subtract this differently depending on speed>1 or <1
							lastFrameOffset=stretcher.getCurrentSrcPosition();
							lastFrameOffset-=sample_fpos_floor(lastFrameOffset); // only save fractional part

								/*??? HMM, I'm not sure if this is necessary or not, in the ==1.0 case above I have to do 1.0 so it will read the same number of samples that it will output, so here I assume that if it were just above 1.0 then I would want also to make sure that I consumed at least one of the interpolation samples */
								/* maybe it shouldn't be x-floor(lastFrameOffset) above, but should be x-(how much it was supposed to produce)  but then again it might be the same thing if I look at the TSoundStretcher implementation */
							if(tPlaySpeed>1.0) 
								lastFrameOffset+=1.0;
						}
						didOutput=true;
					}
				}
			}

			// save the last 2 samples and (later) the offset into the next-to-last so that we can use it for interpolation the next go around
			prevLast2Frames[0][i]=readBuffer[((framesRead-2)*channelCount)+i];
			prevLast2Frames[1][i]=readBuffer[((framesRead-1)*channelCount)+i];
		}

		// if all channels were muted, or none were mapped to an output device then this value never got set
		if(!didOutput)
		{
			lastFrameOffset=outputLengthToUse*tPlaySpeed;
			lastFrameOffset-=sample_fpos_floor(lastFrameOffset);
		}

		outputBufferLength-=outputLengthToUse;
		oBuffer+=outputLengthToUse*nChannels;

		prevLastFrameOffset=lastFrameOffset;
	}

	if(!prebuffering && prebufferedAudioPipe.getSize()<=0)
	{ // here the pipe has just emptied and we're no longer prebuffering so go ahead and shut down the playing state
		playingHasEnded();
	}
}

/*
 * This method simply prebuffers one chunk of data regardless of whether the channel is playing, paused, etc
 * And it may block if the prebufferedChunkPipe is full
 * 
 * Returns true if the endpoint was reached
 * 	 ??? don't need a return value anymore .. I don't think I'm using it anywhere 
 */
bool CSoundPlayerChannel::prebufferChunk()
{
	if(somethingWantsToClearThePrebufferQueue)
	{ // check this flag and don't prebuffer any more data since something might clear the pipes so that this method finishes a blocked write, but then immediately fills the queue back up before getting the chance to lock the mutex
		AThread::yield();
		return false;
	}

	if(!playing || !prebuffering)
	{
		prebuffering=false;
		return true;
	}

	if(paused && seekSpeed==1.0/*not seeking*/)
		return false;

	// nothing better cause an exception to be thrown between here and 
	// the 'GGG' try-catch below without making provisions to unlock this 
	sound->lockSize(); 
	CMutexLocker l(prebufferPositionMutex); /* ??? might be renaming this to prebufferWritingMutex */ 

	// used if loopType==ltLoopSkipMost
	const sample_pos_t skipMiddleMargin=(sample_pos_t)(gSkipMiddleMarginSeconds*prepared_sampleRate);
	bool queueUpAGap=false;

	sample_t buffer[FRAMES_PER_CHUNK*MAX_CHANNELS];
	size_t bufferUsed=0; // in samples
	bool ret=false;

	sample_pos_t prebufferPositionToWrite=0;

	const unsigned channelCount=prepared_channelCount;
	try // GGG
	{
		sample_pos_t pos1=0,pos2=0; // declared out here incase I need their values when queuing up a gap 

		if(!playing || !prebuffering) // stop() may have changed this since the method started (unlikely tho)
		{
			prebuffering=false;
			sound->unlockSize();
			return true;
		}

		const int positionInc=(seekSpeed>0) ? 1 : -1;

		const LoopTypes loopType=this->loopType; // store current value
		if(playSelectionOnly)
		{
			sample_pos_t t;
			// pos1 is selectStart; pos2 is selectStop

			t=startPosition;
			pos1=min(t,sound->getLength()-1);

			t=stopPosition;
			pos2=min(t,sound->getLength()-1);
		}
		else
		{
			// pos1 is 0; pos2 is length-1
			pos1=0;
			pos2=sound->getLength()-1;
		}

		// assure that prebufferPosition is not out of range
		if(prebufferPosition<0)
			prebufferPosition=0;
		else if(prebufferPosition>pos2)
			prebufferPosition=pos2;

		prebufferPositionToWrite=prebufferPosition;

		for(unsigned i=0;i<channelCount;i++)
		{
			const CRezPoolAccesser &src=sound->getAudio(i);

			sample_pos_t tPrebufferPosition=prebufferPosition;

			// ??? I should be able to know ahead of time an exact number of samples to copy before the loop point instead of having to check the loop point every iteration
				
			// avoid +i in (buffer[t*channelCount+i]) the loop by offsetting buffer 
			sample_t *_buffer=buffer+i;
			// avoid the *channelCount in (buffer[t*channelCount+i]) the loop by incrementing by the channelCount
			const size_t last=FRAMES_PER_CHUNK*channelCount;

			if(positionInc>0)
			{
				for(size_t t=0;t<last;t+=channelCount)
				{
					_buffer[t]=src[tPrebufferPosition];
					bufferUsed++;

					// move position for next iteration
					tPrebufferPosition+=positionInc;
					if(tPrebufferPosition>pos2)
					{
						if(loopType==ltLoopNone)
						{
							ret=true;
							break;
						}
						else // is looped
						{
							tPrebufferPosition=pos1;
							if(loopType==ltLoopGapBeforeRepeat)
							{
								// pad with zeros to avoid gap signal played slightly after the beginning of the next loop's audio
								for(size_t k=t+channelCount;k<last;k+=channelCount)
								{
									_buffer[k]=0;
									bufferUsed++; 
								}
								queueUpAGap=true;
								break;
							}
						}
					}
				}
			}
			else // if(positionInc<=0)
			{
				if(tPrebufferPosition>0) // don't even bother if it's not >0 because we're playing backwards and it's going to just click,click,click from the first sample being there and the rest being silence
				{
					for(size_t t=0;t<last;t+=channelCount)
					{
						_buffer[t]=src[tPrebufferPosition];
						bufferUsed++;

						// move position for next iteration
						if(tPrebufferPosition>0)
							tPrebufferPosition+=positionInc;
						else
						{
							tPrebufferPosition=0;
							break;
						}
					}
				}
				else
				{
					for(size_t t=0;t<last;t+=channelCount)
					{
						_buffer[t]=0;
						bufferUsed++;
					}
				}
			}

			// update real prebufferPosition place holder on last iteration
			if(i==channelCount-1)
			{
				// but before we do that, if we're seeking faster than 5.0, then start skipping chunks (and handle loops points)
				const int skipAmount=playSpeedForPrebuffering*FRAMES_PER_CHUNK;
				if(skipAmount>0)
				{
					tPrebufferPosition+=skipAmount;
					if(tPrebufferPosition>pos2)
					{
						if(loopType==ltLoopNone)
						{
							tPrebufferPosition=pos2;
							ret=true;
						}
						else // is looped
							tPrebufferPosition=pos1;
					}
				}
				else if(skipAmount<0)
				{
					if(tPrebufferPosition>(sample_pos_t)(-skipAmount))
						tPrebufferPosition+=skipAmount;
					else
						tPrebufferPosition=0;
				}

				prebufferPosition=tPrebufferPosition;
			}
		}

		// if loopType is to skip most of the middle make pos2 the point to skip most of the middle
		if(loopType==ltLoopSkipMost)
		{
			// make sure there is at least 2 margins between pos1 and pos2
			// and that the gap to be placed between the two margines is shorter than what will be skipped
			if((pos2-pos1)>(skipMiddleMargin*2+gapSignalLength))
			{
				// see if prebufferPosition is within the area we should skip
				if(prebufferPosition>(pos1+skipMiddleMargin) && prebufferPosition<(pos2-skipMiddleMargin))
				{
					if(seekSpeed>0)
						prebufferPosition=pos2-skipMiddleMargin; // skip ahead 
					else // if(seekSpeed<0)
						prebufferPosition=pos1+skipMiddleMargin; // skip backwards
					
					queueUpAGap=true;
				}
			}
		}

		sound->unlockSize();
	}
	catch(...)
	{
		sound->unlockSize();
		throw; // ??? hmm.. what would I want to do here
	}

		// also, the stretching algorithm in the play thread messes up sometimes if 
		// successive chunks aren't the same length (isn't a problem I don't think 
		// with the idea of the algorithm just some particular about it that I'm not 
		// going to worry about since I will always write full chunks... someday it 
		// may crop up again)


	try // wrap to catch TMemoryPipe::EPipeClosed ??? shouldn't be necessary any more
	{
		// this is a blocked i/o write
		RChunkPosition pos;
		pos.position=prebufferPositionToWrite;
		pos.produceGapSignal=queueUpAGap;
		prebufferedPositionsPipe.write(&pos,1);
		const int lengthWritten=prebufferedAudioPipe.write(buffer,bufferUsed);
	}
	catch(TMemoryPipe<sample_t>::EPipeClosed &e)
	{
		// write end closed before or while we were writing to the pipe
	}

	if(ret)
		prebuffering=false;

	return ret;
}

const vector<int16_t> CSoundPlayerChannel::getOutputRoutes() const
{
	vector<int16_t> v;
	const TPoolAccesser<int16_t,CSound::PoolFile_t> a=sound->getGeneralDataAccesser<int16_t>("OutputRoutes_v2");
	v.reserve(a.getSize());
	for(size_t t=0;t<a.getSize();t++)
		v.push_back(a[t]);

	return v;
}

// NOTE: this is called from AAction with the size locked on the CSound object
void CSoundPlayerChannel::updateAfterEdit(const vector<int16_t> &restoreOutputRoutes)
{
	// ??? if I figure out how, I need to do the trick of invalidating the prebufferedData, and reset the prebufferPosition back to where the prebuffered data should start again
	if(prepared_channelCount!=sound->getChannelCount() || prepared_sampleRate!=sound->getSampleRate())
	{ // channel count or sample rate has changed (or this object is just being constructed)

		// prevent mixOntoBuffer() from running
		CMutexLocker readLock(prebufferReadingMutex);

		// prepare for locking the prebufferPositionMutex
		somethingWantsToClearThePrebufferQueue=true;
		prebufferedAudioPipe.clear();
		prebufferedPositionsPipe.clear();

		// prevent prebufferChunk() from running
		CMutexLocker writeLock(prebufferPositionMutex);
		somethingWantsToClearThePrebufferQueue=false;
			
		// close the prebuffering queue
		prebufferedAudioPipe.closeWrite();
		prebufferedAudioPipe.closeRead();
		prebufferedPositionsPipe.closeWrite();
		prebufferedPositionsPipe.closeRead();
		
		// resize the queue
		prebufferedAudioPipe.setSize(CHUNKS_TO_PREBUFFER*FRAMES_PER_CHUNK*sound->getChannelCount());
		prebufferedPositionsPipe.setSize(CHUNKS_TO_PREBUFFER);

		createGapSignal();

		// restore/recreate routing information
		TPoolAccesser<int16_t,CSound::PoolFile_t> a=sound->getGeneralDataAccesser<int16_t>("OutputRoutes_v2");
		if(restoreOutputRoutes.size()>1) /* >1 because the first int16_t is the size of the subsequent data */
		{ // restore from what was given, trusting that it was saved from the information when it had the current number of channels
			a.clear();
			a.append(restoreOutputRoutes.size());
			for(size_t t=0;t<restoreOutputRoutes.size();t++)
				a[t]=restoreOutputRoutes[t];
		}
		else
		{ // recreate the default routing
			a.clear();
			createInitialOutputRoute();
		}

		// reopen prebuffering queue
		prebufferedAudioPipe.open();
		prebufferedPositionsPipe.open();

		// reset for the next time
		prepared_channelCount=sound->getChannelCount();
		prepared_sampleRate=sound->getSampleRate();
	}
	else
		unprebuffer(seekSpeed,startPosition,stopPosition);
}

void CSoundPlayerChannel::createGapSignal()
{
	// generate the signal to be produced if the channel is producing the gap between or within loops (see enum LoopTypes)
	// I generate the signal as the entire length needed according to gLoopGapLengthSeconds, but I chop up this signal
	// into multiple parts when I queue up the chunks so that the play cursor can animate across the skipped portion

	const unsigned channelCount=sound->getChannelCount();
	const unsigned sampleRate=sound->getSampleRate();

	gapSignalPosition=gapSignalLength=(sample_pos_t)(gLoopGapLengthSeconds*sampleRate);
	gapSignalBuffer.setSize(gapSignalLength*channelCount);

	/* I should make this load a wav now that it can be any length (round-robin the channels, convert sample rate .. I suppose generate if I can't load it .. let registry define file location) */

	/*
	// light hiss
	for(unsigned i=0;i<channelCount;i++)
	{
		for(unsigned t=0;t<gapSignalLength;t++)
			gapSignalBuffer[t*channelCount+i]=convert_sample<float,sample_t>((float)rand()/RAND_MAX/20.0);
	}
	*/

	// tone
	for(unsigned i=0;i<channelCount;i++)
	{
		const float freq1=300;
		const float freq2=freq1*1.6;
		const float freq3=freq2*1.7;
		for(unsigned t=0;t<gapSignalLength;t++)
		{
			gapSignalBuffer[t*channelCount+i]=convert_sample<float,sample_t>(
			(0.5-0.5*cos(2*M_PI*t/gapSignalLength))*	// fade in/out window
			((						// produce the tone
				sin(t*freq1/sampleRate*2.0*M_PI)+
				sin(t*freq2/sampleRate*2.0*M_PI)+
				sin(t*freq3/sampleRate*2.0*M_PI)
			)/20.0));
		}
	}
}


// --- Output Routing ---------------------------------

/*
	The output routing information is preserved in .rez files when the file is saved.
	The only issue is that the file may be loaded on a machine that has a different 
	output device configuration.  The number of device may change or the number of 
	channels on each output device may change.   But, at least preserving the 
	information in the .rez file will allow any changes in the routing made by the 
	user not to have to be done again by just closing and opening the file later.

	The format of the output information is just a long set of 16bit integers.
	The way to interpret that sequence of integers is as follows:
	
	The first integer tells how many output route tables there are.  There would be
	one for each audio output device.

	Then follows the first output route table which is just part of the sequence of
	integers.  After the first output route table follows the next one.

	The first integer in an output route table indicates to number of rows in the 
	output route table, and the second integer indicates the number of columns.

	Following these two counts are rowCount X columnCount integers which are either 
	0 or 1 indicating if that part of the route is on or off.  And the following 
	example should show how these integers are to be interpreted
	

	This would be an example of one output route table where the device has 5 output
	channels (possibly a 5.1 surround sound sound card) and the audio file has 2 
	channels (a stereo sound):

	Audio      Device's Channels-->
        Data's    -------------------------
	Channels | 1 0 0 0 0
	   |     | 0 1 0 1 0
	   |     |
	   V 

	channel 0 of the audio is mapped to channel 0 of the output device
	channel 1 of the audio is mapped to channel 1 of the output device
	channel 1 of the audio is mapped to channel 3 of the output device

	So the sequence of integers for this output route table would be:

		2 5 1 0 0 0 0 0 1 0 1 0
		    ^       ^ ^       ^
                    |1st row| |2nd row|

	where the rows in the table follow each other.
	
	So, the entire output routing information stored in the file would be first the 
	count of how many route tables there are and then following would be each output 
	route table.

	In the future I may allow there to actually be multiple sets of routing
	tables where integer would preceed all routing tables indicating how many
	sets of routes there are
*/


/*
	Creates the initial output route which just maps each channel in the audio
	to a channel in the first output device.
*/
void CSoundPlayerChannel::createInitialOutputRoute()
{
	TPoolAccesser<int16_t,CSound::PoolFile_t> a=sound->getGeneralDataAccesser<int16_t>("OutputRoutes_v2");

	const size_t outputDeviceIndex=0;
	
	if(a.getSize()<=0)
	{
		const unsigned audioChannelCount=sound->getChannelCount();
		const unsigned deviceChannelCount=player->devices[outputDeviceIndex].channelCount;

		size_t p=0;
		a.append(1);
		a[p++]=1; // 1 output route table
		
		a.append(1);
		a[p++]=audioChannelCount; // row count in route table

		a.append(1);
		a[p++]=deviceChannelCount; // row count in route table

		if(audioChannelCount==1 && deviceChannelCount>=2)
		{ // special case of mono sound played in a stereo (or greater) sound card (map the 1 mono audio channel to the first 2 channels on the device)
			for(size_t x=0;x<deviceChannelCount;x++)
			{
				a.append(1);
				a[p++]= (x<2) ? 1 : 0;
			}
		}
		else
		{
			// assign the channels in the sound to the device's channels in a round-robin fashion
			// to make sure each channel in the audio data is heard at least on one channel in the
			// output device
			for(size_t y=0;y<audioChannelCount;y++)
			for(size_t x=0;x<deviceChannelCount;x++)
			{
				a.append(1);
				a[p++]= (x==(y%deviceChannelCount)) ? 1 : 0;

			}
		}
	}
}

/*
	Returns a row from the output route table given which output device (which output route table)
	and which channel in the audio data (which gets you to one row in one output route table)
 */
const vector<bool> CSoundPlayerChannel::getOutputRoute(unsigned deviceIndex,unsigned audioChannel) const
{
	const TPoolAccesser<int16_t,CSound::PoolFile_t> a=sound->getGeneralDataAccesser<int16_t>("OutputRoutes_v2");

	if(a.getSize()<=0)
		throw runtime_error(string(__func__)+" -- internal error -- somehow the initial route didn't get created and output route information was requested");

	size_t p=0;
	const size_t deviceCount=a[p++];
	if(deviceIndex>=deviceCount)
		throw runtime_error(string(__func__)+" -- deviceIndex out of bounds: "+istring(deviceIndex)+">="+istring(deviceCount));

	// skip to the output route table requested
	for(size_t t=0;t<deviceIndex;t++)
	{
		const size_t rowCount=a[p++];
		const size_t columnCount=a[p++];
		p+=rowCount*columnCount;
	}
	
	// now we're at the device requested
	const size_t rowCount=a[p++];
	const size_t columnCount=a[p++];
		
	if(audioChannel>=rowCount)
		throw runtime_error(string(__func__)+" -- audioChannel out of bounds: "+istring(audioChannel)+">="+istring(rowCount));

	vector<bool> row;
	p+=(columnCount*audioChannel); // skip to the requested row in the table
	for(size_t t=0;t<columnCount;t++)
		row.push_back( a[p++] ? true : false);

	return row;
}



// --- Prebuffering Thread ---------------------------

CSoundPlayerChannel::CPrebufferThread::CPrebufferThread(CSoundPlayerChannel *_parent) :
	AThread(),
	parent(_parent),
	kill(false)
{
}


// This is the thread function that constantly calls prebufferChunk() which writes prebuffered data into prebufferedChunkPipe
// This thread also goes into a wait-state while the channel is not playing
void CSoundPlayerChannel::CPrebufferThread::main()
{
	while(!kill)
	{
		/* 
		 * This code puts this thread into a wait state while the 
		 * CSoundPlayerChannel object is not playing or prebuffers 
		 * chunks if it is in a playing state.  Note that 
		 * prebufferChunk() will return true if this channel 
		 * is not in a playing state or is finished prebuffering.
		 */
		CMutexLocker l(waitForPlayMutex);
		while(!parent->prebuffering || (parent->paused && parent->seekSpeed==1.0))
		{
			if(kill)
				break;
			waitForPlay.wait(waitForPlayMutex);
		}
		parent->prebufferChunk();
	}
}

