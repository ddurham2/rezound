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

#ifndef __CSound_H__
#define __CSound_H__

#include "../../config/common.h"

#include <string.h>

#include <string>
#include <map>

#include "CSound_defs.h"
#include <endian_util.h>

// it would be nice if I didn't have to include all this ???
#include <TPoolFile.h>
#include <TPoolAccesser.h>
#define REZOUND_POOLFILE_BLOCKSIZE 32768
#define REZOUND_POOLFILE_SIGNATURE "ReZoundF"
#define REZOUND_WORKING_POOLFILE_SIGNATURE "ReZoundW"

typedef TStaticPoolAccesser<sample_t,TPoolFile<sample_pos_t,CMultiFile::l_addr_t> > CRezPoolAccesser;

struct RPeakChunk
{ 
public:
	sample_t min;
	sample_t max;
private:
	friend class CSound;
	bool dirty;
};

class CSound
{
public:
	// the p_addr_t needs to agree with CSound::getAudioDataSize()'s implementation too
	typedef TPoolFile<sample_pos_t,uint64_t> PoolFile_t;

	CSound();
		// works like createWorkingPoolFile
	CSound(const string &_filename,const unsigned _sampleRate,const unsigned _channelCount,const sample_pos_t _size);
	virtual ~CSound();

	// creates a poolFile with a temp name based off of originalFilename and populates the given meta-info
	// 	- don't want as public except that each derivation of ASoundTranslator would have ot be a friend
	void createWorkingPoolFile(const string originalFilename,const unsigned _sampleRate,const unsigned _channelCount,const sample_pos_t _size);

	// attempts to open a poolFile with the given name and if it succeeds then it populates the data-members with the format-info
	bool createFromWorkingPoolFileIfExists(const string originalFilename,bool promptIfFound=true);



	void closeSound();

	void changeWorkingFilename(const string newOriginalFilename);


	CRezPoolAccesser getAudio(unsigned channel);
	const CRezPoolAccesser getAudio(unsigned channel) const;

	// tempAudioPoolKey is returned from moveDataTemp or moveDataToTempAndReplaceSpace
	CRezPoolAccesser getTempAudio(unsigned tempAudioPoolKey,unsigned channel);
	const CRezPoolAccesser getTempAudio(unsigned tempAudioPoolKey,unsigned channel) const;


	/*
	 * - This returns a min and max sample value of the data between [dataPos,nextDataPos)
	 * - This should be used to render the waveform as it gives a more consistance
	 *   shape when scaling the data 
	 */
	RPeakChunk getPeakData(unsigned channel,sample_pos_t dataPos,sample_pos_t nextDataPos,const CRezPoolAccesser &dataAccesser) const;

	/* 
	 * - Should be called by any code that modified the data in such a way that peak data needs to be modified
	 * - It's not necessary to call it for data that was touched by the methods in CSound that modify or resize 
	 *   the data
	 */
	void invalidatePeakData(unsigned channel,sample_pos_t start,sample_pos_t stop);
	void invalidatePeakData(const bool doChannel[MAX_CHANNELS],sample_pos_t start,sample_pos_t stop);
	void invalidateAllPeakData();



	sample_pos_t getLength() const		{ return size; }
	unsigned getChannelCount() const	{ return channelCount; }
	unsigned getSampleRate() const		{ return sampleRate; }
	void setSampleRate(unsigned newSampleRate);
	bool isEmpty() const			{ return size==0 || channelCount==0; }

	/*
	 * It is very important to lock the sound (writer's lock) before calling these methods.
	 * And except will occur if a resizeLock has not been obtained before calling these methods
	 * (??? I could make a static bool variable that says it's okay not to have a lock if I know the app isn't multi-threaded)
	 */
	// ??? all of the space modifier methods should make sure that a resize lock has been obtained... if it hasn't do INTERNAL_ERROR



	// Structure Manipulation Members (Most require a resize lock to have been obtained)
	
	void addChannel(bool doZeroData); // adds a new channel at the end
	void addChannels(unsigned where,unsigned count,bool doZeroData);

	void removeChannel(); // removes the last channel
	void removeChannels(unsigned where,unsigned count);

