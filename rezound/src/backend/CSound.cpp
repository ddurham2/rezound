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

#include "CSound.h"
#include "AStatusComm.h"

#include <math.h>
#include <stdio.h> // ??? just for console info printfs

#include <stdexcept>

#include <CPath.h>

#include <istring>

#include "settings.h"


/* TODO:
	- add try and catch around space modifier methods that call matchUpChannelLengths even upon exception
	- create MAX_LENGTH for an arbitrary maximum length incase I need to enforce such a limit
	- I need to rework the bounds checking not to ever add where+len since it could overflow, but do it correctly where it should never have a problem
		- make some macros for doing this since I do it in several places
		- I have already fixed some of the methods

	- the peak chunk mantainance is not absolutly perfect... if I delete a portion of the data and thus remove some of the 
	  entries in the peak chunk data, the min and maxes aren't exactly correct now because the PEAK_CHUNK_SIZE samples which 
	  were calculated for teh originaly data is now scewed a little on top of the new data... normally not enough to worry 
	  about in just a frontend rendering of it... However overtime this error in the peak chunk data accumulates which is 
	  why I have a refresh button which recalculates all the peak data.  Perhaps there is some other method which could better 
	  reduce or eliminate this error without completely recaclulating the peak chunk data anytime an operation is done on a 
	  channel... perhaps with undo, I could store the peak chunk data in temp pools just as I did the audio data and could 
	  just as readily restore it on undo
*/

// ??? I could just set peakChunk data for space that I know I just silenced to min=max=0 and not bother setting it dirty


#define PEAK_CHUNK_SIZE 500

// ??? probably check a static variable that doesn't require a lock if the application is not threaded
#define ASSERT_RESIZE_LOCK \
	if(!poolFile.isExclusiveLocked()) \
		throw(runtime_error(string(__func__)+" -- this CSound object has not been locked for resize"));

#define ASSERT_SIZE_LOCK \
	if(!poolFile.isExclusiveLocked() && poolFile.getSharedLockCount()<=0) \
		throw(runtime_error(string(__func__)+" -- this CSound object's size has not been locked"));


// current pool names for the current version
#define FORMAT_INFO_POOL_NAME "Format Info"
#define AUDIO_POOL_NAME "Channel "
#define PEAK_CHUNK_POOL_NAME "PeakChunk "
#define TEMP_AUDIO_POOL_NAME "TempAudioPool_"
#define CUES_POOL_NAME "Cues"
#define NOTES_POOL_NAME "UserNotes"

#define GET_WORKING_FILENAME(workDir,filename) (workDir+CPath::dirDelim+CPath(filename).baseName()+".pf$")

CSound::CSound() :
	poolFile(REZOUND_POOLFILE_BLOCKSIZE,REZOUND_WORKING_POOLFILE_SIGNATURE),

	size(0),
	sampleRate(0),
	channelCount(0),

	metaInfoPoolID(0),

	tempAudioPoolKeyCounter(1),

	_isModified(true),

	cueAccesser(NULL)
{
	for(unsigned t=0;t<MAX_CHANNELS;t++)
		peakChunkAccessers[t]=NULL;
}

CSound::CSound(const string &_filename,const unsigned _sampleRate,const unsigned _channelCount,const sample_pos_t _size) :
	poolFile(REZOUND_POOLFILE_BLOCKSIZE,REZOUND_WORKING_POOLFILE_SIGNATURE),

	size(0),
	sampleRate(0),
	channelCount(0),

	metaInfoPoolID(0),

	tempAudioPoolKeyCounter(1),

	_isModified(true),

	cueAccesser(NULL)
{
	for(unsigned t=0;t<MAX_CHANNELS;t++)
		peakChunkAccessers[t]=NULL;

	try
	{
		createWorkingPoolFile(_filename,_sampleRate,_channelCount,_size);
	}
	catch(...)
	{
		try 
		{
			poolFile.closeFile(false,true);
		} catch(...) {}
		throw;
	}
}

CSound::~CSound()
{
    // going away, so release all read locks
    // ??? add back, but implement correctly
/*
    while(poolFile.getSharedLockCount()>0)
        unlockSize();
*/
        
	if(poolFile.isOpen())
	{
		deletePeakChunkAccessers();
		deleteCueAccesser();
    		poolFile.closeFile(false,false);
	}
}

void CSound::changeWorkingFilename(const string newOriginalFilename)
{
	const string workDir=CPath(poolFile.getFilename()).dirName();
	poolFile.rename(GET_WORKING_FILENAME(workDir,newOriginalFilename));
}

void CSound::closeSound()
{
	// ??? probably should get a lock?
	deletePeakChunkAccessers();
	deleteCueAccesser();
	poolFile.closeFile(false,true);
}

// locks to keep the size from changing (multiple locks can be obtained of this type)
void CSound::lockSize() const
{
	poolFile.sharedLock();
}

bool CSound::trylockSize() const
{
	return(poolFile.sharedTrylock());
}

void CSound::unlockSize() const
{
	poolFile.sharedUnlock();
}

// locks to be able to change the size (only one lock can be obtained of this type)
void CSound::lockForResize() const
{
	poolFile.exclusiveLock();
}

bool CSound::trylockForResize() const
{
    return(poolFile.exclusiveTrylock());
}

void CSound::unlockForResize() const
{
	poolFile.exclusiveUnlock();
}

CSound::CInternalRezPoolAccesser CSound::getAudioInternal(unsigned channel)
{
	return(poolFile.getPoolAccesser<sample_t>(channelPoolIDs[channel]));
}

CSound::CInternalRezPoolAccesser CSound::getTempDataInternal(unsigned tempAudioPoolKey,unsigned channel)
{
	return(poolFile.getPoolAccesser<sample_t>(createTempAudioPoolName(tempAudioPoolKey,channel)));
}

CRezPoolAccesser CSound::getAudio(unsigned channel)
{
	if(channel>=channelCount)
		throw(runtime_error(string(__func__)+" -- invalid channel: "+istring(channel)));

	return(poolFile.getPoolAccesser<sample_t>(channelPoolIDs[channel]));
}

const CRezPoolAccesser CSound::getAudio(unsigned channel) const
{
	if(channel>=channelCount)
		throw(runtime_error(string(__func__)+" -- invalid channel: "+istring(channel)));

	return(poolFile.getPoolAccesser<sample_t>(channelPoolIDs[channel]));
}



CRezPoolAccesser CSound::getTempAudio(unsigned tempAudioPoolKey,unsigned channel)
{
	return(poolFile.getPoolAccesser<sample_t>(createTempAudioPoolName(tempAudioPoolKey,channel)));
}

const CRezPoolAccesser CSound::getTempAudio(unsigned tempAudioPoolKey,unsigned channel) const
{
	return(poolFile.getPoolAccesser<sample_t>(createTempAudioPoolName(tempAudioPoolKey,channel)));
}

/*
 *  - This method gets the peak chunk information between [dataPos,nextDataPos)
 *    where dataPos and nextDataPos are positions in the sample data.
 *  - That is, the min and max sample value between those data positions.  
 *  - This information is used to render the waveform data on screen.
 * 
 *  - This information is stored in a pool of RPeakChunk struct objects in the
 *    pool file for this sound object.  And a min and max is stored for every
 *    PEAK_CHUNK_SIZE samples.
 *  - The RPeakChunk struct has a bool flag, 'dirty', that get's set to true
 *    for every chunk that we need to re-calculate the min and max for.  This 
 *    is necessary when perhaps an action modifies the data, so the view on 
 *    screen also needs to change.
 *
 *  - Sometimes it reads this precalculated peak chunk information, and sometimes
 *    it reads the data directly.  If the nextDataPos-dataPos < PEAK_CHUNK_SIZE, then
 *    we should just read the data normally and find the min and max on the fly.
 *    Otherwise, we can use the precalculated information.
 *
 *  - When using the precalculated information:
 *	    - I could simply return peakChunks[floor(dataPos/PEAK_CHUNK_SIZE)], but I 
 *	      don't since the next time getPeakValue is called, it may have skipped over
 *	      more than a chunk size, so the min and max would not necessarily be accurate.
 *	    - So, I also have the caller pass in the dataPos of the next time this method
 *	      will be called and I combine the mins and maxes of all the chunks that will
 *	      be skipped over by the next call.
 *
 *  - Also, for practical reasons, it is okay if nextDataPos is >= getLength(), it's really
 *    'what WOULD be the next data position' not than it will be necessarily called with that
 *    position
 */
