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
#error This file must be included through TPoolFile.h NOT compiled with a project file or a makefile.
#endif

/* ??? 
 * something I keep thinking about is the fact that the logical space table contains duplicate information from the physical space table
 * perhaps i could combine the 2 into a single list
 * perhaps the SAT[0] would be a map to all the free space... but I need the data sorted by physicalStart too.. perhaps this is an index?
 * it may not buy me much afterall
 */

/* ???
 * There are places that I have to do: container.erase(container.begin+index);  
 * see if I can eliminate this
 */

/*
 * try to fix the problem that poolIds are not gaurenteed after removing a pool
 * 	- use an ever increasing value
 * 	- this value would be reset to 0 after the pool file has been opened and closed tho unless I saved that info too
 */

/*???
 * 	In many functions i have SAT[poolId][index].. I should probably reduce that to  vector<RLogicalBlock> &poolSAT=SAT[poolId];
 */

/* ??? I don't know what this checkCurrent thing was in blockFile's size... but I can probably remove it now unless it used to do something interesting
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

// ??? I didn't think of this until just now, but for fault tolerancy during a space modifications operation, I could simply
// restore to the backed up SAT just before the space modification method began if there was an error either in the logic or in
// extending the file's length

#ifndef __TPoolFile_CPP__
#define __TPoolFile_CPP__

#include <stdio.h> // for the printf errors... should probably use assert (but assumably temporary>

#include <stdexcept>
#include <utility>
#include <algorithm>
#include <iterator>

#include <istring>

#include "TPoolFile.h"
#include "TStaticPoolAccesser.h"
//#include "CStreamPoolAccesser.h"

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
	TPoolFile<l_addr_t,p_addr_t>::TPoolFile(const size_t _maxBlockSize,const char *_formatSignature) :

	formatSignature(_formatSignature),

	maxBlockSize(_maxBlockSize),
	maxLogicalAddress(~((l_addr_t)0)),
	maxPhysicalAddress(~((p_addr_t)0))
{
	if(maxBlockSize<2)
		throw(runtime_error(string(__func__)+" -- maxBlockSize is less than 2"));
	if(maxLogicalAddress>maxPhysicalAddress)
		throw(runtime_error(string(__func__)+" -- the logical address space is greater than the physical address space"));
	if(maxBlockSize>maxLogicalAddress)
		throw(runtime_error(string(__func__)+" -- the maxBlockSize is bigger than the logical address space"));

	init();
}

template<class l_addr_t,class p_addr_t>
	TPoolFile<l_addr_t,p_addr_t>::TPoolFile(const TPoolFile<l_addr_t,p_addr_t> &src)
{
	throw(runtime_error(string(__func__)+" -- copy constructor invalid"));
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
 		throw(runtime_error(string(__func__)+" -- file already opened"));

	//writeStructureInfoLock();
	try
	{
		init();

		blockFile.open(_filename,canCreate);
		filename=_filename;

        	setup();

        //writeStructureInfoUnlock();
	}
	catch(...)
	{
		//writeStructureInfoUnlock();
		blockFile.close(false);
		throw;
	}


}

/* LLL -- could implement this if I need it
template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::createTemp()
{
   	if(opened)
   		throw(runtime_error(string(__func__)+" -- file already opened"));

	//writeStructureInfoLock();

	fileHandle=-1;
	blockFile=NULL;
	try
	{
		init();

        if((blockFile=tmpfile())==NULL)
            throw(runtime_error(string(__func__)+" -- could not create temp file"));

        openedTemp=true;

        setup();

        //writeStructureInfoUnlock();
	}
	catch(...)
	{
		//writeStructureInfoUnlock();

		if(fileHandle!=-1)
			close(fileHandle);

		if(blockFile)
		{
			fclose(blockFile);
			blockFile=NULL;
		}
		throw;
	}
}
*/


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
			char buffer[8]={0};
			blockFile.read(buffer,8,SIGNATURE_OFFSET);
			if(strncmp(buffer,FORMAT_SIGNATURE,8))
				throw(runtime_error("invalid format signature"));

			// check format version
			uint32_t formatVersion;
			blockFile.read(&formatVersion,sizeof(formatVersion),FORMAT_VERSION_OFFSET);
			if(formatVersion!=FORMAT_VERSION)
				throw(runtime_error("invalid or unsupported format version: "+istring((int)formatVersion)));

			int8_t dirty;
			blockFile.read(&dirty,sizeof(dirty),DIRTY_INDICATOR_OFFSET);

			if(dirty)
			{	// file is marked as dirty
				blockFile.read(&whichSATFile,sizeof(whichSATFile),WHICH_SAT_FILE_OFFSET);

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
				if(metaDataOffset<LEADING_DATA_SIZE)
					throw(runtime_error("invalid meta data offset"));

				buildSATFromFile(&blockFile,metaDataOffset);
				joinAllAdjacentBlocks();
			}
		}
		else
			createContiguousSAT();

		SATFilename=filename+".SAT";

		writeMetaData(&blockFile);

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
		throw(runtime_error(string(__func__)+" -- "+string(e.what())));
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
	//readStructureInfoLock();
	try
	{
		if(!opened)
			throw(runtime_error(string(__func__)+" -- no file is open"));
		if(_filename==getFilename())
			throw(runtime_error(string(__func__)+" -- attempting to copy to the file which is open"));

		invalidateAllCachedBlocks();

		CMultiFile copyFile;
		copyFile.open(_filename,true);

		CMultiFile::RHandle multiFileHandle1;
		CMultiFile::RHandle multiFileHandle2;

		try
		{
			TAutoBuffer<int8_t> temp(maxBlockSize);
			//auto_ptr<int8_t> temp(new int8_t[maxBlockSize]);

			const p_addr_t length=blockFile.getSize();

			blockFile.seek(0,multiFileHandle1);
			copyFile.seek(0,multiFileHandle2);
		
			for(l_addr_t t=0;t<length/maxBlockSize;t++)
			{
				blockFile.read(temp,maxBlockSize,multiFileHandle1);
				copyFile.write(temp,maxBlockSize,multiFileHandle2);
			}
			if((length%maxBlockSize)!=0)
			{
				blockFile.read(temp,length%maxBlockSize,multiFileHandle1);
				copyFile.write(temp,length%maxBlockSize,multiFileHandle2);
			}

			writeMetaData(&copyFile);
			writeDirtyIndicator(false,&copyFile);
			copyFile.close(false);
		}
		catch(exception &e)
		{
			copyFile.close(true);
			throw(runtime_error(string(__func__)+" -- error copying to file -- "+e.what()));
		}

		//readStructureInfoUnlock();
	}
	catch(...)
	{
		//readStructureInfoUnlock();
		throw;
	}
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::closeFile(const bool _defrag,const bool _removeFile)
{
	if(!opened)
		return;

	//writeStructureInfoLock();
	try
	{
		// need to probably wait for any outstanding accessers to deallocate
		if(_removeFile)
		{
			closeSATFiles();
			blockFile.close(true);
		}
		else
		{
			invalidateAllCachedBlocks();

			if(_defrag)
				defrag();

			writeMetaData(&blockFile);
			writeDirtyIndicator(false,&blockFile);

			closeSATFiles();

			blockFile.sync();
			blockFile.close(false);
		}
		init();

		//writeStructureInfoUnlock();
	}
	catch(...)
	{
		//writeStructureInfoUnlock();
		throw;
	}
}

template<class l_addr_t,class p_addr_t>
	const bool TPoolFile<l_addr_t,p_addr_t>::isTemp() const
{
	return(openedTemp);
}

template<class l_addr_t,class p_addr_t>
	const bool TPoolFile<l_addr_t,p_addr_t>::isOpen() const
{
	return(opened);
}