	// moves each channel specified by whichChannels to a temp pool and returns a handle (tempAudioPoolKey) to those temp pools to be restored
	int moveChannelsToTemp(const bool whichChannels[MAX_CHANNELS]);
	
	// moves channels back into position from a moveChannelsToTempPool()
	// tempAudioPoolKey was returned by moveChannelsToTempPool()
	// whichChannels MUST be the same as when moveChannelsToTempPool() was called
	// the audio length MUST be the same as when moveChannelsToTempPool() was called
	// the channel layout MUST be the same as after the moveChannelsToTempPool() was finished
	void moveChannelsFromTemp(int tempAudioPoolKey,const bool whichChannels[MAX_CHANNELS]);



	/* 
	 * - Adds 'length' samples of space at position 'where' to all channels
	 * - If 'doZeroData' is true, then the newly created space will be initialized to zero
	 */
	void addSpace(sample_pos_t where,sample_pos_t length,bool doZeroData=false);

	/* 
	 * - Adds 'length' samples of space at position 'where' to channels where whichChannels[i] is true
	 * - Any channel that was not affected will have 'length' samples of silence appended to it
	 * - If 'doZeroData' is true, then the newly created space will be initialized to zero
	 * - If 'maxLength' is given, then any channels longer than that will be truncated, and silence appended to unaffected channels will not make the channel exceed this maximum length
	 */
	void addSpace(const bool whichChannels[MAX_CHANNELS],sample_pos_t where,sample_pos_t length,bool doZeroData=false,sample_pos_t maxLength=NIL_SAMPLE_POS);


	/*
	 * - Removed 'length' samples of space from position 'where' to all channels
	 */
	void removeSpace(sample_pos_t where,sample_pos_t length);

	/*
	 * - Removed 'length' samples of space from position 'where' to channels where whichChannels[i] is true
	 * - If all channels are not affected, then channels that were affected will have 'length' samples of silence appended to it
	 * - If 'maxLength' is given, then any channels longer than that will be truncated, and silence appended to unaffected channels will not make the channel exceed this maximum length
	 */
	void removeSpace(const bool whichChannels[MAX_CHANNELS],sample_pos_t where,sample_pos_t length,sample_pos_t maxLength=NIL_SAMPLE_POS);


	/*
	 * - Moves 'length' samples of data from position 'where' for each channel where whichChannels[i] is true to a newly created temporary pool in the pool file for this sound object
	 * - If all channels were not affected, then each channel that was affected will have 'length' samples of silence appended to it
	 * - The value returned is a handle to the temporary pools created by the move, it is the tempAudioPoolKey parameter passed to getTempData, moveDataFromTemp, removeSpaceAndMoveDataFromTemp, etc
	 * - If fudgeFactor is greater than zero, then that many samples will also be appended to the temp pool by copying them not moving them
	 *   	- The fudgeFactor can specify lengths that extend beyond the end of the sound, silence will just result int he temp pool
	 *   	- This is useful for functions which may need to read ahead in a temp buffer, but don't wish to incur the cost of an if statement to check bounds for just a few needed samples
	 * - If 'maxLength' is given, then any channels longer than that will be truncated, and silence appended to unaffected channels will not make the channel exceed this maximum length
	 */
	unsigned moveDataToTemp(const bool whichChannels[MAX_CHANNELS],sample_pos_t where,sample_pos_t length,sample_pos_t fudgeFactor=0,sample_pos_t maxLength=NIL_SAMPLE_POS);

	/*
	 * - Copies 'length' samples of data from position 'where' for each channel where whichChannels[i] is true to a newly created temporary pool in the pool file for this sound object
	 * - The value returned is a handle to the temporary pools created by the copy, it is the tempAudioPoolKey parameter passed to getTempData, moveDataFromTemp, removeSpaceAndMoveDataFromTemp, etc
	 */
	unsigned copyDataToTemp(const bool whichChannels[MAX_CHANNELS],sample_pos_t where,sample_pos_t length);