RPeakChunk CSound::getPeakData(unsigned channel,sample_pos_t dataPos,sample_pos_t nextDataPos,const CRezPoolAccesser &dataAccesser) const
{
	if(channel>=channelCount)
		throw(runtime_error(string(__func__)+" -- channel parameter is out of change: "+istring(channel)));

/*
if(dataPos==getLength()-1)
{
	RPeakChunk p;
	p.min=MIN_SAMPLE;
	p.max=MAX_SAMPLE;

	return(p);
}
*/

	// I could check the bounds in dataPos and nextDataPos, but I'll just let the caller have already done that
	if((nextDataPos-dataPos)<PEAK_CHUNK_SIZE)
	{ // just find min and max on the fly
		//printf("NOT using precalculated data\n");
		sample_t _min=dataAccesser[dataPos];
		sample_t _max=dataAccesser[dataPos];

		// don't attempt to read past the end of the data
		if(nextDataPos>dataAccesser.getSize())
			nextDataPos=dataAccesser.getSize();

		for(sample_pos_t t=dataPos+1;t<nextDataPos;t++)
		{
			sample_t s=dataAccesser[t];
			_min=min(_min,s);
			_max=max(_max,s);
		}

		RPeakChunk p;
		p.min=_min;
		p.max=_max;
		return(p);
	}
	else
	{
		// scale down and truncate the data positions to offsets into the peak chunk information 
		// which only ranges [0,size/PEAK_CHUNK_SIZE]
		sample_pos_t firstChunk=dataPos/PEAK_CHUNK_SIZE;
		sample_pos_t lastChunk=nextDataPos/PEAK_CHUNK_SIZE;

		RPeakChunk ret;

		CPeakChunkRezPoolAccesser &peakChunkAccesser=*(peakChunkAccessers[channel]);

		// don't attempt to read from skipped chunks that don't exist
		if(lastChunk>peakChunkAccesser.getSize())
			lastChunk=peakChunkAccesser.getSize();

		// - we combine all the mins and maxes of the peak chunk information between firstChunk and lastChunk
		// - Also, if any chunk is dirty along the way, we recalculate it
		for(sample_pos_t t=firstChunk;t<lastChunk;t++)
		{
			RPeakChunk &p=peakChunkAccesser[t];

			// if dirty, then recalculate for this chunk
			if(p.dirty)
			{
				sample_pos_t start=t*PEAK_CHUNK_SIZE;
				sample_pos_t end=start+PEAK_CHUNK_SIZE;
				if(start<getLength())
				{
					if(end>getLength())
						end=getLength();

					sample_t _min=dataAccesser[start];
					sample_t _max=dataAccesser[start];

					for(sample_pos_t i=start+1;i<end;i++)
					{
						sample_t s=dataAccesser[i];
						_min=min(_min,s);
						_max=max(_max,s);
					}

					p.min=_min;
					p.max=_max;
				}
				p.dirty=false;
			}

			if(t==firstChunk)
			{
				ret.min=p.min;
				ret.max=p.max;
			}
			else
			{
				ret.min=min(ret.min,p.min);
				ret.max=max(ret.max,p.max);
			}

		}
		
		return(ret);
	}
}

void CSound::invalidatePeakData(unsigned channel,sample_pos_t start,sample_pos_t stop)
{
	if(channel>=channelCount)
		throw(runtime_error(string(__func__)+" -- channel parameter is out of change: "+istring(channel)));

	// ??? check ranges of start and stop

	CPeakChunkRezPoolAccesser &peakChunkAccesser=*(peakChunkAccessers[channel]);

	// fudge one peak chunk size longer
	if(stop<MAX_LENGTH-PEAK_CHUNK_SIZE)
	{
		stop+=PEAK_CHUNK_SIZE;
		if(stop>getLength()-1)
			stop=getLength()-1;
	}

	const sample_pos_t firstChunk=start/PEAK_CHUNK_SIZE;
	const sample_pos_t lastChunk=stop/PEAK_CHUNK_SIZE;

	for(sample_pos_t t=firstChunk;t<=lastChunk;t++)
		peakChunkAccesser[t].dirty=true;
}

void CSound::invalidatePeakData(const bool doChannel[MAX_CHANNELS],sample_pos_t start,sample_pos_t stop)
{
	for(sample_pos_t t=0;t<channelCount;t++)
	{
		if(doChannel[t])
			invalidatePeakData(t,start,stop);
	}
}

void CSound::invalidateAllPeakData()
{
	deletePeakChunkAccessers();
	createPeakChunkAccessers();
}


void CSound::addChannel()
{
	prvAddChannel(true);
}

void CSound::prvAddChannel(bool addAudioSpaceForNewChannel)
{
	ASSERT_RESIZE_LOCK

	if((getChannelCount()+1)>MAX_CHANNELS)
		throw(runtime_error(string(__func__)+" -- adding another channel would exceed the maximum of "+istring(MAX_CHANNELS)+" channels"));

	const string audioPoolName=AUDIO_POOL_NAME+istring(channelCount+1);
	const string peakChunkPoolName=PEAK_CHUNK_POOL_NAME+istring(channelCount);

	peakChunkAccessers[channelCount]=NULL;
	bool addedToChannelCount=false;
	try
	{
		CInternalRezPoolAccesser audioPool=poolFile.createPool<sample_t>(audioPoolName);
		channelPoolIDs[channelCount]=poolFile.getPoolIdByName(audioPoolName);

		CPeakChunkRezPoolAccesser peakChunkPool=poolFile.createPool<RPeakChunk>(peakChunkPoolName);
		peakChunkAccessers[channelCount]=new CPeakChunkRezPoolAccesser(peakChunkPool);

		channelCount++;
		addedToChannelCount=true;

		if(addAudioSpaceForNewChannel)
			matchUpChannelLengths(getLength());
	}
	catch(...)
	{ // attempt to recover
		if(addedToChannelCount)
			channelCount--;

		poolFile.removePool(audioPoolName,false);
		delete peakChunkAccessers[channelCount];
		poolFile.removePool(peakChunkPoolName,false);

		throw;
	}
	
	saveMetaInfo();
}

void CSound::addChannels(unsigned where,unsigned count)
{
	ASSERT_RESIZE_LOCK

	if(where>getChannelCount())
		throw(runtime_error(string(__func__)+" -- where out of range: "+istring(where)+">"+istring(getChannelCount())));
	if((count+getChannelCount())>MAX_CHANNELS)
		throw(runtime_error(string(__func__)+" -- adding "+istring(count)+" channels would exceed the maximum of "+istring(MAX_CHANNELS)+" channels"));

	/*
	 * This way may seem a little obtuse, but it's the easiest way without 
	 * renaming pools, regetting poolIds, invalidating any outstanding 
	 * accessors (because now their poolIds are invalid) and stuff.. simply 
	 * add a channel to the end and move it where it needs to be and do this 
	 * for each channel to add
	 */

	size_t swapCount=getChannelCount()-where;
	for(unsigned t=0;t<count;t++)
	{
		// add a channel to the end
		addChannel();

		// through a series of swaps move that channel to where+t
		for(unsigned i=0;i<swapCount;i++)
			swapChannels(channelCount-i-1,channelCount-i-2,0,getLength());
	}
}

void CSound::removeChannel()
{
	ASSERT_RESIZE_LOCK

	if(getChannelCount()<=1)
		throw(runtime_error(string(__func__)+" -- removing a channel would cause channel count to go to zero"));

	const string audioPoolName=AUDIO_POOL_NAME+istring(channelCount);
	const string peakChunkPoolName=PEAK_CHUNK_POOL_NAME+istring(channelCount-1);

	poolFile.removePool(audioPoolName);
	poolFile.removePool(peakChunkPoolName);

	channelCount--;
	saveMetaInfo();
}

void CSound::removeChannels(unsigned where,unsigned count)
{
	ASSERT_RESIZE_LOCK

	if(where>getChannelCount())
		throw(runtime_error(string(__func__)+" -- where out of range: "+istring(where)+">"+istring(getChannelCount())));
	if(count>(getChannelCount()-where))
		throw(runtime_error(string(__func__)+" -- where/count out of range: "+istring(where)+"/"+istring(count)));
	if(where==0 && count>=getChannelCount())
		throw(runtime_error(string(__func__)+" -- removing "+istring(count)+" channels at "+istring(where)+" would cause the channel count to go to zero"));

	unsigned swapCount=channelCount-where-1;
	for(unsigned t=0;t<count;t++)
	{
		// through a series of swaps move the channel at where to the end
		if(swapCount>0)
		{
			for(unsigned i=0;i<swapCount;i++)
				swapChannels(where+i,where+i+1,0,getLength());
			swapCount--;
		}

		// remove the last channel
		removeChannel();
	}
}

int CSound::moveChannelsToTemp(const bool whichChannels[MAX_CHANNELS])
{
	ASSERT_RESIZE_LOCK

	unsigned removeCount=0;
	for(unsigned t=0;t<MAX_CHANNELS;t++)
	{
		removeCount+=whichChannels[t] ? 1 : 0;
		if(whichChannels[t] && t>=getChannelCount())
			throw(runtime_error(string(__func__)+" -- whichChannels specifies to remove channel "+istring(t)+" which does not exist"));
	}
	if(removeCount>=getChannelCount())
		throw(runtime_error(string(__func__)+" -- whichChannels specifies all the channels, which would cause the channel count to go to zero"));

	// through a series of swaps move all the channels to be removed to become the last channels in the sound
	removeCount=0;
	for(unsigned _t=getChannelCount();_t>0;_t--)
	{
		unsigned t=_t-1;
		if(whichChannels[t])
		{
			for(unsigned i=t;i<getChannelCount()-1-removeCount;i++)
				swapChannels(i,i+1,0,getLength());
			removeCount++;
		}
	}

	const int tempAudioPoolKey=tempAudioPoolKeyCounter++;

	// move the last 'removeCount' channels to a temp pool
	for(unsigned t=getChannelCount()-removeCount;t<getChannelCount();t++)
		moveDataOutOfChannel(tempAudioPoolKey,t,0,getLength());

	// remove the last 'removeCount' channels
	for(unsigned t=0;t<removeCount;t++)
		removeChannel();

	return(tempAudioPoolKey);
}

/*
 * - tempPoolKey is the value returned by the previous call to moveChannelsToTemp()
 * - whichChannels MUST match the whichChannels that was given to moveChannelsToTemp and 
 *   this sound MUST have the channel layout that it had after the return from.  The 
 *   sound MUST also be the same length as it was when moveChannelsToTemp() was called
 *   the previous call to moveChannelsToTemp()
 */
