/*/
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

/* this would be needed if I were utilizing gcc's implicit instantiation for TPoolFile
#ifndef __TPoolFile_H__CPP
#error this file must be included through TPoolFile.h NOT compiled on its own
#endif
*/

/* ???
 * There are places that I have to do: container.erase(container.begin+index);  
 * see if I can eliminate this
 */

/*???
 * 	In many functions i re-evaluate over and over SAT[poolId][index].. I should probably reduce that to  vector<RLogicalBlock> &poolSAT=SAT[poolId];
 */

/*???
 *	For now I have chosen simply to write all data as little endian.  This could be improved by writing an endian indicator
 *	and always write as the native endian.  Then upon reading the data check the endian indicated and convert only if necessary.
 *	Have a version number increase so that previous versions of rezound loading a new file will say 'this code cannot read that version'
 *	This way people running rezound on big endian hardware do not get penalized.  Conversion is done only when a file is take from one endian platform to another
 */

/* ??? One possible improvment, would be to fix this situation:
 * 	blockSize=2048
 * 	all pools are empty
 * 	append 1024 bytes into pool0
 * 	append 1 byte into pool1
 * 	append 1024 more bytes into pool0
 *
 * 	- Right now, the append into pool1 uses the space immediately after the preceeding 1024 bytes of pool0
 * 	  thus the second append into pool0 has to create a new entry in both the SAT and the physicalBlockList
 *	- If I had reserved 1024 more bytes in the first append into pool0, then append into pool1 would have 
 *	  gone into physical address 2048, so the second append into pool0 could have simply increased the size
 *	  of the logical block in the SAT for pool0
 */

/* perhaps some debugging #define could be enabled to avoid all the internal verification ??? */

// ??? I didn't think of this until just now, but for fault tolerancy during a space modifications operation, I could simply
// restore to the backed up SAT just before the space modification method began if there was an error either in the logic or in
// extending the file's length

#include <stdio.h> // for the printf errors... should probably use assert (but assumably temporary)  (It's also for printSAT())

#if defined(__GNUC__) && (__GNUC__>=3)
	#ifdef TESTING_TPOOLFILE
		#define dprintf(...) printf(__VA_ARGS__)
	#else
		#define dprintf(...)
	#endif
#else
	#define dprintf
#endif


#include <stdexcept>
#include <utility>
#include <algorithm>
#include <iterator>

#include <istring>

#include <endian_util.h>

#include "TPoolFile.h"
#include "TStaticPoolAccesser.h"

#include <TAutoBuffer.h>


// -- Structural integrity is ensured by keeping a separate file to contain the SAT
// of the last known stable structure of the file.  These files are created when the file
// is opened and is removed just before the file is closed.  All changes to the structure
// of the file are made in such a way that the structure information from a SATFile will
// apply even if disaster occurs durring the structure change.  There are two sat files
// that I alternate between uses.
// -- At the moment, I don't sync() after structural changes, although I could... If 
// data was of the utmost importance I could implement a flag that causes syncs after
// most every operation.
// --- Now every space modification causes the whole SAT to be written to disk, which makes
// TPoolFile not very efficient with small numerous space changes.  It was designed for large
// and relatively (compared to the data size) few changes to the space.  However, the bigger
// the maxBlockSize, the smaller the SAT will be in length, but the more expensive it is to
// hold a block in memory and read/write it to disk.
// ??? Optimization: I could avoid writting the WHOLE SAT to disk if I know that only a single
// value changed in the SAT and the number of the SAT entries did not change.  This would 
// greatly improve the performance of small changes in a pools space when the block size is
// comparitivly large.

// -- When using a TPoolFile among threads the mutual exclusion methods must be used
// before calling most any method (exceptions are simple methods like isOpen(), etc...).
// Generally, when a routine is to change the size of a pool, it should get an exclusive
// lock on the pool file ( exclusiveLock() ).  When a routine is not going to change
// the size of a pool, it can get a shared lock ( sharedLock() ) which allows multiple locks
// to be obtained while only one exclusive lock can be obtained at a time, only when no shared
// locks are existing.

// -- I still have yet to resolve all the places where we find an inconsistance and printf-then-exit
// Some of these could be converted to exceptions where it's on startup (like constructing the SAT)
// But some are sanity checks that verify the logic, and if they happen, then I've probably got a 
// bug in my logic, or an unhandled situation that I didn't anticipate.  Perhaps I should put an
// #ifdef around each of these so I can turn them off, but so are so cheap that I should just leave
// them in.  


#define MAX_POOL_NAME_LENGTH 256
#define FORMAT_SIGNATURE formatSignature // now a data member in the class instead of a constant
#define FORMAT_VERSION 0
#define INITIAL_CACHED_BLOCK_COUNT 4

// Signature, EOF, Format Version, Dirty, Which SAT File, Meta Data Offset, Filler To Align To Disk Block Buffer
#define LEADING_DATA_SIZE (512)

#define SIGNATURE_OFFSET 0
#define EOF_OFFSET (SIGNATURE_OFFSET+8)
#define FORMAT_VERSION_OFFSET (EOF_OFFSET+1)
#define DIRTY_INDICATOR_OFFSET (FORMAT_VERSION_OFFSET+4)
#define WHICH_SAT_FILE_OFFSET (DIRTY_INDICATOR_OFFSET+1)
#define META_DATA_OFFSET (WHICH_SAT_FILE_OFFSET+1)
//would be next #define NEXT_OFFSET (META_DATA_OFFSET+8)


template<class l_addr_t,class p_addr_t>
	TPoolFile<l_addr_t,p_addr_t>::TPoolFile(const blocksize_t _maxBlockSize,const char *_formatSignature) :

	formatSignature(_formatSignature),

	maxBlockSize(_maxBlockSize),
	maxLogicalAddress(~((l_addr_t)0)),
	maxPhysicalAddress(~((p_addr_t)0)),

	pasm(blockFile)
{
	if(maxBlockSize<2)
		throw runtime_error(string(__func__)+" -- maxBlockSize is less than 2");
	if(maxLogicalAddress>maxPhysicalAddress)
		throw runtime_error(string(__func__)+" -- the logical address space is greater than the physical address space");
	if(maxBlockSize>maxLogicalAddress)
		throw runtime_error(string(__func__)+" -- the maxBlockSize is bigger than the logical address space");

	init();
}

template<class l_addr_t,class p_addr_t>
	TPoolFile<l_addr_t,p_addr_t>::TPoolFile(const TPoolFile<l_addr_t,p_addr_t> &src) :
	pasm(blockFile)
{
	throw runtime_error(string(__func__)+" -- copy constructor invalid");
}

template<class l_addr_t,class p_addr_t>
	TPoolFile<l_addr_t,p_addr_t>::~TPoolFile()
{
	exclusiveLock();
	try
	{
		if(opened)
		{
			// or set all accessers's poolFile members to NULL
			// need to probably wait for any outstanding accessers to deallocate
			while(!accessers.empty());

			try
			{
				invalidateAllCachedBlocks();
			} catch(...) {}
			closeSATFiles(false);
			blockFile.close(false);
		}
		init(false);
		exclusiveUnlock();
	}
	catch(...)
	{
		exclusiveUnlock();
		// throw exceptions from the destructor ???
		throw;
	}
}