	/*
	 * - Moves 'length' samples of data from position 'where' but replaces the space with 'replaceLength' samples, for each channel where whichChannels[i] is true to a newly created temporary pool in the pool file for this sound object
	 * - If all channels were not affected then:
	 *   	- if more space is moved than replaced, then each channel affected will have 'length-replaceLength' samples of silence appended to it
	 *   	- if more space is replaced than moved, then each channel NOT affected will have 'replaceLength-length' samples of silencs appended to it
	 * - The value returned is a handle to the temporary pools created by the move, it is the tempAudioPoolKey parameter passed to getTempData, moveDataFromTemp, removeSpaceAndMoveDataFromTemp, etc
	 * - If fudgeFactor is greater than zero, then that many samples will also be appended to the temp pool by copying them not moving them
	 *   	- The fudgeFactor can specify lengths that extend beyond the end of the sound, silence will just result in the temp pool
	 *   	- This is useful for functions which may need to read ahead in a temp buffer, but don't wish to incur the cost of an if statement to check bounds for just a few needed samples
	 * - If 'maxLength' is given, then any channels longer than that will be truncated, and silence appended to unaffected channels will not make the channel exceed this maximum length
	 */
	unsigned moveDataToTempAndReplaceSpace(const bool whichChannels[MAX_CHANNELS],sample_pos_t where,sample_pos_t length,sample_pos_t replaceLength,sample_pos_t fudgeFactor=0,sample_pos_t maxLength=NIL_SAMPLE_POS);


	/*
	 * - Moves 'moveLength' samples of data from the beginning of a temporary pool specified by 'tempAudioPoolKey' to position 'moveWhere' in the normal audio data pools for each channel where whichChannels[i] is true
	 * - If all channels where not affected then, each channel that was not affected will have 'moveLength' samples of silenced appended to it
	 * - If 'removeTempAudioPools' is true, then all temporarly pools associated with 'tempAudioPoolKey' will be removed
	 * - If 'maxLength' is given, then any channels longer than that will be truncated, and silence appended to unaffected channels will not make the channel exceed this maximum length
	 */
	void moveDataFromTemp(const bool whichChannels[MAX_CHANNELS],unsigned tempAudioPoolKey,sample_pos_t moveWhere,sample_pos_t moveLength,bool removeTempAudioPools=true,sample_pos_t maxLength=NIL_SAMPLE_POS);

	/*
	 * - Removes 'removeLength' samples of space from position 'removeWhere' and then moves 'moveLength' samples of data from the beginning of a temporary pool specified by 'tempAudioPoolKey' to position 'moveWhere' in the normal audio data pools for each channel where whichChannels[i] is true
	 * - If all channels where not affected then, each channel that was not affected will have 'moveLength' samples of silenced appended to it
	 * - If 'removeTempAudioPools' is true, then all temporarly pools associated with 'tempAudioPoolKey' will be removed
	 * - If 'maxLength' is given, then any channels longer than that will be truncated, and silence appended to unaffected channels will not make the channel exceed this maximum length
	 */
	void removeSpaceAndMoveDataFromTemp(const bool whichChannels[MAX_CHANNELS],sample_pos_t removeWhere,sample_pos_t removeLength,unsigned tempAudioPoolKey,sample_pos_t moveWhere,sample_pos_t moveLength,bool removeTempAudioPools=true,sample_pos_t maxLength=NIL_SAMPLE_POS);

	/*
	 * - Removes all temporarly pools associated with 'tempAudioPoolKey'
	 */
	void removeTempAudioPools(unsigned tempAudioPoolKey);

	// rotates 'amount' samples of data in the specified channel to the left or right between the 'start' and 'stop' positions
	void rotateLeft(const bool whichChannels[MAX_CHANNELS],const sample_pos_t start,const sample_pos_t stop,const sample_pos_t amount);
	void rotateRight(const bool whichChannels[MAX_CHANNELS],const sample_pos_t start,const sample_pos_t stop,const sample_pos_t amount);

	// swaps the audio between channel A and channel B within the given region
	void swapChannels(unsigned channelA,unsigned channelB,const sample_pos_t where,const sample_pos_t length);


	void silenceSound(unsigned channel,sample_pos_t where,sample_pos_t length,bool doInvalidatePeakData=true,bool showProgressBar=true);