void CSound::moveChannelsFromTemp(int tempAudioPoolKey,const bool whichChannels[MAX_CHANNELS])
{
	ASSERT_RESIZE_LOCK

	// just make sure that we will be able to add the channels without exceeding MAX_CHANNELS
	unsigned addCount=0;
	for(unsigned t=0;t<MAX_CHANNELS;t++)
		addCount+=whichChannels[t] ? 1 : 0;
	if((addCount+getChannelCount())>MAX_CHANNELS)
		throw(runtime_error(string(__func__)+" -- re-adding the channels specified by whichChannels would exceed the maximum of "+istring(MAX_CHANNELS)+" channels"));

	// make sure all the temp audio pools exist for the channels specified by whichChannels and that they have the correct length
	// and note that the temp pool channel number would be as if it were at the end (k), because that's where it would have been when removed in moveChannelsToTemp
	int k=getChannelCount();
	for(unsigned t=0;t<MAX_CHANNELS;t++)
	{
		if(whichChannels[t])
		{
			if(!poolFile.containsPool(createTempAudioPoolName(tempAudioPoolKey,k)))
				throw(runtime_error(string(__func__)+" -- whichChannels specifies to add channel "+istring(t)+" from temp which does not exist as a temp audio pool"));
			if(getTempAudio(tempAudioPoolKey,k).getSize()!=getLength())
				throw(runtime_error(string(__func__)+" -- the length of the audio in this sound object is not the same now as it was when moveChannelsToTemp() was called"));
			k++;
		}
	}

	
	// move the temp audio pools back as channels at the end of the sound
	for(unsigned t=0;t<MAX_CHANNELS;t++)
	{
		if(whichChannels[t])
		{
			prvAddChannel(false);
			moveDataIntoChannel(tempAudioPoolKey,getChannelCount()-1,getChannelCount()-1,0,getLength(),true);
		}
	}
	
	// through a series of swaps put the channels back in their original positions
	unsigned movedChannelIndex=getChannelCount()-addCount;
	for(unsigned t=0;t<getChannelCount();t++)
	{
		if(whichChannels[t])
		{
			for(unsigned i=movedChannelIndex;i>t;i--)
				swapChannels(i-1,i,0,getLength());
			movedChannelIndex++;
		}
	}
}


static const bool isAllChannels(CSound *sound,const bool whichChannels[MAX_CHANNELS])
{
	unsigned count=0;
	for(unsigned t=0;t<sound->getChannelCount();t++)
		if(whichChannels[t])
			count++;
	return(count==sound->getChannelCount());
}


void CSound::addSpace(sample_pos_t where,sample_pos_t length,bool doZeroData)
{
	ASSERT_RESIZE_LOCK 

	bool whichChannels[MAX_CHANNELS];
	for(unsigned t=0;t<MAX_CHANNELS;t++)
		whichChannels[t]=t<getChannelCount() ? true : false;

	addSpace(whichChannels,where,length,doZeroData);
}

void CSound::addSpace(const bool whichChannels[MAX_CHANNELS],sample_pos_t where,sample_pos_t length,bool doZeroData,sample_pos_t maxLength)
{
	ASSERT_RESIZE_LOCK 

	if(where>size)
		throw(runtime_error(string(__func__)+" -- where parameter out of range: "+istring(where)));
/*
	if(length>MAX_LENGTH)
		throw(runtime_error(string(__func__)+" -- length parameter out of range: "+istring(length)));
*/

	for(unsigned t=0;t<getChannelCount();t++)
	{
		if(whichChannels[t])
			addSpaceToChannel(t,where,length,doZeroData);
	}

	if(isAllChannels(this,whichChannels))
		adjustCues(where,where+length);

	matchUpChannelLengths(maxLength);
}

void CSound::removeSpace(sample_pos_t where,sample_pos_t length)
{
	ASSERT_RESIZE_LOCK 

	bool whichChannels[MAX_CHANNELS];
	for(unsigned t=0;t<MAX_CHANNELS;t++)
		whichChannels[t]=t<getChannelCount() ? true : false;

	removeSpace(whichChannels,where,length);
}

void CSound::removeSpace(const bool whichChannels[MAX_CHANNELS],sample_pos_t where,sample_pos_t length,sample_pos_t maxLength)
{
	ASSERT_RESIZE_LOCK 

	if(where>size)
		throw(runtime_error(string(__func__)+" -- where parameter out of range: "+istring(where)));
	if(length>(size-where))
		throw(runtime_error(string(__func__)+" -- length parameter out of range: "+istring(length)));

	for(unsigned t=0;t<channelCount;t++)
	{
		if(whichChannels[t])
			removeSpaceFromChannel(t,where,length);
	}

	if(isAllChannels(this,whichChannels))
		adjustCues(where+length,where);

	matchUpChannelLengths(maxLength);
}

unsigned CSound::moveDataToTemp(const bool whichChannels[MAX_CHANNELS],sample_pos_t where,sample_pos_t length,sample_pos_t fudgeFactor,sample_pos_t maxLength)
{
	ASSERT_RESIZE_LOCK 

	return(moveDataToTempAndReplaceSpace(whichChannels,where,length,0,fudgeFactor));
}

unsigned CSound::copyDataToTemp(const bool whichChannels[MAX_CHANNELS],sample_pos_t where,sample_pos_t length)
{
	ASSERT_SIZE_LOCK 

	if(where>size)
		throw(runtime_error(string(__func__)+" -- where parameter out of range: "+istring(where)));
	if(length>(size-where))
		throw(runtime_error(string(__func__)+" -- length parameter out of range: "+istring(length)));

	const unsigned tempAudioPoolKey=tempAudioPoolKeyCounter++;

	for(unsigned t=0;t<channelCount;t++)
	{
		if(whichChannels[t])
			copyDataFromChannel(tempAudioPoolKey,t,where,length);
	}

	return(tempAudioPoolKey);
}

unsigned CSound::moveDataToTempAndReplaceSpace(const bool whichChannels[MAX_CHANNELS],sample_pos_t where,sample_pos_t length,sample_pos_t replaceLength,sample_pos_t fudgeFactor,sample_pos_t maxLength)
{
	ASSERT_RESIZE_LOCK 

	if(where>size)
		throw(runtime_error(string(__func__)+" -- where parameter out of range: "+istring(where)));
	if(length>(size-where))
		throw(runtime_error(string(__func__)+" -- length parameter out of range: "+istring(length)));
/*
	if(replaceLength>MAX_LENGTH)
		throw(runtime_error(string(__func__)+" -- replaceLength parameter out of range: "+istring(replaceLength)));
*/

	const unsigned tempAudioPoolKey=tempAudioPoolKeyCounter++;

	for(unsigned t=0;t<channelCount;t++)
	{
		if(whichChannels[t])
		{
			// move data
			moveDataOutOfChannel(tempAudioPoolKey,t,where,length);

			// handle fudgeFactor
			appendForFudgeFactor(getTempDataInternal(tempAudioPoolKey,t),getAudioInternal(t),where,fudgeFactor);

			// replace space
			addSpaceToChannel(t,where,replaceLength,false);
		}
	}

	if(isAllChannels(this,whichChannels))
		adjustCues(where+length,where+replaceLength);

	matchUpChannelLengths(maxLength);

	return(tempAudioPoolKey);
}

void CSound::moveDataFromTemp(const bool whichChannels[MAX_CHANNELS],unsigned tempAudioPoolKey,sample_pos_t moveWhere,sample_pos_t moveLength,bool removeTempAudioPools,sample_pos_t maxLength)
{
	ASSERT_RESIZE_LOCK 

	removeSpaceAndMoveDataFromTemp(whichChannels,0,0,tempAudioPoolKey,moveWhere,moveLength,removeTempAudioPools,maxLength);
}

void CSound::removeSpaceAndMoveDataFromTemp(const bool whichChannels[MAX_CHANNELS],sample_pos_t removeWhere,sample_pos_t removeLength,unsigned tempAudioPoolKey,sample_pos_t moveWhere,sample_pos_t moveLength,bool removeTempAudioPools,sample_pos_t maxLength)
{
	ASSERT_RESIZE_LOCK 

	if(removeLength!=0 && (removeWhere>size || removeLength>(size-removeWhere)))
		throw(runtime_error(string(__func__)+" -- removeWhere/removeLength parameter out of range: "+istring(removeWhere)+"/"+istring(removeLength)));
	if(moveWhere>(size-removeLength))
		throw(runtime_error(string(__func__)+" -- moveWhere parameter out of range: "+istring(moveWhere)+" for removeWhere "+istring(removeLength)));

	for(unsigned t=0;t<channelCount;t++)
	{
		if(whichChannels[t])
		{ // remove then add the space (by moving from temp pool)
			removeSpaceFromChannel(t,removeWhere,removeLength);
			moveDataIntoChannel(tempAudioPoolKey,t,t,moveWhere,moveLength,removeTempAudioPools);
		}
	}

	/* ???
	if(isAllChannels(this,whichChannels))
		// I may need to handle this.... but as of now I don't because it's use only 
		// for undo (and AAction takes care of restoring the cues) and it's a little 
		// complicated since we can remove from one place and add to another
	*/

	matchUpChannelLengths(maxLength);
}

void CSound::removeTempAudioPools(unsigned tempAudioPoolKey)
{
	for(unsigned t=0;t<MAX_CHANNELS;t++)
	{
		const string tempAudioPoolName=createTempAudioPoolName(tempAudioPoolKey,t);
		if(poolFile.containsPool(tempAudioPoolName))
			poolFile.removePool(tempAudioPoolName);
	}
}

void CSound::appendForFudgeFactor(CInternalRezPoolAccesser dest,const CInternalRezPoolAccesser src,sample_pos_t srcWhere,sample_pos_t fudgeFactor)
{
	if(fudgeFactor==0)
		return;

	sample_pos_t srcLength=src.getSize();

	const sample_pos_t l1=dest.getSize();
	dest.append(fudgeFactor);

	sample_pos_t t;
	for(t=0;t<fudgeFactor && (t+srcWhere)<srcLength;t++)
		dest[t+l1]=src[t+srcWhere];

	for(;t<fudgeFactor;t++) // 000
		dest[t+l1]=0;
}


