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
#include <algorithm>

#include <istring>

#include "CSound.h"
#include "ASoundPlayer.h"
#include "DSP/TSoundStretcher.h"
#include "settings.h"

#define PREBUFFERED_CHUNK_SIZE 1024 // in frames

				// try to prebuffer 1.5 times of data as the output device will be prebuffering itself (1.51 because I'm going to truncate the decimal and I want to make sure it's on the upper side of the int)
#define CHUNK_COUNT_TO_PREBUFFER ((unsigned)(1.51*(gDesiredOutputBufferCount*gDesiredOutputBufferSize/PREBUFFERED_CHUNK_SIZE)))

/* TODO
 * - Provisions have been made in here for supporting multiple simultaneous output devices, 
 *   however, ASoundPlayer doesn't haven't this notion yet.  When it does, it should pass
 *   mixOntoChannels multiple buffers, one for each device, and multiple nChannel values.
 * - I must do it this way so that all the play positioning can be done knowing
 *   the last iteration's results so it can save the new values in the data members.
 *   (synced positions among devices)
 *
 * - See AudioIO for more info about how these class fits into the whole picture
 *
 * - If I ever do have TSoundStretcher support more than 2 pointer sample interpolation then I 
 *   should make mixOntoBuffer support saving N samples instead of a fixed 1 samples.
 *
 * - the JJJ comments indicate something I don't really like about the way I'm mutexing
 *
 */

CSoundPlayerChannel::CSoundPlayerChannel(ASoundPlayer *_player,CSound *_sound) :
	sound(_sound),
	prebufferPosition(0),
	prebufferThread(this),
	prebufferedChunkPipe(CHUNK_COUNT_TO_PREBUFFER),
	gapSignalBufferLength(0),
	gapSignalBuffer(NULL),
	player(_player),
	playing(false),
	seekSpeed(1.0),
		playSpeedForMixer(1.0),
		playSpeedForChunker(0)
{
	init();
	createInitialOutputRoute();
}

CSoundPlayerChannel::~CSoundPlayerChannel()
{
	deinit();
}

CSound *CSoundPlayerChannel::getSound() const
{
	return(sound);
}

void CSoundPlayerChannel::init()
{
	createPrebufferedChunks();

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
	prebufferedChunkPipe.closeWrite();

	player->removeSoundPlayerChannel(this);

	destroyPrebufferedChunks();
}

void CSoundPlayerChannel::play(sample_pos_t position,LoopTypes _loopType,bool _playSelectionOnly)
{
	if(!player->isInitialized())
		throw(runtime_error(string(__func__)+" -- the sound player is not initialized"));

	if(playing)
		stop();

	// zero out previous frame for interpolation
	for(size_t t=0;t<MAX_CHANNELS;t++)
		prevFrame[t]=0;

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

	prebuffering=true;
	playing=true;
	paused=false;
	playTrigger.trip();
	pauseTrigger.trip();

	prebufferedChunkPipe.clear();

	// prime the pipe with a couple of chunks of data
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

		// remove any prebuffered data
		prebufferedChunkPipe.clear();
	
		CMutexLocker l(prebufferPositionMutex); // lock so that prebufferChunk won't be running

		prebuffering=false;
		playing=false;
		paused=false;
	
		// remove any prebuffered data
		prebufferedChunkPipe.clear();
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
	return(playing && loopType!=ltLoopNone);
}

