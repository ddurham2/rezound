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

#include <stdexcept>

#include <cc++/path.h>

#include <istring>


/* TODO:
	- add try and catch around space modifier methods that call matchUpChannelLengths even upon exception
	- create MAX_LENGTH for an arbitrary maximum length incase I need to enforce such a limit
	- I need to rework the bounds checking not to ever add where+len since it could overflow, but do it correctly where it should never have a problem
		- make some macros for doing this since I do it in several places

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


#define CURRENT_VERSION 1
// current pool names for the current version
#define FORMAT_INFO_POOL_NAME "Format Info"
#define AUDIO_POOL_NAME "Channel "
#define PEAK_CHUNK_POOL_NAME "PeakChunk "
#define TEMP_AUDIO_POOL_NAME "TempAudioPool_"
#define CUES_POOL_NAME "Cues"
#define NOTES_POOL_NAME "UserNotes"


CSound::CSound() :
	poolFile(REZOUND_POOLFILE_BLOCKSIZE,REZOUND_POOLFILE_SIGNATURE),

	size(0),
	sampleRate(0),
	channelCount(0),

	metaInfoPoolID(0),

	tempAudioPoolKeyCounter(0),

	_isModified(true),

	cueAccesser(NULL)
{
	for(unsigned t=0;t<MAX_CHANNELS;t++)
		peakChunkAccessers[t]=NULL;

}

CSound::CSound(const string &_filename,const unsigned _sampleRate,const unsigned _channelCount,const sample_pos_t _size) :
	poolFile(REZOUND_POOLFILE_BLOCKSIZE,REZOUND_POOLFILE_SIGNATURE),

	size(0),
	sampleRate(0),
	channelCount(0),

	metaInfoPoolID(0),

	tempAudioPoolKeyCounter(0),

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
    // destructor of descendent should saveSound???


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
	poolFile.rename(getWorkingFilename(newOriginalFilename));
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
    if(channel>=channelCount)
        throw(runtime_error(string(__func__)+" -- invalid channel: "+istring(channel)));

    return(poolFile.getPoolAccesser<sample_t>(createTempAudioPoolName(tempAudioPoolKey,channel)));
}

const CRezPoolAccesser CSound::getTempAudio(unsigned tempAudioPoolKey,unsigned channel) const
{
    if(channel>=channelCount)
        throw(runtime_error(string(__func__)+" -- invalid channel: "+istring(channel)));

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

void CSound::deleteChannel(unsigned channel)
{

	throw(runtime_error(string(__func__)+" -- unimplemented"));
#if 0
/*
    poolFile.exclusiveLock();
    try
    {
*/
        if(channel<1 || channel>=channelCount)
            throw(runtime_error(string(__func__)+" -- (cannot delete channel when only one exists) channel parameter out of range: "+istring(channel)));

	// need to rename the pools
	// need to make channelPoolIDs a TBasicList so I can remove the item
	// need to remove the peak data pools for this channel
		// and therefore modify peakChunkAccessers (make a TBasicList also)
	
	
        poolFile.removePool(channelPoolIDs[channel]);
        channelCount--;
