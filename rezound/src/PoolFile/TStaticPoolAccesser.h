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

#ifndef __TStaticPoolAccesser_H__
#define __TStaticPoolAccesser_H__

#include "../../config/common.h"

#include "TPoolFile.h"

// ??? make the parametes const than can be

template <class pool_element_t,class pool_file_t> class TStaticPoolAccesser
{
public:
	typedef typename pool_file_t::l_addr_t l_addr_t;
	typedef typename pool_file_t::p_addr_t p_addr_t;
	typedef typename pool_file_t::poolId_t poolId_t;

	TStaticPoolAccesser(const TStaticPoolAccesser<pool_element_t,pool_file_t> &src);
	virtual ~TStaticPoolAccesser();

	const l_addr_t getSize() const { return(poolFile->getPoolSize(poolId)/sizeof(pool_element_t)); }


	// stream-like access methods
	void read(pool_element_t buffer[],const l_addr_t count) const;
	void write(const pool_element_t buffer[],const l_addr_t count);

	void seek(const l_addr_t where);
	const l_addr_t tell() const;


	// random access methods (best performance when consecutive indexes are within the same maxBlockSize alignments)
	inline pool_element_t &operator[](const l_addr_t where)
	{
		if(where>endAddress || where<startAddress)
		{
			if(poolFile!=NULL)
				cacheBlock(where);
			else
				throw(runtime_error(string(__func__)+" -- TPoolFile object no longer exists"));
		}
		dirty=true;
		return(cacheBuffer[where-startAddress]);
	}

	/* !!! NOTE !!!
	 * It is very important to use this const subscript operator whenever
	 * the data is being read but not modified.  If you use the non-const
	 * subscript operator it will cause the block to be flagged as dirty
	 * and the data will be written back to disk even though it did not
	 * need to be.
	 */
	inline pool_element_t operator[](const l_addr_t where) const
	{
		if(where>endAddress || where<startAddress)
		{
			if(poolFile!=NULL)
				cacheBlock(where);
			else
				throw(runtime_error(string(__func__)+" const -- CPoolFile object no longer exists"));
		}
		return(cacheBuffer[where-startAddress]);
	}



	// bulk transfer methods
	void copyData(const l_addr_t destWhere,const TStaticPoolAccesser<pool_element_t,pool_file_t> &src,const l_addr_t srcWhere,const l_addr_t length);
	void zeroData(const l_addr_t where,const l_addr_t length);


	// invalid operations
	TStaticPoolAccesser<pool_element_t,pool_file_t> &operator=(const TStaticPoolAccesser<pool_element_t,pool_file_t> &rhs);



	
	//friend pool_file_t; // ??? I don't know what the work-around or proper way for doing this is
	public:


//protected:

	pool_file_t * const poolFile;
	const poolId_t poolId;

	mutable l_addr_t startAddress,endAddress;
	mutable pool_element_t *cacheBuffer;
	mutable bool dirty;

	mutable l_addr_t position;


	void overflowWrite(const pool_element_t buffer[],const l_addr_t count,const bool append);

	void cacheBlock(const l_addr_t where) const;

//private:


	// stuff managed by TPoolFile
	mutable typename TPoolFile<l_addr_t,p_addr_t>::RCachedBlock *cachedBlock;
	TStaticPoolAccesser(pool_file_t * const _poolFile,const poolId_t _poolId);

	void init() const;

};

#include "TStaticPoolAccesser.cpp"

#endif
