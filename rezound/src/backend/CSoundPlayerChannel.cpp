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
#include "TSoundStretcher.h"

#define PREBUFFERED_CHUNK_SIZE 1024 // in frames
#define CHUNK_COUNT_TO_PREBUFFER 6

/* TODO
 * - Provisions have been made in here for supporting multiple simultaneous output devices, 
 *   however, ASoundPlayer doesn't haven't this notion yet.  When it does, it should pass
 *   mixOntoChannels multiple buffers, one for each device, and multiple nChannel values.
 * - I must do it this way so that all the play positioning can be done knowing
 *   the last iteration's results so it can save the new values in the data members.
 *   (synced positions among devices)
 *
 * - ??? Need to make the output routing a simple matrix of bools
 *   - or array of matricies to support muliple route configs
 * - Well, I made the provisions for assigning routes, but I did it in such a way that each
 *   channel in a sound file gets assigned to only 1 device and channel.  This sounds fine, 
 *   except, a loaded mono sound can only play out of one speaker... Perhaps I could handle
 *   them separately or alter the route defining structure to be more of a list of destiations
 *   instead of 1 value.
 *
 * - See AudioIO for more info about how these class fits into the whole picture
 *
 * - ??? If I *EVER* alter the number of channels in a CSound object, I must invalidate and
 *   recreate the prebuffereChunks.. probably should have the channel object stopped
 *   playing too
 *
 * - If I ever do have TSoundStretcher support more than 2 pointer sample interpolation then I 
 *   should make mixOntoBuffer support saving N samples instead of a fixed 1 samples.
 *
 */

CSoundPlayerChannel::CSoundPlayerChannel(ASoundPlayer *_player,CSound *_sound) :
	sound(_sound),
	prebufferPosition(0),
	prebufferThread(this),
	prebufferedChunkPipe(CHUNK_COUNT_TO_PREBUFFER),
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
	playPosition=0;
	startPosition=stopPosition=0;
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

