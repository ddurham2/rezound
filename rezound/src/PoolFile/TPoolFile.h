/* 
 * Copyright (C) 2002 - David W. Durham
 * 
 * This file is part of ReZound, an audio editing application, but
 * could be used for other applications which could use the PoolFile
 * class's functionality.
 * 
 * PoolFile is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 * 
 * PoolFile is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */

#ifndef __TPoolFile_H__
#define __TPoolFile_H__

#include "../../config/common.h"

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <map>
#include <vector>
#include <queue>

#include <CMutex.h>
#include <CRWLock.h>

#include "CMultiFile.h"

template<class pool_element_t,class pool_file_t> class TStaticPoolAccesser;

// ??? perhaps rename the word 'pool' it's more of a line  that doesn't change order.. pool implies almost that the order doesnt matter

/*
 - This class maintains a file as blocks of large typed arrays called pools.

 - The two template parametes are the logical address space type and the 
   physical address space type.  Both are address space IN BYTES.
   - !!!NOTE!!! they must be unsigned types

 - Use createPool to create pools giving it a name and the type of data being 
   stored in the pool.

 - Use getPoolAccesser to retrieve a pool accesser object to manipulate the
   size of and data within a pool.

 - The idea of thread-safeness is that the TPoolFile object's interface
   methods are thread-safe, but PoolAccessers are not.  They do however, have
   lock, unlock, and trylock methods for mutual exclusion.
*/
template<class _l_addr_t,class _p_addr_t> class TPoolFile
{
public:
	typedef _l_addr_t l_addr_t;	// is written to file
	typedef _p_addr_t p_addr_t;	// is written to file
	typedef uint32_t alignment_t;	// is written to file
	typedef size_t poolId_t;


	// formatSignature MUST be an 8 byte (not counting an unncessary null terminator) signature to look for when opening a file
	TPoolFile(const size_t maxBlockSize=32768,const char *formatSignature="DavyBlox");
	explicit TPoolFile(const TPoolFile<l_addr_t,p_addr_t> &src); //INVALID
	virtual ~TPoolFile();

	// open/close methods
	void openFile(const string _filename,const bool canCreate=true);

	// removes all files that would be generated if a pool file was opened with the given name
	// it cleans up the .SAT[12] files (and in the future should remove multiple files if mutiple files were create for a pool file larger than the 2gig limit)
	static void removeFile(const string filename);

	const bool isOpen() const;

	void copyToFile(const string filename);

	const string getFilename() const;
	void rename(const string newFilename);

	void flushData();

	// for now, since insertSpace doesn't backupSAT for speed reasons, I need 
	// to be able to call it incase of a crash after I'm finished inserting
	void backupSAT();

	void defrag();

	void closeFile(const bool defrag,const bool removeFile);



	// mutex methods
	void sharedLock() const;
	const bool sharedTrylock() const;
	void sharedUnlock() const;
	const size_t getSharedLockCount() const;

	void exclusiveLock() const;
	const bool exclusiveTrylock() const;
	void exclusiveUnlock() const;
	const bool isExclusiveLocked() const;


	// accesser methods
	template<class pool_element_t> TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > getPoolAccesser(const poolId_t poolId);
	template<class pool_element_t> TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > getPoolAccesser(const string poolName);

	template<class pool_element_t> const TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > getPoolAccesser(const poolId_t poolId) const;
	template<class pool_element_t> const TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > getPoolAccesser(const string poolName) const;

	// these methods can be used to read raw data from a pool disregarding the alignment of the pool
	size_t readPoolRaw(const poolId_t poolId,void *buffer,size_t readSize);
	size_t readPoolRaw(const string poolName,void *buffer,size_t readSize);


	// pool information/managment methods
	const l_addr_t getPoolSize(poolId_t poolId) const;
	const l_addr_t getPoolSize(const string poolName) const;

	template<class pool_element_t> TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > createPool(const string poolName,const bool throwOnExistance=true);

	void removePool(const poolId_t poolId);
	void removePool(const string poolName,const bool throwIfNotExists=true);
	void clear(); // removes all pools

	const bool containsPool(const string poolName) const;

	// ??? these may not be ncessary to have as public.. then remove the one that takes a string
	const alignment_t getPoolAlignment(const poolId_t poolId) const;
	const alignment_t getPoolAlignment(const string poolName) const;

	// these can be used to change the alignment of a pool, but the pool MUST be empty when this operation is performed
	void setPoolAlignment(const poolId_t poolId,size_t alignment);
	void setPoolAlignment(const string poolName,size_t alignment);

	void swapPools(const poolId_t poolId1,const poolId_t poolId2);
	void swapPools(const string poolName1,const string poolName2);

	const size_t getPoolIndexCount() const; // to iterate through the pools use this method and getPoolIdByIndex(); the indexes will access the pools in alphabetical order

	const poolId_t getPoolIdByName(const string poolName) const;
	const poolId_t getPoolIdByIndex(const size_t index) const; // where index is 0 to getPoolIndexCount()-1
	const string getPoolNameById(const poolId_t poolId) const;
	const bool isValidPoolId(const poolId_t poolId) const;

	void clearPool(const poolId_t poolId);
	void clearPool(const string poolName);

	const p_addr_t getFileSize() const;


	// methods to make sure that the pool file is consistant
	void verifyBlockInfo(const poolId_t poolId) const;
	void verifyAllBlockInfo(const bool expectContiguousPhysicalSpace) const;
	void printSAT() const;

#ifndef TESTING_TPOOLFILE
private:
#endif

	struct RLogicalBlock;
	struct RPhysicalBlock;

		// ??? is there some way to say any type T but only L and P being l_addr_t and p_addr_t
	template <class T,class P> friend class TPoolAccesser;
	template <class T,class P> friend class TStaticPoolAccesser;

	friend struct RLogicalBlock;
	friend struct RPhysicalBlock;

	mutable CRWLock structureMutex;

	mutable CMutex accesserInfoMutex;

	const char *formatSignature;

	const size_t maxBlockSize;
	const l_addr_t maxLogicalAddress;
	const p_addr_t maxPhysicalAddress;

	bool opened;
	string filename,SATFilename;

	CMultiFile blockFile;

	struct RPoolInfo
	{
		l_addr_t size;
		alignment_t alignment;
		bool isValid; // false if the pool has been removed, but the poolId hasn't been reused yet

		RPoolInfo();
		RPoolInfo(const l_addr_t _size,const alignment_t _alignment,const bool _isValid);
		RPoolInfo(const RPoolInfo &src);
		RPoolInfo &operator=(const RPoolInfo &src);

		void writeToFile(CMultiFile *f,CMultiFile::RHandle &multiFileHandle) /*const*/;
		void readFromFile(CMultiFile *f,CMultiFile::RHandle &multiFileHandle);
	};

	map<const string,poolId_t> poolNames;		// the integer data indexes into pools and SAT given a name string key, which is the 'poolId'
	vector<RPoolInfo> pools;			// this list is parallel to SAT
	vector<vector<RLogicalBlock> > SAT;		// the outer vector is parallel to pools; note, the inner vector is kept sorted by logicalStart, so we shan't modify logicalStart such that it becomes out of order

	vector<RPhysicalBlock> physicalBlockList;	// note it's sorted by physicalStart, so we shan't modify physicalStart such that it becomes out of order


	// Misc
	void setup();
	void init(const bool createInitialCachedBlocks=true);
	const bool prvGetPoolIdByName(const string poolName,poolId_t &poolId) const;
	const p_addr_t getProceedingPoolSizes(const poolId_t poolId) const;
	const l_addr_t getMaxBlockSizeFromAlignment(const alignment_t alignment) const;
	void writeMetaData(CMultiFile *f);
	void prvCreatePool(const string poolName,const alignment_t alignment,const bool throwOnExistance=true,const bool useOldPoolIds=true);
	void addPool(const poolId_t poolId,const alignment_t alignment,bool isValid);
	const string getPoolDescription(const poolId_t poolId) const;
	void writeDirtyIndicator(const bool dirty,CMultiFile *f);
	void makeBlockFileSmallest();
	void appendNewSAT();

	// Structural Integrity Methods
	CMultiFile SATFiles[2]; // for now, I just use the same IO module for storing the SATs as well as the data... when 64bit FS is normal.. there won't be a difference
	uint8_t whichSATFile;
	void openSATFiles(const bool mustExist=false);
	void closeSATFiles(const bool removeFiles=true);
	void writeWhichSATFile();
	void writeSATToFile(CMultiFile *f,const p_addr_t writeWhere);
	void restoreSAT();
	void buildSATFromFile(CMultiFile *f,const p_addr_t readWhere);

	// SAT operations
	const size_t findSATBlockContaining(const poolId_t poolId,const l_addr_t where,bool &atStartOfBlock) const;

	const size_t findPhysicalBlockContaining(const p_addr_t physicalWhere) const;

	const p_addr_t findHole(const l_addr_t size,const p_addr_t not_in_start=1,const p_addr_t not_in_stop=0);
	const p_addr_t makeHole(const l_addr_t size);
	const bool isInWindow(const p_addr_t start,const p_addr_t end,const p_addr_t windowStart,const p_addr_t windowEnd) const;
	void joinAllAdjacentBlocks();
	void joinAdjacentBlocks(const poolId_t poolId);
	void joinAdjacentBlocks(const poolId_t poolId,const size_t firstBlockIndex,const size_t blockCount);

	// Pool modification
	void insertSpace(const poolId_t poolId,const l_addr_t peWhere,const l_addr_t peCount);
	void removeSpace(const poolId_t poolId,const l_addr_t peWhere,const l_addr_t peCount);
		// moves peCount pool-elements of data from the srcPoolId at peSrcWhere to the destPoolId at peDestWhere
	void moveData(const poolId_t destPoolId,const l_addr_t peDestWhere,const poolId_t srcPoolId,const l_addr_t peSrcWhere,const l_addr_t peCount);

	// Pool Data Access
	struct RCachedBlock
	{
		poolId_t poolId;
		void *buffer;
		size_t referenceCount;
		bool dirty;
		l_addr_t logicalStart,size;

		RCachedBlock(const size_t maxBlockSize);
		virtual ~RCachedBlock();
		bool containsAddress(l_addr_t where) const;
		void init(const poolId_t _poolId,const l_addr_t _logicalStart,const l_addr_t _size);
	};

	// 'char', just to have something since the list needs to hold pointers to all types of TStaticAccesser instantiation
	typedef TStaticPoolAccesser<char,TPoolFile<l_addr_t,p_addr_t> > CGenericPoolAccesser;
	vector<const CGenericPoolAccesser *> accessers;

	queue<RCachedBlock *> unusedCachedBlocks;		// available, not caching anything
	deque<RCachedBlock *> unreferencedCachedBlocks;		// is caching data, but is not currently referenced by any PoolAccesser object
	deque<RCachedBlock *> activeCachedBlocks;		// is caching data, and is currently being used by one or more PoolAccesser objects


	template<class pool_element_t> void cacheBlock(const l_addr_t byteWhere,const TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > *accesser);
	template<class pool_element_t> void invalidateAccesser(const TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > *accesser);
	void invalidateCachedBlock(RCachedBlock *cachedBlock);
	template<class pool_element_t> void unreferenceCachedBlock(const TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > *accesser);
	void invalidateAllCachedBlocks();
	template<class pool_element_t> void addAccesser(const TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > *accesser);
	template<class pool_element_t> void removeAccesser(const TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > *accesser);

	void changeBlockFileSize(const p_addr_t newSize);

	// defragging
	map<poolId_t,map<l_addr_t,bool> > correctionsTried;
	void createContiguousSAT();
	void correctBlockPosition(const poolId_t poolId,const size_t blockIndex,const p_addr_t previousPoolSizes);
	void recurCorrectBlockPosition(const poolId_t poolId,const size_t blockIndex,const p_addr_t previousPoolSizes);
	void physicallyMoveBlock(RLogicalBlock &block,const p_addr_t physicallyWhere);


	struct RLogicalBlock
	{
		l_addr_t logicalStart;	// key
		l_addr_t size;		// data (??? could be something more like unsigned since it will NEVER be bigger than MAX_BLOCK_SIZE)
		p_addr_t physicalStart;	// data

		RLogicalBlock();
		RLogicalBlock(const l_addr_t _logicalStart);
		RLogicalBlock(const RLogicalBlock &src);
		RLogicalBlock &operator=(const RLogicalBlock &src);

		const bool operator==(const RLogicalBlock &src) const;
		const bool operator!=(const RLogicalBlock &src) const { return !operator==(src); }
		const bool operator>(const RLogicalBlock &src) const { return !operator<=(src); }
		const bool operator>=(const RLogicalBlock &src) const { return !operator<(src); }
		const bool operator<(const RLogicalBlock &src) const;
		const bool operator<=(const RLogicalBlock &src) const { return operator<(src) || operator==(src); }

		const size_t getMemSize();
		void writeToMem(uint8_t *mem,size_t &offset) /*const*/;
		void readFromMem(const uint8_t *mem,size_t &offset);

		void print() const;
	};

	struct RPhysicalBlock
	{
		p_addr_t physicalStart;	// key
		l_addr_t size;		// data (??? could be something more like unsigned since it will NEVER be bigger than MAX_BLOCK_SIZE)

		RPhysicalBlock();
		RPhysicalBlock(const p_addr_t _physicalStart,const l_addr_t _size=0);
		RPhysicalBlock(const RPhysicalBlock &src);
		RPhysicalBlock(const RLogicalBlock &src);

		RPhysicalBlock &operator=(const RPhysicalBlock &src);

		const bool operator==(const RPhysicalBlock &src) const;
		const bool operator!=(const RPhysicalBlock &src) const { return !operator==(src); }
		const bool operator>(const RPhysicalBlock &src) const { return !operator<=(src); }
		const bool operator>=(const RPhysicalBlock &src) const { return !operator<(src); }
		const bool operator<(const RPhysicalBlock &src) const;
		const bool operator<=(const RPhysicalBlock &src) const { return operator<(src) || operator==(src); }

		void print() const;
	};

	// inserts i, into container, c, in its sorted location
	template<class C,class I> static void sortedInsert(C &c,const I &i);

	// read/write a string to a CMultiFile
	static void readString(string &s,CMultiFile *f,CMultiFile::RHandle &multiFileHandle);
	static void writeString(const string &s,CMultiFile *f,CMultiFile::RHandle &multiFileHandle);

};

/* this would be needed if I were utilizing gcc's implicit instantiation for TPoolFile
#define __TPoolFile_H__CPP
#include "TPoolFile.cpp"
#undef __TPoolFile_H__CPP
*/

#endif