void CSound::rotateLeft(const bool whichChannels[MAX_CHANNELS],const sample_pos_t start,const sample_pos_t stop,const sample_pos_t amount)
{
	ASSERT_RESIZE_LOCK 

	if(stop<start)
		throw(runtime_error(string(__func__)+" -- stop is less than start"));
	if(amount>(stop-start))
		throw(runtime_error(string(__func__)+" -- amount is greater than the distance between start and stop"));
	
	for(unsigned int i=0;i<channelCount;i++)
	{
		if(whichChannels[i])
		{
			CInternalRezPoolAccesser accesser=getAudioInternal(i);
			
			accesser.moveData(stop-amount+1,accesser,start,amount);
			invalidatePeakData(i,start,stop); // for want of a more efficient way (I already tried using move data on the peak accessers with the same parametesr as the previous call except /PEAK_CHUNK_SIZE, but error after many many operations was unacceptable)  ??? perhaps I need a flag which means 'how dirty' and it would absolutely recalculate it if it was just so dirty)
		}
	}

}

void CSound::rotateRight(const bool whichChannels[MAX_CHANNELS],const sample_pos_t start,const sample_pos_t stop,const sample_pos_t amount)
{
	ASSERT_RESIZE_LOCK 

	if(stop<start)
		throw(runtime_error(string(__func__)+" -- stop is less than start"));
	if(amount>(stop-start))
		throw(runtime_error(string(__func__)+" -- amount is greater than the distance between start and stop"));

	for(unsigned int i=0;i<channelCount;i++)
	{
		if(whichChannels[i])
		{
			CInternalRezPoolAccesser accesser=getAudioInternal(i);
			
			accesser.moveData(start,accesser,stop-amount+1,amount);
			invalidatePeakData(i,start,stop); // for want of a more efficient way 
		}
	}
}


void CSound::swapChannels(unsigned channelA,unsigned channelB,const sample_pos_t where,const sample_pos_t length)
{
	ASSERT_RESIZE_LOCK 

	if(channelA>=getChannelCount())
		throw(runtime_error(string(__func__)+" -- channelA is out of range: "+istring(channelA)+">="+istring(getChannelCount())));
	if(channelB>=getChannelCount())
		throw(runtime_error(string(__func__)+" -- channelB is out of range: "+istring(channelB)+">="+istring(getChannelCount())));

	if(where>size)
		throw(runtime_error(string(__func__)+" -- where parameter out of range: "+istring(where)));
	if(length>(size-where))
		throw(runtime_error(string(__func__)+" -- where/length parameter out of range: "+istring(where)+"/"+istring(length)));

	if(channelA==channelB)
		return;
	if(length<=0)
		return;


	const unsigned tempAudioPoolKeyA=tempAudioPoolKeyCounter++;
	const unsigned tempAudioPoolKeyB=tempAudioPoolKeyCounter++;

	// move data from each channel to its temp pool
	moveDataOutOfChannel(tempAudioPoolKeyA,channelA,where,length);
	moveDataOutOfChannel(tempAudioPoolKeyB,channelB,where,length);

	// move the data back into the channel from each temp pool but swapped (and pass true to remove the temp pool)
	moveDataIntoChannel(tempAudioPoolKeyA,channelA,channelB,where,length,true);
	moveDataIntoChannel(tempAudioPoolKeyB,channelB,channelA,where,length,true);
}


void CSound::addSpaceToChannel(unsigned channel,sample_pos_t where,sample_pos_t length,bool doZeroData)
{
	if(length==0)
		return;

	CInternalRezPoolAccesser accesser=getAudioInternal(channel);

	const sample_pos_t peakChunkCountHave=peakChunkAccessers[channel]==NULL ? 0 : peakChunkAccessers[channel]->getSize();
	const sample_pos_t peakChunkCountNeeded=calcPeakChunkCount(accesser.getSize()+length);

	// modify the audio data pools
	accesser.insert(where,length);

	if(doZeroData)
		accesser.zeroData(where,length);

	if(peakChunkAccessers[channel]!=NULL)
	{
		// add more peak chunks if the needed size is more than we have
		if(peakChunkCountNeeded>peakChunkCountHave)
		{
			const sample_pos_t insertWhere=where/PEAK_CHUNK_SIZE;
			const sample_pos_t insertCount=peakChunkCountNeeded-peakChunkCountHave;

			peakChunkAccessers[channel]->insert(insertWhere,insertCount);
			for(sample_pos_t t=0;t<insertCount;t++)
				(*(peakChunkAccessers[channel]))[insertWhere+t].dirty=true;
		}
		else // just mark the one we inserted into as dirty
			(*(peakChunkAccessers[channel]))[where/PEAK_CHUNK_SIZE].dirty=true;
	}
}

void CSound::removeSpaceFromChannel(unsigned channel,sample_pos_t where,sample_pos_t length)
{
	if(length==0)
		return;

	CInternalRezPoolAccesser accesser=getAudioInternal(channel);

	const sample_pos_t peakChunkCountHave=peakChunkAccessers[channel]==NULL ? 0 : peakChunkAccessers[channel]->getSize();
	const sample_pos_t peakChunkCountNeeded=calcPeakChunkCount(accesser.getSize()-length);

	accesser.remove(where,length);

	if(peakChunkAccessers[channel]!=NULL)
	{
		// modify the peak data pools if the size is dropping below the required size 
		if(peakChunkCountHave>peakChunkCountNeeded)
		{
			const sample_pos_t removeWhere=where/PEAK_CHUNK_SIZE;
			const sample_pos_t removeCount=peakChunkCountHave-peakChunkCountNeeded;

			peakChunkAccessers[channel]->remove(removeWhere,removeCount);
		}	

		// make the a the peak chunk at where recalculate
		if((where/PEAK_CHUNK_SIZE)<peakChunkAccessers[channel]->getSize())
			(*(peakChunkAccessers[channel]))[where/PEAK_CHUNK_SIZE].dirty=true;
	}
}

void CSound::copyDataFromChannel(unsigned tempAudioPoolKey,unsigned channel,sample_pos_t where,sample_pos_t length)
{
	CInternalRezPoolAccesser destAccesser=createTempAudioPool(tempAudioPoolKey,channel);

	if(length==0)
		return;

	CInternalRezPoolAccesser srcAccesser=getAudioInternal(channel);

	destAccesser.append(length);
	destAccesser.copyData(0,srcAccesser,where,length);
}

void CSound::moveDataOutOfChannel(unsigned tempAudioPoolKey,unsigned channel,sample_pos_t where,sample_pos_t length)
{
	CInternalRezPoolAccesser destAccesser=createTempAudioPool(tempAudioPoolKey,channel);

	if(length==0)
		return;

	CInternalRezPoolAccesser srcAccesser=getAudioInternal(channel);

	const sample_pos_t peakChunkCountHave=peakChunkAccessers[channel]==NULL ? 0 : peakChunkAccessers[channel]->getSize();
	const sample_pos_t peakChunkCountNeeded=calcPeakChunkCount(srcAccesser.getSize()-length);

	destAccesser.moveData(0,srcAccesser,where,length);

	if(peakChunkAccessers[channel]!=NULL)
	{
		// modify the peak data pools if the size is dropping below the required size 
		if(peakChunkCountHave>peakChunkCountNeeded)
		{
			const sample_pos_t removeWhere=where/PEAK_CHUNK_SIZE;
			const sample_pos_t removeCount=peakChunkCountHave-peakChunkCountNeeded;

			peakChunkAccessers[channel]->remove(removeWhere,removeCount);
		}

		// make the a the peak chunk at where recalculate
		if((where/PEAK_CHUNK_SIZE)<peakChunkAccessers[channel]->getSize())
			(*(peakChunkAccessers[channel]))[where/PEAK_CHUNK_SIZE].dirty=true;
	}
}

void CSound::moveDataIntoChannel(unsigned tempAudioPoolKey,unsigned channelInTempPool,unsigned channelInAudio,sample_pos_t where,sample_pos_t length,bool removeTempAudioPool)
{
	if(length==0)
		return;

	CInternalRezPoolAccesser destAccesser=getAudioInternal(channelInAudio);

	const sample_pos_t peakChunkCountHave=peakChunkAccessers[channelInAudio]==NULL ? 0 : peakChunkAccessers[channelInAudio]->getSize();
	const sample_pos_t peakChunkCountNeeded=calcPeakChunkCount(destAccesser.getSize()+length);

	CInternalRezPoolAccesser srcAccesser=getTempDataInternal(tempAudioPoolKey,channelInTempPool);
	if(length>srcAccesser.getSize())
		throw(runtime_error(string(__func__)+" -- length parameter out of range: "+istring(length)));
		
	destAccesser.moveData(where,srcAccesser,0,length);

	if(removeTempAudioPool)
		poolFile.removePool(createTempAudioPoolName(tempAudioPoolKey,channelInTempPool));

	if(peakChunkAccessers[channelInAudio]!=NULL)
	{
		// add more peak chunks if the needed size is more than we have
		if(peakChunkCountNeeded>peakChunkCountHave)
		{
			const sample_pos_t insertWhere=where/PEAK_CHUNK_SIZE;
			const sample_pos_t insertCount=peakChunkCountNeeded-peakChunkCountHave;

			peakChunkAccessers[channelInAudio]->insert(insertWhere,insertCount);
			for(sample_pos_t t=0;t<insertCount;t++)
				(*(peakChunkAccessers[channelInAudio]))[insertWhere+t].dirty=true;
		}
		else // just mark the one we inserted into as dirty
			(*(peakChunkAccessers[channelInAudio]))[where/PEAK_CHUNK_SIZE].dirty=true;
	}

}

