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

#ifndef __ASound_H__
#define __ASound_H__

#include "../../config/common.h"

#include <stdint.h>

#include <string>
#include <map>

// it would be nice if I didn't have to include all this ???
#include <TPoolFile.h>
#include <TPoolAccesser.h>

	// ??? rename to something more specific to audio, PCM, or rezound
	// create a MAX_LENGTH that I check for in the methods... I've made it, but I don't think I use it yet much
	// right now I made this lower only because I haven't made the frontend dialogs not show unnecessary check boxes and such...so the dialogs were huge even when many channels were not in use
#define MAX_CHANNELS 8



// ??? sample_pos_t is the best name I could think of even though it is the type that ASound::getLength returns
/* 
 * audio size specifications
 *
 *    sample_pos_t should always be unsigned since I use this assumption and never check for < 0 in the code
 *    sample_fpos_t should abe floating point, but big enough to hold all values of sample_pos_t
 */
#if 1 // 32 bit
	typedef uint32_t 	sample_pos_t;	// integral sample count
	typedef double		sample_fpos_t;	// floating-point sample count

	#define MAX_LENGTH (0x7fffffff-1024)

	#define sample_fpos_floor(a)	(floor(a))
	#define sample_fpos_ceil(a)	(ceil(a))
	#define sample_fpos_round(a)	(nearbyint(a))
	#define sample_fpos_log(a)	(log(a))
	#define sample_fpos_exp(a)	(exp(a))
	#define sample_fpos_fabs(a)	(fabs(a))
	#define sample_fpos_sin(a)	(sin(a))

#elif 0 // 64 bit
	typedef uint64_t 	sample_pos_t;	// integral sample position
	typedef long double 	sample_fpos_t;	// floating-point sample position

	#define MAX_LENGTH (0x7fffffffffffffff-1024)

	#define sample_fpos_floor(a)	(floorl(a))
	#define sample_fpos_ceil(a)	(ceill(a))
	#define sample_fpos_round(a)	(nearbyintl(a))
	#define sample_fpos_log(a)	(logl(a))
	#define sample_fpos_exp(a)	(expl(a))
	#define sample_fpos_fabs(a)	(fabsl(a))
	#define sample_fpos_sin(a)	(sinl(a))

#else
	#error please enable one section above
#endif

static const sample_pos_t NIL_SAMPLE_POS=~((sample_pos_t)0);



// ??? probably should add conversion macros which convert to and from several types of formats to and from the native format 
//	- this would help in importing and exporting audio data
//	- might deal with endian-ness too

// audio type specifications
#if 1 // 16 bit PCM

	#define MAX_SAMPLE (32767)
	#define MIN_SAMPLE (-MAX_SAMPLE) // these should stay symetric so I don't have to handle the possilbility of them being off center in the frontend rendering
	typedef int16_t sample_t;
	typedef int_fast32_t mix_sample_t; // this needs to hold at least a value of MAX_SAMPLE squared

#elif 0 // 32 bit floating point PCM
	#define MAX_SAMPLE (1.0)
	#define MIN_SAMPLE (-MAX_SAMPLE) // these should stay symetric so I don't have to handle the possilbility of them being off center in the frontend rendering
	typedef float sample_t;
	typedef float mix_sample_t;

#else
	#error please enable one section above
#endif


#define REZOUND_POOLFILE_BLOCKSIZE 32768
#define REZOUND_POOLFILE_SIGNATURE "ReZoundF"

// ??? I should even typedef the uint64_t as something else... it needs to match whatever CMultiFile has though
typedef TStaticPoolAccesser<sample_t,TPoolFile<sample_pos_t,uint64_t> > CRezPoolAccesser;

static mix_sample_t _SSS;
#define ClipSample(s) ((sample_t)(_SSS=((mix_sample_t)(s)), _SSS>MAX_SAMPLE ? MAX_SAMPLE : ( _SSS<MIN_SAMPLE ? MIN_SAMPLE : _SSS ) ) )

enum MixMethods
{
    mmOverwrite,
    mmAdd,
    mmMultiply,
    mmAverage 
};

struct RPeakChunk
{ 
public:
	sample_t min;
	sample_t max;
private:
	friend class ASound;
	bool dirty;
};

class ASound
{
public:
	typedef TPoolFile<sample_pos_t,uint64_t> PoolFile_t;

	virtual ~ASound();

	virtual void loadSound(const string filename)=0;

	// ??? really this should be static, and it should get passed another ASound * from which it gets the format info and uses getData to export the data
		// - this way I can convert from one format to another... exporting to different compression types and such would require front-end involvment and flags to the saveSound method
		// - and again, I can have CSoundManager analyze the extension to determine which format to use
	virtual void saveSound(const string filename)=0;

	virtual void saveSound();
	virtual void closeSound();

	void changeFilename(const string newFilename);

	// locks to keep the size from changing (multiple locks can be obtained of this type)
	void lockSize() const;
	bool trylockSize() const;
	void unlockSize() const;

	// locks to be able to change the size (only one lock can be obtained of this type)
	void lockForResize() const;
	bool trylockForResize() const;
	void unlockForResize() const;



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
	 * - It's not necessary to call it for data that was touched by the methods in ASound that modify or resize 
	 *   the data
	 */
	void invalidatePeakData(unsigned channel,sample_pos_t start,sample_pos_t stop);
	void invalidatePeakData(const bool doChannel[MAX_CHANNELS],sample_pos_t start,sample_pos_t stop);
	void invalidateAllPeakData();