void CSoundPlayerChannel::setSeekSpeed(float _seekSpeed)
{
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
		// let playSpeedForChunker be the nearest multiple of 5
		playSpeedForChunker=(int)floor(seekSpeed);

		// let the mixer play the chunks at a speed between [1.0, 5.0)
		playSpeedForMixer=1;
	}
	else
	{
		playSpeedForChunker=0;
		playSpeedForMixer=seekSpeed;
	}


	if((fabs(origSeekSpeed)>5.0 && fabs(seekSpeed)<=5.0) || (fabs(origSeekSpeed)<=5.0 && fabs(seekSpeed)>5.0))
	{ // the play speed just cross over or back from the 5.0 threshold so invalidate prebuffered data because what has been buffered so far is skipping or not skipping data
		// and set the prebuffer position to the most recently played position
		CMutexLocker l(prebufferPositionMutex);
		prebufferedChunkPipe.clear(); // JJJ need to clean the buffer
		if(!lastBufferWasGapSignal)
			prebufferPosition=playPosition;
		prebufferedChunkPipe.clear(); // JJJ (but a write() might have been pending on the buffer)
	}
	else if((seekSpeed*origSeekSpeed)<0.0)
	{ // invalidate prebuffered data since the signs of the new and old play speeds where different (which means direction changed)
		// and set the prebuffer position to the most recently played position
		CMutexLocker l(prebufferPositionMutex);
		prebufferedChunkPipe.clear(); // JJJ need to clean the buffer
		if(!lastBufferWasGapSignal)
			prebufferPosition=playPosition;
		prebufferedChunkPipe.clear(); // JJJ (but a write() might have been pending on the buffer)
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
	return(seekSpeed);
}

void CSoundPlayerChannel::setPosition(sample_pos_t newPosition)
{
	if(newPosition>sound->getLength())
		throw(runtime_error(string(__func__)+" -- newPosition parameter out of bounds: "+istring(newPosition)));

	prebufferedChunkPipe.clear();

	// update the play position
	{
		// JJJ => this is not such a good thing... I have to clear the pipe once, then lock the mutex, then clear it again because the first clear clears it, then an instance of prebufferChunk() might have been waiting while writing, then the second clear clears that one.  This is not good mutex practice, I don't know if it will work correctly on multi-cpu machines; I probably need to think this situation out more
		prebufferedChunkPipe.clear(); // invalidate prebuffered data

		CMutexLocker l(prebufferPositionMutex);
		prebufferPosition=newPosition;
		lastBufferWasGapSignal=false;
		playPosition=newPosition;

		prebufferedChunkPipe.clear(); // invalidate prebuffered data
	}


	// just in case it had stopped and the prebuffer pipe was draining, we need to start prebuffering again
	// QQQ* CMutexLocker l(prebufferThread.waitForPlayMutex);   QQQ => causes dead lock when an action calls this method and also has a the sound locked for resize because the prebuffering thread has this mutex locked, and it's not absolutely important that this mutex be obtained to signal the prebuffering thread to wake back up
	prebuffering=true;
	prebufferThread.waitForPlay.signal();

}


void CSoundPlayerChannel::setStartPosition(sample_pos_t newPosition)
{
	if(newPosition>sound->getLength())
		throw(runtime_error(string(__func__)+" -- newPosition parameter out of bounds: "+istring(newPosition)));
	if(stopPosition<newPosition)
		stopPosition=newPosition;
	startPosition=newPosition;

	if(playing && playSelectionOnly && min(prebufferPosition,playPosition)<startPosition)
	{
		prebufferedChunkPipe.clear(); // JJJ invalidate prebuffered data

		// update prebufferPosition
		CMutexLocker l(prebufferPositionMutex);
		prebufferPosition=startPosition;
		lastBufferWasGapSignal=false;
		playPosition=startPosition;

		prebufferedChunkPipe.clear(); // JJJ invalidate prebuffered data
	}

	/* takes too long for UI
	// create more prebuffered data
	prebufferChunk();
	*/

	// just in case it had stopped and the prebuffer pipe was draining, we need to start prebuffering again
	// QQQ* CMutexLocker l(prebufferThread.waitForPlayMutex);
	prebuffering=true;
	prebufferThread.waitForPlay.signal();
}