void CSound::silenceSound(unsigned channel,sample_pos_t where,sample_pos_t length,bool doInvalidatePeakData,bool showProgressBar)
{
	ASSERT_SIZE_LOCK

	// ??? need a progress bar
	getAudio(channel).zeroData(where,length);
	if(doInvalidatePeakData)
		invalidatePeakData(channel,where,where+length);
}

#include "DSP/TSoundStretcher.h"

void CSound::mixSound(unsigned channel,sample_pos_t where,const CRezPoolAccesser src,sample_pos_t srcWhere,unsigned srcSampleRate,sample_pos_t length,MixMethods mixMethod,SourceFitTypes fitSrc,bool doInvalidatePeakData,bool showProgressBar)
{
	ASSERT_SIZE_LOCK

	if(srcSampleRate==0)
		throw(runtime_error(string(__func__)+" -- srcSampleRate is 0"));

	if(length==0)
		return;

	CRezPoolAccesser dest=getAudio(channel);
	const sample_pos_t destOffset=where;
	const unsigned destSampleRate=getSampleRate();

	switch(mixMethod)
	{
	case mmOverwrite:
		if(fitSrc!=sftNone)
		{
			if(fitSrc==sftChangeRate)
			{
				TSoundStretcher<CRezPoolAccesser> srcStretcher(src,srcWhere,src.getSize()-srcWhere,length);
				const sample_pos_t last=where+length;
				if(showProgressBar)
				{
					CStatusBar statusBar(_("Copying/Fitting Data -- Channel ")+istring(channel),where,last);
					for(sample_pos_t t=where;t<last;t++)
					{
						dest[t]=srcStretcher.getSample();
						statusBar.update(t);
					}
				}
				else
				{
					for(sample_pos_t t=where;t<last;t++)
						dest[t]=srcStretcher.getSample();
				}
			}
			else
				throw runtime_error(string(__func__)+" -- unimplemented fitSrc type: "+istring(fitSrc));
		}
		else if(srcSampleRate!=destSampleRate)
		{ // do sample rate conversion
			TSoundStretcher<CRezPoolAccesser> srcStretcher(src,srcWhere,(sample_pos_t)((sample_fpos_t)length/destSampleRate*srcSampleRate),length);
			const sample_pos_t last=where+length;
			if(showProgressBar)
			{
				CStatusBar statusBar(_("Copying Data -- Channel ")+istring(channel),where,last);
				for(sample_pos_t t=where;t<last;t++)
				{
					dest[t]=srcStretcher.getSample();
					statusBar.update(t);
				}
			}
			else
			{
				for(sample_pos_t t=where;t<last;t++)
					dest[t]=srcStretcher.getSample();
			}
		}
		else
		{
			// ??? need a progress bar
			dest.copyData(destOffset,src,srcWhere,length);
		}

		break;

	case mmAdd:
		if(fitSrc!=sftNone)
		{
			if(fitSrc==sftChangeRate)
			{
				TSoundStretcher<CRezPoolAccesser> srcStretcher(src,srcWhere,src.getSize()-srcWhere,length);
				const sample_pos_t last=where+length;
				if(showProgressBar)
				{
					CStatusBar statusBar(_("Mixing/Fitting Data (add) -- Channel ")+istring(channel),where,last);
					for(sample_pos_t t=where;t<last;t++)
					{
						dest[t]=ClipSample((mix_sample_t)dest[t]+(mix_sample_t)srcStretcher.getSample());
						statusBar.update(t);
					}
				}
				else
				{
					for(sample_pos_t t=where;t<last;t++)
						dest[t]=ClipSample((mix_sample_t)dest[t]+(mix_sample_t)srcStretcher.getSample());
				}
			}
			else
				throw runtime_error(string(__func__)+" -- unimplemented fitSrc type: "+istring(fitSrc));
		}
		else if(srcSampleRate!=destSampleRate)
		{
			TSoundStretcher<CRezPoolAccesser> srcStretcher(src,srcWhere,(sample_pos_t)((sample_fpos_t)length/destSampleRate*srcSampleRate),length);
			const sample_pos_t last=where+length;
			if(showProgressBar)
			{
				CStatusBar statusBar(_("Mixing Data (add) -- Channel ")+istring(channel),where,last);
				for(sample_pos_t t=where;t<last;t++)
				{
					dest[t]=ClipSample((mix_sample_t)dest[t]+(mix_sample_t)srcStretcher.getSample());
					statusBar.update(t);
				}
			}
			else
			{
				for(sample_pos_t t=where;t<last;t++)
					dest[t]=ClipSample((mix_sample_t)dest[t]+(mix_sample_t)srcStretcher.getSample());
			}
		}
		else
		{ // not fiting src and sample rates match
			const sample_pos_t last=where+length;
			if(showProgressBar)
			{
				CStatusBar statusBar(_("Mixing Data (add) -- Channel ")+istring(channel),where,last);
				for(sample_pos_t t=where;t<last;t++)
				{
					dest[t]=ClipSample((mix_sample_t)dest[t]+(mix_sample_t)src[srcWhere++]);
					statusBar.update(t);
				}
			}
			else
			{
				for(sample_pos_t t=where;t<last;t++)
					dest[t]=ClipSample((mix_sample_t)dest[t]+(mix_sample_t)src[srcWhere++]);
			}
		}

		break;

	case mmSubtract:
		if(fitSrc!=sftNone)
		{
			if(fitSrc==sftChangeRate)
			{
				TSoundStretcher<CRezPoolAccesser> srcStretcher(src,srcWhere,src.getSize()-srcWhere,length);
				const sample_pos_t last=where+length;
				if(showProgressBar)
				{
					CStatusBar statusBar(_("Mixing/Fitting Data (subtract) -- Channel ")+istring(channel),where,last);
					for(sample_pos_t t=where;t<last;t++)
					{
						dest[t]=ClipSample((mix_sample_t)dest[t]-(mix_sample_t)srcStretcher.getSample());
						statusBar.update(t);
					}
				}
				else
				{
					for(sample_pos_t t=where;t<last;t++)
						dest[t]=ClipSample((mix_sample_t)dest[t]-(mix_sample_t)srcStretcher.getSample());
				}
			}
			else
				throw runtime_error(string(__func__)+" -- unimplemented fitSrc type: "+istring(fitSrc));
		}
		else if(srcSampleRate!=destSampleRate)
		{
			TSoundStretcher<CRezPoolAccesser> srcStretcher(src,srcWhere,(sample_pos_t)((sample_fpos_t)length/destSampleRate*srcSampleRate),length);
			const sample_pos_t last=where+length;
			if(showProgressBar)
			{
				CStatusBar statusBar(_("Mixing Data (subtract) -- Channel ")+istring(channel),where,last);
				for(sample_pos_t t=where;t<last;t++)
				{
					dest[t]=ClipSample((mix_sample_t)dest[t]-(mix_sample_t)srcStretcher.getSample());
					statusBar.update(t);
				}
			}
			else
			{
				for(sample_pos_t t=where;t<last;t++)
					dest[t]=ClipSample((mix_sample_t)dest[t]-(mix_sample_t)srcStretcher.getSample());
			}
		}
		else
		{ // not fiting src and sample rates match
			const sample_pos_t last=where+length;
			if(showProgressBar)
			{
				CStatusBar statusBar(_("Mixing Data (subtract) -- Channel ")+istring(channel),where,last);
				for(sample_pos_t t=where;t<last;t++)
				{
					dest[t]=ClipSample((mix_sample_t)dest[t]-(mix_sample_t)src[srcWhere++]);
					statusBar.update(t);
				}
			}
			else
			{
				for(sample_pos_t t=where;t<last;t++)
					dest[t]=ClipSample((mix_sample_t)dest[t]-(mix_sample_t)src[srcWhere++]);
			}
		}

		break;

	case mmMultiply:
		if(fitSrc!=sftNone)
		{
			if(fitSrc==sftChangeRate)
			{
				TSoundStretcher<CRezPoolAccesser> srcStretcher(src,srcWhere,src.getSize()-srcWhere,length);
				const sample_pos_t last=where+length;
				if(showProgressBar)
				{
					CStatusBar statusBar(_("Mixing/Fitting Data (multiply) -- Channel ")+istring(channel),where,last);
					for(sample_pos_t t=where;t<last;t++)
					{
						dest[t]=ClipSample((mix_sample_t)dest[t]*(mix_sample_t)srcStretcher.getSample()/MAX_SAMPLE);
						statusBar.update(t);
					}
				}
				else
				{
					for(sample_pos_t t=where;t<last;t++)
						dest[t]=ClipSample((mix_sample_t)dest[t]*(mix_sample_t)srcStretcher.getSample()/MAX_SAMPLE);
				}
			}
			else
				throw runtime_error(string(__func__)+" -- unimplemented fitSrc type: "+istring(fitSrc));
		}
		else if(srcSampleRate!=destSampleRate)
		{
			TSoundStretcher<CRezPoolAccesser> srcStretcher(src,srcWhere,(sample_pos_t)((sample_fpos_t)length/destSampleRate*srcSampleRate),length);
			const sample_pos_t last=where+length;
			if(showProgressBar)
			{
				CStatusBar statusBar(_("Mixing Data (multiply) -- Channel ")+istring(channel),where,last);
				for(sample_pos_t t=where;t<last;t++)
				{
					dest[t]=ClipSample((mix_sample_t)dest[t]*(mix_sample_t)srcStretcher.getSample()/MAX_SAMPLE);
					statusBar.update(t);
				}
			}
			else
			{
				for(sample_pos_t t=where;t<last;t++)
					dest[t]=ClipSample((mix_sample_t)dest[t]*(mix_sample_t)srcStretcher.getSample()/MAX_SAMPLE);
			}
		}
		else
		{ // not fiting src and sample rates match
			const sample_pos_t last=where+length;
			if(showProgressBar)
			{
				CStatusBar statusBar(_("Mixing Data (multiply) -- Channel ")+istring(channel),where,last);
				for(sample_pos_t t=where;t<last;t++)
				{
					dest[t]=ClipSample((mix_sample_t)dest[t]*(mix_sample_t)src[srcWhere++]/MAX_SAMPLE);
					statusBar.update(t);
				}
			}
			else
			{
				for(sample_pos_t t=where;t<last;t++)
					dest[t]=ClipSample((mix_sample_t)dest[t]*(mix_sample_t)src[srcWhere++]/MAX_SAMPLE);
			}
		}

		break;

	case mmAverage:
		if(fitSrc!=sftNone)
		{
			if(fitSrc==sftChangeRate)
			{
				TSoundStretcher<CRezPoolAccesser> srcStretcher(src,srcWhere,src.getSize()-srcWhere,length);
				const sample_pos_t last=where+length;
				if(showProgressBar)
				{
					CStatusBar statusBar(_("Mixing/Fitting Data (average) -- Channel ")+istring(channel),where,last);
					for(sample_pos_t t=where;t<last;t++)
					{
						dest[t]=((mix_sample_t)dest[t]+(mix_sample_t)srcStretcher.getSample())/2;
						statusBar.update(t);
					}
				}
				else
				{
					for(sample_pos_t t=where;t<last;t++)
						dest[t]=((mix_sample_t)dest[t]+(mix_sample_t)srcStretcher.getSample())/2;
				}
			}
			else
				throw runtime_error(string(__func__)+" -- unimplemented fitSrc type: "+istring(fitSrc));
		}
		else if(srcSampleRate!=destSampleRate)
		{
			TSoundStretcher<CRezPoolAccesser> srcStretcher(src,srcWhere,(sample_pos_t)((sample_fpos_t)length/destSampleRate*srcSampleRate),length);
			const sample_pos_t last=where+length;
			if(showProgressBar)
			{
				CStatusBar statusBar(_("Mixing Data (average) -- Channel ")+istring(channel),where,last);
				for(sample_pos_t t=where;t<last;t++)
				{
					dest[t]=((mix_sample_t)dest[t]+(mix_sample_t)srcStretcher.getSample())/2;
					statusBar.update(t);
				}
			}
			else
			{
				for(sample_pos_t t=where;t<last;t++)
					dest[t]=((mix_sample_t)dest[t]+(mix_sample_t)srcStretcher.getSample())/2;
			}
		}
		else
		{ // not fiting src and sample rates match
			const sample_pos_t last=where+length;
			if(showProgressBar)
			{
				CStatusBar statusBar(_("Mixing Data (average) -- Channel ")+istring(channel),where,last);
				for(sample_pos_t t=where;t<last;t++)
				{
					dest[t]=((mix_sample_t)dest[t]+(mix_sample_t)src[srcWhere++])/2;
					statusBar.update(t);
				}
			}
			else
			{
				for(sample_pos_t t=where;t<last;t++)
					dest[t]=((mix_sample_t)dest[t]+(mix_sample_t)src[srcWhere++])/2;
			}
		}

		break;

	default:
		throw(runtime_error(string(__func__)+" -- unhandled mixMethod: "+istring(mixMethod)));
	}

	if(doInvalidatePeakData)
		invalidatePeakData(channel,where,where+length);
}