/*
        poolFile.exclusiveUnlock();
    }
    catch(...)
    {
        poolFile.exclusiveUnlock();
        throw;
    }
*/
#endif
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
	if(where+length>size)
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
	if(where+length>size)
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
	if(where+length>size)
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

	if(removeLength!=0 && removeWhere+removeLength>size)
		throw(runtime_error(string(__func__)+" -- removeWhere/removeLength parameter out of range: "+istring(removeWhere)+"/"+istring(removeLength)));
	if(moveWhere>(size-removeLength))
		throw(runtime_error(string(__func__)+" -- moveWhere parameter out of range: "+istring(moveWhere)+" for removeWhere "+istring(removeLength)));

	for(unsigned t=0;t<channelCount;t++)
	{
		if(whichChannels[t])
		{ // remove then add the space (by moving from temp pool)
			removeSpaceFromChannel(t,removeWhere,removeLength);
			moveDataIntoChannel(tempAudioPoolKey,t,moveWhere,moveLength,removeTempAudioPools);
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

void CSound::moveDataIntoChannel(unsigned tempAudioPoolKey,unsigned channel,sample_pos_t where,sample_pos_t length,bool removeTempAudioPool)
{
	if(length==0)
		return;

	CInternalRezPoolAccesser destAccesser=getAudioInternal(channel);

	const sample_pos_t peakChunkCountHave=peakChunkAccessers[channel]==NULL ? 0 : peakChunkAccessers[channel]->getSize();
	const sample_pos_t peakChunkCountNeeded=calcPeakChunkCount(destAccesser.getSize()+length);

	CInternalRezPoolAccesser srcAccesser=getTempDataInternal(tempAudioPoolKey,channel);
	if(length>srcAccesser.getSize())
		throw(runtime_error(string(__func__)+" -- length parameter out of range: "+istring(length)));
		
	destAccesser.moveData(where,srcAccesser,0,length);

	if(removeTempAudioPool)
		poolFile.removePool(createTempAudioPoolName(tempAudioPoolKey,channel));

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

void CSound::silenceSound(unsigned channel,sample_pos_t where,sample_pos_t length,bool doInvalidatePeakData,bool showProgressBar)
{
	ASSERT_SIZE_LOCK

	// ??? need a progress bar
	getAudio(channel).zeroData(where,length);
	if(doInvalidatePeakData)
		invalidatePeakData(channel,where,where+length);
}

void CSound::mixSound(unsigned channel,sample_pos_t where,const CRezPoolAccesser src,sample_pos_t srcWhere,sample_pos_t length,MixMethods mixMethod,bool doInvalidatePeakData,bool showProgressBar)
{
	ASSERT_SIZE_LOCK

	CRezPoolAccesser dest=getAudio(channel);

	const sample_pos_t destOffset=where;

	switch(mixMethod)
	{
	case mmOverwrite:
		// ??? need a progress bar
		dest.copyData(destOffset,src,srcWhere,length);
		break;
	
	case mmAdd:
		{
		const sample_pos_t last=where+length;
		//sample_pos_t srcPos=srcWhere;
		if(showProgressBar)
		{
			BEGIN_PROGRESS_BAR("Mixing Data -- Channel "+istring(channel),where,last);
			for(sample_pos_t t=where;t<last;t++)
			{
				dest[t]=ClipSample((mix_sample_t)dest[t]+(mix_sample_t)src[srcWhere++]);

				UPDATE_PROGRESS_BAR(t);
			}
			END_PROGRESS_BAR();
		}
		else
		{
			for(sample_pos_t t=where;t<last;t++)
				dest[t]=ClipSample((mix_sample_t)dest[t]+(mix_sample_t)src[srcWhere++]);
		}

		break;
		}

	case mmMultiply:
		{
		const sample_pos_t last=where+length;
		//sample_pos_t srcPos=srcWhere;
		if(showProgressBar)
		{
			BEGIN_PROGRESS_BAR("Mixing Data -- Channel "+istring(channel),where,last);
			for(sample_pos_t t=where;t<last;t++)
			{
				dest[t]=ClipSample((mix_sample_t)dest[t]*(mix_sample_t)src[srcWhere++]);

				UPDATE_PROGRESS_BAR(t);
			}
			END_PROGRESS_BAR();
		}
		else
		{	
			for(sample_pos_t t=where;t<last;t++)
				dest[t]=ClipSample((mix_sample_t)dest[t]*(mix_sample_t)src[srcWhere++]);
		}
		break;
		}

	case mmAverage:
		{
		const sample_pos_t last=where+length;
		//sample_pos_t srcPos=srcWhere;
		if(showProgressBar)
		{
			BEGIN_PROGRESS_BAR("Mixing Data -- Channel "+istring(channel),where,last);
			for(sample_pos_t t=where;t<last;t++)
			{
				dest[t]=((mix_sample_t)dest[t]+(mix_sample_t)src[srcWhere++])/2;

				UPDATE_PROGRESS_BAR(t);
			}
			END_PROGRESS_BAR();
		}
		else
		{
			for(sample_pos_t t=where;t<last;t++)
				dest[t]=((mix_sample_t)dest[t]+(mix_sample_t)src[srcWhere++])/2;
		}
		break;
		}

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



void CSound::flush()
{
	poolFile.flushData();
}

/*
	By the time this is called, all the protected data members about format info have be set
*/
void CSound::createWorkingPoolFile(const string originalFilename,const unsigned _sampleRate,const unsigned _channelCount,const sample_pos_t _size)
{
	if(poolFile.isOpen())
		throw(runtime_error(string(__func__)+" -- poolFile is already opened"));

	if(_channelCount>MAX_CHANNELS)
		throw(runtime_error(string(__func__)+" -- invalid number of channels: "+istring(_channelCount)));

	channelCount=_channelCount;
	sampleRate=_sampleRate;
	size=_size;

	const string filename=getWorkingFilename(originalFilename);
	remove(filename.c_str());
	poolFile.openFile(filename,true);
	removeAllTempAudioPools();

	CFormat1InfoPoolAccesser a=poolFile.createPool<RFormatInfo1>(FORMAT_INFO_POOL_NAME);
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
		if(promptIfFound)
		{
			if(ost::Path(getWorkingFilename(originalFilename)).Exists())
			{
				// ??? probably have a cancel button to avoid loaded the sound at all.. probably throw an exception of a different type which is an ESkipLoadingFile
				if(Question("File: "+getWorkingFilename(originalFilename)+"\n\nA temporary file was found indicating that this file was previously being edited when a crash occurred or the process was killed.\n\nDo you wish to attempt to recover from this temporary file (otherwise the file will be deleted)?",yesnoQues)==noAns)
				{
						// ??? doesn't remove other file sin the set if it was a multi file set >2gb
					remove(getWorkingFilename(originalFilename).c_str());
					return(false);
				}
			}
			else
				return(false);
		}

		poolFile.openFile(getWorkingFilename(originalFilename),false);
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
			const CFormat1InfoPoolAccesser a=poolFile.getPoolAccesser<RFormatInfo1>(metaInfoPoolID);
			RFormatInfo1 r;
			r=a[0];

			if(r.size>MAX_LENGTH)
			{
				// ??? what should I do? truncate the sound or just error out?
			}

			size=r.size; // actually overwritten by matchUpChannelLengths
			sampleRate=r.sampleRate;
			channelCount=r.channelCount;
		}
		// else if saving to a new version... clear and change the alignement of the "Format Info" pool
		else
			throw(runtime_error(string(__func__)+" -- unhandled format version: "+istring(version)));

		if(channelCount<0 || channelCount>MAX_CHANNELS)
		{
			deletePeakChunkAccessers();
			deleteCueAccesser();
			poolFile.closeFile(false,false);
			throw(runtime_error(string(__func__)+" -- invalid number of channels: "+istring(channelCount)));
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

void CSound::saveMetaInfo()
{
	if(!poolFile.isOpen())
		throw(runtime_error(string(__func__)+" -- poolFile is not opened"));

	CFormat1InfoPoolAccesser b=poolFile.getPoolAccesser<RFormatInfo1>(metaInfoPoolID);
	if(b.getSize()==1)
	{
		RFormatInfo1 &r=b[0];

		// always write the newest format
		r.version=CURRENT_VERSION;
		r.size=size;
		r.sampleRate=sampleRate;
		r.channelCount=channelCount;
	}
	else
	{
		RFormatInfo1 r;

		// always write the newest format
		r.version=CURRENT_VERSION;
		r.size=size;
		r.sampleRate=sampleRate;
		r.channelCount=channelCount;

		poolFile.clearPool(metaInfoPoolID);
		poolFile.setPoolAlignment(metaInfoPoolID,sizeof(RFormatInfo1));

		CFormat1InfoPoolAccesser b=poolFile.getPoolAccesser<RFormatInfo1>(metaInfoPoolID);
		b.append(1);
		b[0]=r;
	}

	// really slows things down especially for recording 
	// flush();
}

const string CSound::getWorkingFilename(const string originalFilename)
{
	// ??? this directory needs to be a setting.. and perhaps in the home directory too ~/.ReZound or some other configured temp sapce directory
 	return("/tmp/"+ost::Path(originalFilename).BaseName()+".pf$");
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
	for(size_t t=0;t<poolFile.getPoolCount();t++)
	{
		const string _poolName=poolFile.getPoolNameById(t);
		const char *poolName=_poolName.c_str();
			// ??? since I'm using strstr, string probably needs some string searching methods
		if(strstr(poolName,TEMP_AUDIO_POOL_NAME)==poolName)
		{
			poolFile.removePool(t);
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
	if(containsCue(name))
		throw(runtime_error(string(__func__)+" -- a cue with that name already exists"));

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

	if(containsCue(name))
		throw(runtime_error(string(__func__)+" -- a cue with that name already exists"));

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

bool CSound::containsCue(const string &name) const
{
	for(size_t t=0;t<cueAccesser->getSize();t++)
	{
		if(strncmp((*cueAccesser)[t].name,name.c_str(),MAX_SOUND_CUE_NAME_LENGTH)==0)
			return(true);
	}
	return(false);
}

bool CSound::findCue(const sample_pos_t time,size_t &index) const
{
	const map<sample_pos_t,size_t>::const_iterator i=cueIndex.find(time);

	if(i!=cueIndex.end())
	{
		index=i->second;
		return(true);
	}
	else
		return(false);
}

bool CSound::findNearestCue(const sample_pos_t time,size_t &index,sample_pos_t &distance) const
{
	if(cueIndex.empty())
		return(false);

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

	return(true);
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