void CSoundPlayerChannel::setStopPosition(sample_pos_t newPosition)
{
	if(newPosition>sound->getLength())
		throw(runtime_error(string(__func__)+" -- newPosition parameter out of bounds: "+istring(newPosition)));
	if(startPosition>newPosition)
		startPosition=newPosition;
	stopPosition=newPosition;

	if(playing && playSelectionOnly && max(prebufferPosition,playPosition)>stopPosition)
	{
		prebufferedChunkPipe.clear(); // JJJ invalidate prebuffered data

		// update prebufferPosition
		CMutexLocker l(prebufferPositionMutex);
		prebufferPosition=stopPosition;
		lastBufferWasGapSignal=false;
		playPosition=stopPosition;

		prebufferedChunkPipe.clear(); // JJJ invalidate prebuffered data
	}

	/* takes too long for UI
	// create more prebuffered data
	prebufferChunk();
	*/

	// just in case it had stopped and the prebuffer pipe was draining, we need to start prebuffering again
	// QQQ* CMutexLocker l(prebufferThread.waitForPlayMutex);
	prebuffering=true;
	prebufferThread.waitForPlay.signal();
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


void CSoundPlayerChannel::mixOntoBuffer(const unsigned nChannels,sample_t * const _oBuffer,const size_t _oBufferLength)
{
	if(paused && seekSpeed==1.0/*not seeking*/)
		return;

	if(prebufferedChunkPipe.getSize()<=0)
	{
		// here the pipe has emptied and we're no longer prebuffering so go ahead and shut down the playing state
		if(!prebuffering)
			playingHasEnded();
		
		return;
	}
	
	const CSound &sound=*this->sound;
	const unsigned channelCount=sound.getChannelCount();

	const size_t deviceIndex=0; // ??? would loop through all devices later (probably actually in a more inner loop than here)

	// heed the sampling rate of the sound also when adjusting the play position (??? only have device 0 for now)
	const sample_fpos_t tPlaySpeed= fabs(playSpeedForMixer)  *  (((sample_fpos_t)sound.getSampleRate())/((sample_fpos_t)player->devices[deviceIndex].sampleRate));

	CMutexLocker l2(routingInfoMutex);	// protect routing info (??? should probably do a try-lock and bail if not locked to avoid problems with JACK)

	sample_t *oBuffer=_oBuffer;
	int outputBufferLength=_oBufferLength;
	sample_fpos_t last=0.0; // either 'last' or chunk->offset carries the initial part of the first sample already used in the previous calculation

	while(outputBufferLength>0 && !(paused && seekSpeed==1.0))
	{
		RPrebufferedChunk *chunk;
		// read data from prebuffer pipe (non-blocked -- if the data isn't there, let the audio have a gap)
		const int ret=prebufferedChunkPipe.peek(&chunk,1,false); // and blocking would be a bad idea since the clear() method (which locks the reader mutex) could cause this to wait indefinately when the sound is stopping
		if(ret<1)
			break;

		lastBufferWasGapSignal=chunk->isGap;
		playPosition=chunk->playPosition; // update the playPosition so it will be updated on screen

		const sample_fpos_t amountInChunk=(sample_fpos_t)chunk->size-chunk->offset;
		const int maxOutputLengthToUse=(int)ceil((amountInChunk-last)/tPlaySpeed); // max that could be produced
		const int outputLengthToUse=min(outputBufferLength,maxOutputLengthToUse); // length that there is room in the output buffer to produce
		const sample_fpos_t srcStart=chunk->offset+last;

		// mix onto the buffer according to the routing information
		bool didOutput=false;
		for(unsigned i=0;i<channelCount;i++)
		{
			sample_t * const dataBuffer= chunk->isGap ? (gapSignalBuffer+((chunk->gapSignalBufferOffset+1)*chunk->channelCount)) : chunk->data;
			if(!muted[i]) 
			{
				// returns a vector of bools indicating which channels in the output device to which we should write this channel's (i's) data
					// ??? since this accesses the pool file, it could be bad for JACK. if there were an inmemory copy of this routing info, it would be better for JACK's safe
				const vector<bool> deviceRoute=getOutputRoute(deviceIndex,i);
				for(size_t outputDeviceChannel=0;outputDeviceChannel<deviceRoute.size();outputDeviceChannel++)
				{
					if(deviceRoute[outputDeviceChannel])
					{
						register sample_t *ooBuffer=oBuffer+outputDeviceChannel;

						if(!chunk->isGap) // avoid modifying the gapSignalBuffer
							dataBuffer[-channelCount+i]=prevFrame[i];

						const size_t _outputLengthToUse=outputLengthToUse*nChannels;
						if(tPlaySpeed==1.0 && floor(srcStart)==srcStart)
						{ // simple 1.0 speed copy
							size_t p=((size_t)srcStart)*channelCount+i;
							for(size_t t=0;t<_outputLengthToUse;t+=nChannels)
							{
								ooBuffer[t]=ClipSample(ooBuffer[t]+dataBuffer[p]);
								p+=channelCount;
							}
							last=outputLengthToUse+(size_t)srcStart;
						}
						else
						{
							TSoundStretcher<sample_t *> stretcher(dataBuffer-channelCount,srcStart,(sample_fpos_t)outputLengthToUse*tPlaySpeed,(sample_fpos_t)outputLengthToUse,channelCount,i,true);
							for(size_t t=0;t<_outputLengthToUse;t+=nChannels)
								ooBuffer[t]=ClipSample(ooBuffer[t]+stretcher.getSample());
							last=stretcher.getCurrentSrcPosition();
						}
						didOutput=true;
					}
				}
			}

			// save the last sample so that we can use it for interpolation the next go around
			if(floor(last)==chunk->size)
				prevFrame[i]=dataBuffer[(chunk->size-1)*channelCount+i];
		}
		// if all channels were muted, or none were mapped to an output device 
		// we need to at least set last so the playing progress will advance
		if(!didOutput)
			last=srcStart+(outputLengthToUse*tPlaySpeed);

		outputBufferLength-=outputLengthToUse;
		oBuffer+=outputLengthToUse*nChannels;

		if(outputBufferLength<0)
			printf("SANITY CHECK -- outputBufferLength just went <0 %d  %d\n",outputBufferLength,outputLengthToUse);fflush(stdout);

		chunk->offset=last;
		if(chunk->offset>=(sample_fpos_t)chunk->size)
		{
			prebufferedChunkPipe.skip(1,false); // all of the chunk was used so move it out of the pipe
			last-=(sample_fpos_t)chunk->size;
		}
		else			
		{
			last=0.0; // actually shouldn't be necessary since the output buffer should have been exhausted at this point

			// sanity check
			if(outputBufferLength>0)
				printf("SANITY CHECK -- we're not skipping the data in the pipe and we're about to loop in this while loop again!!!\n");fflush(stdout);
		}
	}

	if(prebufferedChunkPipe.getSize()<=0 && !prebuffering)
	{ // here the pipe has emptied and we're no longer prebuffering so go ahead and shut down the playing state
		playingHasEnded();
	}
}

/*
 * This method simply prebuffers one chunk of data regardless of whether the channel is playing, paused, etc
 * And it may block if the prebufferedChunkPipe is full
 * 
 * Returns true if the endpoint was reached
 */
bool CSoundPlayerChannel::prebufferChunk()
{
	if(!playing || !prebuffering)
	{
		prebuffering=false;
		return(true);
	}

	if(paused && seekSpeed==1.0/*not seeking*/)
		return(false);

	const unsigned channelCount=sound->getChannelCount();

	// used if loopType==ltLoopSkipMost
	const sample_pos_t skipMiddleMargin=(sample_pos_t)(gSkipMiddleMarginSeconds*sound->getSampleRate());
	bool queueUpAGap=false;

	RPrebufferedChunk *chunk=getPrebufferChunk();

	sample_t *buffer=chunk->data;
	size_t bufferUsed=0;
	bool ret=false;

	sample_pos_t pos1=0,pos2=0; // declared out here incase I need their values when queuing up a gap


	// fill chunk with 1 chunk's worth of audio
	sound->lockSize();
	try
	{
		CMutexLocker l(prebufferPositionMutex); // protect prebufferPosition from set.*Position() /// NO, NOT REALLY NECESSARY SENSE WE"RE NOT GONNA CALL prebufferChunk() NOW FROM WITHIN set.*Position(): ... and assures that we aren't past this point simultaneously trying to calculate the same 1 extra prebuffer chunk that is in the prebufferedChunks vector

		const int positionInc=(seekSpeed>0) ? 1 : -1;

		const LoopTypes loopType=this->loopType; // store current value
		if(playSelectionOnly)
		{
			// pos1 is selectStart; pos2 is selectStop
			pos1=startPosition;
			pos2=stopPosition;
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

		chunk->playPosition=prebufferPosition;

		for(unsigned i=0;i<channelCount;i++)
		{
			const CRezPoolAccesser &src=sound->getAudio(i);

			sample_pos_t tPrebufferPosition=prebufferPosition;

			// ??? I should be able to know ahead of time an exact number of samples to copy before the loop point instead of having to check the loop point every iteration
				
			// avoid +i in (buffer[t*channelCount+i]) the loop by offsetting buffer 
			sample_t *_buffer=buffer+i;
			// avoid the *channelCount in (buffer[t*channelCount+i]) the loop by incrementing by the channelCount
			const size_t last=PREBUFFERED_CHUNK_SIZE*channelCount;

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
								queueUpAGap=true;
								/* I would like to do this but I will just have to settle for a little of the loop to be audible before the gap signal because of NOTE TTT
								break; // prebufferPosition becomes pos1 but we also stop putting data into the chunk so that we can immediately insert the gap signal into the queue
								*/
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
			}

			// update real prebufferPosition place holder on last iteration
			if(i==channelCount-1)
			{
				// if we're seeking faster than 5.0, then start skipping chunks (and handle loops points)
				const int skipAmount=playSpeedForChunker*PREBUFFERED_CHUNK_SIZE;
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

// may need to put this up in teh last channel case and skip what's there if queueUpAGap==true

		// if loopType is to skip most of the middle make pos2 the point to skip most of the middle
		if(loopType==ltLoopSkipMost)
		{
			// make sure there is at least 2 margins between pos1 and pos2
			// and that the gap to be placed between the two margines is shorter than what will be skipped
			if((pos2-pos1)>(skipMiddleMargin*2+gapSignalBufferLength))
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

	// set remainder of chunk to silence so we always write full chunks which keeps CPU usage down when chunks are actually 1 sample long and the play thread keeps reading 1 sample chunks
		// TTT
		// also, the stretching algorithm in the play thread messes up sometimes of 
		// successive chunks aren't the same length (isn't a problem I don't think 
		// with the idea of the algorithm just some particular about it that I'm not 
		// going to worry about since I will always write full chunks... someday it 
		// may crop up again)
	memset(chunk->data+bufferUsed,0,(PREBUFFERED_CHUNK_SIZE-(bufferUsed/channelCount))*channelCount*sizeof(sample_t));
	bufferUsed=PREBUFFERED_CHUNK_SIZE*channelCount; // I'm doing this because of the above note

	chunk->offset=0.0;
	chunk->size=bufferUsed/channelCount;
	chunk->isGap=false;

	// this is a blocked i/o write
	const int lengthWritten=prebufferedChunkPipe.write(&chunk,1);
	if(lengthWritten!=1)
		printf("SANITY CHECK --  uh oh.. didn't write the right amount of data\n");

	if(queueUpAGap)
	{ // we've been flagged to insert a gap of silence after the audio chunk

		// queue up the gap signal chunk

		const unsigned wholeChunkCount=gapSignalBufferLength/PREBUFFERED_CHUNK_SIZE;
		sample_pos_t gapSignalBufferOffset=0;
		for(unsigned t=0;t<wholeChunkCount;t++)
		{
			RPrebufferedChunk *chunk=getPrebufferChunk();

			if(loopType==ltLoopSkipMost)
			{ // make the play position appear to skip the middle
				const sample_fpos_t tpos1=pos1+skipMiddleMargin;
				const sample_fpos_t tpos2=pos2-skipMiddleMargin;
				if(seekSpeed>0)
					chunk->playPosition=(sample_pos_t)(tpos1+(((tpos2-tpos1)*t)/wholeChunkCount));
				else
					chunk->playPosition=(sample_pos_t)(tpos2+(((tpos1-tpos2)*t)/wholeChunkCount));
			}
			else
				chunk->playPosition=prebufferPosition;

			chunk->offset=0;
			chunk->size=PREBUFFERED_CHUNK_SIZE;
			chunk->isGap=true;
			chunk->gapSignalBufferOffset=gapSignalBufferOffset;

			gapSignalBufferOffset+=PREBUFFERED_CHUNK_SIZE;

			const int lengthWritten=prebufferedChunkPipe.write(&chunk,1);
			if(lengthWritten!=1)
				printf("SANITY CHECK --  uh oh.. didn't write the right amount of data (while inserting gap)\n");
		}

		if((gapSignalBufferLength-gapSignalBufferOffset)>0)
		{
			RPrebufferedChunk *chunk=getPrebufferChunk();

			chunk->playPosition=prebufferPosition;
			chunk->offset=0;
			chunk->size=gapSignalBufferLength-gapSignalBufferOffset;
			chunk->isGap=true;
			chunk->gapSignalBufferOffset=gapSignalBufferOffset;

			const int lengthWritten=prebufferedChunkPipe.write(&chunk,1);
			if(lengthWritten!=1)
				printf("SANITY CHECK --  uh oh.. didn't write the right amount of data (while inserting gap 2)\n");
		}
	}

	if(ret)
		prebuffering=false;

	return(ret);
}

const vector<int16_t> CSoundPlayerChannel::getOutputRoutes() const
{
	CMutexLocker l(routingInfoMutex);

	vector<int16_t> v;
	const TPoolAccesser<int16_t,CSound::PoolFile_t> a=sound->getGeneralDataAccesser<int16_t>("OutputRoutes_v2");
	v.reserve(a.getSize());
	for(size_t t=0;t<a.getSize();t++)
		v.push_back(a[t]);

	return(v);
}

void CSoundPlayerChannel::updateAfterEdit(const vector<int16_t> &restoreOutputRoutes)
{
	if(prebufferedChunks[0]->channelCount!=sound->getChannelCount() || prebufferedChunks[0]->sampleRate!=sound->getSampleRate())
	{ // channel count or sample rate has changed
		stop();
		CMutexLocker l(prebufferPositionMutex); // lock so that prebufferChunk won't be running

		// re-create prebuffer chunks
		destroyPrebufferedChunks();
		createPrebufferedChunks();

		// restore/recreate routing information
		TPoolAccesser<int16_t,CSound::PoolFile_t> a=sound->getGeneralDataAccesser<int16_t>("OutputRoutes_v2");
		if(restoreOutputRoutes.size()>1)
		{ // restore from what was given, trusting that it was saved from the information when it had the current number of channels
			CMutexLocker l2(routingInfoMutex);
			a.clear();
			a.append(restoreOutputRoutes.size());
			for(size_t t=0;t<restoreOutputRoutes.size();t++)
				a[t]=restoreOutputRoutes[t];
		}
		else
		{ // recreate the default routing
			CMutexLocker l2(routingInfoMutex);
			a.clear();
			createInitialOutputRoute();
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

		2 3 1 0 0 0 0 0 1 0 1 0
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
		throw(runtime_error(string(__func__)+" -- internal error -- somehow the initial route didn't get created and output route information was requested"));

	size_t p=0;
	const size_t deviceCount=a[p++];
	if(deviceIndex>=deviceCount)
		// ??? perhaps I don't want such a serious error on this
		throw(runtime_error(string(__func__)+" -- deviceIndex out of bounds: "+istring(deviceIndex)+">="+istring(deviceCount)));

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
		// ??? perhaps I don't want such a serious error on this
		throw(runtime_error(string(__func__)+" -- audioChannel out of bounds: "+istring(audioChannel)+">="+istring(rowCount)));

	vector<bool> row;
	p+=(columnCount*audioChannel); // skip to the requested row in the table
	for(size_t t=0;t<columnCount;t++)
		row.push_back( a[p++] ? true : false);

	return(row);
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

void CSoundPlayerChannel::createPrebufferedChunks()
{
	const unsigned channelCount=sound->getChannelCount();
	const unsigned sampleRate=sound->getSampleRate();

	for(size_t t=0;t<=CHUNK_COUNT_TO_PREBUFFER;t++) // I do one extra so the pipe can be full and one more can be calculating
		prebufferedChunks.push_back(new RPrebufferedChunk(channelCount,sampleRate));

	prebufferedChunksIndex=0;



	// generate the signal to be produced if the channel is producing the gap between or within loops (see enum LoopTypes)
	// I generate the signal as the entire length needed according to gLoopGapLengthSeconds, but I chop up this signal
	// into multiple parts when I queue up the chunks so that the play cursor can animate across the skipped portion

	gapSignalBufferLength=(sample_pos_t)(gLoopGapLengthSeconds*sampleRate);
		/* because of NOTE TTT I round the gapSignalBufferLength up to the nearest PREBUFFERED_CHUNK_SIZE to avoid having chunks that aren't of exactly PREBUFFERED_CHUNK_SIZE samples*/
		gapSignalBufferLength = ((gapSignalBufferLength/PREBUFFERED_CHUNK_SIZE)+1)*PREBUFFERED_CHUNK_SIZE;
	gapSignalBuffer=new sample_t[(gapSignalBufferLength+1)*channelCount];

	// set first frame to silence
	for(unsigned i=0;i<channelCount;i++)
		gapSignalBuffer[i]=0;

	// frist frame is reserved for something else (so generate the signal after it)
	sample_t *_gapSignalBuffer=gapSignalBuffer+channelCount;

	/*
	// light hiss
	for(unsigned i=0;i<channelCount;i++)
	{
		for(unsigned t=0;t<gapSignalBufferLength;t++)
			_gapSignalBuffer[t*channelCount+i]=ClipSample((float)rand()/RAND_MAX*MAX_SAMPLE/20.0);
	}
	*/

	// tone
	for(unsigned i=0;i<channelCount;i++)
	{
		const float freq1=300;
		const float freq2=freq1*1.6;
		const float freq3=freq2*1.7;
		for(unsigned t=0;t<gapSignalBufferLength;t++)
			_gapSignalBuffer[t*channelCount+i]=ClipSample(
			(0.5-0.5*cos(2*M_PI*t/gapSignalBufferLength))*	// fade in/out window
			((						// produce the tone
				sin(t*freq1/sampleRate*2.0*M_PI)+
				sin(t*freq2/sampleRate*2.0*M_PI)+
				sin(t*freq3/sampleRate*2.0*M_PI)
			)*MAX_SAMPLE/20.0));
	}

}

void CSoundPlayerChannel::destroyPrebufferedChunks()
{
	for(size_t t=0;t<prebufferedChunks.size();t++)
		delete prebufferedChunks[t];
	prebufferedChunks.clear();
	prebufferedChunksIndex=0;

	gapSignalBufferLength=0;
	delete [] gapSignalBuffer;
}

CSoundPlayerChannel::RPrebufferedChunk *CSoundPlayerChannel::getPrebufferChunk()
{
	RPrebufferedChunk *chunk=prebufferedChunks[prebufferedChunksIndex];
	prebufferedChunksIndex=(prebufferedChunksIndex+1)%prebufferedChunks.size();
	return chunk;
}


// --- RPrebufferedChunk -------------------------------

CSoundPlayerChannel::RPrebufferedChunk::RPrebufferedChunk(const unsigned _channelCount,const unsigned _sampleRate) :
	channelCount(_channelCount),
	sampleRate(_sampleRate)
{
	if(channelCount>MAX_CHANNELS)
		throw(runtime_error(string(__func__)+" -- channelCount is more than MAX_CHANNELS: "+istring(channelCount)+">"+istring(MAX_CHANNELS)));

	// allocate 1 extra frame for the possible 2-point sample interpolation (except it's always used whether interpolating or not)
	data=new sample_t[(PREBUFFERED_CHUNK_SIZE+1)*channelCount];
	data+=channelCount;
}

CSoundPlayerChannel::RPrebufferedChunk::~RPrebufferedChunk()
{
	data-=channelCount;
	delete [] data;
}