	/*
	 * - mixes audio from src onto channel 'channel' starting at 'where' in this sound for 'length' samples (in this sound's sampleRate)
	 * - srcSampleRate specifies what sample rate src is in so that when it reads from src it will convert to this sound's sample rate
	 * - mixMethod specifies how to mix the data
	 * - if srcFit is no sftNone then src.getSize() samples from source will be squeezed into the [where,where+length) region in this sound using the specifed method to fit
	 */
	void mixSound(unsigned channel,sample_pos_t where,const CRezPoolAccesser src,sample_pos_t srcWhere,unsigned srcSampleRate,sample_pos_t length,MixMethods mixMethod,SourceFitTypes fitSrc,bool doInvalidatePeakData,bool showProgressBar);


	const string getTimePosition(sample_pos_t samplePos,int secondsDecimalPlaces=3,bool includeUnits=true) const;
	const sample_pos_t getPositionFromTime(const string time,bool &wasInvalid) const;
	const string getAudioDataSize() const;
	const string getAudioDataSize(sample_pos_t sampleCount) const; // with this object's format
	const string getPoolFileSize() const;

	void setIsModified(bool v);
	const bool isModified() const;

	// --------------------------------------------------------------------------
	// these members are used to manage the cues which a sound object can contain
	// --------------------------------------------------------------------------

	#define MAX_SOUND_CUE_NAME_LENGTH 64
	struct RCue
	{
		RCue(const char _name[MAX_SOUND_CUE_NAME_LENGTH],sample_pos_t _time,bool _isAnchored)
		{
			strncpy(name,_name,MAX_SOUND_CUE_NAME_LENGTH);
			name[MAX_SOUND_CUE_NAME_LENGTH]=0;
			memset(reserved,0,sizeof(reserved));

			time=_time;
			isAnchored=_isAnchored ? 1 : 0;
		}

		RCue(const RCue &src)
		{
			operator=(src);
		}

		RCue &operator=(const RCue &rhs)
		{
			strncpy(name,rhs.name,MAX_SOUND_CUE_NAME_LENGTH);

			name[MAX_SOUND_CUE_NAME_LENGTH]=0;
			time=rhs.time;
			isAnchored=rhs.isAnchored;

			return *this;
		}

		/* 
		 * Instead of char I would like to have int8_t
		 * I have a test in configure.ac that makes SURE char is 1 byte, if there's
		 * ever a platform with char size other than 1, I will deal with it then 
		 * because making this int8_t causes the string class and strxxx() functions 
		 * to be incompatible
		 */
		char name[MAX_SOUND_CUE_NAME_LENGTH+1];


		// If for some reason, this isn't enough space in the future,
		// I could just go with a new pool name and when loading an
		// older version, just update to the new pool name and convert
		// the older cues
		uint8_t reserved[58];
		uint8_t isAnchored;
		uint64_t time;

		typedef uint8_t PackedChunk[
			MAX_SOUND_CUE_NAME_LENGTH+1+	//sizeof(name)+
			58+				//sizeof(reserved)+
			1+				//sizeof(isAnchored)+
			8				//sizeof(time)
		];

		void pack(PackedChunk &r) const
		{
			// pack the values of the data members into r in little-endian
			
			unsigned offset=0;

			memcpy(r+offset,name,sizeof(name));
			offset+=sizeof(name);

			memset(r+offset,0,sizeof(reserved));
			offset+=sizeof(reserved);

			memcpy(r+offset,&isAnchored,sizeof(isAnchored));
			offset+=sizeof(isAnchored);

			uint64_t tTime=hetle(time);
			memcpy(r+offset,&tTime,sizeof(time));
			offset+=sizeof(time);
		}

		void unpack(const PackedChunk &r)
		{
			// unpack the little-endian values from r into the data members

			unsigned offset=0;

			memcpy(&name,r+offset,sizeof(name));
			offset+=sizeof(name);

			// no need to copy reserved
			offset+=sizeof(reserved);

			memcpy(&isAnchored,r+offset,sizeof(isAnchored));
			offset+=sizeof(isAnchored);
			
			memcpy(&time,r+offset,sizeof(time));
			lethe(&time);
			offset+=sizeof(time);
		}

	private:
		friend class CrezSoundTranslator;
		RCue() {}

	};

	const size_t getCueCount() const;

	const string getCueName(size_t index) const;

	const sample_pos_t getCueTime(size_t index) const;
	void setCueTime(size_t index,sample_pos_t newTime);