const string CSound::getTimePosition(sample_pos_t samplePos,int secondsDecimalPlaces,bool includeUnits) const
{
	const sample_fpos_t sampleRate=getSampleRate();
	const sample_fpos_t sTime=samplePos/sampleRate;

	string time;

	if(sTime>3600)
	{ // make it HH:MM:SS.sss
		const int hours=(int)(sTime/3600);
		const int mins=(int)((sTime-(hours*3600))/60);
		const double secs=sTime-((hours*3600)+(mins*60));

		time=istring(hours,2,true)+":"+istring(mins,2,true)+":"+istring(secs,3+secondsDecimalPlaces,secondsDecimalPlaces,true);
	}
	else
	{ // make it MM:SS.sss
		const int mins=(int)(sTime/60);
		const double secs=sTime-(mins*60);

		time=istring(mins,2,true)+":"+istring(secs,3+secondsDecimalPlaces,secondsDecimalPlaces,true);
	}

	if(includeUnits)
		return(time+"s");
	else
		return(time);
}

#include <stdio.h> // for sscanf
const sample_pos_t CSound::getPositionFromTime(const string time,bool &wasInvalid) const
{
	wasInvalid=false;
	sample_pos_t samplePos=0;

	if(istring(time).count(':')==2)
	{ // supposedly HH:MM:SS.sssss
		unsigned h=0,m=0;
		double s=0.0;
				// ??? this may be a potential porting issue
		sscanf(time.c_str()," %u:%u:%lf ",&h,&m,&s);
		samplePos=(sample_pos_t)(((sample_fpos_t)h*3600.0+(sample_fpos_t)m*60.0+(sample_fpos_t)s)*(sample_fpos_t)getSampleRate());
	}
	else if(istring(time).count(':')==1)
	{ // supposedly MM:SS.sssss
		unsigned m=0;
		double s=0.0;
				// ??? this may be a potential porting issue
		sscanf(time.c_str()," %u:%lf ",&m,&s);
		samplePos=(sample_pos_t)(((sample_fpos_t)m*60.0+(sample_fpos_t)s)*(sample_fpos_t)getSampleRate());
	}
	else if(istring(time).count(':')==0)
	{ // supposedly SSSS.sssss
		double s=0.0;
				// ??? this may be a potential porting issue
		sscanf(time.c_str()," %lf ",&s);
		samplePos=(sample_pos_t)(((sample_fpos_t)s)*(sample_fpos_t)getSampleRate());
	}
	else
	{
		wasInvalid=true;
		samplePos=0;
	}

	return(samplePos);
}

const string CSound::getAudioDataSize(const sample_pos_t sampleCount) const
{
	sample_fpos_t audioDataSize=sampleCount*sizeof(sample_t)*channelCount;
	if(audioDataSize>=1024*1024*1024)
	{ // return as gb
		return(istring(audioDataSize/1024.0/1024.0/1024.0,5,3)+"gb");
	}
	else if(audioDataSize>=1024*1024)
	{ // return as mb
		return(istring(audioDataSize/1024.0/1024.0,5,2)+"mb");
	}
	else if(audioDataSize>=1024)
	{ // return as kb
		return(istring(audioDataSize/1024.0,5,1)+"kb");
	}
	else
	{ // return as b
		return(istring((sample_pos_t)audioDataSize)+"b");
	}
}

const string CSound::getAudioDataSize() const
{
	return(getAudioDataSize(isEmpty() ? 0 : getAudio(0).getSize()));
}

const string CSound::getPoolFileSize() const
{
	sample_pos_t iPoolFileSize=poolFile.getFileSize();
	if(iPoolFileSize>=1024*1024*1024)
	{ // return as gb
		return(istring((sample_fpos_t)iPoolFileSize/1024.0/1024.0/1024.0,5,3)+"gb");
	}
	else if(iPoolFileSize>=1024*1024)
	{ // return as mb
		return(istring((sample_fpos_t)iPoolFileSize/1024.0/1024.0,5,2)+"mb");
	}
	else if(iPoolFileSize>=1024)
	{ // return as kb
		return(istring((sample_fpos_t)iPoolFileSize/1024.0,5,1)+"kb");
	}
	else
	{ // return as b
		return(istring(iPoolFileSize)+"b");
	}
}

void CSound::defragPoolFile()
{
	lockForResize();
	try
	{
		poolFile.defrag();
		unlockForResize();
	}
	catch(...)
	{
		unlockForResize();
		throw;
	}
}

void CSound::printSAT()
{
	poolFile.printSAT();
}

void CSound::verifySAT()
{
	poolFile.verifyAllBlockInfo(false);
}


void CSound::flush()
{
	poolFile.flushData();
}


/*
	Finds a working directory for a working file
	for the given filename.  A suitable working 
	directory should be writable and have free 
	space of 110% of the given file's size
*/
#include <sys/vfs.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
const string findWorkDir(const string filename)
{
	vector<string> workingDirs;
		workingDirs.push_back(CPath(filename).dirName()); // first try the dirname of the given filename
		workingDirs.push_back(gFallbackWorkDir); // next try to fallback working dir (in the future, this may be a list)

	for(size_t t=0;t<workingDirs.size();t++)
	{ 
		const string workDir=workingDirs[t];
		const string testFilename=workDir+CPath::dirDelim+"rezrez842rezrez";
		FILE *f=fopen(testFilename.c_str(),"wb");
		if(f!=NULL)
		{ // directry is writable
			fclose(f);
			remove(testFilename.c_str());

			struct statfs s;
			if(statfs(workDir.c_str(),&s)!=0)
			{
				int e=errno;
				fprintf(stderr,"error getting free space on working directory candidate: %s -- %s\n",workDir.c_str(),strerror(e));
				continue; // couldn't stat the fs for some reason
			}

				// ??? this would only be true if it's an uncompressed format
				// I really need a way to know how big the working file will be after loading
			const int64_t fsSize= (int64_t)s.f_bsize * (int64_t)s.f_bfree;
			const int64_t fileSize=CPath(filename).getSize(false);

			if(fsSize<(fileSize+(fileSize/10))) // ??? 10% overhead
			{
				fprintf(stderr,"insufficient free space in working directory candidate: %s\n",workDir.c_str());
				continue; // not enough free space on that partition
			}

			return(workDir);
		}
		else
		{
			int e=errno;
			fprintf(stderr,"cannot write to working directory candidate: %s -- %s\n",workDir.c_str(),strerror(e));
		}
	}

	throw(runtime_error(string(__func__)+" -- no suitable working directory available to load the file: "+filename));
}