	const string getFilename() const	{ return(filename); }
	sample_pos_t getLength() const		{ return(size); }
	unsigned getChannelCount() const	{ return(channelCount); }
	unsigned getSampleRate() const		{ return(sampleRate); }
	bool isEmpty() const			{ return(size==0 || channelCount==0); }

	/*
	 * It is very important to lock the sound (writer's lock) before calling these methods.
	 * And except will occur if a resizeLock has not been obtained before calling these methods
	 * (??? I could make a static bool variable that says it's okay not to have a lock if I know the app isn't multi-threaded)
	 */
	// ??? all of the space modifier methods should make sure that a resize lock has been obtained... if it hasn't do INTERNAL_ERROR

	// Structure Manipulation Members
	void deleteChannel(unsigned channel);

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


	/*
	void blendSelectEdges();
	void blendSelectEdges(TWhichChannels &WhichChannels);
	*/

	void silenceSound(unsigned channel,sample_pos_t where,sample_pos_t length,bool doInvalidatePeakData=true,bool showProgressBar=true);
	void mixSound(unsigned channel,sample_pos_t where,const CRezPoolAccesser src,sample_pos_t srcWhere,sample_pos_t length,MixMethods mixMethod,bool doInvalidatePeakData=true,bool showProgressBar=true);


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

	#define MAX_SOUND_CUE_NAME_LENGTH 32
	struct RCue
	{
		RCue(const char _name[MAX_SOUND_CUE_NAME_LENGTH],sample_pos_t _time,bool _isAnchored)
		{
			strncpy(name,_name,MAX_SOUND_CUE_NAME_LENGTH);
			name[MAX_SOUND_CUE_NAME_LENGTH]=0;
			memset(reserved,0,90);

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

			return(*this);
		}

		// ??? char better remain 8 bits
		char name[MAX_SOUND_CUE_NAME_LENGTH+1];
		// If for some reason, this isn't enough space in the future,
		// I could just go with a new pool name and when loading an
		// older version, just update to the new pool name and convert
		// the older cues
		uint8_t reserved[90];
		uint8_t isAnchored;
		uint64_t time;
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

	bool containsCue(const string &name) const;
	bool findCue(const sample_pos_t time,size_t &index) const;
		// finds the cue nearest to the given time
	bool findNearestCue(const sample_pos_t time,size_t &index,sample_pos_t &distance) const;

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
			return(poolFile.PoolFile_t::createPool<T>("__GENERAL__ "+poolName));
	}

	void flush();

	void defragPoolFile();
	void printSAT(); // temporary for debugging ???

protected:

	typedef TPoolAccesser<sample_t,PoolFile_t > CInternalRezPoolAccesser;

	ASound();
	ASound(const string &_filename,const unsigned _sampleRate,const unsigned _channelCount,const sample_pos_t _size);

	CInternalRezPoolAccesser getAudioInternal(unsigned channel);
	CInternalRezPoolAccesser getTempDataInternal(unsigned tempAudioPoolKey,unsigned channel);

	PoolFile_t poolFile;
	string filename;

	sample_pos_t size; // ??? rename to sampleCount
	unsigned sampleRate;
	unsigned channelCount;


	// creates a poolFile with the temp name and populates the meta-info with
	// what's currently in the data-members
	void createWorkingPoolFile(const string originalFilename);

	// attempts to open a poolFile with the given name and if it succeeds then
	// it populates the data-members with the format-info
	bool createFromWorkingPoolFileIfExists(const string originalFilename,bool promptIfFound=true);

	// poolFile must be closed
	void removeWorkingPoolFile(const string originalFilename) const;

	// this should be called after every space modification and even loading of a file
	// and pass NIL_SAMPLE_POS as the maxLength parameter to match to the longest channel
	void matchUpChannelLengths(sample_pos_t maxLength);

private:

	friend class CrezSound;

	void addSpaceToChannel(unsigned channel,sample_pos_t where,sample_pos_t length,bool doZeroData);
	void removeSpaceFromChannel(unsigned channel,sample_pos_t where,sample_pos_t length);
	void moveDataOutOfChannel(unsigned tempAudioPoolKey,unsigned channel,sample_pos_t where,sample_pos_t length);
	void moveDataIntoChannel(unsigned tempAudioPoolKey,unsigned channel,sample_pos_t where,sample_pos_t length,bool removeTempAudioPool);

	static void appendForFudgeFactor(CInternalRezPoolAccesser dest,const CInternalRezPoolAccesser src,sample_pos_t srcWhere,sample_pos_t fudgeFactor);

	const string getWorkingFilename(const string originalFilename) const;

	struct RFormatInfo1
	{
		uint32_t version;
		uint64_t size;
		uint32_t sampleRate;
		uint32_t channelCount;

		void operator=(const RFormatInfo1 &src)
		{
			version=src.version;
			size=src.size;
			sampleRate=src.sampleRate;
			channelCount=src.channelCount;
		}
	};
	typedef TPoolAccesser<RFormatInfo1,PoolFile_t > CFormat1InfoPoolAccesser;

	int metaInfoPoolID;
	int channelPoolIDs[MAX_CHANNELS];

	typedef TPoolAccesser<RPeakChunk,PoolFile_t > CPeakChunkRezPoolAccesser;
	CPeakChunkRezPoolAccesser *peakChunkAccessers[MAX_CHANNELS];

	unsigned tempAudioPoolKeyCounter;

	bool _isModified;


	typedef TPoolAccesser<RCue,PoolFile_t > CCuePoolAccesser;
	CCuePoolAccesser *cueAccesser;
	map<sample_pos_t,size_t> cueIndex; // index into cueAccesser by time

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
};

#endif