	const bool isCueAnchored(size_t index) const;

	void addCue(const string &name,const sample_pos_t time,const bool isAnchored);
	void insertCue(size_t index,const string &name,const sample_pos_t time,const bool isAnchored);
	void removeCue(size_t index);
	void clearCues();
	void enableCueAdjustmentsOnSpaceChanges(bool enabled);

	static size_t __default_cue_index;
	bool containsCue(const string &name,size_t &index=__default_cue_index) const;
		// these following find methods return true if something is found else false
	bool findCue(const sample_pos_t time,size_t &index) const;
	bool findNearestCue(const sample_pos_t time,size_t &index,sample_pos_t &distance) const; // finds the cue nearest to the given time
	bool findPrevCue(const sample_pos_t time,size_t &index) const; // finds the previous cue in time at the cue with the given time (false if none is found)
	bool findNextCue(const sample_pos_t time,size_t &index) const; // finds the following cue in time at the cue with the given time (false if none is found)

	bool findPrevCueInTime(const sample_pos_t time,size_t &index) const; // finds the previous cue in time from the given time (false if none is found)
	bool findNextCueInTime(const sample_pos_t time,size_t &index) const; // finds the next cue in time from the given time (false if none is found)

	const string getUnusedCueName(const string &prefix="noname") const;

	// --------------------------------------------------------------------------
	// --------------------------------------------------------------------------
	// --------------------------------------------------------------------------


	
	// --- Methods for managing user notes
	const string getUserNotes() const;
	void setUserNotes(const string &notes);


	
	// - This method returns an accesser to a pool of the template type which can be used
	//   by other subsystems within ReZound to store any data they need to persist after
	//   the file is closed.  
	// - I prefix the pool name with "__GENERAL__ " so that they will be easily identifiable if
	//   necessary.  And it means the method cannot be used to get at say one of the audio pools.
	// - For now, the CSoundPlayerChannel class uses this to store the output routes of the
	//   channels within this sound, so a 4 channel sound can be played on a stereo sound card.
	template<class T> TPoolAccesser<T,PoolFile_t > getGeneralDataAccesser(const string poolName)
	{
		if(poolFile.containsPool("__GENERAL__ "+poolName))
			return poolFile.PoolFile_t::getPoolAccesser<T>("__GENERAL__ "+poolName);
		else
			return poolFile.PoolFile_t::createPool<T>("__GENERAL__ "+poolName);
	}

	template<class T> const TPoolAccesser<T,PoolFile_t > getGeneralDataAccesser(const string poolName) const
	{
		if(poolFile.containsPool("__GENERAL__ "+poolName))
			return poolFile.PoolFile_t::getPoolAccesser<T>("__GENERAL__ "+poolName);
		else
			throw runtime_error(string(__func__)+" -- general data pool does not exist: "+poolName);
	}

	bool containsGeneralDataPool(const string poolName) const { return poolFile.containsPool("__GENERAL__ "+poolName); }
	
	void removeGeneralDataPool(const string poolName) { poolFile.removePool("__GENERAL__ "+poolName,false); }


	void flush();

	void defragPoolFile();
	void printSAT(); // temporary for debugging ???
	void verifySAT(); // temporary for debugging ???

private:
	friend class ASoundTranslator; // so it can verify some things
	friend class CrezSoundTranslator;
	friend class ASoundRecorder; // so it can backupSAT() on the pool file when recording is done


	typedef TPoolAccesser<sample_t,PoolFile_t > CInternalRezPoolAccesser;

	CInternalRezPoolAccesser getAudioInternal(unsigned channel);
	CInternalRezPoolAccesser getTempDataInternal(unsigned tempAudioPoolKey,unsigned channel);

	PoolFile_t poolFile;

	sample_pos_t size; // ??? rename to sampleCount
	unsigned sampleRate;
	unsigned channelCount;

	// this should be called after every space modification and even loading of a file
	// and pass NIL_SAMPLE_POS as the maxLength parameter to match to the longest channel
	void matchUpChannelLengths(sample_pos_t maxLength,bool doZeroData=true);