void CSound::createWorkingPoolFile(const string originalFilename,const unsigned _sampleRate,const unsigned _channelCount,const sample_pos_t _size)
{
	if(poolFile.isOpen())
		throw(runtime_error(string(__func__)+" -- poolFile is already opened"));

	if(_channelCount>MAX_CHANNELS)
		throw(runtime_error(string(__func__)+" -- invalid number of channels: "+istring(_channelCount)));

	channelCount=_channelCount;
	sampleRate=_sampleRate;
	size=_size;

	// determine a suitable place for the working file
	const string workDir=findWorkDir(originalFilename);

	const string workingFilename=GET_WORKING_FILENAME(workDir,originalFilename);
	PoolFile_t::removeFile(workingFilename);
	poolFile.openFile(workingFilename,true);
	removeAllTempAudioPools();

	CFormatInfoPoolAccesser a=poolFile.createPool<RFormatInfo>(FORMAT_INFO_POOL_NAME);
	metaInfoPoolID=poolFile.getPoolIdByName(FORMAT_INFO_POOL_NAME);
	a.append(1);

	// create an audio pool for each channel
	for(unsigned t=0;t<channelCount;t++)
	{
		string poolName=AUDIO_POOL_NAME+istring(t+1);
		CInternalRezPoolAccesser a1=poolFile.createPool<sample_t>(poolName);
		channelPoolIDs[t]=poolFile.getPoolIdByName(poolName);
		a1.append(size);
	}

	// create all dirty peakChunks after we know the data size
	createPeakChunkAccessers();

	createCueAccesser();

	matchUpChannelLengths(NIL_SAMPLE_POS);

	saveMetaInfo();
}

bool CSound::createFromWorkingPoolFileIfExists(const string originalFilename,bool promptIfFound)
{
	if(poolFile.isOpen())
		throw(runtime_error(string(__func__)+" -- poolFile is already opened"));

	try
	{

		vector<string> workingDirs;
			workingDirs.push_back(CPath(originalFilename).dirName());
			workingDirs.push_back(gFallbackWorkDir);

		// look in all possible working spaces 
		string workingFilename="";
		for(size_t t=0;t<workingDirs.size();t++)
		{
			const string f=GET_WORKING_FILENAME(workingDirs[t],originalFilename);
				// ??? and we need to know that if this file is being used by any other loaded file.. then this is not the file and we need to alter the working filename at that point some how.. or just refuse to load the file at all
			if(CPath(f).exists())
			{
				workingFilename=f;
				break;
			}
		}

		if(workingFilename=="")
			return(false); // wasn't found

		if(promptIfFound)
		{
			// ??? probably have a cancel button to avoid loaded the sound at all.. probably throw an exception of a different type which is an ESkipLoadingFile
			if(Question(_("File: ")+workingFilename+"\n\n"+_("A temporary file was found indicating that this file was previously being edited when a crash occurred or the process was killed.\n\nDo you wish to attempt to recover from this temporary file (otherwise the file will be deleted)?"),yesnoQues)==noAns)
			{
				// ??? doesn't remove other files in the set of files if it was a multi file set >2gb
				PoolFile_t::removeFile(workingFilename);
				return(false);
			}
		}

		poolFile.openFile(workingFilename,false);
		_isModified=true;

		removeAllTempAudioPools();
		deletePeakChunkAccessers();

		deleteCueAccesser();

		// now that file is successfully opened, ask user if they want
		// to try to pick up where they left off or forget all edit
		// history

		metaInfoPoolID=poolFile.getPoolIdByName(FORMAT_INFO_POOL_NAME);

		// check version at the beginning of RFormat and perhaps handle things differently
		uint32_t version=0xffffffff;
		poolFile.readPoolRaw(metaInfoPoolID,&version,sizeof(version));
		if(version==1)
		{
			const CFormatInfoPoolAccesser a=poolFile.getPoolAccesser<RFormatInfo>(metaInfoPoolID);
			RFormatInfo r;
			r=a[0];

			if(r.size>MAX_LENGTH)
			{
				// ??? what should I do? truncate the sound or just error out?
			}

			size=r.size; // actually overwritten by matchUpChannelLengths
			sampleRate=r.sampleRate;
			channelCount=r.channelCount;
		}
		else
			throw(runtime_error(string(__func__)+" -- unhandled format version: "+istring(version)+" in found working file: "+workingFilename));

		if(channelCount<0 || channelCount>MAX_CHANNELS)
		{
			deletePeakChunkAccessers();
			deleteCueAccesser();
			poolFile.closeFile(false,false);
			throw(runtime_error(string(__func__)+" -- invalid number of channels: "+istring(channelCount)+" in found working file: "+workingFilename));
		}

		for(unsigned t=0;t<channelCount;t++)
			channelPoolIDs[t]=poolFile.getPoolIdByName(AUDIO_POOL_NAME+istring(t+1));


		// just in case the channels have different lengths
		matchUpChannelLengths(NIL_SAMPLE_POS);

		// create all dirty peakChunks after we know the data size
		createPeakChunkAccessers();

		createCueAccesser();

		saveMetaInfo();

		return(true);
	}
	catch(...)
	{
		// remove the file if it was corrupt and just has a problem opening
		deletePeakChunkAccessers();
		deleteCueAccesser();
		poolFile.closeFile(false,true);
		return(false);
	}
}

void CSound::setSampleRate(unsigned newSampleRate)
{
	sampleRate=newSampleRate;
	saveMetaInfo();
}

void CSound::saveMetaInfo()
{
	if(!poolFile.isOpen())
		throw(runtime_error(string(__func__)+" -- poolFile is not opened"));

	CFormatInfoPoolAccesser b=poolFile.getPoolAccesser<RFormatInfo>(metaInfoPoolID);
	if(b.getSize()==1)
	{
		RFormatInfo &r=b[0];

		// always write the newest format
		r.version=1;
		r.size=size;
		r.sampleRate=sampleRate;
		r.channelCount=channelCount;
	}
	else
	{
		RFormatInfo r;

		// always write the newest format
		r.version=1;
		r.size=size;
		r.sampleRate=sampleRate;
		r.channelCount=channelCount;

		poolFile.clearPool(metaInfoPoolID);
		poolFile.setPoolAlignment(metaInfoPoolID,sizeof(RFormatInfo));

		CFormatInfoPoolAccesser b=poolFile.getPoolAccesser<RFormatInfo>(metaInfoPoolID);
		b.append(1);
		b[0]=r;
	}

	// really slows things down especially for recording 
	// flush();
}

void CSound::createPeakChunkAccessers()
{
	if(poolFile.isOpen())
	{
		sample_pos_t peakCount=calcPeakChunkCount(size);
		for(unsigned i=0;i<channelCount;i++)
		{
			peakChunkAccessers[i]=new CPeakChunkRezPoolAccesser(poolFile.createPool<RPeakChunk>(PEAK_CHUNK_POOL_NAME+istring(i),false));
			peakChunkAccessers[i]->clear();
			peakChunkAccessers[i]->append(peakCount);
			for(sample_pos_t t=0;t<peakCount;t++)
				(*(peakChunkAccessers[i]))[t].dirty=true;
		}
	}
}

void CSound::deletePeakChunkAccessers()
{
	if(poolFile.isOpen())
	{
		for(unsigned t=0;t<MAX_CHANNELS;t++)
		{
			delete peakChunkAccessers[t];
			peakChunkAccessers[t]=NULL;
		}
	}
}

// returns the number of peak chunks that there needs to be for the given size
sample_pos_t CSound::calcPeakChunkCount(sample_pos_t givenSize)
{
	sample_pos_t v=((sample_pos_t)ceil(((sample_fpos_t)givenSize)/((sample_fpos_t)PEAK_CHUNK_SIZE)));
	if(v<=0)
		v=1;
	return(v);
}


const string CSound::createTempAudioPoolName(unsigned tempAudioPoolKey,unsigned channel)
{
	return(TEMP_AUDIO_POOL_NAME+istring(tempAudioPoolKey)+"_"+istring(channel));
}

CSound::CInternalRezPoolAccesser CSound::createTempAudioPool(unsigned tempAudioPoolKey,unsigned channel)
{
	return(poolFile.createPool<sample_t>(createTempAudioPoolName(tempAudioPoolKey,channel)));
}

void CSound::removeAllTempAudioPools()
{
	for(size_t t=0;t<poolFile.getPoolIndexCount();t++)
	{
		const PoolFile_t::poolId_t poolId=poolFile.getPoolIdByIndex(t);
		const string _poolName=poolFile.getPoolNameById(poolId);
		const char *poolName=_poolName.c_str();
			// ??? since I'm using strstr, string probably needs/has some string searching methods
		if(strstr(poolName,TEMP_AUDIO_POOL_NAME)==poolName)
		{
			poolFile.removePool(poolId);
			t--;
		}
	}
}


// appends silence to the end of any channel that is shorter than the longest one
void CSound::matchUpChannelLengths(sample_pos_t maxLength)
{
/*
	if(maxLength>MAX_LENGTH)
		throw(runtime_error(string(__func__)+" -- invalid maxLength: "+istring(maxLength)));
*/

	// get the max size of all the audio pools
	sample_pos_t maxAudioPoolSize=0;
	for(unsigned t=0;t<channelCount;t++)
		maxAudioPoolSize=max(maxAudioPoolSize,getAudioInternal(t).getSize());

	if(maxLength==NIL_SAMPLE_POS)
	{
		// add space to the end of any audio pool that is shorter
		for(unsigned t=0;t<channelCount;t++)
		{
			const sample_pos_t channelSize=getAudioInternal(t).getSize();
			if(channelSize<maxAudioPoolSize)
				addSpaceToChannel(t,channelSize,maxAudioPoolSize-channelSize,true);
		}
	}
	else
	{
		maxAudioPoolSize=min(maxAudioPoolSize,maxLength);

		// add space to the end of any audio pool that is shorter and truncate ones that are longer
		for(unsigned t=0;t<channelCount;t++)
		{
			const sample_pos_t channelSize=getAudioInternal(t).getSize();
			if(channelSize>maxAudioPoolSize)
				removeSpaceFromChannel(t,maxAudioPoolSize,channelSize-maxAudioPoolSize);
			else if(channelSize<maxAudioPoolSize)
				addSpaceToChannel(t,channelSize,maxAudioPoolSize-channelSize,true);
		}
	}

	size=maxAudioPoolSize;
	ensureNonZeroLength();
	saveMetaInfo();
}