template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::openFile(const string _filename,const bool canCreate)
{
 	if(opened)
 		throw runtime_error(string(__func__)+" -- file already opened");

	try
	{
		init();

		blockFile.open(_filename,canCreate);
		filename=_filename;

        	setup();
	}
	catch(...)
	{
		blockFile.close(false);
		throw;
	}


}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::removeFile(const string filename)
{
		// ??? in the furture needs to as CMultiFile to remove the file since each file may be made up of several files if they go over the 2gig limit
	remove(filename.c_str());
	remove((filename+".SAT1").c_str());
	remove((filename+".SAT2").c_str());
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::setup()
{
	try
	{
        	// if the file was not just created then it ought to have a valid pool file in it
		if(blockFile.getSize()>0)
		{
			// check signature string
			int8_t buffer[8]={0};
			blockFile.read(buffer,8,SIGNATURE_OFFSET);
		// ??? if sizeof(char)!=sizeof(int8_t) then strncmp won't work
			if(strncmp((const char *)buffer,FORMAT_SIGNATURE,8))
				throw runtime_error("invalid format signature");

			// check format version
			uint32_t formatVersion;
			blockFile.read(&formatVersion,sizeof(formatVersion),FORMAT_VERSION_OFFSET);
			lethe(&formatVersion);
			if(formatVersion!=FORMAT_VERSION)
				throw runtime_error("invalid or unsupported format version: "+istring((int)formatVersion));

			int8_t dirty;
			blockFile.read(&dirty,sizeof(dirty),DIRTY_INDICATOR_OFFSET);
			lethe(&dirty);

			if(dirty)
			{	// file is marked as dirty
				blockFile.read(&whichSATFile,sizeof(whichSATFile),WHICH_SAT_FILE_OFFSET);
				lethe(&whichSATFile);

				SATFilename=filename+".SAT";

				openSATFiles(true);
				restoreSAT();
				joinAllAdjacentBlocks();

				opened=true;

				return;
			}
			else
			{
				uint64_t metaDataOffset;
				blockFile.read(&metaDataOffset,sizeof(metaDataOffset),META_DATA_OFFSET);
				lethe(&metaDataOffset);
				if(metaDataOffset<LEADING_DATA_SIZE)
					throw runtime_error("invalid meta data offset");

				buildSATFromFile(&blockFile,metaDataOffset);
				joinAllAdjacentBlocks();
			}
		}
		else
			clear();

		SATFilename=filename+".SAT";

		writeMetaData(&blockFile,false);

		openSATFiles();
		whichSATFile=1;
		backupSAT();

		writeDirtyIndicator(true,&blockFile);
		opened=true;
	}
	catch(exception &e)
	{
		opened=false;
		closeSATFiles(false);
		init();
		throw runtime_error(string(__func__)+" -- "+string(e.what()));
	}
}

/*
 * NOTE: in a multi-threaded app, this method should be called with an 
 * exclusive locksince the file's seek positions will matter while it's 
 * copying.  Though, I do not check for this locked status since this
 * might not be used in a multi-threaded app.
 */
template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::copyToFile(const string _filename)
{
	if(!opened)
		throw runtime_error(string(__func__)+" -- no file is open");
	if(_filename==getFilename())
		throw runtime_error(string(__func__)+" -- attempting to copy to the file which is open");

	invalidateAllCachedBlocks();

	CMultiFile copyFile;
	copyFile.open(_filename,true);

	CMultiFile::RHandle multiFileHandle1;
	CMultiFile::RHandle multiFileHandle2;

	try
	{
		TAutoBuffer<int8_t> temp(maxBlockSize);

		const p_addr_t length=blockFile.getSize();

		blockFile.seek(0,multiFileHandle1);
		copyFile.seek(0,multiFileHandle2);
	
		for(blocksize_t t=0;t<length/maxBlockSize;t++)
		{
			blockFile.read(temp,maxBlockSize,multiFileHandle1);
			copyFile.write(temp,maxBlockSize,multiFileHandle2);
		}
		if((length%maxBlockSize)!=0)
		{
			blockFile.read(temp,length%maxBlockSize,multiFileHandle1);
			copyFile.write(temp,length%maxBlockSize,multiFileHandle2);
		}

		writeMetaData(&copyFile,true);
		writeDirtyIndicator(false,&copyFile);
		copyFile.close(false);
	}
	catch(exception &e)
	{
		copyFile.close(true);
		throw runtime_error(string(__func__)+" -- error copying to file -- "+e.what());
	}
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::closeFile(const bool _defrag,const bool _removeFile)
{
	if(!opened)
		return;

	invalidateAllCachedBlocks();

	// need to probably wait for any outstanding accessers to deallocate
	if(_removeFile)
	{
		closeSATFiles();
		blockFile.close(true);
	}
	else
	{
		if(_defrag)
			defrag();

		writeMetaData(&blockFile,true);
		writeDirtyIndicator(false,&blockFile);

		closeSATFiles();

		blockFile.sync();
		blockFile.close(false);
	}
	init();
}

template<class l_addr_t,class p_addr_t>
	const bool TPoolFile<l_addr_t,p_addr_t>::isOpen() const
{
	return opened;
}

template<class l_addr_t,class p_addr_t>
	const p_addr_t TPoolFile<l_addr_t,p_addr_t>::getFileSize() const
{
	return blockFile.getActualSize();
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::rename(const string newFilename)
{
	if(!opened)
		throw runtime_error(string(__func__)+" -- no file is open");

	blockFile.rename(newFilename);
	SATFiles[0].rename(newFilename+".SAT1");
	SATFiles[1].rename(newFilename+".SAT2");
}


template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::flushData()
{
	invalidateAllCachedBlocks();

	backupSAT();

	SATFiles[0].sync();
	SATFiles[1].sync();
	blockFile.sync();
}

template<class l_addr_t,class p_addr_t>
	const string TPoolFile<l_addr_t,p_addr_t>::getFilename() const
{
	return filename;
}

// Non-Const Pool Accesser Methods
template<class l_addr_t,class p_addr_t>
	template<class pool_element_t> TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > TPoolFile<l_addr_t,p_addr_t>::getPoolAccesser(const poolId_t poolId)
{
	if(!opened)
		throw runtime_error(string(__func__)+" -- no file is open");
	if(!isValidPoolId(poolId))
		throw runtime_error(string(__func__)+" -- invalid poolId parameter: "+istring(poolId));
	if(sizeof(pool_element_t)!=pools[poolId].alignment)
		throw runtime_error(string(__func__)+" -- method instantiated with a type whose size does not match the pool's alignment: sizeof(pool_element_t): "+istring(sizeof(pool_element_t))+" -- "+getPoolDescription(poolId));

	TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > accesser(this,poolId);
	return accesser;
}

template<class l_addr_t,class p_addr_t>
	template<class pool_element_t> TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > TPoolFile<l_addr_t,p_addr_t>::getPoolAccesser(const string poolName)
{
	return getPoolAccesser<pool_element_t>(getPoolIdByName(poolName));
}


// Const Pool Accesser Methods
template<class l_addr_t,class p_addr_t>
	template<class pool_element_t> const TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > TPoolFile<l_addr_t,p_addr_t>::getPoolAccesser(const poolId_t poolId) const
{
	if(!opened)
		throw runtime_error(string(__func__)+" -- no file is open");
	if(!isValidPoolId(poolId))
		throw runtime_error(string(__func__)+" -- invalid poolId parameter: "+istring(poolId));
	if(sizeof(pool_element_t)!=pools[poolId].alignment)
		throw runtime_error(string(__func__)+" -- method instantiated with a type whose size does not match the pool's alignment: sizeof(pool_element_t): "+istring(sizeof(pool_element_t))+" -- "+getPoolDescription(poolId));

	const TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > accesser(const_cast<TPoolFile<l_addr_t,p_addr_t> *>(this),poolId);
	return accesser;
}

template<class l_addr_t,class p_addr_t>
	template<class pool_element_t> const TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > TPoolFile<l_addr_t,p_addr_t>::getPoolAccesser(const string poolName) const
{
	return getPoolAccesser<pool_element_t>(getPoolIdByName(poolName));
}



template<class l_addr_t,class p_addr_t>
	l_addr_t TPoolFile<l_addr_t,p_addr_t>::readPoolRaw(const poolId_t poolId,void *buffer,l_addr_t readSize)
{
	if(!opened)
		throw runtime_error(string(__func__)+" -- no file is open");
	if(!isValidPoolId(poolId))
		throw runtime_error(string(__func__)+" -- invalid poolId parameter: "+istring(poolId));

	const TStaticPoolAccesser<uint8_t,TPoolFile<l_addr_t,p_addr_t> > accesser(const_cast<TPoolFile<l_addr_t,p_addr_t> *>(this),poolId);
	readSize=min(readSize,accesser.getSize());
	accesser.read((uint8_t *)buffer,readSize);
	return readSize;
}

template<class l_addr_t,class p_addr_t>
	l_addr_t TPoolFile<l_addr_t,p_addr_t>::readPoolRaw(const string poolName,void *buffer,l_addr_t readSize)
{
	return readPoolRaw(getPoolIdByName(poolName),buffer,readSize);
}

template<class l_addr_t,class p_addr_t>
	template<class pool_element_t> TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > TPoolFile<l_addr_t,p_addr_t>::createPool(const string poolName,const bool throwOnExistance)
{
	if(!opened)
		throw runtime_error(string(__func__)+" -- no file is open");
	prvCreatePool(poolName,sizeof(pool_element_t),throwOnExistance);

	return getPoolAccesser<pool_element_t>(poolName);
}



template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::clear()
{
	if(!accessers.empty())
		throw runtime_error(string(__func__)+" -- there are outstanding accessors");

	invalidateAllCachedBlocks();
	poolNames.clear();
	SAT.clear();
	pools.clear();
	pasm.free_all();
	pasm.make_file_smallest();
	if(isOpen())
		backupSAT();
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::removePool(const poolId_t poolId)
{
	if(!opened)
		throw runtime_error(string(__func__)+" -- no file is open");
	if(!isValidPoolId(poolId))
		throw runtime_error(string(__func__)+" -- invalid poolId: "+istring(poolId));

	invalidateAllCachedBlocks(false,poolId);

	// remove poolName with poolId of the parameter
	for(map<string,poolId_t>::const_iterator t=poolNames.begin();t!=poolNames.end();t++)
	{
		if(t->second==poolId)
		{
			poolNames.erase(t->first);
			break;
		}
	}

	// mark pool as not valid
	pools[poolId].size=0;
	pools[poolId].alignment=0;
	pools[poolId].isValid=false;

	for(size_t t=0;t<SAT[poolId].size();t++)
		pasm.free(SAT[poolId][t].physicalStart); // ??? there might be a more efficient way than calling free_physical for each
	SAT[poolId].clear();

	pasm.make_file_smallest();

	backupSAT();
}


template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::removePool(const string poolName,const bool throwIfNotExists)
{
	poolId_t id;
	try
	{
		id=getPoolIdByName(poolName);
	}
	catch(...)
	{
		if(throwIfNotExists)
			throw;
		else
			return;
	}

	removePool(id);
}

template<class l_addr_t,class p_addr_t>
	const typename TPoolFile<l_addr_t,p_addr_t>::alignment_t TPoolFile<l_addr_t,p_addr_t>::getPoolAlignment(const poolId_t poolId) const
{
	if(!opened)
		throw runtime_error(string(__func__)+" -- no file is open");
	if(!isValidPoolId(poolId))
		throw runtime_error(string(__func__)+" -- invalid poolId parameter: "+istring(poolId));

	const alignment_t alignment=pools[poolId].alignment;

	return alignment;
}

template<class l_addr_t,class p_addr_t>
	const typename TPoolFile<l_addr_t,p_addr_t>::alignment_t TPoolFile<l_addr_t,p_addr_t>::getPoolAlignment(const string poolName) const
{
	return getPoolAlignment(getPoolIdByName(poolName));
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::setPoolAlignment(const poolId_t poolId,size_t alignment)
{
	if(!opened)
		throw runtime_error(string(__func__)+" -- no file is open");
	if(!isValidPoolId(poolId))
		throw runtime_error(string(__func__)+" -- invalid poolId parameter: "+istring(poolId));
	if(pools[poolId].size>0)
		throw runtime_error(string(__func__)+" -- pool must be empty to change alignment");
	if(alignment==0 || alignment>maxBlockSize)
		throw runtime_error(string(__func__)+" -- invalid alignment: "+istring(alignment)+" alignment must be 0 < alignment <= maxBlockSize (which is: "+istring(maxBlockSize)+")");

	invalidateAllCachedBlocks(false,poolId);

	pools[poolId].alignment=alignment;

	backupSAT();
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::setPoolAlignment(const string poolName,size_t alignment)
{
	setPoolAlignment(getPoolIdByName(poolName),alignment);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::swapPools(const poolId_t poolId1,const poolId_t poolId2)
{
	if(!opened)
		throw runtime_error(string(__func__)+" -- no file is open");

	if(!isValidPoolId(poolId1))
		throw runtime_error(string(__func__)+" -- invalid poolId1 parameter: "+istring(poolId1));
	if(!isValidPoolId(poolId2))
		throw runtime_error(string(__func__)+" -- invalid poolId2 parameter: "+istring(poolId2));
	if(poolId1==poolId2)
		return;

	invalidateAllCachedBlocks(false,poolId1);
	invalidateAllCachedBlocks(false,poolId2);

	// swap SATs for two pools
	vector<RLogicalBlock> tempSAT=SAT[poolId1];
	SAT[poolId1]=SAT[poolId2];
	SAT[poolId2]=tempSAT;

	// swap pool size info
	const RPoolInfo tempPool=pools[poolId1];
	pools[poolId1]=pools[poolId2];
	pools[poolId2]=tempPool;
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::swapPools(const string poolName1,const string poolName2)
{
	swapPools(getPoolIdByName(poolName1),getPoolIdByName(poolName2));
}



template<class l_addr_t,class p_addr_t>
	const bool TPoolFile<l_addr_t,p_addr_t>::prvGetPoolIdByName(const string poolName,poolId_t &poolId) const
{
	map<const string,poolId_t>::const_iterator i=poolNames.find(poolName);
	if(i==poolNames.end())
		return false;

	poolId=i->second;
	return true;
}

template<class l_addr_t,class p_addr_t>
	const typename TPoolFile<l_addr_t,p_addr_t>::poolId_t TPoolFile<l_addr_t,p_addr_t>::getPoolIdByName(const string poolName) const
{
	if(!opened)
		throw runtime_error(string(__func__)+" -- no file is open");

	poolId_t poolId=0;
	if(prvGetPoolIdByName(poolName,poolId))
		return poolId;
	else
		throw runtime_error(string(__func__)+" -- name not found: "+poolName);
}

template<class l_addr_t,class p_addr_t>
	const size_t TPoolFile<l_addr_t,p_addr_t>::getPoolIndexCount() const
{
	if(!opened)
		throw runtime_error(string(__func__)+" -- no file is open");
	return poolNames.size();
}

template<class l_addr_t,class p_addr_t>
	const typename TPoolFile<l_addr_t,p_addr_t>::poolId_t TPoolFile<l_addr_t,p_addr_t>::getPoolIdByIndex(const size_t index) const // where index is 0 to getPoolIndexCount()-1
{
	if(index>=poolNames.size())
		throw runtime_error(string(__func__)+" -- index out of bounds: "+istring(index));

	map<const string,poolId_t>::const_iterator i=poolNames.begin();
	advance(i,index);
	return i->second;
}

template<class l_addr_t,class p_addr_t>
	const string TPoolFile<l_addr_t,p_addr_t>::getPoolNameById(const poolId_t poolId) const
{
	if(!isValidPoolId(poolId))
		throw runtime_error(string(__func__)+" -- poolId parameter out of bounds: "+istring(poolId));

	for(map<const string,poolId_t>::const_iterator i=poolNames.begin();i!=poolNames.end();i++)
	{
		if(i->second==poolId)
			return i->first;
	}

	printf("uh oh.. pool name not found for pool id: %u\n",poolId);
	exit(0);
	return "";
}
template<class l_addr_t,class p_addr_t>
	const bool  TPoolFile<l_addr_t,p_addr_t>::isValidPoolId(const poolId_t poolId) const
{
	return poolId<pools.size() && pools[poolId].isValid;
}


template<class l_addr_t,class p_addr_t>
	const bool TPoolFile<l_addr_t,p_addr_t>::containsPool(const string poolName) const
{
	return poolNames.find(poolName)!=poolNames.end();
}




// Mutual Exclusion Methods
template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::sharedLock() const
{
	structureMutex.readLock();
}

template<class l_addr_t,class p_addr_t>
	const bool TPoolFile<l_addr_t,p_addr_t>::sharedTrylock() const
{
	return structureMutex.tryReadLock();
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::sharedUnlock() const
{
	structureMutex.unlock();
}

template<class l_addr_t,class p_addr_t>
	const size_t TPoolFile<l_addr_t,p_addr_t>::getSharedLockCount() const
{
	return structureMutex.getReadLockCount();
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::exclusiveLock() const
{
	structureMutex.writeLock();
}

template<class l_addr_t,class p_addr_t>
	const bool TPoolFile<l_addr_t,p_addr_t>::exclusiveTrylock() const
{
	return structureMutex.tryWriteLock();
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::exclusiveUnlock() const
{
	structureMutex.unlock();
}

template<class l_addr_t,class p_addr_t>
	const bool TPoolFile<l_addr_t,p_addr_t>::isExclusiveLocked() const
{
	return structureMutex.isLockedForWrite();
}





// --- Private Methods --------------------------------------------------------

// Misc
template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::init(const bool createInitialCachedBlocks)
{
	filename="";

	SAT.clear();
	SAT.reserve(64);

	pasm.free_all();

	poolNames.clear();

	pools.clear();
	pools.reserve(64);

	accessers.clear();

	if(createInitialCachedBlocks)
	{
		while(unusedCachedBlocks.size()>INITIAL_CACHED_BLOCK_COUNT)
		{
			delete unusedCachedBlocks.front();
			unusedCachedBlocks.pop_front();
		}

		while(unusedCachedBlocks.size()<INITIAL_CACHED_BLOCK_COUNT)
			unusedCachedBlocks.push_back(new RCachedBlock(maxBlockSize));
	}
	else
	{
		while(!unusedCachedBlocks.empty())
		{
			delete unusedCachedBlocks.front();
			unusedCachedBlocks.pop_front();
		}
	}

	while(!unreferencedCachedBlocks.empty())
	{
		typename set<RCachedBlock *>::iterator i=unreferencedCachedBlocks.begin();
		delete *i;
		unreferencedCachedBlocks.erase(i);
	}
	while(!activeCachedBlocks.empty())
	{
		typename set<RCachedBlock *>::iterator i=activeCachedBlocks.begin();
		delete *i;
		activeCachedBlocks.erase(i);
	}

	opened=false;
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::verifyAllBlockInfo(bool expectContinuousPhysicalAllocs)
{
	// for each pool, make sure the logical space is continuous with no gaps
	// and for each block in the logical space, make sure there is a physical block with the right size
	for(size_t poolId=0;poolId<pools.size();poolId++)
	{
		if(!pools[poolId].isValid)
			continue;

		l_addr_t expectedStart=0;
	
		const blocksize_t maxBlockSize=getMaxBlockSizeFromAlignment(pools[poolId].alignment);
		
		for(size_t t=0;t<SAT[poolId].size();t++)
		{
			const RLogicalBlock &logicalBlock=SAT[poolId][t];

			if(logicalBlock.logicalStart!=expectedStart)
			{
				printSAT();
				printf("pool: %u -- logical start wasn't what was expected in logical block: %lld\n",poolId,(long long)expectedStart);
				logicalBlock.print();
				exit(1);
			}

			if(!pasm.isAlloced(logicalBlock.physicalStart))
			{
				printSAT();
				printf("pool: %u -- physicalStart isn't allocated\n",poolId);
				logicalBlock.print();
				exit(1);
			}

			if(pasm.getAllocedSize(logicalBlock.physicalStart)!=logicalBlock.size)
			{
				printSAT();
				printf("pool: %u physical block's size isn't the same as the logical block's:\n",poolId);
				logicalBlock.print();
				printf("physical block's size: %lld\n",(long long)pasm.getAllocedSize(logicalBlock.physicalStart));
				exit(1);
			}

			if(t!=SAT[poolId].size()-1)
			{ // check if next block could be joined with this block, just notify if this is true, it's not a serious problem
				const RLogicalBlock &nextLogicalBlock=SAT[poolId][t+1];
				
				if((logicalBlock.physicalStart+logicalBlock.size)==nextLogicalBlock.physicalStart && 
				   (logicalBlock.size+nextLogicalBlock.size)<=maxBlockSize)
				{
					printf("NOTE: two blocks could be joined: %u and %u\n",t,t+1);
					logicalBlock.print();
					nextLogicalBlock.print();
				}
			}

			expectedStart+=logicalBlock.size;

			// make sure no other logical block exists in any pool with an overlapping physical start
			for(size_t x=poolId;x<pools.size();x++)
			{
				for(size_t y= (x==poolId) ? t+1 : 0;y<SAT[x].size();y++)
				{
					if(CPhysicalAddressSpaceManager::overlap(logicalBlock.physicalStart,logicalBlock.size,SAT[x][y].physicalStart,SAT[x][y].size))
					{
						printSAT();
						printf("pool: %u -- two blocks are occupying the same physical space\n",poolId);
						logicalBlock.print();
						printf("and pool: %u\n",x);
						SAT[x][y].print();
						exit(1);
					}
				}
			}
		}

		if(SAT[poolId].size()>0)
		{
			const RLogicalBlock &b=SAT[poolId][SAT[poolId].size()-1];
			if((b.logicalStart+b.size)!=getPoolSize(poolId))
			{
				printSAT();
				printf("pool: %u -- last address doesn't equal up to poolSize\n",poolId);
				exit(1);
			}

		}
	}

	pasm.verify(expectContinuousPhysicalAllocs);
}

template<class l_addr_t,class p_addr_t>
	const p_addr_t TPoolFile<l_addr_t,p_addr_t>::getProceedingPoolSizes(const poolId_t poolId) const
{
	p_addr_t proceedingPoolSizes=0;
	for(size_t t=0;t<poolId;t++)
	{
		if(pools[t].isValid) // wouldn't really matter cause size of invalids is supposed to be zero
			proceedingPoolSizes+=pools[t].size;
	}
	return proceedingPoolSizes;
}

template<class l_addr_t,class p_addr_t>
	const typename TPoolFile<l_addr_t,p_addr_t>::blocksize_t TPoolFile<l_addr_t,p_addr_t>::getMaxBlockSizeFromAlignment(const alignment_t alignment) const
{
	if(maxBlockSize<=(maxBlockSize%alignment))
		throw runtime_error(string(__func__)+" -- alignment size is too big");

	return maxBlockSize-(maxBlockSize%alignment);
}

template<class l_addr_t,class p_addr_t>
	const l_addr_t TPoolFile<l_addr_t,p_addr_t>::getPoolSize(const poolId_t poolId) const
{
	if(!isValidPoolId(poolId))
		throw runtime_error(string(__func__)+" -- invalid poolId parameter: "+istring(poolId));

	return pools[poolId].size;
}

template<class l_addr_t,class p_addr_t>
	const l_addr_t TPoolFile<l_addr_t,p_addr_t>::getPoolSize(const string poolName) const
{
	return getPoolSize(getPoolIdByName(poolName));
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::writeMetaData(CMultiFile *f,bool writeSAT)
{
	if(f==&blockFile) // only do if we're working on our blockFile
		pasm.make_file_smallest();

	if(blockFile.getSize()<LEADING_DATA_SIZE)
		blockFile.setSize(LEADING_DATA_SIZE);

	// write meta and user info
	uint64_t metaDataOffset=f->getSize();

	if(writeSAT)
		writeSATToFile(f,metaDataOffset);

	// Signature
	f->write(FORMAT_SIGNATURE,8,SIGNATURE_OFFSET);

	// EOF
	int8_t eofChar=26;
	f->write(&eofChar,sizeof(eofChar),EOF_OFFSET);

	// Format Version
	{
	uint32_t formatVersion=FORMAT_VERSION;
	hetle(&formatVersion);
	f->write(&formatVersion,sizeof(formatVersion),FORMAT_VERSION_OFFSET);
	}

	// Dirty
	int8_t dirty=1;
	f->write(&dirty,sizeof(dirty),DIRTY_INDICATOR_OFFSET);

	// Which SAT File
	{
	uint8_t _whichSATFile=0;
	f->write(&_whichSATFile,sizeof(_whichSATFile),WHICH_SAT_FILE_OFFSET);
	}

	// Meta Data Offset
	hetle(&metaDataOffset);
	f->write(&metaDataOffset,sizeof(metaDataOffset),META_DATA_OFFSET);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::prvCreatePool(const string poolName,const alignment_t alignment,const bool throwOnExistance,const bool reuseOldPoolIds)
{
	/* the stuff about a the poolName being "__internal_invalid_pool" deals with a restoring of the SAT from disk and needing to create same poolIds (isValid and !isValid) as before */
	poolId_t poolId;
	if(poolName=="__internal_invalid_pool" || !prvGetPoolIdByName(poolName,poolId))
	{	// poolName not found
		if(poolName.length()>MAX_POOL_NAME_LENGTH)
			throw runtime_error(string(__func__)+" -- pool name too long: "+poolName);

		// this is where the poolId get's created.. either pools.size() or a recycled element in the pools vector
		
		// check for any unused positions in the pools vector
		poolId_t newPoolId=pools.size(); // <-- the would-be new one if we don't find one to use
		if(reuseOldPoolIds) // sometimes (buildSATFromFile()) we don't want to use the old poolIds because we want to preserve original poolId numbers
		{
			for(size_t t=0;t<pools.size();t++)
			{
				if(!pools[t].isValid)
				{
					newPoolId=t;
					break;
				}
			}
		}


		// either record the name of the new pool or set as not valid
		if(poolName!="__internal_invalid_pool")
			poolNames.insert(make_pair(poolName,newPoolId));
		addPool(newPoolId,alignment,poolName!="__internal_invalid_pool");
	}
	else
	{
		// pool name already exists
		if(throwOnExistance)
			throw runtime_error(string(__func__)+" -- pool name already exists: "+poolName);
	}
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::addPool(const poolId_t poolId,const alignment_t alignment,bool isValid)
{
	invalidateAllCachedBlocks(false,poolId);

	if((isValid && alignment==0) || alignment>maxBlockSize)
		throw runtime_error(string(__func__)+" -- invalid alignment: "+istring(alignment)+" alignment must be 0 < alignment <= maxBlockSize (which is: "+istring(maxBlockSize)+")");

	if(poolId<pools.size() && !pools[poolId].isValid)
	{ // reusing existing element the SAT vector
		pools[poolId].size=0;
		pools[poolId].alignment=alignment;
		pools[poolId].isValid=isValid;
	}
	else if(poolId==pools.size())
	{ // create new entry in the SAT vector
		appendNewSAT();
		pools.push_back(RPoolInfo(0,alignment,isValid));
	}
	else
		throw runtime_error(string(__func__)+" -- invalid new pool id or pool already exists: "+istring(poolId));
}


template<class l_addr_t,class p_addr_t>
	const string TPoolFile<l_addr_t,p_addr_t>::getPoolDescription(const poolId_t poolId) const
{
	return "poolId: "+istring(poolId)+" name: '"+getPoolNameById(poolId)+"' byte size: "+istring(getPoolSize(poolId))+" byte alignment: "+istring(getPoolAlignment(poolId));
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::writeDirtyIndicator(const bool dirty,CMultiFile *f)
{
	int8_t temp;
	temp=dirty ? 1 : 0;
	f->write(&temp,sizeof(temp),DIRTY_INDICATOR_OFFSET);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::appendNewSAT()
{
	SAT.push_back(vector<RLogicalBlock>());
	SAT[SAT.size()-1].reserve(1024);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::offsetLogicalAddressSpace(typename vector<RLogicalBlock>::iterator first,typename vector<RLogicalBlock>::iterator end,const l_addr_t offset,int add_or_sub)
{
	if(add_or_sub>0)
	{
		for(typename vector<RLogicalBlock>::iterator i=first;i!=end;i++)
			i->logicalStart+=offset;
	}
	else
	{
		for(typename vector<RLogicalBlock>::iterator i=first;i!=end;i++)
			i->logicalStart-=offset;
	}
}




// Structural Integrity Methods
template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::openSATFiles(const bool mustExist)
{
	SATFiles[0].open(SATFilename+"1",!mustExist);
	SATFiles[1].open(SATFilename+"2",!mustExist);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::closeSATFiles(const bool removeFiles)
{
	SATFiles[0].sync();
	SATFiles[1].sync();

	SATFiles[0].close(removeFiles);
	SATFiles[1].close(removeFiles);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::writeWhichSATFile()
{
	uint8_t temp;
	temp=whichSATFile;
	blockFile.write(&temp,sizeof(temp),WHICH_SAT_FILE_OFFSET);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::backupSAT()
{
	whichSATFile= ((whichSATFile==0) ? 1 : 0);

	writeSATToFile(&SATFiles[whichSATFile],0);
	writeWhichSATFile();
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::writeSATToFile(CMultiFile *f,const p_addr_t writeWhere)
{
	// ??? probably want to put a SAT file signature here to make sure it's a SAT file

	CMultiFile::RHandle multiFileHandle;
	f->seek(writeWhere,multiFileHandle);

	// write number of pools
	{
	uint32_t poolCount=SAT.size();
	hetle(&poolCount);
	f->write(&poolCount,sizeof(poolCount),multiFileHandle);
	}

	for(size_t poolId=0;poolId<SAT.size();poolId++)
	{
		// write name of pool
		if(pools[poolId].isValid)
			writeString(getPoolNameById(poolId),f,multiFileHandle);
		else
			writeString(string("__internal_invalid_pool"),f,multiFileHandle);

		// write pool info structure
		pools[poolId].writeToFile(f,multiFileHandle);

		// write number of SAT entries for this pool
		{
		uint32_t SATSize=SAT[poolId].size();
		hetle(&SATSize);
		f->write(&SATSize,sizeof(SATSize),multiFileHandle);
		}

		TAutoBuffer<uint8_t> mem(SAT[poolId].size()*RLogicalBlock().getMemSize());

		// write each SAT entry
		size_t offset=0;
		for(size_t t=0;t<SAT[poolId].size();t++)
			SAT[poolId][t].writeToMem(mem,offset);

		f->write(mem,mem.getSize(),multiFileHandle);
	}
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::restoreSAT()
{
	invalidateAllCachedBlocks();


	uint8_t buffer;
	blockFile.read(&buffer,sizeof(buffer),WHICH_SAT_FILE_OFFSET);
	whichSATFile=buffer;
	if(whichSATFile!=0 && whichSATFile!=1)
	{
		printf("invalid which SAT file value\n");
		exit(0);
	}

	buildSATFromFile(&SATFiles[whichSATFile],0);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::buildSATFromFile(CMultiFile *f,const p_addr_t readWhere)
{
	CMultiFile::RHandle multiFileHandle;
	f->seek(readWhere,multiFileHandle);

	// ??? probably want to put a SAT file signature here to make sure it's a SAT file

	// destory current SAT and pool information
	pools.clear();
	poolNames.clear();
	SAT.clear();
	pasm.free_all();

	// read number of pools
	uint32_t poolCount;
	f->read(&poolCount,sizeof(poolCount),multiFileHandle);
	lethe(&poolCount);
	for(size_t poolId=0;poolId<poolCount;poolId++)
	{
		// read pool name
		string poolName;
		readString(poolName,f,multiFileHandle);

		// read pool info structure
		RPoolInfo poolInfo;
		poolInfo.readFromFile(f,multiFileHandle);

		if(poolInfo.alignment==0 || poolInfo.alignment>this->maxBlockSize)
			throw runtime_error(string(__func__)+" -- invalid pool alignment read from file: "+istring(poolInfo.alignment)+" alignment must be 0 < alignment <= maxBlockSize (which is: "+istring(this->maxBlockSize)+")");

		try
		{
			prvCreatePool(poolName,poolInfo.alignment,true,false);
		}
		catch(...)
		{
			printf("error creating pool\n");
			exit(0);
		}

		// read number of SAT entries
		uint32_t SATSize;
		f->read(&SATSize,sizeof(SATSize),multiFileHandle);
		lethe(&SATSize);

		// read SAT into mem buffer
		TAutoBuffer<uint8_t> mem(SATSize*RLogicalBlock().getMemSize());
		f->read(mem,mem.getSize(),multiFileHandle);

		const blocksize_t maxBlockSize=getMaxBlockSizeFromAlignment(poolInfo.alignment);

		// read each SAT entry from that mem buffer and put into the actual SAT data-member
		size_t offset=0;
		for(size_t t=0;t<SATSize;t++)
		{
			RLogicalBlock logicalBlock;
			logicalBlock.readFromMem(mem,offset);

			// divide the size of the block just read into pieces that will fit into maxBlockSize sizes blocks
			// just in case the maxBlockSize is smaller than it used to be
			const blocksize_t blockSize=logicalBlock.size;
			for(blocksize_t j=0;j<blockSize/maxBlockSize;j++)
			{
				logicalBlock.size=maxBlockSize;

				sortedInsert(SAT[poolId],logicalBlock);

				logicalBlock.logicalStart+=maxBlockSize;
				logicalBlock.physicalStart+=maxBlockSize;
			}
			logicalBlock.size=blockSize%maxBlockSize;
			if(logicalBlock.size>0)
				sortedInsert(SAT[poolId],logicalBlock);

			pools[poolId].size+=blockSize;
		}

		if(pools[poolId].size!=poolInfo.size)
		{
			printf("buildSATFromFile -- pool size from SAT read does not match pool size from pool info read\n");
			exit(0);
		}
	}
	pasm.buildFromSAT(SAT);
	pasm.make_file_smallest();
}




// SAT operations
template<class l_addr_t,class p_addr_t>
	const size_t TPoolFile<l_addr_t,p_addr_t>::findSATBlockContaining(const poolId_t poolId,const l_addr_t where,bool &atStartOfBlock) const
{
	if(SAT[poolId].empty())
		throw runtime_error(string(__func__)+" -- SAT is empty");

	const size_t i=(upper_bound(SAT[poolId].begin(),SAT[poolId].end(),RLogicalBlock(where))-1)-SAT[poolId].begin();
	atStartOfBlock=(SAT[poolId][i].logicalStart==where);
	return i;
}


template<class l_addr_t,class p_addr_t>
	const bool TPoolFile<l_addr_t,p_addr_t>::isInWindow(const p_addr_t start,const p_addr_t end,const p_addr_t windowStart,const p_addr_t windowEnd)
{
	if(start<=windowStart && end>=windowStart)
		return true;
	if(end>=windowEnd && start<=windowEnd)
		return true;
	if(start>=windowStart && end<=windowEnd)
		return true;
	return false;
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::joinAllAdjacentBlocks()
{
	for(size_t poolId=0;poolId<pools.size();poolId++)
	{
		if(pools[poolId].isValid)
			joinAdjacentBlocks(poolId);
	}
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::joinAdjacentBlocks(const poolId_t poolId)
{
	joinAdjacentBlocks(poolId,0,SAT[poolId].size());
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::joinAdjacentBlocks(const poolId_t poolId,const size_t firstBlockIndex,const size_t blockCount)
{
	const size_t totalBlocks=SAT[poolId].size();
	const blocksize_t maxBlockSize=getMaxBlockSizeFromAlignment(pools[poolId].alignment);

	for(size_t t=firstBlockIndex;t<=firstBlockIndex+blockCount;t++)
	{
		if(t==firstBlockIndex)
			continue; // skip first iteration because we're looking at t and the one before t (also avoids t-1 being underflowing)
		if(t>=totalBlocks)
			break; // just in case firstBlockIndex + blockCount specifies too many blocks

		RLogicalBlock &b1=SAT[poolId][t-1];
		const RLogicalBlock &b2=SAT[poolId][t];

		if((b1.physicalStart+b1.size)==b2.physicalStart)
		{ // blocks are physically next to each other -- candidate for joining
			const l_addr_t newSize=b1.size+b2.size;

			// size of blocks if joined doesn't make a block too big
			if(newSize<=maxBlockSize)
			{ // now join blocks blockIndex and blockIndex+1

				pasm.join_blocks(b1.physicalStart,b2.physicalStart);

				b1.size=newSize;
				SAT[poolId].erase(SAT[poolId].begin()+t);

				// check this block again the next time around
				t--;
			}
		}
	}
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::printSAT() const
{
	printf("\nSAT(s): (maxBlockSize: %u)\n",maxBlockSize);
	for(size_t poolId=0;poolId<pools.size();poolId++)
	{
		if(!pools[poolId].isValid)
			continue;

		printf("\t%-4u Pool: '%s' size: %lld alignment: %lld\n",poolId,getPoolNameById(poolId).c_str(),(long long)getPoolSize(poolId),(long long)getPoolAlignment(poolId));
		for(size_t t=0;t<SAT[poolId].size();t++)
		{
			printf("\t\t%-4u ",t);
			SAT[poolId][t].print();
		}

	}
	pasm.print();

	printf("\n");
}

template<class l_addr_t,class p_addr_t>
	bool TPoolFile<l_addr_t,p_addr_t>::defrag()
{
	if(!isOpen())
		throw runtime_error(string(__func__)+" -- file not open");

	invalidateAllCachedBlocks();

	/* ???
	 * Right now, defragging can happen and be much more inefficiant than it needs to be.
	 * This is because it always makes the first poolId come first in the file.  Fragementation
	 * should not care which order than the pools exist, only that they are contiguous on
	 * disk.  I would do better to move as few blocks as possible, this would mean knowing
	 * which order is best for the pools on disk.  Perhaps I could go thru each permutation of
	 * poolId, but I'm not quite sure how do know which is the best ordering. 
	 */


	/*
	 * The defragging algorithm works as follows:
	 * From the SAT, create a list of physical blocks that are used.
	 * This is used to determine where we can put things later.
	 * For each pool and each block in the pool, call a function
	 * that moves that block physically where it it tells it too.  This
	 * function is told the correct position from this defrag method, but
	 * it might move things out of the way to fulfil its duty.
	 *
	 * Finally, when everything is consecutive in a pool, a new SAT is built
	 * and the physical address space manager is told to build it's info 
	 * from this new SAT
	 *
	 * This algorithm is no less than O(n^2) where n is the number of physical 
	 * blocks before defragging
	 */

	bool didSomething=false;
	TAutoBuffer<int8_t> temp(maxBlockSize);

	//    addr     size
	map<p_addr_t,p_addr_t> physicalBlockList;

	// build physical block list
	for(poolId_t poolId=0;poolId<pools.size();poolId++)
	{
		if(!pools[poolId].isValid)
			continue;

		for(size_t t=0;t<SAT[poolId].size();t++)
		{
			const RLogicalBlock &b=SAT[poolId][t];
			physicalBlockList[b.physicalStart]=b.size;
		}
	}

	// call method to correct each block's position
	p_addr_t physicallyWhere=0;
	for(poolId_t poolId=0;poolId<pools.size();poolId++)
	{
		if(!pools[poolId].isValid)
			continue;

		for(size_t t=0;t<SAT[poolId].size();t++)
		{
			RLogicalBlock &b=SAT[poolId][t];
			didSomething|=physicallyMoveBlock(b,physicallyWhere,physicalBlockList,temp);
			physicallyWhere+=b.size;
		}
	}

	if(didSomething)
	{
		pasm.make_file_smallest();
	
		// create new SAT
		physicallyWhere=0;
		for(poolId_t poolId=0;poolId<pools.size();poolId++)
		{
			SAT[poolId].clear();
			if(!pools[poolId].isValid)
				continue;

			const blocksize_t maxBlockSize=getMaxBlockSizeFromAlignment(pools[poolId].alignment);

			const l_addr_t blockCount=pools[poolId].size/maxBlockSize;
			for(l_addr_t t=0;t<=blockCount;t++)
			{
				const l_addr_t blockSize= (t!=blockCount) ? maxBlockSize : pools[poolId].size%maxBlockSize;
				if(blockSize>0)
				{
					RLogicalBlock b;
					b.logicalStart=t*maxBlockSize;
					b.physicalStart=physicallyWhere;
					b.size=blockSize;
		
					SAT[poolId].push_back(b);
	
					physicallyWhere+=blockSize;
				}
			}
		}

		pasm.buildFromSAT(SAT);
		backupSAT();
	}

	return didSomething;
}


template<class l_addr_t,class p_addr_t>
	bool TPoolFile<l_addr_t,p_addr_t>::physicallyMoveBlock(RLogicalBlock &block,p_addr_t physicallyWhere,map<p_addr_t,p_addr_t> &physicalBlockList,int8_t *temp)
{
	if(block.physicalStart!=physicallyWhere)
	{
		// find what may exist in the location we want to move block to
		for(poolId_t x=0;x<SAT.size();x++)
		{
			if(!pools[x].isValid)
				continue;

			for(size_t y=0;y<SAT[x].size();y++)
			{
				RLogicalBlock &b=SAT[x][y];

				// see if b is in the way of where we want to put block
				if(&b!=&block && CPhysicalAddressSpaceManager::overlap(physicallyWhere,block.size,b.physicalStart,b.size))
				{ // b is in the way
					p_addr_t moveTo=0;

					// find a new place to put b (look backwards, because we're building up the beginning)
					for(typename map<p_addr_t,p_addr_t>::reverse_iterator i=physicalBlockList.rbegin();i!=physicalBlockList.rend();i++)
					{
						typename map<p_addr_t,p_addr_t>::reverse_iterator prev_i=i; prev_i++;
						if(prev_i!=physicalBlockList.rend())
						{
							const p_addr_t holeStart=prev_i->first+prev_i->second;
							const p_addr_t holeSize=i->first - holeStart;
							if(holeSize>=b.size)
							{ // hole is large enough
								// now make sure that this hole isn't where we want to put block
								if(!CPhysicalAddressSpaceManager::overlap(physicallyWhere,block.size,holeStart,holeSize))
								{ // move it here
									moveTo=holeStart;
									break;
								}
							}
						}
					}

					if(moveTo==0)
					{ // no place to move b to, so create new space
						moveTo=pasm.get_file_size();
						pasm.set_file_size(pasm.get_file_size()+b.size);
						dprintf("just grew file from %lld to %lld\n",(long long)moveTo,(long long)pasm.get_file_size());
					}

					// now actually move b
					{
						dprintf("moving block from %lld to %lld\n",(long long)b.physicalStart,(long long)moveTo);

						// read b off disk
						blockFile.read(temp,b.size,b.physicalStart+LEADING_DATA_SIZE);

						// update physicalBlockList and SAT
						physicalBlockList.erase(physicalBlockList.find(b.physicalStart));
						b.physicalStart=moveTo;
						physicalBlockList[moveTo]=b.size;

						// write b back out in the new location
						blockFile.write(temp,b.size,moveTo+LEADING_DATA_SIZE);
					}
				}
			}
		}

		// now nothing is in the way to move block, so move it
		{
			dprintf("moving block from %lld to %lld\n",(long long)block.physicalStart,(long long)physicallyWhere);

			// read block off disk
			blockFile.read(temp,block.size,block.physicalStart+LEADING_DATA_SIZE);

			// update physicalBlockList and SAT
			physicalBlockList.erase(physicalBlockList.find(block.physicalStart));
			block.physicalStart=physicallyWhere;
			physicalBlockList[physicallyWhere]=block.size;

			// write block back out in the proper location
			blockFile.write(temp,block.size,physicallyWhere+LEADING_DATA_SIZE);
		}

		return true;
	}
	return false;
}




// Pool Modification (pe -- pool elements, b -- bytes)
template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::insertSpace(const poolId_t poolId,const l_addr_t peWhere,const l_addr_t peCount)
{
	if(!opened)
		throw runtime_error(string(__func__)+" -- no file is open");
	if(peCount==0)
		return;

	if(!isValidPoolId(poolId))
		throw runtime_error(string(__func__)+" -- invalid poolId: "+istring(poolId));

	const alignment_t bAlignment=pools[poolId].alignment;
	const l_addr_t bPoolSize=pools[poolId].size;
	const l_addr_t pePoolSize=bPoolSize/bAlignment;
	const l_addr_t pePoolMaxSize=maxLogicalAddress/bAlignment;
	
	if(pePoolMaxSize-pePoolSize<peCount)
		throw runtime_error(string(__func__)+" -- insufficient logical address space to insert "+istring(peCount)+" elements into pool ("+getPoolDescription(poolId)+")");
	if(peWhere>pePoolSize)
		throw runtime_error(string(__func__)+" -- out of range peWhere "+istring(peWhere)+" for pool ("+getPoolDescription(poolId)+")");

	invalidateAllCachedBlocks(false,poolId);

	const l_addr_t bWhere=peWhere*bAlignment;
	const l_addr_t bCount=peCount*bAlignment;

	const blocksize_t maxBlockSize=getMaxBlockSizeFromAlignment(bAlignment);
	const size_t newLogicalBlockCount=(bCount/maxBlockSize);
	bool didSplitOne=false;

	size_t logicalBlockIndex=SAT[poolId].size();
	if(bWhere<bPoolSize)
	{ // in the middle (not appending)
		bool atStartOfBlock;
		logicalBlockIndex=findSATBlockContaining(poolId,bWhere,atStartOfBlock);

		if(!atStartOfBlock)
		{ // split the block at logicalBlockIndex since where isn't exacly at the beginning of the block
dprintf("insertSpace - case 1/6\n");
			didSplitOne=true;

			RLogicalBlock &logicalBlock=SAT[poolId][logicalBlockIndex];

			// sanity check
			if(bWhere<=logicalBlock.logicalStart)
			{ // logical impossibility since atStartOfBlock wasn't true (unless it was wrong)
				printf("oops...\n");
				exit(1);
			}

			// modify block at logicalBlockIndex and create a new RLogicalBlock for the second part of the split block
			const l_addr_t firstPartSize=bWhere-logicalBlock.logicalStart;
			const l_addr_t secondPartSize=logicalBlock.size-firstPartSize;

			// inform the physical address space manager that we want to split the physical block into two parts
			const p_addr_t newPhysicalStart=pasm.split_block(logicalBlock.physicalStart,firstPartSize);

			// shrink the logical block's size
			logicalBlock.size=firstPartSize;

			// create new logical block which is the second part of the old block
			RLogicalBlock newLogicalBlock;
			newLogicalBlock.logicalStart=logicalBlock.logicalStart+firstPartSize;
			newLogicalBlock.physicalStart=newPhysicalStart;
			newLogicalBlock.size=secondPartSize;

			// add the new logical block
			sortedInsert(SAT[poolId],newLogicalBlock);

			// alter the logical mapping by increasing all blocks' logicalStarts after the insertion point
			offsetLogicalAddressSpace(SAT[poolId].begin()+logicalBlockIndex+1,SAT[poolId].end(),bCount,1);
		}
		else
		{ // increase all blocks at and after insertion point
dprintf("insertSpace - case 2/6\n");
			offsetLogicalAddressSpace(SAT[poolId].begin()+logicalBlockIndex,SAT[poolId].end(),bCount,1);

			if(logicalBlockIndex>0)
				logicalBlockIndex--;
			else
				didSplitOne=true;
		}
	}
	else if(bWhere==bPoolSize)
	{ // at the end (or just starting)
		if(bPoolSize>0)
		{
dprintf("insertSpace - case 3/6\n");
			logicalBlockIndex=SAT[poolId].size()-1;
			if((SAT[poolId][logicalBlockIndex].logicalStart+SAT[poolId][logicalBlockIndex].size)!=bWhere)
			{
				printf("huh??\n");
				exit(0);
			}
		}
	}


	// create all the new logical and physical blocks needed to fill the gap from the split of the old block

	RLogicalBlock newLogicalBlock;
	for(size_t t=0;t<=newLogicalBlockCount;t++)
	{
		if(t==0)
		{ // first block
			dprintf("insertSpace - case 4/6\n");
			newLogicalBlock.size=bCount%maxBlockSize;
			if(newLogicalBlock.size==0)
				continue; // it wasn't necessary to do this first iteration that handles the remainder
			newLogicalBlock.logicalStart=bWhere;
		}
		else
		{
			dprintf("insertSpace - case 5/6\n");
			newLogicalBlock.size=maxBlockSize;
			newLogicalBlock.logicalStart=bWhere+((t-1)*maxBlockSize)+(bCount%maxBlockSize);
		}

		newLogicalBlock.physicalStart=pasm.alloc(newLogicalBlock.size);

		/* ??? I could perhaps move this logic in the t==0 case above and join as much
		 * of the new space with SAT[poolId][logicalBlockIndex] as possible (if the allocator
		 * gives us space adjacent to it's physical space.  This MIGHT (i need to work it out
		 * more) result in fewer blocks on average */
		if(!didSplitOne && t==0) // the below would only be true iff this is
		{
			if(logicalBlockIndex<SAT[poolId].size()) // false when pool is empty
			{
				// if we're about to create a new logical block that could just as well be joined with the one before it
				// then don't create a new one (appending optimization)
				RLogicalBlock &logicalBlock=SAT[poolId][logicalBlockIndex];
				if(
				   ((logicalBlock.physicalStart+logicalBlock.size)==newLogicalBlock.physicalStart) && 
			   	   ((logicalBlock.size+newLogicalBlock.size)<=maxBlockSize)
				)
				{ 
					dprintf("insertSpace case - 6/6\n");
					logicalBlock.size+=newLogicalBlock.size;
					pasm.join_blocks(logicalBlock.physicalStart,newLogicalBlock.physicalStart);
					pools[poolId].size+=newLogicalBlock.size;
					continue; // skip insertion of new block
				}
			}
		}

		sortedInsert(SAT[poolId],newLogicalBlock);
		pools[poolId].size+=newLogicalBlock.size;
	}

	// ZZZ ??? Takes too long in a real-time situation (i.e. Recording in ReZound)
	//backupSAT();
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::removeSpace(const poolId_t poolId,const l_addr_t peWhere,const l_addr_t peCount)
{
	if(!opened)
		throw runtime_error(string(__func__)+" -- no file is open");
	if(peCount==0)
		return;

	// validate the parameters
	if(!isValidPoolId(poolId))
		throw runtime_error(string(__func__)+" -- invalid poolId: "+istring(poolId));

	const alignment_t bAlignment=pools[poolId].alignment;
	const l_addr_t bPoolSize=pools[poolId].size;
	const l_addr_t pePoolSize=bPoolSize/bAlignment;

	if(peWhere>=pePoolSize)
		throw runtime_error(string(__func__)+" -- out of range peWhere "+istring(peWhere)+" for pool ("+getPoolDescription(poolId)+")");
	if(pePoolSize-peWhere<peCount)
		throw runtime_error(string(__func__)+" -- out of range peWhere "+istring(peWhere)+" and peCount "+istring(peCount)+" for pool ("+getPoolDescription(poolId)+")");

	invalidateAllCachedBlocks(false,poolId);

	const l_addr_t bWhere=peWhere*bAlignment;
	const l_addr_t bCount=peCount*bAlignment;

	bool atStartOfBlock;
	const size_t logicalBlockIndex=findSATBlockContaining(poolId,bWhere,atStartOfBlock);

	l_addr_t removeSize=bCount;
	size_t t=logicalBlockIndex;
	while(removeSize>0 && t<SAT[poolId].size())
	{
		RLogicalBlock &block=SAT[poolId][t];
		const l_addr_t block_start=block.logicalStart;
		const l_addr_t block_end=block_start+(block.size-1);
		const l_addr_t remove_start= (bWhere<block_start) ? block_start : bWhere;
		const l_addr_t remove_end= ((bWhere+(bCount-1))>block_end) ? block_end : (bWhere+(bCount-1));
		const l_addr_t remove_in_block_size=remove_end-remove_start+1;

		if(remove_start==block_start && remove_end==block_end)
		{ // case 1 -- remove whole block -- on first and only block, middle or last block
			// |[.......]|	([..] -- block ; |..| -- section to remove)
dprintf("removeSpace - case 1/4\n");

			pasm.free(block.physicalStart);
			SAT[poolId].erase(SAT[poolId].begin()+t);
		}
		else if(remove_start==block_start && remove_end<block_end)
		{ // case 2 -- remove a head of block -- on first and only block, last block
			// |[.....|..]
dprintf("removeSpace - case 2/4\n");
			
			const blocksize_t new_blockSize=block.size-remove_in_block_size;
			const p_addr_t newPhysicalStart=pasm.partial_free(block.physicalStart,block.physicalStart+remove_in_block_size,new_blockSize);

			block.logicalStart-=bCount-remove_in_block_size; // ...-remove_in_block_size because, this block is not going to be affected by the loop which does this later since we increment t
			block.physicalStart=newPhysicalStart;
			block.size=new_blockSize;
			t++;
		}
		else if(remove_start>block_start && remove_end==block_end)
		{ // case 3 -- remove a tail of block -- first and only block, first block
			// [..|.....]|
dprintf("removeSpace - case 3/4\n");
			const blocksize_t newBlockSize=block.size-remove_in_block_size;

			pasm.partial_free(block.physicalStart,block.physicalStart,newBlockSize);

			block.size=newBlockSize;
			t++;
		}
		else if(remove_start>block_start && remove_end<block_end)
		{ // case 4 -- split block -- on first and only block
			// [..|...|..]
			//    p1  p2
dprintf("removeSpace - case 4/4\n");
			
			const blocksize_t p1=remove_start-block_start;
			const blocksize_t p2=remove_end-block_start;
		
			// the phyical operation consists of a split, then a partial free of the right of the split block
			p_addr_t newPhysicalStart=pasm.split_block(block.physicalStart,p1);
				// now a new physical block starts at block.physicalStart+p1;
			newPhysicalStart=pasm.partial_free(newPhysicalStart,block.physicalStart+p2+1,block.size-p2-1);
				// now the new physical block starts at block.physicalStart+p2;
			
			block.size=p1;
			
			RLogicalBlock newLogicalBlock;
			newLogicalBlock.logicalStart=remove_start;
			newLogicalBlock.physicalStart=newPhysicalStart;
			newLogicalBlock.size=block_end-remove_end;

			sortedInsert(SAT[poolId],newLogicalBlock);
			t+=2;
		}
		else
		{
			printf("error in cases 1-4\n");
			exit(1);
		}

		// ??? sanity check
		if(remove_in_block_size>removeSize)
		{
			printf("remove_in_block_size is greater than removeSize\n");
			exit(1);
		}

		removeSize-=remove_in_block_size;
	}
	
	pools[poolId].size-=bCount;

	// move all the subsequent logicalStarts downward
	offsetLogicalAddressSpace(SAT[poolId].begin()+t,SAT[poolId].end(),bCount,-1);

	// join the blocks that just became adjacent if possible
	joinAdjacentBlocks(poolId,logicalBlockIndex>0 ? logicalBlockIndex-1 : 0,1);
	
	pasm.make_file_smallest();

	backupSAT();
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::moveData(const poolId_t destPoolId,const l_addr_t peDestWhere,const poolId_t srcPoolId,const l_addr_t peSrcWhere,const l_addr_t peCount)
{
	/*
	 * Basically, this logic works the same as removeSpace except every time it removes some
	 * space from the src pool, it adds it to the dest pool
	 */

	if(!opened)
		throw runtime_error(string(__func__)+" -- no file is open");
	if(peCount==0)
		return;

	// validate the parameters
	if(!isValidPoolId(srcPoolId))
		throw runtime_error(string(__func__)+" -- invalid srcPoolId: "+istring(srcPoolId));
	if(!isValidPoolId(destPoolId))
		throw runtime_error(string(__func__)+" -- invalid destPoolId: "+istring(destPoolId));

	const alignment_t bAlignment=pools[srcPoolId].alignment;

	// instead of implementing this separately... I move from src to temp then temp to dest
	if(srcPoolId==destPoolId)
	{
		const string tempPoolName="__internal_moveData_pool__";
		removePool(tempPoolName,false);
		prvCreatePool(tempPoolName,bAlignment,false);
		const poolId_t tempPoolId=getPoolIdByName(tempPoolName);

		try
		{
dprintf("moveData -- case 1/6\n");
			moveData(tempPoolId,0,srcPoolId,peSrcWhere,peCount);
			printf("------------------------------------\n");
			moveData(destPoolId,peDestWhere,tempPoolId,0,peCount);
			removePool(tempPoolName,false);
		}
		catch(...)
		{
			removePool(tempPoolName,false);
			throw;
		}

		backupSAT();
		return;
	}

	if(pools[destPoolId].alignment!=bAlignment)
		throw runtime_error(string(__func__)+" -- alignments do not match for srcPool ("+getPoolDescription(srcPoolId)+") and destPool ("+getPoolDescription(destPoolId)+")");

	const l_addr_t bSrcPoolSize=pools[srcPoolId].size;
	const l_addr_t peSrcPoolSize=bSrcPoolSize/bAlignment;

	if(peSrcWhere>=peSrcPoolSize)
		throw runtime_error(string(__func__)+" -- out of range peSrcWhere "+istring(peSrcWhere)+" for srcPool ("+getPoolDescription(srcPoolId)+")");
	if(peSrcPoolSize-peSrcWhere<peCount)
		throw runtime_error(string(__func__)+" -- out of range peSrcWhere "+istring(peSrcWhere)+" and peCount "+istring(peCount)+" for pool ("+getPoolDescription(srcPoolId)+")");


	const l_addr_t bDestPoolSize=pools[destPoolId].size;
	const l_addr_t peDestPoolSize=bDestPoolSize/bAlignment;

	const l_addr_t pePoolMaxSize=maxLogicalAddress/bAlignment;


	if(pePoolMaxSize-peDestPoolSize<peCount)
		throw runtime_error(string(__func__)+" -- insufficient logical address space to insert "+istring(peCount)+" elements into destPool ("+getPoolDescription(destPoolId)+")");
	if(peDestWhere>peDestPoolSize)
		throw runtime_error(string(__func__)+" -- out of range peDestWhere "+istring(peDestWhere)+" for pool ("+getPoolDescription(destPoolId)+")");

	invalidateAllCachedBlocks(false,srcPoolId);
	invalidateAllCachedBlocks(false,destPoolId);

	const l_addr_t bSrcWhere=peSrcWhere*bAlignment;
	l_addr_t bDestWhere=peDestWhere*bAlignment;
	const l_addr_t bCount=peCount*bAlignment;

	if(bDestWhere==0 && bDestPoolSize==0 && bSrcWhere==0 && bCount==bSrcPoolSize)
	{ // the whole src pool is moving to an empty dest pool, so just assign the whole SAT
		// move src to dest
		SAT[destPoolId]=SAT[srcPoolId];
		pools[destPoolId].size=bSrcPoolSize;

		// clear src
		SAT[srcPoolId].clear();
		pools[srcPoolId].size=0;
	}
	else
	{ // only part of the src pool is moving or all of it is moving to a non-empty pool
		bool atStartOfSrcBlock;
		const size_t srcBlockIndex=findSATBlockContaining(srcPoolId,bSrcWhere,atStartOfSrcBlock);

		size_t destBlockIndex=0;
		if(bDestWhere<bDestPoolSize)
		{
dprintf("moveData -- case 2/6\n");
			bool atStartOfDestBlock;
			destBlockIndex=findSATBlockContaining(destPoolId,bDestWhere,atStartOfDestBlock);
			if(!atStartOfDestBlock)
			{ // go ahead and split the destination block so that we can simply insert new ones along the way
dprintf("moveData -- case 2.5/6\n");

				RLogicalBlock &destLogicalBlock=SAT[destPoolId][destBlockIndex];

				if(bDestWhere<=destLogicalBlock.logicalStart)
				{ // logcal impossibility since atStartOfBlock wasn't true (unless it was wrong)
					printf("oops...\n");
					exit(1);
				}

				const l_addr_t firstPartSize=bDestWhere-destLogicalBlock.logicalStart;
				const l_addr_t secondPartSize=destLogicalBlock.size-firstPartSize;


				// inform the physical address space manager that we want to split the dest physical block into two parts
				pasm.split_block(destLogicalBlock.physicalStart,firstPartSize);
		
				// shrink the dest logical block's size
				destLogicalBlock.size=firstPartSize;

				// create the new logical block which are the second part of the old block
				RLogicalBlock newLogicalBlock;
				newLogicalBlock.logicalStart=destLogicalBlock.logicalStart+firstPartSize;
				newLogicalBlock.physicalStart=destLogicalBlock.physicalStart+firstPartSize;
				newLogicalBlock.size=secondPartSize;

				// add the new logical block
				sortedInsert(SAT[destPoolId],newLogicalBlock);

				destBlockIndex++;
			}

			// move upward all the logicalStarts in the dest pool past the destination where point
			offsetLogicalAddressSpace(SAT[destPoolId].begin()+destBlockIndex,SAT[destPoolId].end(),bCount,1);
		}


		size_t loopCount=0;
		l_addr_t moveSize=bCount;
		size_t src_t=srcBlockIndex;
		while(moveSize>0 && src_t<SAT[srcPoolId].size())
		{
			RLogicalBlock &srcBlock=SAT[srcPoolId][src_t];
			const l_addr_t src_block_start=srcBlock.logicalStart;
			const l_addr_t src_block_end=src_block_start+(srcBlock.size-1);
			const l_addr_t src_remove_start= (bSrcWhere<src_block_start) ? src_block_start : bSrcWhere;
			const l_addr_t src_remove_end= ((bSrcWhere+(bCount-1))>src_block_end) ? src_block_end : (bSrcWhere+(bCount-1));
			const l_addr_t remove_in_src_block_size=src_remove_end-src_remove_start+1;

			if(src_remove_start==src_block_start && src_remove_end==src_block_end)
			{ // case 1 -- remove whole block from src pool -- on first and only block, middle or last block
dprintf("moveData -- case 3/6 -- %d\n",src_t);
				// |[.......]|	([..] -- block ; |..| -- section to remove)
				
				RLogicalBlock newDestLogicalBlock(srcBlock);
				newDestLogicalBlock.logicalStart=bDestWhere;

				SAT[srcPoolId].erase(SAT[srcPoolId].begin()+src_t);

				sortedInsert(SAT[destPoolId],newDestLogicalBlock);
			}
			else if(src_remove_start==src_block_start && src_remove_end<src_block_end)
			{ // case 2 -- remove a head of block -- on first and only block, last block
dprintf("moveData -- case 4/6 -- %d\n",src_t);
				// |[.....|..]

				const blocksize_t new_srcBlockSize=srcBlock.size-remove_in_src_block_size;
				
				// create physical block for src block now to point to
				const p_addr_t new_srcPhysicalStart=pasm.split_block(srcBlock.physicalStart,remove_in_src_block_size);

				// create the new logical block in dest pool
				RLogicalBlock newDestLogicalBlock; 
				newDestLogicalBlock.logicalStart=bDestWhere;
				newDestLogicalBlock.size=remove_in_src_block_size;
				newDestLogicalBlock.physicalStart=srcBlock.physicalStart;
				sortedInsert(SAT[destPoolId],newDestLogicalBlock);

				// modify existing src block
				srcBlock.logicalStart-=bCount-remove_in_src_block_size; // do this because, this srcBlock is not gonna be affected by the offsetLogicalAddressSpace() below
				srcBlock.physicalStart=new_srcPhysicalStart;
				srcBlock.size=new_srcBlockSize;

				src_t++;
			}
			else if(src_remove_start>src_block_start && src_remove_end==src_block_end)
			{ // case 3 -- remove a tail of block -- first and only block, first block
dprintf("moveData -- case 5/6\n");
				// [..|.....]|
				
				const blocksize_t new_srcBlockSize=srcBlock.size-remove_in_src_block_size;

				// create physical block for new dest block
				const p_addr_t new_destPhysicalStart=pasm.split_block(srcBlock.physicalStart,new_srcBlockSize);

				// create the new logical block in dest pool
				RLogicalBlock newDestLogicalBlock;
				newDestLogicalBlock.logicalStart=bDestWhere;
				newDestLogicalBlock.size=remove_in_src_block_size;
				newDestLogicalBlock.physicalStart=new_destPhysicalStart;
				sortedInsert(SAT[destPoolId],newDestLogicalBlock);

				// modify the existing src block
				srcBlock.size=new_srcBlockSize;

				src_t++;
			}
			else if(src_remove_start>src_block_start && src_remove_end<src_block_end)
			{ // case 4 -- split block -- on first and only block
dprintf("moveData -- case 6/6\n");
				// [..|...|..]
				//    p1  p2
			
				const blocksize_t p1=src_remove_start-src_block_start;
				const blocksize_t p2=src_remove_end-src_block_start;
			
				// create new physical block by splitting existing src physical block
				const p_addr_t new_destPhysicalStart=pasm.split_block(srcBlock.physicalStart,p1);

				// modify existing src block
				srcBlock.size=p1;
				
				// split the new physical block; left side -> new dest block, right side -> new src block
				const p_addr_t new_srcPhysicalStart=pasm.split_block(new_destPhysicalStart,remove_in_src_block_size);

				// create new src block
				RLogicalBlock newSrcLogicalBlock;
				newSrcLogicalBlock.logicalStart=srcBlock.logicalStart+srcBlock.size;
				newSrcLogicalBlock.physicalStart=new_srcPhysicalStart;
				newSrcLogicalBlock.size=src_block_end-src_remove_end;
				sortedInsert(SAT[srcPoolId],newSrcLogicalBlock);

				// create new dest block
				RLogicalBlock newDestLogicalBlock;
				newDestLogicalBlock.logicalStart=bDestWhere;
				newDestLogicalBlock.physicalStart=new_destPhysicalStart;
				newDestLogicalBlock.size=remove_in_src_block_size;
				sortedInsert(SAT[destPoolId],newDestLogicalBlock);

				src_t+=2;
			}
			else
			{
				printf("error in cases 1-4\n");
				exit(1);
			}

			loopCount++;
			bDestWhere+=remove_in_src_block_size;

			// ??? sanity check
			if(remove_in_src_block_size>moveSize)
			{
				printf("remove_in_src_block_size is greater than moveSize\n");
				exit(1);
			}

			moveSize-=remove_in_src_block_size;
		}

		pools[srcPoolId].size-=bCount;
		pools[destPoolId].size+=bCount;

		// move all the subsequent logicalStarts of the src pool downward
		offsetLogicalAddressSpace(SAT[srcPoolId].begin()+src_t,SAT[srcPoolId].end(),bCount,-1);


		// join blocks at first or least dest blocks delt with if possible
		joinAdjacentBlocks(destPoolId,destBlockIndex>0 ? destBlockIndex-1 : 0,1);
		if(loopCount>0 && SAT[destPoolId].size()>0)
			joinAdjacentBlocks(destPoolId,(destBlockIndex+loopCount-1)<SAT[destPoolId].size() ? destBlockIndex+loopCount-2 : SAT[destPoolId].size()-1,2);

		// join the blocks that just became adjacent in the src pool if possible
		joinAdjacentBlocks(srcPoolId,srcBlockIndex>0 ? srcBlockIndex-1 : 0,1);
	}

	backupSAT();
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::clearPool(const poolId_t poolId)
{
	if(!opened)
		throw runtime_error(string(__func__)+" -- no file is open");
	if(!isValidPoolId(poolId))
		throw runtime_error(string(__func__)+" -- invalid poolId: "+istring(poolId));

	invalidateAllCachedBlocks(false,poolId);

	// free all physical blocks associated with this pool
	for(size_t t=0;t<SAT[poolId].size();t++)
		pasm.free(SAT[poolId][t].physicalStart);

	// remove all localBlocks in the SAT associated with this pool 
	SAT[poolId].clear();

	pools[poolId].size=0;

	pasm.make_file_smallest();

	backupSAT();
}


// Pool Data Access
template<class l_addr_t,class p_addr_t>
	template<class pool_element_t> void TPoolFile<l_addr_t,p_addr_t>::cacheBlock(const l_addr_t peWhere,const TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > *accesser)
{
	CMutexLocker lock(accesserInfoMutex);

		// assume is valid
	const poolId_t poolId=accesser->poolId;

	if(peWhere>=maxLogicalAddress/sizeof(pool_element_t))
		throw runtime_error(string(__func__)+" -- invalid peWhere "+istring(peWhere)+" for pool ("+getPoolDescription(poolId)+")");

	const l_addr_t byteWhere=peWhere*sizeof(pool_element_t);

	unreferenceCachedBlock(accesser);

	RCachedBlock *found=NULL;

	// look to see if this block is already cached in the active cached blocks
	for(typename set<RCachedBlock *>::iterator i=activeCachedBlocks.begin();i!=activeCachedBlocks.end();i++)
	{
		RCachedBlock *cachedBlock=(*i);
		if(cachedBlock->poolId==poolId && cachedBlock->containsAddress(byteWhere))
		{
			found=cachedBlock;
			break;
		}
	}

	// if not found then look to see if this block is already cached in the unreferenced cached blocks
	if(found==NULL)
	{
		for(typename set<RCachedBlock *>::iterator i=unreferencedCachedBlocks.begin();i!=unreferencedCachedBlocks.end();i++)
		{
			RCachedBlock *cachedBlock=(*i);
			if(cachedBlock->poolId==poolId && cachedBlock->containsAddress(byteWhere))
			{
				found=cachedBlock;
				unreferencedCachedBlocks.erase(i);
				break;
			}
		}

		if(found==NULL)
		{	// use an unused block if available or take the LRUed unreferenced cached block if available or finally create a new one

			// validate address
			if(byteWhere>=getPoolSize(poolId))
			{
				accesser->init();
				throw runtime_error(string(__func__)+" -- invalid peWhere "+istring(peWhere)+" for pool ("+getPoolDescription(poolId)+")");
			}

			bool dummy;
			const size_t SATIndex=findSATBlockContaining(poolId,byteWhere,dummy);

			// use an unused one if available
			if(!unusedCachedBlocks.empty())
			{
				found=unusedCachedBlocks.front();
				unusedCachedBlocks.pop_front();
			}

			if(found==NULL)
			{	// invalidate an unreferenced one or create a new one if none are available
				if(!unreferencedCachedBlocks.empty())
				{	// create an unused one from the unreferencedCachedBlocks
					invalidateCachedBlock(*unreferencedCachedBlocks.begin());
					// ??? sanity check
					if(unusedCachedBlocks.empty())
					{
						printf("what? one's supposed to be there now...\n");
						exit(0);
					}
					found=unusedCachedBlocks.front();
					unusedCachedBlocks.pop_front();
				}
				else
					found=new RCachedBlock(maxBlockSize);
			}

			// could check of found==NULL here... error if so

			blockFile.read(found->buffer,SAT[poolId][SATIndex].size,SAT[poolId][SATIndex].physicalStart+LEADING_DATA_SIZE);

			// initialize the cachedBlock
			found->init(poolId,SAT[poolId][SATIndex].logicalStart,SAT[poolId][SATIndex].size);
		}
		activeCachedBlocks.insert(found);
	}

	found->referenceCount++;
	accesser->startAddress=(found->logicalStart)/sizeof(pool_element_t);
	accesser->endAddress=((found->logicalStart+found->size)/sizeof(pool_element_t))-1;
	accesser->cacheBuffer=(pool_element_t *)(found->buffer);
	accesser->dirty=false;
	accesser->cachedBlock=found;
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::clearPool(const string poolName)
{
	clearPool(getPoolIdByName(poolName));
}

template<class l_addr_t,class p_addr_t>
	template<class pool_element_t> void TPoolFile<l_addr_t,p_addr_t>::invalidateAccesser(const TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > *accesser)
{
	RCachedBlock *cachedBlock=accesser->cachedBlock;
	if(cachedBlock!=NULL)
		unreferenceCachedBlock(accesser);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::invalidateCachedBlock(RCachedBlock *cachedBlock)
{
	for(size_t t=0;cachedBlock->referenceCount>0 && t<accessers.size();t++)
	{
		if(accessers[t]->cachedBlock==cachedBlock)
			unreferenceCachedBlock(accessers[t]);
	}

	if(cachedBlock->dirty)
	{
		bool atStartOfBlock;
		size_t SATIndex=findSATBlockContaining(cachedBlock->poolId,cachedBlock->logicalStart,atStartOfBlock);
		// if atStartOfBlock is not true.. problem!!!
		blockFile.write(cachedBlock->buffer,SAT[cachedBlock->poolId][SATIndex].size,SAT[cachedBlock->poolId][SATIndex].physicalStart+LEADING_DATA_SIZE);
	}

	// the cached block structure is now unreferenced and unused
	typename set<RCachedBlock *>::iterator i=unreferencedCachedBlocks.find(cachedBlock);
	if(i!=unreferencedCachedBlocks.end())
	{
		unreferencedCachedBlocks.erase(i);
		unusedCachedBlocks.push_back(cachedBlock);
	}
	else
	{
		typename set<RCachedBlock *>::iterator i=activeCachedBlocks.find(cachedBlock);
		if(i!=activeCachedBlocks.end())
		{
			activeCachedBlocks.erase(i);
			printf("I think a mem-leak just occurred!\n");
			if(find(unusedCachedBlocks.begin(),unusedCachedBlocks.end(),cachedBlock)==unusedCachedBlocks.end()
			&& find(unreferencedCachedBlocks.begin(),unreferencedCachedBlocks.end(),cachedBlock)==unreferencedCachedBlocks.end())
				printf("  yep..\n");
			else
				printf("  nope..\n");
		}
	}
}

template<class l_addr_t,class p_addr_t>
	template<class pool_element_t> void TPoolFile<l_addr_t,p_addr_t>::unreferenceCachedBlock(const TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > *accesser)
{
	RCachedBlock *cachedBlock=accesser->cachedBlock;
	if(cachedBlock!=NULL)
	{
		cachedBlock->dirty |= accesser->dirty;
		cachedBlock->referenceCount--;
		if(cachedBlock->referenceCount==0)
		{	// move cachedBlock from activeQueue to unreferencedQueue
			typename set<RCachedBlock *>::iterator i=activeCachedBlocks.find(cachedBlock);
			if(i!=activeCachedBlocks.end())
				activeCachedBlocks.erase(i);
			unreferencedCachedBlocks.insert(cachedBlock);
		}
		accesser->init();
	}
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::invalidateAllCachedBlocks(bool allPools,poolId_t poolId)
{
	for(typename set<RCachedBlock *>::iterator i=activeCachedBlocks.begin();i!=activeCachedBlocks.end();)
	{
		typename set<RCachedBlock *>::iterator ii=i;
		i++;
		if(allPools || (*ii)->poolId==poolId)
			invalidateCachedBlock(*ii);
	}
	for(typename set<RCachedBlock *>::iterator i=unreferencedCachedBlocks.begin();i!=unreferencedCachedBlocks.end();)
	{
		typename set<RCachedBlock *>::iterator ii=i;
		i++;
		if(allPools || (*ii)->poolId==poolId)
			invalidateCachedBlock(*ii);
	}
}

template<class l_addr_t,class p_addr_t>
	template<class pool_element_t> void TPoolFile<l_addr_t,p_addr_t>::addAccesser(const TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > *accesser)
{
	CMutexLocker lock(accesserInfoMutex);
	accessers.push_back((const CGenericPoolAccesser *)accesser);
		// ??? I used to make sure that it wasn't already there
}

template<class l_addr_t,class p_addr_t>
	template<class pool_element_t> void TPoolFile<l_addr_t,p_addr_t>::removeAccesser(const TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > *accesser)
{
	CMutexLocker lock(accesserInfoMutex);
	invalidateAccesser(accesser);

	//						??? see about changing this to lower_bound... figure out just if lower_bound-1 or upper_bound-1 should be used
	typename vector<const CGenericPoolAccesser *>::iterator i=find(accessers.begin(),accessers.end(),(const CGenericPoolAccesser *)accesser);
	if(i!=accessers.end())
		accessers.erase(i);
}





// ---- RPoolInfo ---------------------------------------------------------
template<class l_addr_t,class p_addr_t>
	TPoolFile<l_addr_t,p_addr_t>::RPoolInfo::RPoolInfo()
{
	size=0;
	alignment=0;
	isValid=false;
}

template<class l_addr_t,class p_addr_t>
	TPoolFile<l_addr_t,p_addr_t>::RPoolInfo::RPoolInfo(const l_addr_t _size,const alignment_t _alignment,const bool _isValid)
{
	size=_size;
	alignment=_alignment;
	isValid=_isValid;
}

template<class l_addr_t,class p_addr_t>
	TPoolFile<l_addr_t,p_addr_t>::RPoolInfo::RPoolInfo(const RPoolInfo &src)
{
	operator=(src);
}

template<class l_addr_t,class p_addr_t>
	typename TPoolFile<l_addr_t,p_addr_t>::RPoolInfo &TPoolFile<l_addr_t,p_addr_t>::RPoolInfo::operator=(const RPoolInfo &src)
{
	size=src.size;
	alignment=src.alignment;
	isValid=src.alignment;
	return *this;
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::RPoolInfo::writeToFile(CMultiFile *f,CMultiFile::RHandle &multiFileHandle) /*const  makes typeof create const types */
{
	typeof(size) tSize=size;
	typeof(alignment) tAlignment=alignment;

	hetle(&tSize);
	f->write(&tSize,sizeof(size),multiFileHandle);
	hetle(&tAlignment);
	f->write(&tAlignment,sizeof(alignment),multiFileHandle);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::RPoolInfo::readFromFile(CMultiFile *f,CMultiFile::RHandle &multiFileHandle)
{
	f->read(&size,sizeof(size),multiFileHandle);
	lethe(&size);
	f->read(&alignment,sizeof(alignment),multiFileHandle);
	lethe(&alignment);
}




// ---- RCachedBlock ---------------------------------------------------
template<class l_addr_t,class p_addr_t>
	TPoolFile<l_addr_t,p_addr_t>::RCachedBlock::RCachedBlock(const blocksize_t maxBlockSize)
{
	if((buffer=malloc(maxBlockSize))==NULL)
		throw runtime_error(string(__func__)+" -- unable to allocate buffer space");
}

template<class l_addr_t,class p_addr_t>
	TPoolFile<l_addr_t,p_addr_t>::RCachedBlock::~RCachedBlock()
{
	free(buffer);
}

template<class l_addr_t,class p_addr_t>
	bool TPoolFile<l_addr_t,p_addr_t>::RCachedBlock::containsAddress(l_addr_t where) const
{
	return where>=logicalStart && where<(logicalStart+size);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::RCachedBlock::init(const poolId_t _poolId,const l_addr_t _logicalStart,const blocksize_t _size)
{
	poolId=_poolId;
	logicalStart=_logicalStart;
	size=_size;

	dirty=false;
	referenceCount=0;
}




// ---- RLogicalBlock ---------------------------------------------------------

template<class l_addr_t,class p_addr_t>
	TPoolFile<l_addr_t,p_addr_t>::RLogicalBlock::RLogicalBlock()
{
	logicalStart=size=physicalStart=0;
}

template<class l_addr_t,class p_addr_t>
	TPoolFile<l_addr_t,p_addr_t>::RLogicalBlock::RLogicalBlock(const l_addr_t _logicalStart)
{
	size=physicalStart=0;
	logicalStart=_logicalStart;
}

template<class l_addr_t,class p_addr_t>
	TPoolFile<l_addr_t,p_addr_t>::RLogicalBlock::RLogicalBlock(const RLogicalBlock &src)
{
	operator=(src);
}

template<class l_addr_t,class p_addr_t>
	typename TPoolFile<l_addr_t,p_addr_t>::RLogicalBlock &TPoolFile<l_addr_t,p_addr_t>::RLogicalBlock::operator=(const RLogicalBlock &src)
{
	logicalStart=src.logicalStart;
	size=src.size;
	physicalStart=src.physicalStart;
	return *this;
}

template<class l_addr_t,class p_addr_t>
	const bool TPoolFile<l_addr_t,p_addr_t>::RLogicalBlock::operator==(const RLogicalBlock &src) const
{
	if(logicalStart==src.logicalStart)
		return true;
	return false;
}

template<class l_addr_t,class p_addr_t>
	const bool TPoolFile<l_addr_t,p_addr_t>::RLogicalBlock::operator<(const RLogicalBlock &src) const
{
	if(src.logicalStart>logicalStart)
		return true;
	return false;
}

template<class l_addr_t,class p_addr_t>
	const size_t TPoolFile<l_addr_t,p_addr_t>::RLogicalBlock::getMemSize()
{
	return sizeof(logicalStart)+sizeof(size)+sizeof(physicalStart);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::RLogicalBlock::writeToMem(uint8_t *mem,size_t &offset) /*const  makes typeof cause const types */
{
	memcpy(mem+offset,&logicalStart,sizeof(logicalStart));
	hetle((typeof(logicalStart) *)(mem+offset));
	offset+=sizeof(logicalStart);

	memcpy(mem+offset,&size,sizeof(size));
	hetle((typeof(size) *)(mem+offset));
	offset+=sizeof(size);
	
	memcpy(mem+offset,&physicalStart,sizeof(physicalStart));
	hetle((typeof(physicalStart) *)(mem+offset));
	offset+=sizeof(physicalStart);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::RLogicalBlock::readFromMem(const uint8_t *mem,size_t &offset)
{
	memcpy(&logicalStart,mem+offset,sizeof(logicalStart));
	lethe(&logicalStart);
	offset+=sizeof(logicalStart);

	memcpy(&size,mem+offset,sizeof(size));
	lethe(&size);
	offset+=sizeof(size);

	memcpy(&physicalStart,mem+offset,sizeof(physicalStart));
	lethe(&physicalStart);
	offset+=sizeof(physicalStart);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::RLogicalBlock::print() const
{
	printf("logicalStart: %-10lld size: %-5lld physicalStart: %-10lld\n",(long long)logicalStart,(long long)size,(long long)physicalStart);
}



// --- physical address space management ------------------------


template<class l_addr_t,class p_addr_t>
	TPoolFile<l_addr_t,p_addr_t>::CPhysicalAddressSpaceManager::CPhysicalAddressSpaceManager(CMultiFile &_file) :
	file(_file)
{
}

template<class l_addr_t,class p_addr_t>
	p_addr_t TPoolFile<l_addr_t,p_addr_t>::CPhysicalAddressSpaceManager::appendAlloc(const p_addr_t size)
{
	const p_addr_t addr=get_file_size();
	set_file_size(addr+size);
	alloced.insert(alloced.end(),make_pair(addr,size));

	lastAllocAppended=true;
	lastAllocSize=size;

	return addr;
}

template<class l_addr_t,class p_addr_t>
	p_addr_t TPoolFile<l_addr_t,p_addr_t>::CPhysicalAddressSpaceManager::alloc(blocksize_t size)
{
	if(size<=0)
		return 0;

	if(holes.empty())
	{ // need to make more space
dprintf("alloc: case 1\n");
		return appendAlloc(size);
	}
	else
	{
		if(lastAllocAppended && size>=lastAllocSize) /* optimization of repeated consecutive appends */
		{ // need to make more space
dprintf("alloc: case 1.5\n");
			return appendAlloc(size);
		}
		else
		{
			lastAllocAppended=false;

			// check the largest hole
			typename holeSizeIndex_t::iterator i=holeSizeIndex.end();
			i--;
		
			if(i->first>size)
			{ // hole more than big enough
dprintf("alloc: case 2\n");
				const p_addr_t addr=i->second->first;
				alloced[addr]=size;

				const p_addr_t newHoleSize=i->first-size;
				const p_addr_t newHoleStart=i->second->first+size;

				// update holes and holeSizeIndex
				holes.erase(i->second);
				holeSizeIndex.erase(i);
				
				typename holes_t::iterator hole_i=holes.insert(make_pair(newHoleStart,newHoleSize)).first;
				holeSizeIndex.insert(make_pair(newHoleSize,hole_i));
		
				return addr;
			}
			else if(i->first==size)
			{ // hole is the exact size we need
dprintf("alloc: case 3\n");
				const p_addr_t addr=i->second->first;
				alloced[i->second->first]=size;

				holes.erase(i->second);
				holeSizeIndex.erase(i);

				return addr;
			}
			else
			{ // need to make more space
dprintf("alloc: case 4\n");
				return appendAlloc(size);
			}
		}
	}
}

template<class l_addr_t,class p_addr_t>
	p_addr_t TPoolFile<l_addr_t,p_addr_t>::CPhysicalAddressSpaceManager::split_block(p_addr_t addr,blocksize_t newBlockStartsAt)
{
dprintf("split_block: case 1\n");
	typename alloced_t::iterator i=alloced.find(addr);
	if(i==alloced.end())
		throw runtime_error(string(__func__)+" -- addr is not an alloced block: "+istring(addr));

	if(newBlockStartsAt<=0)
		throw runtime_error(string(__func__)+" -- parameters don't cause a split, newBlockStartsAt is <= zero");

	if(newBlockStartsAt>=i->second)
		throw runtime_error(string(__func__)+" -- newBlockStartsAt is out of range: "+istring(i->second)+" < "+istring(newBlockStartsAt));

	// create new block
	const p_addr_t newAddr=addr+newBlockStartsAt;
	alloced.insert(i,make_pair(newAddr,i->second-newBlockStartsAt));

	// update existing block to be smaller
	i->second=newBlockStartsAt;

	return newAddr;
}

template<class l_addr_t,class p_addr_t>
	p_addr_t TPoolFile<l_addr_t,p_addr_t>::CPhysicalAddressSpaceManager::partial_free(p_addr_t addr,p_addr_t newAddr,blocksize_t newSize)
{
	typename alloced_t::iterator i=alloced.find(addr);
	if(i==alloced.end())
		throw runtime_error(string(__func__)+" -- addr is not an alloced block: "+istring(addr));

	if(newSize<=0)
		throw runtime_error(string(__func__)+" -- newSize is <= zero");

		// ??? maybe cases like this don't matter .. maybe only do if testing
	if(newAddr==addr && newSize==i->second)
		throw runtime_error(string(__func__)+" -- partial free does nothing");

	if(newAddr+newSize>addr+i->second)
		throw runtime_error(string(__func__)+" -- newAddr+newSize would extend beyond the originally alloced block: "+istring(newAddr)+"+"+istring(newSize)+" > "+istring(addr)+"+"+istring(i->second));


	lastAllocAppended=false; // a new hole may be about to open up so don't do this optimization next time

	if(newAddr>addr)
	{ // freeing what's left of newAddr

		// either create a new hole left of addr or join with an existing one
		const p_addr_t newHoleSize=newAddr-addr;
		if(i!=alloced.begin())
		{
			// find the hole left of i
			typename alloced_t::iterator prev_i=i; prev_i--;
			typename holes_t::iterator holes_i=holes.find(prev_i->first+prev_i->second);
			if(holes_i==holes.end())
			{ // (case 1) no hole to join with, so create one
dprintf("partial_free: case 1\n");
				holes_i=holes.insert(make_pair(addr,newHoleSize)).first;
				holeSizeIndex.insert(make_pair(newHoleSize,holes_i));
			}
			else
			{ // (case 2) hole found to join with
dprintf("partial_free: case 2\n");
				holeSizeIndex.erase(findHoleSizeIndexEntry(holes_i->second,holes_i->first));
				holes_i->second+=newHoleSize;
				holeSizeIndex.insert(make_pair(holes_i->second,holes_i));
			}

		}
		else
		{ // i is the first alloced block, so check the left most hole or create a new left-most hole

			typename holes_t::iterator holes_i=holes.begin();
			if(!holes.empty() && holes_i->first<addr)
			{ // (case 3) join with this hole
dprintf("partial_free: case 3\n");
				// remove the holeSizeIndex entry for the hole at addr 0
				typename holeSizeIndex_t::iterator holeSizeIndex_i=findHoleSizeIndexEntry(holes_i->second,0);
				holeSizeIndex.erase(holeSizeIndex_i);

				// update the hole at addr 0
				holes_i->second+=newHoleSize;

				// create new holeSizeIndex entry for hole at addr 0
				holeSizeIndex.insert(make_pair(holes_i->second,holes_i));

			}
			else
			{ // (case 4) create a new left-most hole
dprintf("partial_free: case 4\n");
				typename holes_t::iterator holes_i=holes.insert(holes.begin(),make_pair(addr,newHoleSize));
				holeSizeIndex.insert(make_pair(newHoleSize,holes_i));
			}
		}
	}

	if(newAddr+newSize<addr+i->second)
	{ // freeing something to the right of newAddr+newSize
		// either create a new hole to the right of newAddr+newSize or join with an existing after addr+i->second
		const p_addr_t newHoleAddr=newAddr+newSize;
		const p_addr_t newHoleSize=(addr+i->second)-newHoleAddr;

		typename holes_t::iterator holes_i=holes.find(addr+i->second);
		if(holes_i==holes.end())
		{ // (case 5) no hole found right of original allocated block, so create one
dprintf("partial_free: case 5\n");
			holes_i=holes.insert(make_pair(newHoleAddr,newHoleSize)).first;
			holeSizeIndex.insert(make_pair(newHoleSize,holes_i));
		}
		else
		{ // (case 6) hole found right of original allocated block, so join with it
dprintf("partial_free: case 6\n");
			const p_addr_t joinHoleAddr=holes_i->first;
			const p_addr_t joinHoleSize=holes_i->second;

			typename holes_t::iterator holes_i2=holes.insert(holes_i,make_pair(newHoleAddr,joinHoleSize+newHoleSize));
			holeSizeIndex.insert(make_pair(joinHoleSize+newHoleSize,holes_i2));

			holes.erase(holes_i);
			holeSizeIndex.erase(findHoleSizeIndexEntry(joinHoleSize,joinHoleAddr));
		}
	}

	// update alloced
	alloced.erase(i);
	alloced.insert(make_pair(newAddr,newSize));

	return newAddr;
}

template<class l_addr_t,class p_addr_t>
	typename TPoolFile<l_addr_t,p_addr_t>::CPhysicalAddressSpaceManager::holeSizeIndex_t::iterator TPoolFile<l_addr_t,p_addr_t>::CPhysicalAddressSpaceManager::findHoleSizeIndexEntry(const p_addr_t size,const p_addr_t addr)
{
	typename holeSizeIndex_t::iterator holeSizeIndex_i=holeSizeIndex.find(size);
	while(holeSizeIndex_i!=holeSizeIndex.end())
	{
		if(holeSizeIndex_i->first!=size)
		{
			printf("WHAT! there is no holeSizeIndex entry for the hole we're looking for\n");
			break;
		}

		if(holeSizeIndex_i->second->first==addr)
		{ // found it
			return holeSizeIndex_i;
		}

		holeSizeIndex_i++;
	}

	throw runtime_error(string(__func__)+" -- no holeSizeIndex entry found");
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::CPhysicalAddressSpaceManager::free(p_addr_t addr)
{
	typename alloced_t::iterator alloced_i=alloced.find(addr);
	if(alloced_i==alloced.end())
		throw runtime_error(string(__func__)+" -- attempting to free something that was allocated");

	lastAllocAppended=false; // a new hole may be about to open up so don't do this optimization next time

	// attempt to find holes on either side of block being freed and join block's space with either one or both holes
	
	// find hole on left (set to holes.end() if there is no hole on the left)
	typename holes_t::iterator leftwardHole;
	if(alloced_i->first==alloced.begin()->first)
	{ // addr is the first alloced block, so check if there is a hole left of the alloced block
		leftwardHole=holes.begin();
		if(leftwardHole->first!=0) 
			// first hole is after the first alloced block
			leftwardHole=holes.end();
	}
	else
	{
		typename alloced_t::iterator prev_i=alloced_i; prev_i--;
		leftwardHole=holes.find(prev_i->first+prev_i->second);
	}

	// find hole on right (set to holes.end() if there is no hole on the right)
	typename holes_t::iterator rightwardHole=holes.find(alloced_i->first+alloced_i->second);

	if(leftwardHole==holes.end())
	{ // no hole on the left
		if(rightwardHole==holes.end())
		{ // (case 1) no hole on the right either, so just make alloced the first hole
dprintf("free: case 1\n");
			typename holes_t::iterator hole_i=holes.insert(make_pair(alloced_i->first,alloced_i->second)).first;
			holeSizeIndex.insert(make_pair(alloced_i->second,hole_i));
		}
		else
		{ // (case 2) there is a hole only on the right, so join the free block with it
dprintf("free: case 2\n");
			// remove holeSizeIndex entry for rightward hole we just removed
			typename holeSizeIndex_t::iterator holeSizeIndex_i=findHoleSizeIndexEntry(rightwardHole->second,rightwardHole->first);
			holeSizeIndex.erase(holeSizeIndex_i);

			const p_addr_t newHoleSize=alloced_i->second+rightwardHole->second;

			typename holes_t::iterator hole_i=holes.insert(make_pair(alloced_i->first,newHoleSize)).first;
			holeSizeIndex.insert(make_pair(newHoleSize,hole_i));

			// remove rightward hole because we've created a new hole that accounts for its space
			holes.erase(rightwardHole);
		}
	}
	else
	{ // there is a hole on the left
		if(rightwardHole==holes.end())
		{ // (case 3) no hole on the right, so just join the freed block with hole on the left
dprintf("free: case 3\n");
			const p_addr_t oldHoleSize=leftwardHole->second;

			// update the size of the left hole
			leftwardHole->second+=alloced_i->second;

			// remove holeSizeIndex entry for leftward hole we just updated and re-add it with new size
			typename holeSizeIndex_t::iterator holeSizeIndex_i=findHoleSizeIndexEntry(oldHoleSize,leftwardHole->first);
			holeSizeIndex.erase(holeSizeIndex_i);
			holeSizeIndex.insert(make_pair(leftwardHole->second,leftwardHole));
		}
		else
		{ // (case 4) there is a hole on both sides of the alloced block, so join the freed block and the hole on the right with the hole on the left
dprintf("free: case 4\n");
			const p_addr_t oldHoleSize=leftwardHole->second;

			// update the size of the left hole
			leftwardHole->second+=alloced_i->second+rightwardHole->second;

			// remove holeSizeIndex entry for leftward hole we just updated and re-add it with the new size
			typename holeSizeIndex_t::iterator holeSizeIndex_i=findHoleSizeIndexEntry(oldHoleSize,leftwardHole->first);
			holeSizeIndex.erase(holeSizeIndex_i);
			holeSizeIndex.insert(make_pair(leftwardHole->second,leftwardHole));

			// remove the holeSizeIndex entry for rightward hole we just updated
			holeSizeIndex_i=findHoleSizeIndexEntry(rightwardHole->second,rightwardHole->first);
			holeSizeIndex.erase(holeSizeIndex_i);

			// remove the rightward hole
			holes.erase(rightwardHole);
		}
	}

	// remove block from the alloced list 
	alloced.erase(alloced_i);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::CPhysicalAddressSpaceManager::free_all()
{
	lastAllocAppended=false;
	alloced.clear();
	holes.clear();
	holeSizeIndex.clear();

	if(file.isOpen())
		holeSizeIndex.insert(make_pair(get_file_size(),holes.insert(make_pair(0,get_file_size())).first));
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::CPhysicalAddressSpaceManager::join_blocks(p_addr_t addr1,p_addr_t addr2)
{
	typename alloced_t::iterator i1=alloced.find(addr1);
	typename alloced_t::iterator i2=alloced.find(addr2);

	if(i1==alloced.end())
		throw runtime_error(string(__func__)+" -- addr1 is not allocated");
	if(i2==alloced.end())
		throw runtime_error(string(__func__)+" -- addr2 is not allocated");

	if((i1->first+i1->second)==i2->first)
	{ // join blocks
		i1->second+=i2->second;
		alloced.erase(i2);
	}
	else
		throw runtime_error(string(__func__)+" -- addr1 and addr2 are not adjacent");
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::CPhysicalAddressSpaceManager::buildFromSAT(const vector<vector<RLogicalBlock> > &SAT)
{
	lastAllocAppended=false;
	alloced.clear();
	holes.clear();
	holeSizeIndex.clear();

	// create list of just alloced stuff
	for(size_t x=0;x<SAT.size();x++)
	{
		for(size_t y=0;y<SAT[x].size();y++)
		{
			alloced.insert(make_pair(SAT[x][y].physicalStart,SAT[x][y].size));
		}
	}

	if(!alloced.empty())
	{

		// if first alloced block doesn't start at 0, then create a hole that accounts for this
		if(alloced.begin()->first!=0)
		{
			holeSizeIndex.insert(make_pair(alloced.begin()->first,holes.insert(make_pair(0,alloced.begin()->first)).first));
		}
	
		// create holes that are between any alloced blocks
		for(typename alloced_t::const_iterator i=alloced.begin();i!=alloced.end();i++)
		{
			typename alloced_t::const_iterator next_i=i; next_i++;
			if(next_i!=alloced.end())
			{
				const p_addr_t right_addr=i->first+i->second;
				if(right_addr!=next_i->first)
				{
					const p_addr_t holeSize=right_addr-next_i->first;
					holeSizeIndex.insert(make_pair(holeSize,holes.insert(make_pair(right_addr,holeSize)).first));
				}
			}
		}

		// create hole after last allocated block if necessary
		{
			const p_addr_t last_addr=alloced.rbegin()->first+alloced.rbegin()->second;
			const p_addr_t last_hole_size=get_file_size()-last_addr;
			if(last_addr<get_file_size())
				holeSizeIndex.insert(make_pair(last_hole_size,holes.insert(make_pair(last_addr,last_hole_size)).first));
		}
	}
	else
	{
		// create hole to account for all space if necessary
		if(get_file_size()>0)
			holeSizeIndex.insert(make_pair(get_file_size(),holes.insert(make_pair(0,get_file_size())).first));
	}


}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::CPhysicalAddressSpaceManager::make_file_smallest()
{
	lastAllocAppended=false;
	if(alloced.empty())
	{
		set_file_size(0);
		holes.clear();
		holeSizeIndex.clear();
	}
	else
	{
		const p_addr_t lastAddr=alloced.rbegin()->first+alloced.rbegin()->second;
		set_file_size(lastAddr);

		// update holes if necessary (by removing the last hole that was at lastAddr)
		typename holes_t::iterator i=holes.find(lastAddr);
		if(i!=holes.end())
		{
			holeSizeIndex.erase(findHoleSizeIndexEntry(i->second,i->first));
			holes.erase(i);
		}
	}
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::CPhysicalAddressSpaceManager::set_file_size(const p_addr_t newSize)
{
	file.setSize(newSize+LEADING_DATA_SIZE);
}

template<class l_addr_t,class p_addr_t>
	const p_addr_t TPoolFile<l_addr_t,p_addr_t>::CPhysicalAddressSpaceManager::get_file_size() const
{
	return max(file.getSize(),(CMultiFile::l_addr_t)LEADING_DATA_SIZE)-LEADING_DATA_SIZE;
}


template<class l_addr_t,class p_addr_t>
	bool TPoolFile<l_addr_t,p_addr_t>::CPhysicalAddressSpaceManager::overlap(p_addr_t addr1,p_addr_t size1,p_addr_t addr2,p_addr_t size2)
{
	if(addr1<addr2 && (addr1+size1)<=addr2)
		return false;
	if(addr2<addr1 && (addr2+size2)<=addr1)
		return false;
	return true;
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::CPhysicalAddressSpaceManager::verify(bool expectContinuousPhysicalAllocs)
{
	// make sure no alloced blocks overlap
	for(typename alloced_t::const_iterator alloced_i=alloced.begin();alloced_i!=alloced.end();alloced_i++)
	{
		typename alloced_t::const_iterator next_alloced_i=alloced_i; next_alloced_i++;
		if(next_alloced_i!=alloced.end())
		{
			if(alloced_i->first+alloced_i->second>next_alloced_i->first)
				printf("*** FAILURE alloced blocks overlap: %lld+%lld > %lld\n",(long long)alloced_i->first,(long long)alloced_i->second,(long long)next_alloced_i->first);
		}
	}
	
	// make sure no holes overlap or are consecutive(needing joining)
	for(typename holes_t::const_iterator holes_i=holes.begin();holes_i!=holes.end();holes_i++)
	{
		typename holes_t::const_iterator next_holes_i=holes_i; next_holes_i++;
		if(next_holes_i!=holes.end())
		{
			if(holes_i->first+holes_i->second>=next_holes_i->first)
				printf("*** FAILURE holes overlap or are consecutive: %lld+%lld >= %lld\n",(long long)holes_i->first,(long long)holes_i->second,(long long)next_holes_i->first);
		}
	}
	

	// make sure no alloced block overlaps with a holes
	for(typename alloced_t::const_iterator alloced_i=alloced.begin();alloced_i!=alloced.end();alloced_i++)
	{
		for(typename holes_t::const_iterator holes_i=holes.begin();holes_i!=holes.end();holes_i++)
		{
			if(overlap(holes_i->first,holes_i->second,alloced_i->first,alloced_i->second))
				printf("*** FAILURE overlapping hole and alloced:\n\talloced: %lld %lld\t\n\thole:  %lld %lld\n",(long long)alloced_i->first,(long long)alloced_i->second,(long long)holes_i->first,(long long)holes_i->second);
		}
	}

	// make sure that holes + alloced == file size
	p_addr_t total=0;
	for(typename alloced_t::const_iterator alloced_i=alloced.begin();alloced_i!=alloced.end();alloced_i++)
		total+=alloced_i->second;
	for(typename holes_t::const_iterator holes_i=holes.begin();holes_i!=holes.end();holes_i++)
		total+=holes_i->second;
	if(total>get_file_size())
		printf("*** FAILURE alloced + holes > file size :   %lld>%lld\n",(long long)total,(long long)get_file_size());

	// validate that holeSizeIndex is correct by building one from scratch and comparing it to the one we have
	holeSizeIndex_t temp;
	for(typename holes_t::iterator holes_i=holes.begin();holes_i!=holes.end();holes_i++)
		temp.insert(make_pair(holes_i->second,holes_i));

	if(temp.size()!=holeSizeIndex.size())
		printf("*** FAILURE holeSizeIndex is out of sync (size differs)\n");

		// for each element in temp, look for it in holeSizeIndex
	for(typename holeSizeIndex_t::iterator i=temp.begin();i!=temp.end();i++)
	{
		try
		{
			findHoleSizeIndexEntry(i->first,i->second->first);
		}
		catch(...)
		{
			printf("*** FAILURE holeSizeIndex is out of sync\n");
			break;
		}
	}

	// verify that all the physical blocks are contiguous (not an invalid pool file if it happens, but the user may expect it)
	if(expectContinuousPhysicalAllocs)
	{
		p_addr_t expectedStart=0;
		for(typename alloced_t::iterator i=alloced.begin();i!=alloced.end();i++)
		{
			if(i->first!=expectedStart)
			{
				printf("*** FAILURE physical allocated space is not continuous\n");
				break;
			}
			expectedStart+=i->second;
		}
	}

	printf("PASSED\n");
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::CPhysicalAddressSpaceManager::print() const
{
	printf("\nAllocated Physical Blocks:\n");
	for(typename alloced_t::const_iterator t=alloced.begin();t!=alloced.end();t++)
		printf("physicalStart: %-10lld size: %-5lld\n",(long long)t->first,(long long)t->second);

	printf("\nUnallocated Physical Blocks:\n");
	for(typename holes_t::const_iterator t=holes.begin();t!=holes.end();t++)
		printf("physicalStart: %-10lld size: %-5lld\n",(long long)t->first,(long long)t->second);

	printf("\nIndex into Unallocated Physical Blocks:\n");
	for(typename holeSizeIndex_t::const_iterator t=holeSizeIndex.begin();t!=holeSizeIndex.end();t++)
		printf("size: %-10lld addr: %-5lld\n",(long long)t->first,(long long)t->second->first);

	printf("\n\n");
}




// --- util methods ----------------

template<class l_addr_t,class p_addr_t>
	template<class C,class I> void TPoolFile<l_addr_t,p_addr_t>::sortedInsert(C &c,const I &i)
{
		// ??? need an overloaded version which doesn't always start from the beginning
	c.insert(lower_bound(c.begin(),c.end(),i),i);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::readString(string &s,CMultiFile *f,CMultiFile::RHandle &multiFileHandle)
{
	uint32_t len;
	f->read(&len,sizeof(len),multiFileHandle);
	lethe(&len);
/* ??? if sizeof(char) != sizeof(int8_t) then we're in trouble */
	char buffer[1024];
	for(size_t t=0;t<len/1024;t++)
	{
		f->read(buffer,1024,multiFileHandle);
		s.append(buffer,1024);
	}
	if(len%1024)
	{
		f->read(buffer,len%1024,multiFileHandle);
		s.append(buffer,len%1024);
	}
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::writeString(const string &s,CMultiFile *f,CMultiFile::RHandle &multiFileHandle)
{
	const uint32_t len=s.length();
	uint32_t tLen=len;
	hetle(&tLen);
	f->write(&tLen,sizeof(len),multiFileHandle);
/* ??? if sizeof(char) != sizeof(int8_t) then we're in trouble */
	f->write(s.c_str(),len,multiFileHandle);
}