template<class l_addr_t,class p_addr_t>
	const p_addr_t TPoolFile<l_addr_t,p_addr_t>::getFileSize() const
{
	return(blockFile.getActualSize());
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::rename(const string newFilename)
{
	if(!opened)
		throw(runtime_error(string(__func__)+" -- no file is open"));

	/*
	const string currentFilename=getFilename();

	closeFile(false,false);
	// ??? need to call this on blockFile to rename all the files in the multifile set
	rename(currentFilename.c_str(),filename.c_str());
	openFile(filename,false);
	*/
	blockFile.rename(newFilename);
	SATFiles[0].rename(newFilename+".SAT1");
	SATFiles[1].rename(newFilename+".SAT2");
}


template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::flushData()
{
	//writeStructureInfoLock();
	try
	{
		invalidateAllCachedBlocks();

		backupSAT();

		SATFiles[0].sync();
		SATFiles[1].sync();
		blockFile.sync();

		//writeStructureInfoUnlock();
	}
	catch(...)
	{
		//writeStructureInfoUnlock();
		throw;
	}
}

template<class l_addr_t,class p_addr_t>
	const string TPoolFile<l_addr_t,p_addr_t>::getFilename() const
{
	if(isTemp())
		throw(runtime_error(string(__func__)+"() -- file is opened as temp"));

	return(filename);
}

// Non-Const Pool Accesser Methods
template<class l_addr_t,class p_addr_t>
	template<class pool_element_t> TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > TPoolFile<l_addr_t,p_addr_t>::getPoolAccesser(const poolId_t poolId)
{
	//readStructureInfoLock();
	try
	{
		if(!opened)
			throw(runtime_error(string(__func__)+" -- no file is open"));
		if(poolId>=getPoolCount())
			throw(runtime_error(string(__func__)+" -- invalid poolId parameter: "+istring(poolId)));
		if(sizeof(pool_element_t)!=pools[poolId].alignment)
			throw(runtime_error(string(__func__)+" -- method instantiated with a type whose size does not match the pool's alignment: sizeof(pool_element_t): "+istring(sizeof(pool_element_t))+" -- "+getPoolDescription(poolId)));

		TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > accesser(this,poolId);
		//readStructureInfoUnlock();
		return(accesser);
	}
	catch(exception &e)
	{
		//readStructureInfoUnlock();
		throw;
	}
}

template<class l_addr_t,class p_addr_t>
	template<class pool_element_t> TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > TPoolFile<l_addr_t,p_addr_t>::getPoolAccesser(const string poolName)
{
	return(getPoolAccesser<pool_element_t>(getPoolIdByName(poolName)));
}

/*
CStreamPoolAccesser CPoolFile::getStreamPoolAccesser(const poolId_t poolId)
{
	return(CStreamPoolAccesser(getPoolAccesser(poolId)));
}

CStreamPoolAccesser CPoolFile::getStreamPoolAccesser(const string poolName)
{
	return(CStreamPoolAccesser(getPoolAccesser(poolName)));
}
*/

// Const Pool Accesser Methods
template<class l_addr_t,class p_addr_t>
	template<class pool_element_t> const TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > TPoolFile<l_addr_t,p_addr_t>::getPoolAccesser(const poolId_t poolId) const
{
	//readStructureInfoLock();
	try
	{
		if(!opened)
			throw(runtime_error(string(__func__)+" -- no file is open"));
		if(poolId>=getPoolCount())
			throw(runtime_error(string(__func__)+" -- invalid poolId parameter: "+istring(poolId)));
		if(sizeof(pool_element_t)!=pools[poolId].alignment)
			throw(runtime_error(string(__func__)+" -- method instantiated with a type whose size does not match the pool's alignment: sizeof(pool_element_t): "+istring(sizeof(pool_element_t))+" -- "+getPoolDescription(poolId)));

		//const alignment_t alignment=pools[poolId].alignment;

		const TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > accesser(const_cast<TPoolFile<l_addr_t,p_addr_t> *>(this),poolId);
		//readStructureInfoUnlock();
		return(accesser);
	}
	catch(exception &e)
	{
		//readStructureInfoUnlock();
		throw;
	}
}

template<class l_addr_t,class p_addr_t>
	template<class pool_element_t> const TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > TPoolFile<l_addr_t,p_addr_t>::getPoolAccesser(const string poolName) const
{
	return(getPoolAccesser<pool_element_t>(getPoolIdByName(poolName)));
}

/*
const CStreamPoolAccesser CPoolFile::getStreamPoolAccesser(const poolId_t poolId) const
{
	return(CStreamPoolAccesser(getPoolAccesser(poolId)));
}

const CStreamPoolAccesser CPoolFile::getStreamPoolAccesser(const string poolName) const
{
	return(CStreamPoolAccesser(getPoolAccesser(poolName)));
}
*/

template<class l_addr_t,class p_addr_t>
	size_t TPoolFile<l_addr_t,p_addr_t>::readPoolRaw(const poolId_t poolId,void *buffer,size_t readSize)
{
	if(!opened)
		throw(runtime_error(string(__func__)+" -- no file is open"));
	if(poolId>=getPoolCount())
		throw(runtime_error(string(__func__)+" -- invalid poolId parameter: "+istring(poolId)));

	const TStaticPoolAccesser<uint8_t,TPoolFile<l_addr_t,p_addr_t> > accesser(const_cast<TPoolFile<l_addr_t,p_addr_t> *>(this),poolId);
	readSize=min(readSize,accesser.getSize());
	accesser.read((uint8_t *)buffer,readSize);
	return(readSize);
}

template<class l_addr_t,class p_addr_t>
	size_t TPoolFile<l_addr_t,p_addr_t>::readPoolRaw(const string poolName,void *buffer,size_t readSize)
{
	return(readPoolRaw(getPoolIdByName(poolName),buffer,readSize));
}

template<class l_addr_t,class p_addr_t>
	template<class pool_element_t> TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > TPoolFile<l_addr_t,p_addr_t>::createPool(const string poolName,const bool throwOnExistance)
{
	//writeStructureInfoLock();
	try
	{
		if(!opened)
			throw(runtime_error(string(__func__)+" -- no file is open"));
		prvCreatePool(poolName,sizeof(pool_element_t),throwOnExistance);
		//writeStructureInfoUnlock();
	}
	catch(...)
	{
		//writeStructureInfoUnlock();
		throw;
	}
	return(getPoolAccesser<pool_element_t>(poolName));
}

/*
CStreamPoolAccesser CPoolFile::createStreamPool(const string poolName,const bool throwOnExistance)
{
	try
	{
		return(CStreamPoolAccesser(createPool(poolName,sizeof(StreamPoolAccesserType),throwOnExistance)));
	}
	catch(exception &e)
	{
		throw(runtime_error(string(__func__)+" -- "+string(e.what())));
	}
}
*/


template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::clear()
{
	while(poolNames.size()>0)
		removePool(poolNames.begin()->first);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::removePool(const poolId_t poolId)
{
	//writeStructureInfoLock();
	try
	{
		if(!opened)
			throw(runtime_error(string(__func__)+" -- no file is open"));

		invalidateAllCachedBlocks();
        
		// remove poolName with poolId of the parameter
		//for(size_t t=0;t<poolNames.getSize();t++)
		for(map<string,poolId_t>::const_iterator t=poolNames.begin();t!=poolNames.end();t++)
		{
			if(t->second==poolId)
			{
				//poolNames.remove(t->first);
				poolNames.erase(t->first);
				break;
			}
		}

		// decrement any other pools whose poolId is greater than the parameter
		/*
		for(size_t t=0;t<poolNames.getSize();t++)
		{
			if(poolNames[t]>poolId)
				poolNames[t]--;
		}
		*/
		for(map<string,poolId_t>::iterator t=poolNames.begin();t!=poolNames.end();t++)
		{
			if(t->second>poolId)
				t->second--;
		}

		// remove SAT entries with this poolId
		//pools.remove(poolId);
		pools.erase(pools.begin()+poolId);
		//for(size_t t=0;t<SAT[poolId].getSize();t++)
		for(size_t t=0;t<SAT[poolId].size();t++)
		{
			RLogicalBlock &block=SAT[poolId][t];
			/*
			size_t index;
			if(!physicalBlockList.contains(RPhysicalBlock(block.physicalStart,0),index))
			{
				printf("SAT and physical block list inconsistancies");
				exit(0);
			}
			physicalBlockList.remove(index);
			*/
				// could just call physicalBLockList.erase(serach...)
			const size_t index=findPhysicalBlockContaining(block.physicalStart);
			physicalBlockList.erase(physicalBlockList.begin()+index);
		}
		//SAT[poolId].clear();
		//SAT.remove(poolId);
		SAT.erase(SAT.begin()+poolId);

		makeBlockFileSmallest();

		backupSAT();

		//writeStructureInfoUnlock();
	}
	catch(...)
	{
		//writeStructureInfoUnlock();
		throw;
	}
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
	const size_t TPoolFile<l_addr_t,p_addr_t>::getPoolCount() const
{
	if(!opened)
		throw(runtime_error(string(__func__)+" -- no file is open"));
	return(pools.size());
}

template<class l_addr_t,class p_addr_t>
	const typename TPoolFile<l_addr_t,p_addr_t>::alignment_t TPoolFile<l_addr_t,p_addr_t>::getPoolAlignment(const poolId_t poolId) const
{
	//readStructureInfoLock();
	try
	{
		if(!opened)
			throw(runtime_error(string(__func__)+" -- no file is open"));
		if(poolId>=pools.size())
			throw(runtime_error(string(__func__)+" -- invalid poolId parameter"));

		const alignment_t alignment=pools[poolId].alignment;

		//readStructureInfoUnlock();
		return(alignment);
	}
	catch(...)
	{
		//readStructureInfoUnlock();
		throw;
	}
}

template<class l_addr_t,class p_addr_t>
	const typename TPoolFile<l_addr_t,p_addr_t>::alignment_t TPoolFile<l_addr_t,p_addr_t>::getPoolAlignment(const string poolName) const
{
	return(getPoolAlignment(getPoolIdByName(poolName)));
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::setPoolAlignment(const poolId_t poolId,size_t alignment)
{
		if(!opened)
			throw(runtime_error(string(__func__)+" -- no file is open"));
		if(poolId>=pools.size())
			throw(runtime_error(string(__func__)+" -- invalid poolId parameter"));
		if(pools[poolId].size>0)
			throw(runtime_error(string(__func__)+" -- pool is not empty"));
		if(alignment==0 ||alignment>maxBlockSize)
			throw(runtime_error(string(__func__)+" -- invalid alignment: "+istring(alignment)+" alignment must be 0 < alignment <= maxBlockSize (which is: "+istring(maxBlockSize)+")"));

		invalidateAllCachedBlocks();

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
		throw(runtime_error(string(__func__)+" -- no file is open"));

	//writeStructureInfoLock();
	try
	{
		if(poolId1>=pools.getSize())
			throw(runtime_error(string(__func__)+" -- invalid poolId1 parameter: "+istring(poolId1)));
		if(poolId2>=pools.getSize())
			throw(runtime_error(string(__func__)+" -- invalid poolId2 parameter: "+istring(poolId2)));
		if(poolId1==poolId2)
		{
			//writeStructureInfoUnlock();
			return;
		}

		invalidateAllCachedBlocks();

		// swap SATs for two pools
		//STL const TUniqueSortList<RLogicalBlock> temp=SAT[poolId1];
		SAT[poolId1]=SAT[poolId2];
		SAT[poolId2]=temp;

		// swap pool size info
		const RPoolInfo tempPool=pools[poolId1];
		pools[poolId1]=pools[poolId2];
		pools[poolId2]=tempPool;

		//writeStructureInfoUnlock();
	}
	catch(...)
	{
		//writeStructureInfoUnlock();
		throw;
	}
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::swapPools(const string poolName1,const string poolName2)
{
	swapPools(getPoolIdByName(poolName1),getPoolIdByName(poolName2));
}


template<class l_addr_t,class p_addr_t>
	const bool TPoolFile<l_addr_t,p_addr_t>::prvGetPoolIdByName(const string poolName,poolId_t &poolId) const
{
	/*
	size_t index=poolNames.findItem(poolName);
	if(index==DL_NOT_FOUND)
		return(false);
	poolId=poolNames[index];
	return(true);
	*/
	map<const string,poolId_t>::const_iterator i=poolNames.find(poolName);
	if(i==poolNames.end())
		return(false);

	poolId=i->second;
	return(true);
}

template<class l_addr_t,class p_addr_t>
	const typename TPoolFile<l_addr_t,p_addr_t>::poolId_t TPoolFile<l_addr_t,p_addr_t>::getPoolIdByName(const string poolName) const
{
	if(!opened)
		throw(runtime_error(string(__func__)+" -- no file is open"));

	poolId_t poolId=0;
	if(prvGetPoolIdByName(poolName,poolId))
		return(poolId);
	else
		throw(runtime_error(string(__func__)+" -- name not found: "+poolName));
}

template<class l_addr_t,class p_addr_t>
	const typename TPoolFile<l_addr_t,p_addr_t>::poolId_t TPoolFile<l_addr_t,p_addr_t>::getPoolIdByIndex(const size_t index) const // where index is 0 to getPoolCount()-1
{
	if(index>=poolNames.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(index)));

	map<const string,poolId_t>::const_iterator i=poolNames.begin();
	advance(i,index);
	return(i->second);
}

template<class l_addr_t,class p_addr_t>
	const string TPoolFile<l_addr_t,p_addr_t>::getPoolNameById(const poolId_t poolId) const
{
	if(poolId>=pools.size())
		throw(runtime_error(string(__func__)+" -- poolId parameter out of bounds: "+istring(poolId)));

	/*
	for(size_t i=0;i<poolNames.getSize();i++)
	{
		if(poolNames[i]==poolId)
			return(poolNames.getKey(i));
	}
	*/
	for(map<const string,poolId_t>::const_iterator i=poolNames.begin();i!=poolNames.end();i++)
	{
		if(i->second==poolId)
			return(i->first);
	}

	printf("uh oh.. pool name not found for pool id: %u\n",poolId);
	exit(0);
	return("");
}

template<class l_addr_t,class p_addr_t>
	const bool TPoolFile<l_addr_t,p_addr_t>::containsPool(const string poolName) const
{
	//return(poolNames.findItem(poolName)!=DL_NOT_FOUND);
	return(poolNames.find(poolName)!=poolNames.end());
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
	return(structureMutex.tryReadLock());
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::sharedUnlock() const
{
	structureMutex.unlock();
}

template<class l_addr_t,class p_addr_t>
	const size_t TPoolFile<l_addr_t,p_addr_t>::getSharedLockCount() const
{
	return(structureMutex.getReadLockCount());
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::exclusiveLock() const
{
	structureMutex.writeLock();
}

template<class l_addr_t,class p_addr_t>
	const bool TPoolFile<l_addr_t,p_addr_t>::exclusiveTrylock() const
{
	return(structureMutex.tryWriteLock());
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::exclusiveUnlock() const
{
	structureMutex.unlock();
}

template<class l_addr_t,class p_addr_t>
	const bool TPoolFile<l_addr_t,p_addr_t>::isExclusiveLocked() const
{
	return(structureMutex.isLockedForWrite());
}





// Private Methods

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::lockAccesserInfo() const
{
	accesserInfoMutex.lock();
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::unlockAccesserInfo() const
{
	accesserInfoMutex.unlock();
}


// Misc
template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::init(const bool createInitialCachedBlocks)
{
	filename="";

	SAT.clear();
	//SAT.setReallocQuantum(64);
	SAT.reserve(64);

	physicalBlockList.clear();
	//physicalBlockList.setReallocQuantum(8192);
	physicalBlockList.reserve(1024);

	poolNames.clear();
	//poolNames.reserve(64);

	pools.clear();
	//pools.setReallocQuantum(64);
	pools.reserve(64);

	accessers.clear();
	//accessers.setReallocQuantum(64);
	//accessers.reserve(64);

	if(createInitialCachedBlocks)
	{
		/*
		while(unusedCachedBlocks.getSize()>INITIAL_CACHED_BLOCK_COUNT)
			delete unusedCachedBlocks.dequeue();
		*/
		while(unusedCachedBlocks.size()>INITIAL_CACHED_BLOCK_COUNT)
		{
			delete unusedCachedBlocks.front();
			unusedCachedBlocks.pop();
		}

		/*
		while(unusedCachedBlocks.getSize()<INITIAL_CACHED_BLOCK_COUNT)
			unusedCachedBlocks.enqueue(new RCachedBlock(maxBlockSize));
		*/
		while(unusedCachedBlocks.size()<INITIAL_CACHED_BLOCK_COUNT)
			unusedCachedBlocks.push(new RCachedBlock(maxBlockSize));
	}
	else
	{
		/*
		while(!unusedCachedBlocks.isEmpty())
			delete unusedCachedBlocks.dequeue();
		*/
		while(!unusedCachedBlocks.empty())
		{
			delete unusedCachedBlocks.front();
			unusedCachedBlocks.pop();
		}
	}

	/*
	while(!unreferencedCachedBlocks.isEmpty())
		delete unreferencedCachedBlocks.dequeue();
	while(!activeCachedBlocks.isEmpty())
		delete activeCachedBlocks.dequeue();
	*/
	while(!unreferencedCachedBlocks.empty())
	{
		delete unreferencedCachedBlocks.front();
		unreferencedCachedBlocks.pop_front();
	}
	while(!activeCachedBlocks.empty())
	{
		delete activeCachedBlocks.front();
		activeCachedBlocks.pop_front();
	}

	opened=false;
    	openedTemp=false;
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::verifyBlockInfo(const poolId_t poolId) const
{
		// this didn't seem to catch some errors I put in on purpose, so I wrote verifyAllBlockInfo()
	// poolId not verified itself
	p_addr_t start=0;
	//for(size_t t=0;t<SAT[poolId].getSize();t++)
	//for(set<RLogicalBlock>::const_iterator t=SAT[poolId].begin();t!=SAT[poolId].end();t++)
	for(size_t t=0;t<SAT[poolId].size();t++)
	{
		//RLogicalBlock block=SAT[poolId][t];
		const RLogicalBlock &block=SAT[poolId][t];

		if(block.logicalStart!=start)
		{
			printf("Error verifying at logicalBlockIndex: %u\n",t);
			return;
		}

		unsigned p=0;
		for(size_t i=0;i<physicalBlockList.size();i++)
		{
			if(block.physicalStart>=physicalBlockList[i].physicalStart && (block.physicalStart+block.size-1)<=(physicalBlockList[i].physicalStart+physicalBlockList[i].size-1))
			{
				p++;
				if(p>1)
				{
					printf("two blocks are occupying the same physical space\n");
					exit(1);
				}
			}
		}

		start+=block.size;
	}
	if(start!=pools[poolId].size)
		printf("poolSizes don't match\n");
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::verifyAllBlockInfo(const bool expectContiguousPhysicalSpace) const
{
	// for each pool, make sure the logical space is contiguous with no gaps
	// and for each block in the logical space, make sure there is a physical block with the right size
	for(size_t poolId=0;poolId<pools.size();poolId++)
	{
		l_addr_t expectedStart=0;
		//for(size_t t=0;t<SAT[poolId].getSize();t++)
	
		
		//for(set<RLogicalBlock>::const_iterator t=SAT[poolId].begin();t!=SAT[poolId].end();t++)
		for(size_t t=0;t<SAT[poolId].size();t++)
		{
			const RLogicalBlock &logicalBlock=SAT[poolId][t];
			//const RLogicalBlock &logicalBlock=*t;

			if(logicalBlock.logicalStart!=expectedStart)
			{
				printSAT();
				printf("pool: %u -- logical start wasn't what was expected in logical block: %lld\n",poolId,(long long)expectedStart);
				logicalBlock.print();
				exit(1);
			}

			size_t pbIndex=findPhysicalBlockContaining(logicalBlock.physicalStart);
			const RPhysicalBlock &physicalBlock=physicalBlockList[pbIndex];
			//const RPhysicalBlock &physicalBlock=*findPhysicalBlockContaining(logicalBlock.physicalStart);
			if(physicalBlock.physicalStart!=logicalBlock.physicalStart)
			{
				printSAT();
				printf("pool: %u -- physical block doesn't start the same as the logical block:\n",poolId);
				logicalBlock.print();
				physicalBlock.print();
				exit(1);
			}
			if(physicalBlock.size!=logicalBlock.size)
			{
				printSAT();
				printf("pool: %u physical block's size isn't the same as the logical block's:\n",poolId);
				logicalBlock.print();
				physicalBlock.print();
				exit(1);
			}

			expectedStart+=logicalBlock.size;

			bool startFound=false;
			for(size_t i=0;i<physicalBlockList.size();i++)
			//for(set<RPhysicalBlock>::const_iterator i=physicalBlockList.begin();i!=physicalBlockList.end();i++)
			{
				const RPhysicalBlock &physicalBlock=physicalBlockList[i];
				if(logicalBlock.physicalStart==physicalBlock.physicalStart)
				{
					startFound=true;
					if(logicalBlock.size!=physicalBlock.size)
					{
						printSAT();
						printf("pool: %u -- physical block starting on address was found, but the size of the physical block was wrong\n",poolId);
						logicalBlock.print();
						physicalBlock.print();
						exit(1);
					}

					break;
				}
			}
			if(!startFound)
			{
				printSAT();
				printf("pool: %u -- physical block with correct start was not found\n",poolId);
				logicalBlock.print();
				exit(1);
			}


			unsigned p=0;
			for(size_t i=0;i<physicalBlockList.size()-1;i++)
			//for(set<RPhysicalBlock>::const_iterator i=physicalBlockList.begin();i!=physicalBlockList.end();i++)
			{
				const RPhysicalBlock &physicalBlock=physicalBlockList[i];
				if(logicalBlock.physicalStart>=physicalBlock.physicalStart && (logicalBlock.physicalStart+logicalBlock.size-1)<=(physicalBlock.physicalStart+physicalBlock.size-1))
				{
					p++;
					if(p>1)
					{
						printSAT();
						printf("pool: %u -- two blocks are occupying the same physical space\n",poolId);
						logicalBlock.print();
						physicalBlock.print();
						//physicalBlockList[i+1]->print();

						exit(1);
					}
				}
			}
		}
	}

	// make sure that no blocks in physicalBlockList overlap (should have been found above if it's happening)
	//if(physicalBlockList.getSize()>0)
	if(physicalBlockList.size()>0)
	{
		for(size_t t=0;t<physicalBlockList.size()-1;t++)
		//for(set<RPhysicalBlock>::const_iterator t=physicalBlockList.begin();t!=physicalBlockList.end();t++)
		{
			/*
			if(next(t)==physicalBlockList.end())
				break;
			*/

			const RPhysicalBlock &physicalBlock1=physicalBlockList[t];
			//set<RPhysicalBlock>::const_iterator next_t=t;next_t++;
			const RPhysicalBlock &physicalBlock2=physicalBlockList[t+1];
			
			if(physicalBlock1.physicalStart+physicalBlock1.size>physicalBlock2.physicalStart)
			{
				printSAT();
				printf("two physical blocks are overlapping\n");
				physicalBlock1.print();
				physicalBlock2.print();
				exit(0);
			}
		}
	}

	// verify that all the physical blocks are contiguous (not an invalid pool file if it happens, but the user may expect it)
	if(expectContiguousPhysicalSpace)
	{
		p_addr_t expectedStart=0;
		for(size_t t=0;t<physicalBlockList.size();t++)
		//for(set<RPhysicalBlock>::const_iterator t=physicalBlockList.begin();t!=physicalBlockList.end();t++)
		{
			const RPhysicalBlock &physicalBlock=physicalBlockList[t];
			if(physicalBlock.physicalStart!=expectedStart)
			{
				printSAT();
				printf("-- physical start wasn't what was expected in physical block: %lld\n",(long long)expectedStart);
				physicalBlock.print();
				exit(1);
			}

			expectedStart+=physicalBlock.size;
		}
	}
}

template<class l_addr_t,class p_addr_t>
	const p_addr_t TPoolFile<l_addr_t,p_addr_t>::getProceedingPoolSizes(const poolId_t poolId) const
{
	p_addr_t proceedingPoolSizes=0;
	for(size_t t=0;t<poolId;t++)
		proceedingPoolSizes+=pools[t].size;
	return(proceedingPoolSizes);
}

template<class l_addr_t,class p_addr_t>
	const l_addr_t TPoolFile<l_addr_t,p_addr_t>::getMaxBlockSizeFromAlignment(const alignment_t alignment) const
{
	if(maxBlockSize<=(maxBlockSize%alignment))
		throw(runtime_error(string(__func__)+" -- alignment size is too big"));

	return(maxBlockSize-(maxBlockSize%alignment));
}

template<class l_addr_t,class p_addr_t>
	const l_addr_t TPoolFile<l_addr_t,p_addr_t>::getPoolSize(const poolId_t poolId) const
{
	if(poolId>=pools.size())
		throw(runtime_error(string(__func__)+" -- invalid poolId parameter: "+istring(poolId)));

	return(pools[poolId].size);
}

template<class l_addr_t,class p_addr_t>
	const l_addr_t TPoolFile<l_addr_t,p_addr_t>::getPoolSize(const string poolName) const
{
	return(getPoolSize(getPoolIdByName(poolName)));
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::writeMetaData(CMultiFile *f)
{
	makeBlockFileSmallest(false); // doesn't necessarily help if f is not &this->blockFile

	// write meta and user info
	uint64_t metaDataOffset=f->getSize();

	writeSATToFile(f,metaDataOffset);

	// Signature
	f->write(FORMAT_SIGNATURE,8,SIGNATURE_OFFSET);

	// EOF
	int8_t eofChar=26;
	f->write(&eofChar,sizeof(eofChar),EOF_OFFSET);

	// Format Version
	uint32_t formatVersion=FORMAT_VERSION;
	f->write(&formatVersion,sizeof(formatVersion),FORMAT_VERSION_OFFSET);

	// Dirty
	int8_t dirty=1;
	f->write(&dirty,sizeof(dirty),DIRTY_INDICATOR_OFFSET);

	// Which SAT File
	uint8_t _whichSATFile=0;
	f->write(&_whichSATFile,sizeof(_whichSATFile),WHICH_SAT_FILE_OFFSET);

	// Meta Data Offset
	f->write(&metaDataOffset,sizeof(metaDataOffset),META_DATA_OFFSET);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::prvCreatePool(const string poolName,const alignment_t alignment,const bool throwOnExistance)
{
	poolId_t poolId;
	if(!prvGetPoolIdByName(poolName,poolId))
	{	// poolName not found
		if(poolName.length()>MAX_POOL_NAME_LENGTH)
			throw(runtime_error(string(__func__)+" -- pool name too long: "+poolName));

			// ??? this is where the poolId get's created.. pools.size()
		//addPool(poolNames.getOrAdd(poolName)=pools.getSize(),alignment);
		addPool(poolNames.insert(make_pair(poolName,pools.size())).first->second,alignment);
		return;
	}
	else
	{
		// pool name already exists
		if(throwOnExistance)
			throw(runtime_error(string(__func__)+" -- pool name already exists: "+poolName));
	}
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::addPool(const poolId_t poolId,const alignment_t alignment)
{
	if(poolId!=pools.size())
		throw(runtime_error(string(__func__)+" -- invalid new pool id or pool already exists: "+istring(poolId)));
	if(alignment==0 ||alignment>maxBlockSize)
		throw(runtime_error(string(__func__)+" -- invalid alignment: "+istring(alignment)+" alignment must be 0 < alignment <= maxBlockSize (which is: "+istring(maxBlockSize)+")"));

	invalidateAllCachedBlocks();

	appendNewSAT();
	//pools.append(RPoolInfo(0,alignment));
	pools.push_back(RPoolInfo(0,alignment));
}


template<class l_addr_t,class p_addr_t>
	const string TPoolFile<l_addr_t,p_addr_t>::getPoolDescription(const poolId_t poolId) const
{
	return("poolId: "+istring(poolId)+" name: '"+getPoolNameById(poolId)+"' byte size: "+istring(getPoolSize(poolId))+" byte alignment: "+istring(getPoolAlignment(poolId)));
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::writeDirtyIndicator(const bool dirty,CMultiFile *f)
{
	int8_t buffer[1];
	buffer[0]=dirty ? (char)1 : (char)0;
	f->write(buffer,sizeof(*buffer),DIRTY_INDICATOR_OFFSET);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::makeBlockFileSmallest(const bool checkCurrent)
{
	size_t l=physicalBlockList.size();
	if(l>0)
		changeBlockFileSize(physicalBlockList[l-1].physicalStart+physicalBlockList[l-1].size,checkCurrent);
	else
		changeBlockFileSize(0,checkCurrent);
	/*
	if(physicalBlockList.empty())
		changeBlockFileSize(0,checkCurrent);
	else
	{
		const RPhysicalBlock &b=*(--physicalBlockList.end());
		changeBlockFileSize(b.physicalStart+b.size,checkCurrent);
	}
	*/

}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::appendNewSAT()
{
	SAT.push_back(vector<RLogicalBlock>());
	//STL SAT.append(TUniqueSortList<RLogicalBlock>());
	//SAT[SAT.getSize()-1].setReallocQuantum(8192);
	SAT[SAT.size()-1].reserve(1024);
}





// Structural Integrity Methods
template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::openSATFiles(const bool mustExist)
{
	if(isTemp())
		return;

	SATFiles[0].open(SATFilename+"1",!mustExist);
	SATFiles[1].open(SATFilename+"2",!mustExist);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::closeSATFiles(const bool removeFiles)
{
	if(isTemp())
		return;

	SATFiles[0].sync();
	SATFiles[1].sync();

	SATFiles[0].close(removeFiles);
	SATFiles[1].close(removeFiles);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::writeWhichSATFile()
{
	if(isTemp())
		return;

	uint8_t buffer[1];
	buffer[0]=whichSATFile;
	blockFile.write(buffer,sizeof(*buffer),WHICH_SAT_FILE_OFFSET);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::backupSAT()
{
	if(isTemp())
		return;

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
	const uint32_t poolCount=SAT.size();
	f->write(&poolCount,sizeof(poolCount),multiFileHandle);

	for(size_t poolId=0;poolId<SAT.size();poolId++)
	{
		// write name of pool
		writeString(getPoolNameById(poolId),f,multiFileHandle);

		// write pool info structure
		pools[poolId].writeToFile(f,multiFileHandle);

		// write number of SAT entries for this pool
		const uint32_t SATSize=SAT[poolId].size();
		f->write(&SATSize,sizeof(SATSize),multiFileHandle);

		TAutoBuffer<uint8_t> mem(SATSize*RLogicalBlock().getMemSize());

		// write each SAT entry
		size_t offset=0;
		for(size_t t=0;t<SAT[poolId].size();t++)
		//for(set<RLogicalBlock>::const_iterator t=SAT[poolId].begin();t!=SAT[poolId].end();t++)
			SAT[poolId][t].writeToMem(mem,offset);

		f->write(mem,mem.getSize(),multiFileHandle);
	}
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::restoreSAT()
{
    if(isTemp())
        return;

	invalidateAllCachedBlocks();


	uint8_t buffer[1];
	blockFile.read(buffer,sizeof(*buffer),WHICH_SAT_FILE_OFFSET);
	whichSATFile=buffer[0];
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
	if(isTemp())
		return;

	CMultiFile::RHandle multiFileHandle;
	f->seek(readWhere,multiFileHandle);

	// ??? probably want to put a SAT file signature here to make sure it's a SAT file

	// destory current SAT and pool information
	pools.clear();
	poolNames.clear();
	SAT.clear();
	physicalBlockList.clear();

	// read number of pools
	uint32_t poolCount;
	f->read(&poolCount,sizeof(poolCount),multiFileHandle);
	for(size_t poolId=0;poolId<poolCount;poolId++)
	{
		// read pool name
		string poolName;
		readString(poolName,f,multiFileHandle);

		// read pool info structure
		RPoolInfo poolInfo;
		poolInfo.readFromFile(f,multiFileHandle);

		try
		{
			prvCreatePool(poolName,poolInfo.alignment);
		}
		catch(...)
		{
			printf("error creating pool\n");
			exit(0);
		}

		// read number of SAT entries
		uint32_t SATSize;
		f->read(&SATSize,sizeof(SATSize),multiFileHandle);

		// read SAT into mem buffer
		TAutoBuffer<uint8_t> mem(SATSize*RLogicalBlock().getMemSize());
		f->read(mem,mem.getSize(),multiFileHandle);

		// read each SAT entry from that mem buffer and put into the actual SAT data-member
		size_t offset=0;
		for(size_t t=0;t<SATSize;t++)
		{
			RLogicalBlock logicalBlock;
			logicalBlock.readFromMem(mem,offset);

			// divide the size of the block just read into pieces that will fit into maxBlockSize sizes blocks
			// just in case the maxBlockSize is smaller than it used to be
			const l_addr_t blockSize=logicalBlock.size;
			for(l_addr_t j=0;j<blockSize/maxBlockSize;j++)
			{
				logicalBlock.size=maxBlockSize;

				sortedInsert(SAT[poolId],logicalBlock);
				sortedInsert(physicalBlockList,RPhysicalBlock(logicalBlock));

				logicalBlock.logicalStart+=maxBlockSize;
				logicalBlock.physicalStart+=maxBlockSize;
			}
			logicalBlock.size=blockSize%maxBlockSize;
			if(logicalBlock.size>0)
			{
				sortedInsert(SAT[poolId],logicalBlock);
				sortedInsert(physicalBlockList,RPhysicalBlock(logicalBlock));
			}

			pools[poolId].size+=blockSize;
		}

		if(pools[poolId].size!=poolInfo.size)
		{
			printf("buildSATFromFile -- pool size from SAT read does not match pool size from pool info read\n");
			exit(0);
		}
	}
	makeBlockFileSmallest(false);
}




// SAT operations
/*
template<class l_addr_t,class p_addr_t>
	const size_t TPoolFile<l_addr_t,p_addr_t>::findSATBlockContaining(const poolId_t poolId,const l_addr_t where,bool &atStartOfBlock) const
{
	if(SAT[poolId].empty())
		throw(runtime_error(string(__func__)+" -- SAT is empty"));

	atStartOfBlock=false;
	size_t previousBlockIndex=NIL_INDEX;
	for(size_t logicalBlockIndex=0;logicalBlockIndex<SAT[poolId].getSize();logicalBlockIndex++)
	{
		// ??? Optimization: speed up by doing a binary search or using a heap
		if(SAT[poolId][logicalBlockIndex].logicalStart>=where)
		{	// on it or just passed it
			if(SAT[poolId][logicalBlockIndex].logicalStart==where)
			{
				atStartOfBlock=true;
				return(logicalBlockIndex);
			}

			logicalBlockIndex=previousBlockIndex;
			if(logicalBlockIndex==NIL_INDEX)
			{
				printf("invalid...\n");
				exit(1);
			}
			return(logicalBlockIndex);
		}
		previousBlockIndex=logicalBlockIndex;
	}

	if(previousBlockIndex!=NIL_INDEX && where>=SAT[poolId][previousBlockIndex].logicalStart && where<=(SAT[poolId][previousBlockIndex].logicalStart+SAT[poolId][previousBlockIndex].size-1))
	{
		if(where==SAT[poolId][previousBlockIndex].logicalStart)
			atStartOfBlock=true;
		return(previousBlockIndex);
	}

	printf("shouldn't have gotten here\n");
	exit(1);
	return(NIL_INDEX);
}
*/

/*
template<class l_addr_t,class p_addr_t>
	const set<TPoolFile<l_addr_t,p_addr_t>::RLogicalBlock>::iterator TPoolFile<l_addr_t,p_addr_t>::findSATBlockContaining(const poolId_t poolId,const l_addr_t where,bool &atStartOfBlock)
{
	if(SAT[poolId].empty())
		throw(runtime_error(string(__func__)+" -- SAT is empty"));

	const set<RLogicalBlock>::iterator i=(--SAT[poolId].upper_bound(where));
	atStartOfBlock=(i->logicalStart==where);
	return(i);
}
*/

/*
template<class l_addr_t,class p_addr_t>
	const set<TPoolFile<l_addr_t,p_addr_t>::RLogicalBlock>::const_iterator TPoolFile<l_addr_t,p_addr_t>::findSATBlockContaining(const poolId_t poolId,const l_addr_t where,bool &atStartOfBlock) const
{
	if(SAT[poolId].empty())
		throw(runtime_error(string(__func__)+" -- SAT is empty"));

	const set<RLogicalBlock>::const_iterator i=(--SAT[poolId].upper_bound(where));
	atStartOfBlock=(i->logicalStart==where);
	return(i);
}
*/
template<class l_addr_t,class p_addr_t>
	const size_t TPoolFile<l_addr_t,p_addr_t>::findSATBlockContaining(const poolId_t poolId,const l_addr_t where,bool &atStartOfBlock) const
{
	if(SAT[poolId].empty())
		throw(runtime_error(string(__func__)+" -- SAT is empty"));

	const size_t i=(upper_bound(SAT[poolId].begin(),SAT[poolId].end(),RLogicalBlock(where))-1)-SAT[poolId].begin();
	atStartOfBlock=(SAT[poolId][i].logicalStart==where);
	return(i);
}


/*
template<class l_addr_t,class p_addr_t>
	const size_t TPoolFile<l_addr_t,p_addr_t>::findPhysicalBlockContaining(const p_addr_t physicalWhere) const
{
	size_t previousPhysicalBlockIndex=NIL_INDEX;
	for(size_t physicalBlockIndex=0;physicalBlockIndex<physicalBlockList.getSize();physicalBlockIndex++)
	{
		// speed up by doing a binary search or using a heap
		if(physicalBlockList[physicalBlockIndex].physicalStart>=physicalWhere)
		{	// on it or just passed it
			if(physicalBlockList[physicalBlockIndex].physicalStart==physicalWhere)
				return(physicalBlockIndex);

			physicalBlockIndex=previousPhysicalBlockIndex;
			if(physicalBlockIndex==NIL_INDEX)
			{
				printf("invalid...\n");
				exit(1);
			}
			return(physicalBlockIndex);
		}
		previousPhysicalBlockIndex=physicalBlockIndex;
	}

	printf("shouldn't have gotten here\n");
	exit(1);
	return(NIL_INDEX);
}
*/
/*
template<class l_addr_t,class p_addr_t>
	const set<TPoolFile<l_addr_t,p_addr_t>::RPhysicalBlock>::iterator TPoolFile<l_addr_t,p_addr_t>::findPhysicalBlockContaining(const p_addr_t physicalWhere)
{
	return(--physicalBlockList.upper_bound(physicalWhere));
}
*/

/*
template<class l_addr_t,class p_addr_t>
	const set<TPoolFile<l_addr_t,p_addr_t>::RPhysicalBlock>::const_iterator TPoolFile<l_addr_t,p_addr_t>::findPhysicalBlockContaining(const p_addr_t physicalWhere) const
{
	return(--physicalBlockList.upper_bound(physicalWhere));
}
*/
template<class l_addr_t,class p_addr_t>
	const size_t TPoolFile<l_addr_t,p_addr_t>::findPhysicalBlockContaining(const p_addr_t physicalWhere) const
{
	return((upper_bound(physicalBlockList.begin(),physicalBlockList.end(),RPhysicalBlock(physicalWhere))-1)-physicalBlockList.begin());
}


// - Finds the largest hole for "size" bytes in the physical space in the file.
// - If no hole large enough is available, then it returns the last physical block
//   plus its size as a hole position -- implying to create more space.
// - The optional not_in_start and not_in_stop parameters can specify a single area
//   not to use.
// - Returns the physical address of the beginning of the hole
template<class l_addr_t,class p_addr_t>
	const p_addr_t TPoolFile<l_addr_t,p_addr_t>::findHole(const l_addr_t size,const p_addr_t not_in_start,const p_addr_t not_in_stop)
{
	// ??? I should probably easily stop soon if I find the exact sized hole  that way, there is no fragmentation

	if(physicalBlockList.empty())
	{
		if(not_in_start<=not_in_stop)
		{
			printf("optional parameters specified and no spaces exists yet\n");
			exit(1);
		}
		return(0);
	}

	if(size==0)
	{
		printf("%s -- internal error -- size parameter is 0 -- maybe not a problem.. just return 0 or something\n",__func__);
		exit(1);
	}


	// maybe keep this in a heap.. so that we can alway get the largest.. but then again the order may be the same after inserts to the heap???

	// search all existing file space for the maximum size hole
	p_addr_t largestHoleSize=0;
	//set<RPhysicalBlock>::const_iterator largestHoleBlockIndex=physicalBlockList.end();
	size_t largestHoleBlockIndex=physicalBlockList.size();
	const RPhysicalBlock start(0,0);
	const RPhysicalBlock *prev_block=&start;
	for(size_t t=0;t<physicalBlockList.size();t++)
	//for(set<RPhysicalBlock>::const_iterator t=physicalBlockList.begin();t!=physicalBlockList.end();t++)
	{
		//const RPhysicalBlock &block=(*t);
		const RPhysicalBlock &block=physicalBlockList[t];

		const p_addr_t hole_size=block.physicalStart-(prev_block->physicalStart+prev_block->size);
		if(hole_size>largestHoleSize && (not_in_start>not_in_stop || !isInWindow(prev_block->physicalStart+prev_block->size,block.physicalStart-1,not_in_start,not_in_stop)))
		{
			largestHoleSize=hole_size;
			//largestHoleBlockIndex=prev(t);
			largestHoleBlockIndex=t-1;
		}

		prev_block=&block;
	}

	if(largestHoleSize>=size)
	{	// use this space

		// handling the case that the first physical block is not at address 0 and there is space 
		// before the first block, thus the t-1 done in the loop above really is trying to point 
		// before phyicalBlockList even starts
		if(largestHoleBlockIndex==(((size_t)0)-1))
			return(0);

		//const RPhysicalBlock &block=*largestHoleBlockIndex;
		const RPhysicalBlock &block=physicalBlockList[largestHoleBlockIndex];
		return(block.physicalStart+block.size);
	}

	// put new space after the last physical block
	//const RPhysicalBlock &lastBlock=*(--physicalBlockList.end());
	const RPhysicalBlock &lastBlock=physicalBlockList[physicalBlockList.size()-1];
	return(lastBlock.physicalStart+lastBlock.size);
}

// returns the physical address of the beginning of the hole
template<class l_addr_t,class p_addr_t>
	const p_addr_t TPoolFile<l_addr_t,p_addr_t>::makeHole(const l_addr_t size)
{
	// perhaps coeless blocks to make space.. and move actual data in file IF that
	// space is represented in the file yet

	return(blockFile.getSize());
}

template<class l_addr_t,class p_addr_t>
	const bool TPoolFile<l_addr_t,p_addr_t>::isInWindow(const p_addr_t start,const p_addr_t end,const p_addr_t windowStart,const p_addr_t windowEnd) const
{
	if(start<=windowStart && end>=windowStart)
		return(true);
	if(end>=windowEnd && start<=windowEnd)
		return(true);
	if(start>=windowStart && end<=windowEnd)
		return(true);
	return(false);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::joinAllAdjacentBlocks()
{
	for(size_t poolId=0;poolId<pools.size();poolId++)
		joinAdjacentBlocks(poolId);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::joinAdjacentBlocks(const poolId_t poolId)
{
	joinAdjacentBlocks(poolId,0,SAT[poolId].size());
}

template<class l_addr_t,class p_addr_t>
	//void TPoolFile<l_addr_t,p_addr_t>::joinAdjacentBlocks(const poolId_t poolId,const set<RLogicalBlock>::const_iterator firstBlockIndex,const set<RLogicalBlock>::const_iterator end)
	void TPoolFile<l_addr_t,p_addr_t>::joinAdjacentBlocks(const poolId_t poolId,const size_t firstBlockIndex,const size_t blockCount)
{
	const size_t totalBlocks=SAT[poolId].size();

	for(size_t t=firstBlockIndex;t<firstBlockIndex+blockCount;t++)
	//for(set<RLogicalBlock>::const_iterator t=firstBlockIndex;t!=end;t++)
	{
		if(t==firstBlockIndex)
			continue; // skip first iteration because we're looking at t and the one before t (also avoids t-1 being underflowing)
		if(t>=totalBlocks)
			break; // just in case firstBlockIndex + blockCount specifies too many blocks

		RLogicalBlock &b1=SAT[poolId][t-1];
		//const RLogicalBlock &b1=*prev(t);
		//const RLogicalBlock &b2=*t;
		const RLogicalBlock &b2=SAT[poolId][t];

		if((b1.physicalStart+b1.size)==b2.physicalStart)
		{ // blocks are physically next to each other -- candidate for joining
			const l_addr_t newSize=b1.size+b2.size;

			// size of blocks if joined doesn't make a block too big
			if(newSize<=maxBlockSize)
			{ // now join blocks blockIndex and blockIndex+1

				//set<RPhysicalBlock>::const_iterator pbIndex=findPhysicalBlockContaining(b1.physicalStart);
				const size_t pbIndex=findPhysicalBlockContaining(b1.physicalStart);
				//set<RPhysicalBlock>::const_iterator next_pbIndex=next(pbIndex);
				//const size_t next_pbIndex=pbIndex+1;

				// sanity check
				//if(next_pbIndex->physicalStart!=(b1.physicalStart+b1.size))
				if(physicalBlockList[pbIndex+1].physicalStart!=(b1.physicalStart+b1.size))
				{
					printSAT();
					printf("pool: %u -- expecting next physical block to have a certain position\n",poolId);
					physicalBlockList[pbIndex].print();
					physicalBlockList[pbIndex+1].print();
					exit(1);
				}

				//const_cast<l_addr_t &>(b1.size)=newSize;
				b1.size=newSize;
				SAT[poolId].erase(SAT[poolId].begin()+t);
				//totalBlocks--;

				if(/*pbIndex->physicalStart*/physicalBlockList[pbIndex].physicalStart!=b1.physicalStart) // sanity check
				{
					printf("atStartOfBlock is false but should have been\n");
					exit(1);
				}
				physicalBlockList[pbIndex].size=newSize;
				//const_cast<l_addr_t &>(pbIndex->size)=newSize;
				//physicalBlockList.remove(pbIndex+1);
				physicalBlockList.erase(physicalBlockList.begin()+pbIndex+1);

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
		printf("\t%-4u Pool: '%s' size: %lld\n",poolId,getPoolNameById(poolId).c_str(),(long long)getPoolSize(poolId));
		size_t p=0;
		for(size_t t=0;t<SAT[poolId].size();t++)
		//for(set<RLogicalBlock>::const_iterator t=SAT[poolId].begin();t!=SAT[poolId].end();t++)
		{
			printf("\t\t%-4u ",p++);
			//t->print();
			SAT[poolId][t].print();
		}

	}
	printf("\nPhysicalBlockList:\n");
	size_t p=0;
	for(size_t t=0;t<physicalBlockList.size();t++)
	//for(set<RPhysicalBlock>::const_iterator t=physicalBlockList.begin();t!=physicalBlockList.end();t++)
	{
		printf("\t%-4u ",p++);
		//t->print();
		physicalBlockList[t].print();
	}

	printf("\n");
}


// Basic I/O
template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::changeBlockFileSize(const p_addr_t newSize,bool checkCurrent)
{
	blockFile.setSize(newSize+LEADING_DATA_SIZE);
}


// Pool Modification (pe -- pool elements, b -- bytes)
template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::insertSpace(const poolId_t poolId,const l_addr_t peWhere,const l_addr_t peCount)
{
	if(!opened)
		throw(runtime_error(string(__func__)+" -- no file is open"));
	if(peCount==0)
		return;

	//writeStructureInfoLock();
	try
	{
		if(poolId>=pools.size())
			throw(runtime_error(string(__func__)+" -- invalid poolId: "+istring(poolId)));

		const alignment_t bAlignment=pools[poolId].alignment;
		const l_addr_t bPoolSize=pools[poolId].size;
		const l_addr_t pePoolSize=bPoolSize/bAlignment;
		const l_addr_t pePoolMaxSize=maxLogicalAddress/bAlignment;
		
		if(pePoolMaxSize-pePoolSize<peCount)
			throw(runtime_error(string(__func__)+" -- insufficient logical address space to insert "+istring(peCount)+" elements into pool ("+getPoolDescription(poolId)+")"));
		if(peWhere>pePoolSize)
			throw(runtime_error(string(__func__)+" -- out of range peWhere "+istring(peWhere)+" for pool ("+getPoolDescription(poolId)+")"));

		invalidateAllCachedBlocks();

		const l_addr_t bWhere=peWhere*bAlignment;
		const l_addr_t bCount=peCount*bAlignment;

		const l_addr_t maxBlockSize=getMaxBlockSizeFromAlignment(bAlignment);
		const size_t newLogicalBlockCount=(bCount/maxBlockSize);
		bool didSplitOne=false;

		//size_t logicalBlockIndex=NIL_INDEX;
		//set<RLogicalBlock>::const_iterator logicalBlockIndex=SAT[poolId].end();
		size_t logicalBlockIndex=SAT[poolId].size();
		if(bWhere<bPoolSize)
		{ // in the middle (not appending)
			bool atStartOfBlock;
			logicalBlockIndex=findSATBlockContaining(poolId,bWhere,atStartOfBlock);

			if(!atStartOfBlock)
			{ // split the block at logicalBlockIndex since where isn't exacly at the beginning of the block
				didSplitOne=true;


				// sanity check
				//if(bWhere<=logicalBlockIndex->logicalStart)
				if(bWhere<=SAT[poolId][logicalBlockIndex].logicalStart)
				{ // logical impossibility since atStartOfBlock wasn't true (unless it was wrong)
					printf("oops...\n");
					exit(1);
				}

				// modify block at logicalBlockIndex and create a new RLogicalBlock for the second part of the split block
				//const l_addr_t firstPartSize=bWhere-logicalBlockIndex->logicalStart;
				//const l_addr_t secondPartSize=logicalBlockIndex->size-firstPartSize;
				const l_addr_t firstPartSize=bWhere-SAT[poolId][logicalBlockIndex].logicalStart;
				const l_addr_t secondPartSize=SAT[poolId][logicalBlockIndex].size-firstPartSize;


				// find the physical block for this logical block
				/*
				RPhysicalBlock searchPhysicalBlock;
				size_t physicalBlockIndex;
				searchPhysicalBlock.physicalStart=logicalBlockIndex->physicalStart;
				if(!physicalBlockList.contains(searchPhysicalBlock,physicalBlockIndex))
				{
					printf("physical block list inconsistancies...\n");
					exit(1);
				}
				*/
				/*
				set<RPhysicalBlock>::const_iterator physicalBlockIndex=physicalBlockList.find(RPhysicalBlock(logicalBlockIndex->physicalStart));
				if(physicalBlockIndex==physicalBlockList.end())
				{
					printf("physical block list inconsistancies...\n");
					exit(1);
				}
				*/
				const size_t physicalBlockIndex=lower_bound(physicalBlockList.begin(),physicalBlockList.end(),RPhysicalBlock(SAT[poolId][logicalBlockIndex].physicalStart))-physicalBlockList.begin();
				


				// shrink the logical and physical blocks' sizes
				//const_cast<l_addr_t &>(physicalBlockIndex->size)=const_cast<l_addr_t &>(logicalBlockIndex->size)=firstPartSize;
				physicalBlockList[physicalBlockIndex].size=SAT[poolId][logicalBlockIndex].size=firstPartSize;

				// create the new logical and physical blocks which are the second part of the old blocks
				RLogicalBlock newLogicalBlock;
				newLogicalBlock.logicalStart=SAT[poolId][logicalBlockIndex].logicalStart+firstPartSize;
				newLogicalBlock.physicalStart=SAT[poolId][logicalBlockIndex].physicalStart+firstPartSize;
				newLogicalBlock.size=secondPartSize;

				// add the new logical block
				/*
				if(!SAT[poolId].insert(newLogicalBlock).second)
				{	// inconsistancies
					printf("error adding to SAT\n");
					exit(1);
				}
				*/
				sortedInsert(SAT[poolId],newLogicalBlock);

				// add the new physical block
				/*
				if(!physicalBlockList.insert(RPhysicalBlock(newLogicalBlock)).second)
				{	// error
					printf("error adding to physical block list\n");
					exit(1);
				}
				*/
				sortedInsert(physicalBlockList,RPhysicalBlock(newLogicalBlock));

				// alter the logical mapping by increasing all blocks' logicalStarts after the insertion point
						// ??? bad
				//for(logicalBlockIndex++;logicalBlockIndex!=SAT[poolId].end();logicalBlockIndex++)
				for(size_t t=logicalBlockIndex+1;t<SAT[poolId].size();t++)
					SAT[poolId][t].logicalStart+=bCount;
			}
			else
			{ // increase all blocks at and after insertion point
				for(size_t t=logicalBlockIndex;t<SAT[poolId].size();t++)
						// ??? mistakes
				//for(;logicalBlockIndex!=SAT[poolId].end();logicalBlockIndex++)
				//for(;logicalBlockIndex<SAT[poolId].size();logicalBlockIndex++)
					SAT[poolId][t].logicalStart+=bCount;
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
				logicalBlockIndex=SAT[poolId].size()-1;
				//logicalBlockIndex=prev(SAT[poolId].end());
				//if((SAT[poolId][logicalBlockIndex].logicalStart+SAT[poolId][logicalBlockIndex].size)!=bWhere)
				if((SAT[poolId][logicalBlockIndex].logicalStart+SAT[poolId][logicalBlockIndex].size)!=bWhere)
				{
					printf("huh??\n");
					exit(0);
				}
			}
		}


		// create all the new logical and physical blocks needed to fill the gap from the split of the old block

		RLogicalBlock newLogicalBlock;
		bool finalOne=false;
		bool previousReturnedEOF=false;
		p_addr_t _blockFileSize=blockFile.getSize();//this->blockFileSize;
		for(size_t t=0;t<=newLogicalBlockCount;t++)
		{
			if(t<newLogicalBlockCount)
				newLogicalBlock.size=maxBlockSize;
			else
			{
				newLogicalBlock.size=bCount%maxBlockSize;
				if(newLogicalBlock.size==0)
					break;

				//if(t==0 && !didSplitOne && logicalBlockIndex!=physicalBlockList.end() && bPoolSize>0)
				if(t==0 && !didSplitOne && logicalBlockIndex!=physicalBlockList.size() && bPoolSize>0)
				{
					// if this less-than-max block is the only block being added
					// and it didn't cause a split up above, then lets see if it
					// can fit at the end of the block it didn't split. (appending optimization)

					RLogicalBlock &logicalBlock=SAT[poolId][logicalBlockIndex];
					if((logicalBlock.size+newLogicalBlock.size)<=maxBlockSize)
					{	// can fit IF there is room after the hole after this logicalBlock
						/*
						size_t physicalBlockIndex;
						if(!physicalBlockList.contains(RPhysicalBlock(logicalBlock.physicalStart,0),physicalBlockIndex))
						{
							printf("error finding physical block\n");
							exit(0);
						}
						*/
						//set<RPhysicalBlock>::iterator physicalBlockIndex=physicalBlockList.find(RPhysicalBlock(logicalBlock.physicalStart));
						const size_t physicalBlockIndex=lower_bound(physicalBlockList.begin(),physicalBlockList.end(),RPhysicalBlock(logicalBlock.physicalStart))-physicalBlockList.begin();
						/*
						if(physicalBlockIndex==physicalBlockList.end())
						{
							printf("error finding physical block\n");
							exit(0);
						}
						*/

						/*
						if(	physicalBlockIndex==physicalBlockList.end() || 
							next(physicalBlockIndex)==physicalBlockList.end() || 
							((next(physicalBlockIndex)->physicalStart-physicalBlockIndex->physicalStart)-physicalBlockIndex->size)>=newLogicalBlock.size
						)
						*/
						if( (physicalBlockIndex+1)>=physicalBlockList.size() ||
						    ((physicalBlockList[physicalBlockIndex+1].physicalStart-physicalBlockList[physicalBlockIndex].physicalStart)-physicalBlockList[physicalBlockIndex].size)>=newLogicalBlock.size
						  )
						{	// we can use this block
							logicalBlock.size+=newLogicalBlock.size;
							physicalBlockList[physicalBlockIndex].size+=newLogicalBlock.size;
							pools[poolId].size+=newLogicalBlock.size;
							break;
						}
					}
				}
				finalOne=true;
			}

			newLogicalBlock.logicalStart=bWhere+(t*maxBlockSize);

			// This optimization says that if we aren't doing the last piece of the insertion
			// and the previous call to findHole failed to return a hole within the blockFile
			// the assume it's gonna do it again, so don't call findHole
			if(!finalOne && previousReturnedEOF)
				newLogicalBlock.physicalStart=_blockFileSize;
			else
			{
				newLogicalBlock.physicalStart=findHole(newLogicalBlock.size);

				if(newLogicalBlock.physicalStart==_blockFileSize )
					previousReturnedEOF=true;
			}

			/*
			// && some ratio is too high
			{ // having to create more file space and file is getting too big, so coeless
				newLogicalBlock.physicalStart=makeHole(newLogicalBlock.size);
			}
			*/

			// overflow??? (probably shouldn't if I know that I didn't let the file get past maxLogicalAddress in the initial bounds checking
			if((newLogicalBlock.physicalStart+newLogicalBlock.size)>_blockFileSize)
				_blockFileSize+=(newLogicalBlock.physicalStart+newLogicalBlock.size)-_blockFileSize;
			// we should check if the file will be able to grow bigger actually on disk later... ???


			/*
			if(!SAT[poolId].insert(newLogicalBlock).second)
			{
				printf("error adding to SAT\n");
				exit(1);
			}
			*/
			sortedInsert(SAT[poolId],newLogicalBlock);

			/*
			if(!physicalBlockList.insert(RPhysicalBlock(newLogicalBlock)).second)
			{
				// if this error occurs.. we need to remove from SAT list or something
				printf("error adding to physicalBlockList\n");
				exit(1);
			}
			*/
			sortedInsert(physicalBlockList,RPhysicalBlock(newLogicalBlock));

			pools[poolId].size+=newLogicalBlock.size;
		}
		makeBlockFileSmallest();

		// ZZZ ??? Takes too long in a real-time situation (i.e. Recording in ReZound)
		//backupSAT();

		//writeStructureInfoUnlock();
	}
	catch(...)
	{
		//writeStructureInfoUnlock();
		throw;
	}
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::removeSpace(const poolId_t poolId,const l_addr_t peWhere,const l_addr_t peCount)
{
	if(!opened)
		throw(runtime_error(string(__func__)+" -- no file is open"));
	if(peCount==0)
		return;

	//writeStructureInfoLock();
	try
	{
		// validate the parameters
		if(pools.size()<=poolId)
			throw(runtime_error(string(__func__)+" -- invalid poolId: "+istring(poolId)));

		const alignment_t bAlignment=pools[poolId].alignment;
		const l_addr_t bPoolSize=pools[poolId].size;
		const l_addr_t pePoolSize=bPoolSize/bAlignment;

		if(peWhere>=pePoolSize)
			throw(runtime_error(string(__func__)+" -- out of range peWhere "+istring(peWhere)+" for pool ("+getPoolDescription(poolId)+")"));
		if(pePoolSize-peWhere<peCount)
			throw(runtime_error(string(__func__)+" -- out of range peWhere "+istring(peWhere)+" and peCount "+istring(peCount)+" for pool ("+getPoolDescription(poolId)+")"));

		invalidateAllCachedBlocks();

		const l_addr_t bWhere=peWhere*bAlignment;
		const l_addr_t bCount=peCount*bAlignment;

		//const l_addr_t maxBlockSize=getMaxBlockSizeFromAlignment(bAlignment);

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

			// ??? Optimization: I could probably most of the time assume its the next block the previous iteration of this loop... if I checked that and it was false, then I should do a search
			/*
			size_t physicalBlockIndex;
			if(!physicalBlockList.contains(RPhysicalBlock(block.physicalStart,0),physicalBlockIndex))
			{
				printf("inconsistances in SAT and physicalBlockList\n");
				exit(1);
			}
			*/
			const typename vector<RPhysicalBlock>::iterator physicalBlockIndex=lower_bound(physicalBlockList.begin(),physicalBlockList.end(),RPhysicalBlock(block.physicalStart));

			if(remove_start==block_start && remove_end==block_end)
			{ // case 1 -- remove whole block -- on first and only block, middle or last block
				// |[.......]|	([..] -- block ; |..| -- section to remove)

				//SAT[poolId].remove(t);
				SAT[poolId].erase(SAT[poolId].begin()+t);
				//physicalBlockList.remove(physicalBlockIndex);
				physicalBlockList.erase(physicalBlockIndex);
			}
			else if(remove_start==block_start && remove_end<block_end)
			{ // case 2 -- remove a head of block -- on first and only block, last block
				// |[.....|..]

				
				/* I think I can put this back in now that I know it wasn't the problem, instead of doing what's below (removing and adding to the list)
				 * 	I need a testing utility that tests call methods randomly with random positions and should test this code and put it back in if it works
				RPhysicalBlock &newPhysicalBlock=physicalBlockList[physicalBlockIndex];

				block.logicalStart-=bCount-remove_in_block_size; // do this because, this block is not gonna be affected by the loop which does this later since we increment t
				newPhysicalBlock.physicalStart=block.physicalStart=(block.physicalStart+remove_in_block_size);
				newPhysicalBlock.size=block.size=(block.size-remove_in_block_size);
				*/

				// I remove and re-add the logical block to the SAT list because it's logical start may get out of order
				// ??? I shouldn't have to do the physical block this way I don't think

				RLogicalBlock newLogicalBlock(block);
				newLogicalBlock.size-=remove_in_block_size;
				newLogicalBlock.physicalStart+=remove_in_block_size;
				newLogicalBlock.logicalStart-=bCount-remove_in_block_size; // do this because, this block is not gonna be affected by the loop which does this later since we increment t

				//SAT[poolId].remove(t);
				SAT[poolId].erase(SAT[poolId].begin()+t);
				//physicalBlockList.remove(physicalBlockIndex);
				physicalBlockList.erase(physicalBlockIndex);

				/*
				if(!SAT[poolId].add(newLogicalBlock))
				{
					printf("error adding to SAT\n");
					exit(1);
				}
				*/
				sortedInsert(SAT[poolId],newLogicalBlock);

				/*
				if(!physicalBlockList.add(RPhysicalBlock(newLogicalBlock)))
				{
					printf("error adding to physicalBlockList\n");
					exit(1);
				}
				*/
				sortedInsert(physicalBlockList,RPhysicalBlock(newLogicalBlock));

				t++;
			}
			else if(remove_start>block_start && remove_end==block_end)
			{ // case 3 -- remove a tail of block -- first and only block, first block
				// [..|.....]|
				block.size-=remove_in_block_size;
				//physicalBlockList[physicalBlockIndex].size=block.size;
				physicalBlockIndex->size=block.size;
				t++;
			}
			else if(remove_start>block_start && remove_end<block_end)
			{ // case 4 -- split block -- on first and only block
				// [..|...|..]
				
				const l_addr_t newLogicalBlockSize=block_end-remove_end;
				//physicalBlockList[physicalBlockIndex].size=block.size=remove_start-block_start;
				physicalBlockIndex->size=block.size=remove_start-block_start;

				RLogicalBlock newLogicalBlock;
				newLogicalBlock.logicalStart=remove_start;
				newLogicalBlock.physicalStart=block.physicalStart+block.size+remove_in_block_size;
				newLogicalBlock.size=newLogicalBlockSize;
				/*
				if(!SAT[poolId].add(newLogicalBlock))
				{
					printf("error adding to SAT\n");
					exit(1);
				}
				*/
				sortedInsert(SAT[poolId],newLogicalBlock);
				/*
				if(!physicalBlockList.add(RPhysicalBlock(newLogicalBlock)))
				{
					printf("error adding to physicalBlockList\n");
					exit(1);
				}
				*/
				sortedInsert(physicalBlockList,RPhysicalBlock(newLogicalBlock));
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
		for(;t<SAT[poolId].size();t++)
			SAT[poolId][t].logicalStart-=bCount;

		// join the blocks that just became adjacent if possible
		joinAdjacentBlocks(poolId,logicalBlockIndex>0 ? logicalBlockIndex-1 : 0,1);
		
		makeBlockFileSmallest();

		backupSAT();

		//writeStructureInfoUnlock();
	}
	catch(...)
	{
		//writeStructureInfoUnlock();
		throw;
	}
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::moveData(const poolId_t destPoolId,const l_addr_t peDestWhere,const poolId_t srcPoolId,const l_addr_t peSrcWhere,const l_addr_t peCount)
{
	/*
	 * Basically, this logic works the same as removeSpace except every time it removes some
	 * space from the src pool, it adds it to the dest pool
	 */

	if(!opened)
		throw(runtime_error(string(__func__)+" -- no file is open"));
	if(peCount==0)
		return;

	//writeStructureInfoLock();
	try
	{
		// validate the parameters
		if(srcPoolId>=pools.size())
			throw(runtime_error(string(__func__)+" -- invalid srcPoolId: "+istring(srcPoolId)));
		if(destPoolId>=pools.size())
			throw(runtime_error(string(__func__)+" -- invalid destPoolId: "+istring(destPoolId)));

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
				moveData(tempPoolId,0,srcPoolId,peSrcWhere,peCount);
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
			throw(runtime_error(string(__func__)+" -- alignments do not match for srcPool ("+getPoolDescription(srcPoolId)+") and destPool ("+getPoolDescription(destPoolId)+")"));

		const l_addr_t bSrcPoolSize=pools[srcPoolId].size;
		const l_addr_t peSrcPoolSize=bSrcPoolSize/bAlignment;

		if(peSrcWhere>=peSrcPoolSize)
			throw(runtime_error(string(__func__)+" -- out of range peSrcWhere "+istring(peSrcWhere)+" for srcPool ("+getPoolDescription(srcPoolId)+")"));
		if(peSrcPoolSize-peSrcWhere<peCount)
			throw(runtime_error(string(__func__)+" -- out of range peSrcWhere "+istring(peSrcWhere)+" and peCount "+istring(peCount)+" for pool ("+getPoolDescription(srcPoolId)+")"));


		const l_addr_t bDestPoolSize=pools[destPoolId].size;
		const l_addr_t peDestPoolSize=bDestPoolSize/bAlignment;

		const l_addr_t pePoolMaxSize=maxLogicalAddress/bAlignment;


		if(pePoolMaxSize-peDestPoolSize<peCount)
			throw(runtime_error(string(__func__)+" -- insufficient logical address space to insert "+istring(peCount)+" elements into destPool ("+getPoolDescription(destPoolId)+")"));
		if(peDestWhere>peDestPoolSize)
			throw(runtime_error(string(__func__)+" -- out of range peDestWhere "+istring(peDestWhere)+" for pool ("+getPoolDescription(destPoolId)+")"));

		invalidateAllCachedBlocks();

		const l_addr_t bSrcWhere=peSrcWhere*bAlignment;
		l_addr_t bDestWhere=peDestWhere*bAlignment;
		const l_addr_t bCount=peCount*bAlignment;

		//const l_addr_t maxBlockSize=getMaxBlockSizeFromAlignment(bAlignment);


		bool atStartOfSrcBlock;
		const size_t srcBlockIndex=findSATBlockContaining(srcPoolId,bSrcWhere,atStartOfSrcBlock);

		size_t destBlockIndex=0;
		if(bDestWhere<bDestPoolSize)
		{
			bool atStartOfDestBlock;
			destBlockIndex=findSATBlockContaining(destPoolId,bDestWhere,atStartOfDestBlock);
			if(!atStartOfDestBlock)
			{ // go ahead and split the destination block so that we can simply insert new ones along the way

				if(bDestWhere<=SAT[destPoolId][destBlockIndex].logicalStart)
				{ // logcal impossibility since atStartOfBlock wasn't true (unless it was wrong)
					printf("oops...\n");
					exit(1);
				}

				const l_addr_t firstPartSize=bDestWhere-SAT[destPoolId][destBlockIndex].logicalStart;
				const l_addr_t secondPartSize=SAT[destPoolId][destBlockIndex].size-firstPartSize;


				// find the physical block for this logical block
				/*
				RPhysicalBlock searchPhysicalBlock;
				size_t physicalBlockIndex;
				searchPhysicalBlock.physicalStart=SAT[destPoolId][destBlockIndex].physicalStart;
				if(!physicalBlockList.contains(searchPhysicalBlock,physicalBlockIndex))
				{
					printf("physical block list inconsistancies...\n");
					exit(1);
				}
				*/
				const typename vector<RPhysicalBlock>::iterator physicalBlockIndex=lower_bound(physicalBlockList.begin(),physicalBlockList.end(),RPhysicalBlock(SAT[destPoolId][destBlockIndex].physicalStart));

				// shrink the logical and physical blocks' sizes
				//physicalBlockList[physicalBlockIndex].size=SAT[destPoolId][destBlockIndex].size=firstPartSize;
				physicalBlockIndex->size=SAT[destPoolId][destBlockIndex].size=firstPartSize;

				// create the new logical and physical blocks which are the second part of the old blocks
				RLogicalBlock newLogicalBlock;
				newLogicalBlock.logicalStart=SAT[destPoolId][destBlockIndex].logicalStart+firstPartSize;
				newLogicalBlock.physicalStart=SAT[destPoolId][destBlockIndex].physicalStart+firstPartSize;
				newLogicalBlock.size=secondPartSize;

				// add the new logical block
				/*
				if(!SAT[destPoolId].add(newLogicalBlock))
				{	// inconsistancies
					printf("error adding to SAT\n");
					exit(1);
				}
				*/
				sortedInsert(SAT[destPoolId],newLogicalBlock);

				// add the new physical block
				/*
				if(!physicalBlockList.add(RPhysicalBlock(newLogicalBlock)))
				{	// error
					printf("error adding to physical block list\n");
					exit(1);
				}
				*/
				sortedInsert(physicalBlockList,RPhysicalBlock(newLogicalBlock));

				destBlockIndex++;
			}

			// move upward all the logicalStarts in the dest pool past the destination where point
			for(size_t t=destBlockIndex;t<SAT[destPoolId].size();t++)
				SAT[destPoolId][t].logicalStart+=bCount;

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

			// ??? Optimization: I could probably most of the time assume its the next block the previous iteration of this loop... if I checked that and it was false, then I should do a search
			/*
			size_t physicalBlockIndex;
			if(!physicalBlockList.contains(RPhysicalBlock(srcBlock.physicalStart,0),physicalBlockIndex))
			{
				printf("inconsistances in SAT and physicalBlockList\n");
				exit(1);
			}
			*/
			const typename vector<RPhysicalBlock>::iterator physicalBlockIndex=lower_bound(physicalBlockList.begin(),physicalBlockList.end(),RPhysicalBlock(srcBlock.physicalStart));

			if(src_remove_start==src_block_start && src_remove_end==src_block_end)
			{ // case 1 -- remove whole block from src pool -- on first and only block, middle or last block
				// |[.......]|	([..] -- block ; |..| -- section to remove)
				
				RLogicalBlock newDestLogicalBlock(srcBlock);
				newDestLogicalBlock.logicalStart=bDestWhere;

				//SAT[srcPoolId].remove(src_t);
				SAT[srcPoolId].erase(SAT[srcPoolId].begin()+src_t);

				/*
				if(!SAT[destPoolId].add(newDestLogicalBlock))
				{	// inconsistancies
					printf("error adding to SAT\n");
					exit(1);
				}
				*/
				sortedInsert(SAT[destPoolId],newDestLogicalBlock);

			}
			else if(src_remove_start==src_block_start && src_remove_end<src_block_end)
			{ // case 2 -- remove a head of block -- on first and only block, last block
				// |[.....|..]

				/* I think I can put this back in instead of doing what I do below (that is, removing and adding the block)
				// create the new logical block in dest pool and new physical block
				RLogicalBlock newDestLogicalBlock;
				newDestLogicalBlock.logicalStart=bDestWhere;
				newDestLogicalBlock.size=remove_in_src_block_size;
				newDestLogicalBlock.physicalStart=srcBlock.physicalStart;

				// simply modify the src block and physical block's sizes and starts
				RPhysicalBlock &srcPhysicalBlock=physicalBlockList[physicalBlockIndex];
				srcBlock.logicalStart-=bCount-remove_in_src_block_size; // do this because, this srcBlock is not gonna be affected by the loop which does this later since we increment t
				srcPhysicalBlock.physicalStart=srcBlock.physicalStart=(srcBlock.physicalStart+remove_in_src_block_size);
				srcPhysicalBlock.size=srcBlock.size=(srcBlock.size-remove_in_src_block_size);

				// actually add the new logical and physical blocks (had to delay because of unique constraints)
				if(!SAT[destPoolId].add(newDestLogicalBlock))
				{
					printf("error adding to dest SAT\n");
					exit(1);
				}
				if(!physicalBlockList.add(RPhysicalBlock(newDestLogicalBlock)))
				{
					printf("error adding to physicalBlockList\n");
					exit(1);
				}
				*/

				// create the new logical block in dest pool and new physical block
				RLogicalBlock newDestLogicalBlock; 
				newDestLogicalBlock.logicalStart=bDestWhere;
				newDestLogicalBlock.size=remove_in_src_block_size;
				newDestLogicalBlock.physicalStart=srcBlock.physicalStart;


				// I remove and re-add the logical block to the SAT list because it's logical start may get out of order
				// ??? I shouldn't have to do the physical block this way I don't think

				RLogicalBlock newSrcLogicalBlock(srcBlock);
				newSrcLogicalBlock.size-=remove_in_src_block_size;
				newSrcLogicalBlock.physicalStart+=remove_in_src_block_size;
				newSrcLogicalBlock.logicalStart-=bCount-remove_in_src_block_size; // do this because, this block is not gonna be affected by the loop which does this later since we increment t

				//SAT[srcPoolId].remove(src_t);
				SAT[srcPoolId].erase(SAT[srcPoolId].begin()+src_t);
				//physicalBlockList.remove(physicalBlockIndex);
				physicalBlockList.erase(physicalBlockIndex);

				/*
				if(!SAT[srcPoolId].add(newSrcLogicalBlock))
				{
					printf("error adding to SAT\n");
					exit(1);
				}
				*/
				sortedInsert(SAT[srcPoolId],newSrcLogicalBlock);

				/*
				if(!physicalBlockList.add(RPhysicalBlock(newSrcLogicalBlock)))
				{
					printf("error adding to physicalBlockList\n");
					exit(1);
				}
				*/
				sortedInsert(physicalBlockList,RPhysicalBlock(newSrcLogicalBlock));

				// actually add the new logical and physical blocks (had to delay because of unique constraints)
				/*
				if(!SAT[destPoolId].add(newDestLogicalBlock))
				{
					printf("error adding to dest SAT\n");
					exit(1);
				}
				*/
				sortedInsert(SAT[destPoolId],newDestLogicalBlock);
				/*
				if(!physicalBlockList.add(RPhysicalBlock(newDestLogicalBlock)))
				{
					printf("error adding to physicalBlockList\n");
					exit(1);
				}
				*/
				sortedInsert(physicalBlockList,RPhysicalBlock(newDestLogicalBlock));

				src_t++;
			}
			else if(src_remove_start>src_block_start && src_remove_end==src_block_end)
			{ // case 3 -- remove a tail of block -- first and only block, first block
				// [..|.....]|
				
				// create the new logcal block for dest pool and a new physical block
				RLogicalBlock newDestLogicalBlock;
				newDestLogicalBlock.logicalStart=bDestWhere;
				newDestLogicalBlock.size=remove_in_src_block_size;
				newDestLogicalBlock.physicalStart=(srcBlock.physicalStart+(srcBlock.size-remove_in_src_block_size));

				// actually add the new logical and physical blocks
				/*
				if(!SAT[destPoolId].add(newDestLogicalBlock))
				{
					printf("error adding to dest SAT\n");
					exit(1);
				}
				*/
				sortedInsert(SAT[destPoolId],newDestLogicalBlock);
				/*
				if(!physicalBlockList.add(RPhysicalBlock(newDestLogicalBlock)))
				{
					printf("error adding to physicalBlockList\n");
					exit(1);
				}
				*/
				sortedInsert(physicalBlockList,RPhysicalBlock(newDestLogicalBlock));

				// simply modify the logcal and physcal blocks
				srcBlock.size-=remove_in_src_block_size;
				//physicalBlockList[physicalBlockIndex].size=srcBlock.size;
				physicalBlockIndex->size=srcBlock.size;

				src_t++;
			}
			else if(src_remove_start>src_block_start && src_remove_end<src_block_end)
			{ // case 4 -- split block -- on first and only block
				// [..|...|..]
			
				const l_addr_t headSize=src_remove_start-src_block_start; // part at the beginning of the block


				// insert new logical block in dest pool and create new physical block
				RLogicalBlock newDestLogicalBlock;
				newDestLogicalBlock.logicalStart=bDestWhere;
				newDestLogicalBlock.physicalStart=srcBlock.physicalStart+headSize;
				newDestLogicalBlock.size=src_remove_end-src_remove_start+1;

				/*
				if(!SAT[destPoolId].add(newDestLogicalBlock))
				{
					printf("error adding to dest SAT\n");
					exit(1);
				}
				*/
				sortedInsert(SAT[destPoolId],newDestLogicalBlock);
				/*
				if(!physicalBlockList.add(RPhysicalBlock(newDestLogicalBlock)))
				{
					printf("error adding to physicalBlockList\n");
					exit(1);
				}
				*/
				sortedInsert(physicalBlockList,RPhysicalBlock(newDestLogicalBlock));

				// modify logical and physical src blocks' sizes
				//physicalBlockList[physicalBlockIndex].size=srcBlock.size=headSize;
				physicalBlockIndex->size=srcBlock.size=headSize;

				// create new block in src pool and new physical block
				RLogicalBlock newSrcLogicalBlock;
				newSrcLogicalBlock.logicalStart=src_remove_start;
				newSrcLogicalBlock.physicalStart=srcBlock.physicalStart+srcBlock.size+remove_in_src_block_size;
				newSrcLogicalBlock.size=src_block_end-src_remove_end;

				/*
				if(!SAT[srcPoolId].add(newSrcLogicalBlock))
				{
					printf("error adding to src SAT\n");
					exit(1);
				}
				*/
				sortedInsert(SAT[srcPoolId],newSrcLogicalBlock);
				/*
				if(!physicalBlockList.add(RPhysicalBlock(newSrcLogicalBlock)))
				{
					printf("error adding to physicalBlockList\n");
					exit(1);
				}
				*/
				sortedInsert(physicalBlockList,RPhysicalBlock(newSrcLogicalBlock));

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
		for(;src_t<SAT[srcPoolId].size();src_t++)
			SAT[srcPoolId][src_t].logicalStart-=bCount;

		/* ??? don't I want to do this?
		// join any adjacent blocks in the dest pool than could be
		joinAdjacentBlocks(destPoolId,destBlockIndex>0 ? destBlockIndex-1 : 0,loopCount+1);

		// join the blocks that just became adjacent in the src pool if possible
		joinAdjacentBlocks(srcPoolId,srcBlockIndex>0 ? srcBlockIndex-1 : 0,1);
		*/

		backupSAT();

		//writeStructureInfoUnlock();
	}
	catch(...)
	{
		//writeStructureInfoUnlock();
		throw;
	}
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::clearPool(const poolId_t poolId)
{
	if(!opened)
		throw(runtime_error(string(__func__)+" -- no file is open"));
	if(poolId>=pools.size())
		throw(runtime_error(string(__func__)+" -- invalid poolId: "+istring(poolId)));

	invalidateAllCachedBlocks();

	// remove all the entries in the physicalBlockList associated with this pool
	for(size_t t=0;t<SAT[poolId].size();t++)
	{
		const typename vector<RPhysicalBlock>::iterator physicalBlockIndex=lower_bound(physicalBlockList.begin(),physicalBlockList.end(),RPhysicalBlock(SAT[poolId][t].physicalStart));
		physicalBlockList.erase(physicalBlockIndex);
	}

	// remove all localBlocks in the SAT associated with this pool 
	SAT[poolId].clear();

	pools[poolId].size=0;

	makeBlockFileSmallest();

	backupSAT();
}


// Pool Data Access
template<class l_addr_t,class p_addr_t>
	template<class pool_element_t> void TPoolFile<l_addr_t,p_addr_t>::cacheBlock(const l_addr_t peWhere,const TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > *accesser)
{
	lockAccesserInfo();
	//readStructureInfoLock();
	try
	{
		const poolId_t poolId=accesser->poolId;

		if(peWhere>=maxLogicalAddress/sizeof(pool_element_t))
			throw(runtime_error(string(__func__)+" -- invalid peWhere "+istring(peWhere)+" for pool ("+getPoolDescription(poolId)+")"));

		const l_addr_t byteWhere=peWhere*sizeof(pool_element_t);

		unreferenceCachedBlock(accesser);

		RCachedBlock *found=NULL;

		// ??? Optimization: if there were many many pools, I could make this a TDimableList by poolId to eliminate looking through all of them
		// look to see if this block is already cached in the active cached blocks
		//STL TStackQueueIterator<RCachedBlock *> i=activeCachedBlocks.getIterator();
		for(typename deque<RCachedBlock *>::iterator i=activeCachedBlocks.begin();i!=activeCachedBlocks.end();i++)
		{
			RCachedBlock *cachedBlock=(*i);
			if(cachedBlock->poolId==poolId && cachedBlock->containsAddress(byteWhere))
			{
				found=cachedBlock;
				break;
			}
		}

		// ??? Optimization: if there were many many pools, I could make this a TDimableList by poolId to eliminate looking through all of them
		// if not found then look to see if this block is already cached in the unreferenced cached blocks
		if(found==NULL)
		{
			//STL TStackQueueIterator<RCachedBlock *> i=unreferencedCachedBlocks.getIterator();
			//while(!i.atEnd())
			for(typename deque<RCachedBlock *>::iterator i=unreferencedCachedBlocks.begin();i!=unreferencedCachedBlocks.end();i++)
			{
				RCachedBlock *cachedBlock=(*i);
				if(cachedBlock->poolId==poolId && cachedBlock->containsAddress(byteWhere))
				{
					found=cachedBlock;
					//i.removeCurrent();
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
					throw(runtime_error(string(__func__)+" -- invalid peWhere "+istring(peWhere)+" for pool ("+getPoolDescription(poolId)+")"));
				}

				bool dummy;
				//const typename set<RLogicalBlock>::const_iterator SATIndex=findSATBlockContaining(poolId,byteWhere,dummy);
				const size_t SATIndex=findSATBlockContaining(poolId,byteWhere,dummy);

				// use an unused one if available
				if(!unusedCachedBlocks.empty())
				{
					//found=unusedCachedBlocks.dequeue();
					found=unusedCachedBlocks.front();
					unusedCachedBlocks.pop();
				}

				if(found==NULL)
				{	// invalidate an unreferenced one or create a new one if none are available
					if(!unreferencedCachedBlocks.empty())
					{	// create an unused one from the unreferencedCachedBlocks
						invalidateCachedBlock((found=unreferencedCachedBlocks.front()));
						// ??? sanity check
						if(unusedCachedBlocks.empty())
						{
							printf("what? one's supposed to be there now...\n");
							exit(0);
						}
						found=unusedCachedBlocks.front();
						unusedCachedBlocks.pop();
					}
					else
						found=new RCachedBlock(maxBlockSize);
				}

				// could check of found==NULL here... error if so

				//blockFile.read(found->buffer,SATIndex->size,SATIndex->physicalStart+LEADING_DATA_SIZE);
				blockFile.read(found->buffer,SAT[poolId][SATIndex].size,SAT[poolId][SATIndex].physicalStart+LEADING_DATA_SIZE);

				// initialize the cachedBlock
				//found->init(poolId,SATIndex->logicalStart,SATIndex->size);
				found->init(poolId,SAT[poolId][SATIndex].logicalStart,SAT[poolId][SATIndex].size);
			}
			activeCachedBlocks.push_back(found);
		}

		found->referenceCount++;
		//accesser->startAddress=(found->logicalStart)/getPoolAlignment(accesser->poolId);
		accesser->startAddress=(found->logicalStart)/sizeof(pool_element_t);
		//accesser->endAddress=((found->logicalStart+found->size)/getPoolAlignment(accesser->poolId))-1;
		accesser->endAddress=((found->logicalStart+found->size)/sizeof(pool_element_t))-1;
		accesser->cacheBuffer=(pool_element_t *)(found->buffer);
		accesser->dirty=false;
		accesser->cachedBlock=found;

		//readStructureInfoUnlock();
		unlockAccesserInfo();
	}
	catch(...)
	{
		//readStructureInfoUnlock();
		unlockAccesserInfo();
		throw;
	}
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
		//const typename set<RLogicalBlock>::const_iterator SATIndex=findSATBlockContaining(cachedBlock->poolId,cachedBlock->logicalStart,atStartOfBlock);
		size_t SATIndex=findSATBlockContaining(cachedBlock->poolId,cachedBlock->logicalStart,atStartOfBlock);
		// if atStartOfBlock is not true.. problem!!!
		//blockFile.write(cachedBlock->buffer,SATIndex->size,SATIndex->physicalStart+LEADING_DATA_SIZE);
		blockFile.write(cachedBlock->buffer,SAT[cachedBlock->poolId][SATIndex].size,SAT[cachedBlock->poolId][SATIndex].physicalStart+LEADING_DATA_SIZE);
	}

	// the cached block structure is now unreferenced and unused
	typename deque<RCachedBlock *>::iterator i=find(unreferencedCachedBlocks.begin(),unreferencedCachedBlocks.end(),cachedBlock);
	if(i!=unreferencedCachedBlocks.end())
	{
		unreferencedCachedBlocks.erase(i);
		unusedCachedBlocks.push(cachedBlock);
	}
	else
	{
		i=find(activeCachedBlocks.begin(),activeCachedBlocks.end(),cachedBlock);
		if(i!=activeCachedBlocks.end())
			activeCachedBlocks.erase(i);
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
			// could check the return value
			typename deque<RCachedBlock *>::iterator i=find(activeCachedBlocks.begin(),activeCachedBlocks.end(),cachedBlock);
			if(i!=activeCachedBlocks.end())
				activeCachedBlocks.erase(i);
			unreferencedCachedBlocks.push_back(cachedBlock);
		}
		accesser->init();
	}
}

/*
template<class l_addr_t,class p_addr_t>
	template<class pool_element_t> const size_t TPoolFile<l_addr_t,p_addr_t>::findAccesserIndex(const TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > *accesser,const bool throwIfNotFound)
{
	size_t foundAt;
	if(accessers.contains(accesser,foundAt))
		return(foundAt);
	if(throwIfNotFound)
		throw(runtime_error(string(__func__)+" -- error..."));
	return(NIL_INDEX);
}
*/

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::invalidateAllCachedBlocks()
{
	lockAccesserInfo();
	try
	{
		while(!activeCachedBlocks.empty())
			invalidateCachedBlock(activeCachedBlocks.front());
		while(!unreferencedCachedBlocks.empty())
			invalidateCachedBlock(unreferencedCachedBlocks.front());
		unlockAccesserInfo();
	}
	catch(...)
	{
		unlockAccesserInfo();
		throw;
	}
}

template<class l_addr_t,class p_addr_t>
	template<class pool_element_t> void TPoolFile<l_addr_t,p_addr_t>::addAccesser(const TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > *accesser)
{
	lockAccesserInfo();
	try
	{
		accessers.push_back((const CGenericPoolAccesser *)accesser);
		// ??? I used to make sure that it wasn't already there
		unlockAccesserInfo();
	}
	catch(...)
	{
		unlockAccesserInfo();
		throw;
	}
}

template<class l_addr_t,class p_addr_t>
	template<class pool_element_t> void TPoolFile<l_addr_t,p_addr_t>::removeAccesser(const TStaticPoolAccesser<pool_element_t,TPoolFile<l_addr_t,p_addr_t> > *accesser)
{
	//readStructureInfoLock();
	lockAccesserInfo();
	try
	{
		invalidateAccesser(accesser);

		//const size_t accesserIndex=findAccesserIndex((const CGenericPoolAccesser *)accesser,false);
		//if(accesserIndex!=NIL_INDEX)
		//						??? see about changing this to lower_bound... figure out just if lower_bound-1 or upper_bound-1 should be used
		typename vector<const CGenericPoolAccesser *>::iterator i=find(accessers.begin(),accessers.end(),(const CGenericPoolAccesser *)accesser);
		if(i!=accessers.end())
			accessers.erase(i);
		unlockAccesserInfo();
		//readStructureInfoUnlock();
	}
	catch(...)
	{
		unlockAccesserInfo();
		//readStructureInfoUnlock();
		throw;
	}
}





// defragging
template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::defrag()
{
	invalidateAllCachedBlocks();

	/* ???
	 * Right now, defragging can happen and be much more inefficiant than it needs to be.
	 * This is because it always makes the first poolId come first in the file.  Fragementation
	 * should not care which order than the pools exist, only that they are contiguous on
	 * disk.  I would do better to move as few blocks as possible, this would mean knowing
	 * which order is best for the pools on disk.  Perhaps I could go thru each permutation of
	 * poolId, but I'm not quite sure how do know which is the best ordering. 
	 */

	// to defrag, correct all block positions
	for(size_t poolId=0;poolId<pools.size();poolId++)
	{
		for(size_t t=0;t<SAT[poolId].size();t++)
	 	//for(typename set<RLogicalBlock>::const_iterator t=SAT[poolId].begin();t!=SAT[poolId].end();t++)
			correctBlockPosition(poolId,t,getProceedingPoolSizes(poolId));
	}
	verifyAllBlockInfo(true);

	// rebuild SAT and physicalBlockList
	createContiguousSAT();
	backupSAT();
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::createContiguousSAT()
{
	p_addr_t accumulatePoolSize=0;

	physicalBlockList.clear();
	SAT.clear();
	for(size_t poolId=0;poolId<pools.size();poolId++)
	{
		appendNewSAT();

		const alignment_t alignment=pools[poolId].alignment;
		const l_addr_t poolSize=pools[poolId].size;
		const l_addr_t maxBlockSize=getMaxBlockSizeFromAlignment(alignment);

		const size_t blockCount=poolSize/maxBlockSize; // ??? hope that blockCount isn't too big for size_t cause in it could be
		for(size_t t=0;t<blockCount;t++)
		{
			RLogicalBlock block;
			RPhysicalBlock physicalBlock;
			block.logicalStart=t*maxBlockSize;
			physicalBlock.physicalStart=block.physicalStart=t*maxBlockSize+accumulatePoolSize;
			physicalBlock.size=block.size=maxBlockSize;

			//SAT[poolId].insert(block);
			sortedInsert(SAT[poolId],block);
			//physicalBlockList.insert(physicalBlock);
			sortedInsert(physicalBlockList,physicalBlock);
		}

		if((poolSize%maxBlockSize)!=0)
		{	// create one more block at the end
			RLogicalBlock block;
			RPhysicalBlock physicalBlock;
			block.logicalStart=blockCount*maxBlockSize;
			physicalBlock.physicalStart=block.physicalStart=blockCount*maxBlockSize+accumulatePoolSize;
			physicalBlock.size=block.size=poolSize%maxBlockSize;

			//SAT[poolId].insert(block);
			sortedInsert(SAT[poolId],block);
			//physicalBlockList.insert(physicalBlock);
			sortedInsert(physicalBlockList,physicalBlock);
		}
		accumulatePoolSize+=poolSize;
	}
	makeBlockFileSmallest();
}

template<class l_addr_t,class p_addr_t>
	//void TPoolFile<l_addr_t,p_addr_t>::correctBlockPosition(const poolId_t poolId,const set<RLogicalBlock>::const_iterator logicalBlockIndex,const p_addr_t previousPoolSizes)
	void TPoolFile<l_addr_t,p_addr_t>::correctBlockPosition(const poolId_t poolId,const size_t logicalBlockIndex,const p_addr_t previousPoolSizes)
{
	correctionsTried.clear();
	recurCorrectBlockPosition(poolId,logicalBlockIndex,previousPoolSizes);
}

template<class l_addr_t,class p_addr_t>
	//void TPoolFile<l_addr_t,p_addr_t>::recurCorrectBlockPosition(const poolId_t poolId,const set<RLogicalBlock>::const_iterator logicalBlockIndex,const p_addr_t previousPoolSizes)
	void TPoolFile<l_addr_t,p_addr_t>::recurCorrectBlockPosition(const poolId_t poolId,const size_t logicalBlockIndex,const p_addr_t previousPoolSizes)
{
	// the correct physical start of a block is its
	// logical start + (total of prev pools)

	//const RLogicalBlock &block=*logicalBlockIndex;
	RLogicalBlock &block=SAT[poolId][logicalBlockIndex];
	const p_addr_t correctStart=block.logicalStart+previousPoolSizes;
	const p_addr_t blockStart=correctStart;
	const p_addr_t blockEnd=blockStart+block.size-1;

	if(block.physicalStart!=correctStart)
	{
		//size_t k;
		typename map<poolId_t,map<l_addr_t,bool> >::iterator k;
		//if((k=correctionsTried.findItem(poolId))==DL_NOT_FOUND || correctionsTried[k].findItem(block.logicalStart)==DL_NOT_FOUND)
		if((k=correctionsTried.find(poolId))==correctionsTried.end() || k->second.find(block.logicalStart)==k->second.end())
		{ // have not tried to fix this one's position
			//correctionsTried.getOrAdd(poolId).getOrAdd(block.logicalStart);
			correctionsTried.insert(make_pair(poolId,map<l_addr_t,bool>())).first->second.insert(make_pair(block.logicalStart,false));

			// for any block in the way (and not the current block), recur
			for(size_t i=0;i<pools.size();i++)
			{
				//for(typename set<RLogicalBlock>::const_iterator t=SAT[i].begin();t!=SAT[i].end();t++)
				for(size_t t=0;t<SAT[i].size();t++)
				{
					//const RLogicalBlock &moveBlock=(*t);
					const RLogicalBlock &moveBlock=SAT[i][t];
					const p_addr_t moveBlockStart=moveBlock.physicalStart;
					const p_addr_t moveBlockEnd=moveBlockStart+moveBlock.size-1;

					if((&moveBlock)!=(&block) && isInWindow(moveBlockStart,moveBlockEnd,blockStart,blockEnd))
						recurCorrectBlockPosition(i,t,getProceedingPoolSizes(i));
				}
			}

			// if we didn't correct it in some recurrance.. move now into the correct position
			if(block.physicalStart!=correctStart)
			{
				// for any block in the way (and not the current block), move it to the END
				for(size_t i=0;i<pools.size();i++)
				{
					//for(typename set<RLogicalBlock>::const_iterator t=SAT[i].begin();t!=SAT[i].end();t++)
					for(size_t t=0;t<SAT[i].size();t++)
					{
						//const RLogicalBlock &moveBlock=(*t);
						RLogicalBlock &moveBlock=SAT[i][t];
						const p_addr_t moveBlockStart=moveBlock.physicalStart;
						const p_addr_t moveBlockEnd=moveBlockStart+moveBlock.size-1;

						if((&moveBlock)!=(&block) && isInWindow(moveBlockStart,moveBlockEnd,blockStart,blockEnd))
						{
							const p_addr_t moveToStart=blockFile.getSize();
							changeBlockFileSize(blockFile.getSize()+moveBlock.size);
							physicallyMoveBlock(moveBlock,moveToStart);
						}
					}
				}
				physicallyMoveBlock(block,correctStart);
			}
		}
		else
		{	// have tried to fix this one's position already
			// physically move blocks that are in the way to other locations not in the
			// window we wish to move "block" into.

			// for any block in the way (and not the current block), move it
			for(size_t i=0;i<pools.size();i++)
			{
				//for(typename set<RLogicalBlock>::const_iterator t=SAT[i].begin();t!=SAT[i].end();t++)
				for(size_t t=0;t<SAT[i].size();t++)
				{
					//const RLogicalBlock &moveBlock=(*t);
					RLogicalBlock &moveBlock=SAT[i][t];
					const p_addr_t moveBlockStart=moveBlock.physicalStart;
					const p_addr_t moveBlockEnd=moveBlockStart+moveBlock.size-1;

					if((&moveBlock)!=(&block) && isInWindow(moveBlockStart,moveBlockEnd,blockStart,blockEnd))
					{
						const p_addr_t moveToStart=findHole(moveBlock.size,blockStart,blockEnd);

							// ??? could overflow
						if(blockFile.getSize()<(moveToStart+moveBlock.size))
							changeBlockFileSize(moveToStart+moveBlock.size);

						physicallyMoveBlock(moveBlock,moveToStart);
					}
				}
			}

			// finally, move the block into the correct position
			physicallyMoveBlock(block,correctStart);
		}
	}
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::physicallyMoveBlock(RLogicalBlock &block,const p_addr_t physicallyWhere)
{
	// should actually move the block if the file space isn't there (mean the block wasn't even created yet)
	if((block.physicalStart+block.size)<=blockFile.getSize())
	{
		TAutoBuffer<int8_t> tempBlockSpace(maxBlockSize);
		//auto_ptr<int8_t> tempBlockSpace(new int8_t[maxBlockSize]);

		// make sure there's space to put the data onces it's read
					// ??? could overflow
		if(blockFile.getSize()<(physicallyWhere+block.size))
			changeBlockFileSize(physicallyWhere+block.size);

		/*
		size_t physicalBlockIndex;
		if(!physicalBlockList.contains(RPhysicalBlock(block.physicalStart,0),physicalBlockIndex))
		{
			printf("physicalBlockList and SAT inconsistancies\n");
			exit(1);
		}
		*/
		//typename set<RPhysicalBlock>::const_iterator physicalBlockIndex=physicalBlockList.find(RPhysicalBlock(block.physicalStart));
		const typename vector<RPhysicalBlock>::iterator physicalBlockIndex=lower_bound(physicalBlockList.begin(),physicalBlockList.end(),RPhysicalBlock(block.physicalStart));
		if(physicalBlockIndex==physicalBlockList.end())
		{
			printf("physicalBlockList and SAT inconsistancies\n");
			exit(1);
		}

		// read block
		blockFile.read(tempBlockSpace,block.size,block.physicalStart+LEADING_DATA_SIZE);

		// write block
		blockFile.write(tempBlockSpace,block.size,physicallyWhere+LEADING_DATA_SIZE);

		block.physicalStart=physicallyWhere;

		// update block information lists
		physicalBlockList.erase(physicalBlockIndex);
		sortedInsert(physicalBlockList,RPhysicalBlock(block));
		/*
		if(!physicalBlockList.insert(RPhysicalBlock(physicallyWhere,block.size)).second)
		{
			printf("error adding to physicalBlockList\n");
			exit(1);
		}
		*/
	}
}





// ---- RPoolInfo ---------------------------------------------------------
template<class l_addr_t,class p_addr_t>
	TPoolFile<l_addr_t,p_addr_t>::RPoolInfo::RPoolInfo()
{
	size=0;
	alignment=0;
}

template<class l_addr_t,class p_addr_t>
	TPoolFile<l_addr_t,p_addr_t>::RPoolInfo::RPoolInfo(const l_addr_t _size,const alignment_t _alignment)
{
	size=_size;
	alignment=_alignment;
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
	return(*this);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::RPoolInfo::writeToFile(CMultiFile *f,CMultiFile::RHandle &multiFileHandle) const
{
	f->write(&size,sizeof(size),multiFileHandle);
	f->write(&alignment,sizeof(alignment),multiFileHandle);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::RPoolInfo::readFromFile(CMultiFile *f,CMultiFile::RHandle &multiFileHandle)
{
	f->read(&size,sizeof(size),multiFileHandle);
	f->read(&alignment,sizeof(alignment),multiFileHandle);
}




// ---- RCachedBlock ---------------------------------------------------
template<class l_addr_t,class p_addr_t>
	TPoolFile<l_addr_t,p_addr_t>::RCachedBlock::RCachedBlock(const size_t maxBlockSize)
{
	if((buffer=malloc(maxBlockSize))==NULL)
		throw(runtime_error(string(__func__)+" -- unable to allocate buffer space"));
}

template<class l_addr_t,class p_addr_t>
	TPoolFile<l_addr_t,p_addr_t>::RCachedBlock::~RCachedBlock()
{
	free(buffer);
}

template<class l_addr_t,class p_addr_t>
	bool TPoolFile<l_addr_t,p_addr_t>::RCachedBlock::containsAddress(l_addr_t where) const
{
	return(where>=logicalStart && where<(logicalStart+size));
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::RCachedBlock::init(const poolId_t _poolId,const l_addr_t _logicalStart,const l_addr_t _size)
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
	return(*this);
}

template<class l_addr_t,class p_addr_t>
	const bool TPoolFile<l_addr_t,p_addr_t>::RLogicalBlock::operator==(const RLogicalBlock &src) const
{
	if(logicalStart==src.logicalStart)
		return(true);
	return(false);
}

template<class l_addr_t,class p_addr_t>
	const bool TPoolFile<l_addr_t,p_addr_t>::RLogicalBlock::operator<(const RLogicalBlock &src) const
{
	if(src.logicalStart>logicalStart)
		return(true);
	return(false);
}

template<class l_addr_t,class p_addr_t>
	const size_t TPoolFile<l_addr_t,p_addr_t>::RLogicalBlock::getMemSize()
{
	return(sizeof(logicalStart)+sizeof(size)+sizeof(physicalStart));
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::RLogicalBlock::writeToMem(uint8_t *mem,size_t &offset) const
{
	memcpy(mem+offset,&logicalStart,sizeof(logicalStart));
	offset+=sizeof(logicalStart);

	memcpy(mem+offset,&size,sizeof(size));
	offset+=sizeof(size);
	
	memcpy(mem+offset,&physicalStart,sizeof(physicalStart));
	offset+=sizeof(physicalStart);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::RLogicalBlock::readFromMem(const uint8_t *mem,size_t &offset)
{
	memcpy(&logicalStart,mem+offset,sizeof(logicalStart));
	offset+=sizeof(logicalStart);

	memcpy(&size,mem+offset,sizeof(size));
	offset+=sizeof(size);

	memcpy(&physicalStart,mem+offset,sizeof(physicalStart));
	offset+=sizeof(physicalStart);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::RLogicalBlock::print() const
{
	printf("logicalStart: %-10lld size: %-5lld physicalStart: %-10lld\n",(long long)logicalStart,(long long)size,(long long)physicalStart);
}



// ---- RPhysicalBlock ------------------------------------------------
template<class l_addr_t,class p_addr_t>
	TPoolFile<l_addr_t,p_addr_t>::RPhysicalBlock::RPhysicalBlock()
{
	size=physicalStart=0;
}

template<class l_addr_t,class p_addr_t>
	TPoolFile<l_addr_t,p_addr_t>::RPhysicalBlock::RPhysicalBlock(const p_addr_t _physicalStart,const l_addr_t _size)
{
	physicalStart=_physicalStart;
	size=_size;
}

template<class l_addr_t,class p_addr_t>
	TPoolFile<l_addr_t,p_addr_t>::RPhysicalBlock::RPhysicalBlock(const RPhysicalBlock &src)
{
	operator=(src);
}

template<class l_addr_t,class p_addr_t>
	TPoolFile<l_addr_t,p_addr_t>::RPhysicalBlock::RPhysicalBlock(const RLogicalBlock &src)
{
	physicalStart=src.physicalStart;
	size=src.size;
}

template<class l_addr_t,class p_addr_t>
	typename TPoolFile<l_addr_t,p_addr_t>::RPhysicalBlock &TPoolFile<l_addr_t,p_addr_t>::RPhysicalBlock::operator=(const RPhysicalBlock &src)
{
	physicalStart=src.physicalStart;
	size=src.size;
	return(*this);
}

template<class l_addr_t,class p_addr_t>
	const bool TPoolFile<l_addr_t,p_addr_t>::RPhysicalBlock::operator==(const RPhysicalBlock &src) const
{
	if(physicalStart==src.physicalStart)
		return(true);
	return(false);
}

template<class l_addr_t,class p_addr_t>
	const bool TPoolFile<l_addr_t,p_addr_t>::RPhysicalBlock::operator<(const RPhysicalBlock &src) const
{
	if(src.physicalStart>physicalStart)
		return(true);
	return(false);
}

template<class l_addr_t,class p_addr_t>
	void TPoolFile<l_addr_t,p_addr_t>::RPhysicalBlock::print() const
{
	printf("physicalStart: %-10lld size: %-5lld\n",(long long)physicalStart,(long long)size);
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
	uint32_t len=s.length();
	f->write(&len,sizeof(len),multiFileHandle);
	f->write(s.c_str(),len,multiFileHandle);
}

#endif