void CSoundPlayerChannel::play(bool _playLooped,bool _playSelectionOnly)
{
	if(!player->isInitialized())
		throw(runtime_error(string(__func__)+" -- the sound player is not initialized"));

	if(!playing)
	{
		// zero out previous frame for interpolation
		for(size_t t=0;t<MAX_CHANNELS;t++)
			prevFrame[t]=0;

		playLooped=_playLooped;
		playSelectionOnly=_playSelectionOnly;
		if(playSelectionOnly)
		{
			prebufferPosition=startPosition;
			playPosition=startPosition;
		}
		else
		{
			prebufferPosition=0;
			playPosition=0;
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
	return(playing && playLooped);
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


	if((seekSpeed*origSeekSpeed)<0.0)
	{ // invalidate prebuffered data since the signs of the new and old play speeds where different (which means direction changed)
		prebufferedChunkPipe.clear();
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

	CMutexLocker l2(routingInfoMutex);	// protect routing info

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

		playPosition=chunk->playPosition; // update the playPosition so it will be updated on screen

		const sample_fpos_t amountInChunk=(sample_fpos_t)chunk->size-chunk->offset;
		const int maxOutputLengthToUse=(int)ceil((amountInChunk-last)/tPlaySpeed); // max that could be produced
		const int outputLengthToUse=min(outputBufferLength,maxOutputLengthToUse); // length that there is room in the output buffer to produce
		const sample_fpos_t srcStart=chunk->offset+last;

		// mix onto the buffer according to the routing information
		for(unsigned i=0;i<channelCount;i++)
		{
			if(muted[i]) 
			{ // if muted we still need to set last according to what the actual output loop would have produced
				last=srcStart+(outputLengthToUse*tPlaySpeed);
			}
			else
			{
				// ... here's where I could put up a for-loop to support mapping a single mono sound into both output channels
				// basically the output route should be a simple matrix of bools and the call to getOutputRouteParams would be just a simple matrix lookup

				unsigned outputDevice,outputDeviceChannel;
				getOutputRouteParams(outputRoute,i,outputDevice,outputDeviceChannel);

				outputDeviceChannel%=nChannels;
				register sample_t *ooBuffer=oBuffer+outputDeviceChannel;

				chunk->data[-channelCount+i]=prevFrame[i];

				const size_t _outputLengthToUse=outputLengthToUse*nChannels;
				if(tPlaySpeed==1.0 && floor(srcStart)==srcStart)
				{ // simple 1.0 speed copy
					size_t p=((size_t)srcStart)*channelCount+i;
					for(size_t t=0;t<_outputLengthToUse;t+=nChannels)
					{
						ooBuffer[t]=ClipSample(ooBuffer[t]+chunk->data[p]);
						p+=channelCount;
					}
					last=outputLengthToUse+(size_t)srcStart;
				}
				else
				{
					TSoundStretcher<sample_t *> stretcher(chunk->data-channelCount,srcStart,(sample_fpos_t)outputLengthToUse*tPlaySpeed,(sample_fpos_t)outputLengthToUse,channelCount,i,true);
					for(size_t t=0;t<_outputLengthToUse;t+=nChannels)
						ooBuffer[t]=ClipSample(ooBuffer[t]+stretcher.getSample());
					last=stretcher.getCurrentSrcPosition();
				}
			}

			// save the last sample so that we can use it for interpolation the next go around
			if(floor(last)==chunk->size)
				prevFrame[i]=chunk->data[(chunk->size-1)*channelCount+i];
		}

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


	RPrebufferedChunk *chunk=prebufferedChunks[prebufferedChunksIndex];
	prebufferedChunksIndex=(prebufferedChunksIndex+1)%prebufferedChunks.size();

	sample_t *buffer=chunk->data;
	size_t bufferUsed=0;
	bool ret=false;

	// fill chunk with 1 chunk's worth of audio
	sound->lockSize();
	try
	{
		CMutexLocker l(prebufferPositionMutex); // protect prebufferPosition from set.*Position() /// NO, NOT REALLY NECESSARY SENSE WE"RE NOT GONNA CALL prebufferChunk() NOW FROM WITHIN set.*Position(): ... and assures that we aren't past this point simultaneously trying to calculate the same 1 extra prebuffer chunk that is in the prebufferedChunks vector

		const int positionInc=(seekSpeed>0) ? 1 : -1;

		const bool loop=playLooped;
		sample_pos_t pos1,pos2;
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
						if(loop)
							tPrebufferPosition=pos1;
						else
						{
							ret=true;
							break;
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
						if(loop)
							tPrebufferPosition=pos1;
						else
						{
							tPrebufferPosition=pos2;
							ret=true;
						}
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

		sound->unlockSize();
	}
	catch(...)
	{
		sound->unlockSize();
		throw; // ??? hmm.. what would I want to do here
	}

	// set remainder of chunk to silence so we always write full chunks which keeps CPU usage down when chunks are actually 1 sample long and the play thread keeps reading 1 sample chunks
		// also, the stretching algorithm in the play thread messes up sometimes of 
		// successive chunks aren't the same length (isn't a problem I don't think 
		// with the idea of the algorithm just some particular about it that I'm not 
		// going to worry about since I will always write full chunks... someday it 
		// may crop up again)
	memset(chunk->data+bufferUsed,0,(PREBUFFERED_CHUNK_SIZE-(bufferUsed/channelCount))*channelCount*sizeof(sample_t));
	bufferUsed=PREBUFFERED_CHUNK_SIZE*channelCount;

	chunk->offset=0.0;
	chunk->size=bufferUsed/channelCount;

	// this is a blocked i/o write
	const int lengthWritten=prebufferedChunkPipe.write(&chunk,1);
	if(lengthWritten!=1)
		printf("SANITY CHECK --  uh oh.. didn't write the right amount of data\n");

	if(ret)
		prebuffering=false;

	return(ret);
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
	CMutexLocker l(routingInfoMutex);

	TPoolAccesser<ROutputRoute,CSound::PoolFile_t> a=sound->getGeneralDataAccesser<ROutputRoute>("OutputRoutes");

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
	/*const*/ TPoolAccesser<ROutputRoute,CSound::PoolFile_t> a=sound->getGeneralDataAccesser<ROutputRoute>("OutputRoutes");
	if(route>=a.getSize())
		route=0; // if out of bounds, use default

	outputDevice=a[route][channel].outputDevice;
	outputDeviceChannel=a[route][channel].outputDeviceChannel;
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


// --- RPrebufferedChunk -------------------------------

CSoundPlayerChannel::RPrebufferedChunk::RPrebufferedChunk(unsigned _channelCount) :
	channelCount(_channelCount)
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

void CSoundPlayerChannel::createPrebufferedChunks()
{
	for(size_t t=0;t<=CHUNK_COUNT_TO_PREBUFFER;t++) // I do one extra one so the pipe can be full and one can be calculating
		prebufferedChunks.push_back(new RPrebufferedChunk(sound->getChannelCount()));
	prebufferedChunksIndex=0;
}

void CSoundPlayerChannel::destroyPrebufferedChunks()
{
	for(size_t t=0;t<prebufferedChunks.size();t++)
		delete prebufferedChunks[t];
	prebufferedChunks.clear();
}