	void addSpaceToChannel(unsigned channel,sample_pos_t where,sample_pos_t length,bool doZeroData);
	void removeSpaceFromChannel(unsigned channel,sample_pos_t where,sample_pos_t length);
	void copyDataFromChannel(unsigned tempAudioPoolKey,unsigned channel,sample_pos_t where,sample_pos_t length);
	void moveDataOutOfChannel(unsigned tempAudioPoolKey,unsigned channel,sample_pos_t where,sample_pos_t length);
	void moveDataIntoChannel(unsigned tempAudioPoolKey,unsigned channelInTempPool,unsigned channelInAudio,sample_pos_t where,sample_pos_t length,bool removeTempAudioPool);

	static void appendForFudgeFactor(CInternalRezPoolAccesser dest,const CInternalRezPoolAccesser src,sample_pos_t srcWhere,sample_pos_t fudgeFactor);

	void prvAddChannel(bool addAudioSpaceForNewChannel,bool doZeroData);

	struct RFormatInfo
	{
		uint32_t version;
		uint64_t size;
		uint32_t sampleRate;
		uint32_t channelCount;

		void operator=(const RFormatInfo &src)
		{
			version=src.version;
			size=src.size;
			sampleRate=src.sampleRate;
			channelCount=src.channelCount;
		}

	};
	typedef TPoolAccesser<RFormatInfo,PoolFile_t > CFormatInfoPoolAccesser;

	int metaInfoPoolID;
	int channelPoolIDs[MAX_CHANNELS];

	typedef TPoolAccesser<RPeakChunk,PoolFile_t > CPeakChunkRezPoolAccesser;
	CPeakChunkRezPoolAccesser *peakChunkAccessers[MAX_CHANNELS];

	unsigned tempAudioPoolKeyCounter;

	bool _isModified;


	typedef TPoolAccesser<RCue,PoolFile_t> CCuePoolAccesser;
	CCuePoolAccesser *cueAccesser;
	map<sample_pos_t,size_t> cueIndex; // index into cueAccesser by time
	bool adjustCuesOnSpaceChanges;

	void adjustCues(const sample_pos_t pos1,const sample_pos_t pos2);
	void createCueAccesser();
	void deleteCueAccesser();
	void rebuildCueIndex();




	void createPeakChunkAccessers();
	void deletePeakChunkAccessers();
	
	static sample_pos_t calcPeakChunkCount(sample_pos_t givenSize);

	static const string createTempAudioPoolName(unsigned tempAudioPoolKey,unsigned channel);
	CInternalRezPoolAccesser createTempAudioPool(unsigned tempAudioPoolKey,unsigned channel);
	void removeAllTempAudioPools();

	// saves the data-members to the meta info in the temporary poolFile
	void saveMetaInfo();

	// called after every space modification to ensure that there is at least 1 sample of length
	// zero length poses problems with selection positions, so I just always make sure there is at least 1 sample
	void ensureNonZeroLength();




	// --- private locking methods only used by CSoundLocker ------------------------------
	friend class CSoundLocker;

	// locks to keep the size from changing (multiple locks can be obtained of this type)
	void lockSize() const;
	bool trylockSize() const;
	void unlockSize() const;

	// locks to be able to change the size (only one lock can be obtained of this type)
	void lockForResize() const;
	bool trylockForResize() const;
	void unlockForResize() const;

};

class CSoundLocker {
	const CSound *mSound;
	const bool mLockForResize;
public:
	CSoundLocker(const CSound *sound, bool lockForResize, bool trylock=false)
		: mSound(sound)
		, mLockForResize(lockForResize)
	{
		if(mLockForResize) {
			if(trylock) {
				// trylock for resize
				if(!mSound->trylockForResize()) {
					mSound = NULL;
				}
			} else {
				// lock for resize
				mSound->lockForResize();
			}
		} else {
			if(trylock) {
				// trylock the size
				if(!mSound->trylockSize()) {
					mSound = NULL;
				}
			} else {
				// lock the size
				mSound->lockSize();
			}
		}
	}

	~CSoundLocker() {
		unlock();
	}

	bool isLocked() const {
		return mSound != NULL;
	}

	void unlock() {
		if(mSound) {
			if(mLockForResize) {
				mSound->unlockForResize();
			} else {
				mSound->unlockSize();
			}
			mSound = NULL;
		}
	}
};

#endif