void CSound::ensureNonZeroLength()
{
	if(size<=0)
	{
		for(unsigned t=0;t<channelCount;t++)
		{
			CInternalRezPoolAccesser a=getAudioInternal(t);
			a.clear();
			a.append(1);
			a[a.getSize()-1]=0;

			// I would also do it for peakChunkAccessers, but I handled that 
			// by always insisting on at least 1 chunk in calcPeakChunkCount
		}
		size=1;
	}
}

void CSound::setIsModified(bool v)
{
	_isModified=v;
}

const bool CSound::isModified() const
{
	return(_isModified);
}




// -----------------------------------------------------
// --- Cue Methods -------------------------------------
// -----------------------------------------------------

const size_t CSound::getCueCount() const
{
	return(cueAccesser->getSize());
}

const string CSound::getCueName(size_t index) const
{
	return((*cueAccesser)[index].name);
}

const sample_pos_t CSound::getCueTime(size_t index) const
{
	return((*cueAccesser)[index].time);
}

void CSound::setCueTime(size_t index,sample_pos_t newTime)
{
	if(index>cueAccesser->getSize())
		throw(runtime_error(string(__func__)+" -- invalid index: "+istring(index)));
	(*cueAccesser)[index].time=newTime;

	// update cueIndex
	rebuildCueIndex();
}

const bool CSound::isCueAnchored(size_t index) const
{
	return((*cueAccesser)[index].isAnchored);
}

void CSound::addCue(const string &name,const sample_pos_t time,const bool isAnchored)
{
	if(name.size()>=MAX_SOUND_CUE_NAME_LENGTH-1)
		throw(runtime_error(string(__func__)+" -- cue name too long"));

	cueAccesser->append(1);
	(*cueAccesser)[cueAccesser->getSize()-1]=RCue(name.c_str(),time,isAnchored);

	// update cueIndex
	cueIndex.insert(map<sample_pos_t,size_t>::value_type(time,cueAccesser->getSize()-1));
}

void CSound::insertCue(size_t index,const string &name,const sample_pos_t time,const bool isAnchored)
{
	if(name.size()>=MAX_SOUND_CUE_NAME_LENGTH-1)
		throw(runtime_error(string(__func__)+" -- cue name too long"));
	if(index>cueAccesser->getSize())
		throw(runtime_error(string(__func__)+" -- invalid index: "+istring(index)));

	cueAccesser->insert(index,1);
	(*cueAccesser)[index]=RCue(name.c_str(),time,isAnchored);

	// update cueIndex
	rebuildCueIndex();
}

void CSound::removeCue(size_t index)
{
	cueAccesser->remove(index,1);

	// update cueIndex
	rebuildCueIndex();
}

size_t CSound::__default_cue_index;
bool CSound::containsCue(const string &name,size_t &index) const
{
	for(size_t t=0;t<cueAccesser->getSize();t++)
	{
		if(strncmp((*cueAccesser)[t].name,name.c_str(),MAX_SOUND_CUE_NAME_LENGTH)==0)
		{
			index=t;
			return true;
		}
	}
	return false;
}

bool CSound::findCue(const sample_pos_t time,size_t &index) const
{
	const map<sample_pos_t,size_t>::const_iterator i=cueIndex.find(time);

	if(i!=cueIndex.end())
	{
		index=i->second;
		return true;
	}
	else
		return false;
}

bool CSound::findNearestCue(const sample_pos_t time,size_t &index,sample_pos_t &distance) const
{
	if(cueIndex.empty())
		return false;

	map<sample_pos_t,size_t>::const_iterator i=cueIndex.lower_bound(time);

	if(i!=cueIndex.begin())
	{
		if(i==cueIndex.end())
		{
			i--; // go back one
			index=i->second;
			distance=(sample_pos_t)sample_fpos_fabs((sample_fpos_t)time-(sample_fpos_t)i->first);
		}
		else
		{
			map<sample_pos_t,size_t>::const_iterator pi=i;
			pi--;

			const sample_pos_t d1=(sample_pos_t)sample_fpos_fabs((sample_fpos_t)time-(sample_fpos_t)pi->first);
			const sample_pos_t d2=(sample_pos_t)sample_fpos_fabs((sample_fpos_t)time-(sample_fpos_t)i->first);

			if(d1<d2)
			{
				index=pi->second;
				distance=d1;
			}
			else
			{
				index=i->second;
				distance=d2;
			}
		}
	}
	else
	{
		index=i->second;
		distance=(sample_pos_t)sample_fpos_fabs((sample_fpos_t)time-(sample_fpos_t)i->first);
	}

	return true;
}

bool CSound::findPrevCue(const sample_pos_t time,size_t &index) const
{
	map<sample_pos_t,size_t>::const_iterator i=cueIndex.find(time);
	if(i==cueIndex.end())
		return false;

	if(i!=cueIndex.begin())
		i--;
	index=i->second;
	return true;
}

bool CSound::findNextCue(const sample_pos_t time,size_t &index) const
{
	map<sample_pos_t,size_t>::const_iterator i=cueIndex.find(time);
	if(i==cueIndex.end())
		return false;

	i++;
	if(i==cueIndex.end())
		return false;
	index=i->second;
	return true;
}

const string CSound::getUnusedCueName(const string &prefix) const
{
	// ??? containsCue is not the most efficient, but we're not talking about huge amounts of data here
	for(unsigned t=1;t<200;t++)
	{
		if(!containsCue(prefix+istring(t)))
			return prefix+istring(t);
	}
	return "";
}

void CSound::clearCues()
{
	cueAccesser->clear();

	// update cueIndex
	cueIndex.clear();
}

/*
 * This method handles the adjustment of cues 
 * pos1 can be less than pos2 indicating an addition of space at pos1 for pos2-pos1 samples
 * or pos2 can be less then pos1 indicating a removal of space as pos2 for pos1-pos2 samples
 */
void CSound::adjustCues(const sample_pos_t pos1,const sample_pos_t pos2)
{
	if(pos1<pos2)
	{ // added data
		sample_pos_t addedLength=pos2-pos1;
		for(size_t t=0;t<getCueCount();t++)
		{
			if(isCueAnchored(t))
				continue; // ignore

			if(getCueTime(t)>=pos1)
				setCueTime(t,getCueTime(t)+addedLength);
		}
	}
	else // if(pos2<=pos1)
	{ // removed data
		sample_pos_t removedLength=pos1-pos2;
		for(size_t t=0;t<getCueCount();t++)
		{
			if(isCueAnchored(t))
				continue; // ignore

			if(getCueTime(t)>pos2 && getCueTime(t)<pos1)
				removeCue(t--);
			else if(getCueTime(t)>=pos1)
				setCueTime(t,getCueTime(t)-removedLength);
		}

	}

	// update cueIndex
	rebuildCueIndex();
}

void CSound::createCueAccesser()
{
	if(poolFile.isOpen())
	{
		poolFile.createPool<RCue>(CUES_POOL_NAME,false);
		cueAccesser=new CCuePoolAccesser(poolFile.getPoolAccesser<RCue>(CUES_POOL_NAME));

		// update cueIndex
		rebuildCueIndex();
	}
}

void CSound::deleteCueAccesser()
{
	if(poolFile.isOpen())
	{
		delete cueAccesser;
		cueAccesser=NULL;

		// update cueIndex
		cueIndex.clear();
	}
}

void CSound::rebuildCueIndex()
{
	cueIndex.clear();

	for(size_t t=0;t<cueAccesser->getSize();t++)
		cueIndex.insert(map<sample_pos_t,size_t>::value_type(getCueTime(t),t));
}

// -----------------------------------------------------
// -----------------------------------------------------
// -----------------------------------------------------


const string CSound::getUserNotes() const
{
	if(poolFile.containsPool(NOTES_POOL_NAME))
	{
		const TStaticPoolAccesser<int8_t,PoolFile_t> a=poolFile.getPoolAccesser<int8_t>(NOTES_POOL_NAME);

		string s;

		char buffer[101];
		for(size_t t=0;t<a.getSize()/100;t++)
		{
				// ??? here we need to assert that char and int8_t are the same
			a.read((int8_t *)buffer,100);
			buffer[100]=0;
			s+=buffer;
		}
		
			// ??? here we need to assert that char and int8_t are the same
		a.read((int8_t *)buffer,a.getSize()%100);
		buffer[a.getSize()%100]=0;
		s+=buffer;

		return(s);
	}
	else
		return("");
}

void CSound::setUserNotes(const string &notes)
{
	TPoolAccesser<int8_t,PoolFile_t> a=poolFile.containsPool(NOTES_POOL_NAME) ? poolFile.getPoolAccesser<int8_t>(NOTES_POOL_NAME) : poolFile.createPool<int8_t>(NOTES_POOL_NAME);

	a.clear();
		// ??? here we need to assert that char and int8_t are the same
	a.write((int8_t *)notes.c_str(),notes.size());
}


// this is the explicit instantiation of the TPoolFile for CSound's purposes
#include <TPoolFile.cpp>
template class TPoolFile<sample_pos_t,uint64_t>;

