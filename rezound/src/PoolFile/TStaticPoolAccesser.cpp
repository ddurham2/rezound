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
#error This file must be included through TStaticPoolAccesser.h NOT compiled with a project file or a makefile.
#endif

#ifndef __TStaticPoolAccesser_CPP__
#define __TStaticPoolAccesser_CPP__

#include <stdexcept>

#include <istring>

template<class pool_element_t,class pool_file_t> TStaticPoolAccesser<pool_element_t,pool_file_t>::TStaticPoolAccesser(pool_file_t * const _poolFile,const poolId_t _poolId) :
	poolFile(_poolFile),
	poolId(_poolId),

	position(0)
{
	init();

	poolFile->addAccesser(this);
}

template <class pool_element_t,class pool_file_t> TStaticPoolAccesser<pool_element_t,pool_file_t>::TStaticPoolAccesser(const TStaticPoolAccesser<pool_element_t,pool_file_t> &src) :
	poolFile(src.poolFile),
	poolId(src.poolId),

	position(0)
{
	init();
	poolFile->addAccesser(this);
}

template <class pool_element_t,class pool_file_t> void TStaticPoolAccesser<pool_element_t,pool_file_t>::init() const
{
	startAddress=1;
	endAddress=0;

	cacheBuffer=NULL;
	dirty=false;

	cachedBlock=NULL;
}

template <class pool_element_t,class pool_file_t> TStaticPoolAccesser<pool_element_t,pool_file_t>::~TStaticPoolAccesser()
{
	if(poolFile!=NULL)
	{
		try
		{
			CMutexLocker lock(poolFile->accesserInfoMutex);
			poolFile->unreferenceCachedBlock(this);
		}
		catch(...)
		{
			// ignore
		}
		poolFile->removeAccesser(this);
	}
}

template <class pool_element_t,class pool_file_t> TStaticPoolAccesser<pool_element_t,pool_file_t> &TStaticPoolAccesser<pool_element_t,pool_file_t>::operator=(const TStaticPoolAccesser<pool_element_t,pool_file_t> &rhs)
{
	throw(runtime_error(string(__func__)+" -- it is invalid to assign TStaticPoolAccesser<pool_element_t,pool_file_t> objects; the copy constructor must be used"));
}

template <class pool_element_t,class pool_file_t> void TStaticPoolAccesser<pool_element_t,pool_file_t>::seek(const l_addr_t where)
{
	if(where>getSize())
		throw(runtime_error(string(__func__)+" -- out of bounds: "+istring(where)));
	position=where;
}

template <class pool_element_t,class pool_file_t> const typename TStaticPoolAccesser<pool_element_t,pool_file_t>::l_addr_t TStaticPoolAccesser<pool_element_t,pool_file_t>::tell() const
{
	return(position);
}

template <class pool_element_t,class pool_file_t> void TStaticPoolAccesser<pool_element_t,pool_file_t>::read(pool_element_t buffer[],l_addr_t count) const
{
	// there could be a problem here because the size could change AFTER the bounds have been checked
	if(count==0)
		return;
	if(position>getSize() || (getSize()-position)<count)
		throw(runtime_error(string(__func__)+" -- count  parameter is out of bounds: "+istring(count)));

	for(l_addr_t i=0;;)
	{
		if(position>endAddress || position<startAddress)
			cacheBlock(position);
		l_addr_t itemsToRead=endAddress-position+1;
		if(count<itemsToRead)
			itemsToRead=count;

		memcpy(buffer+i,cacheBuffer+(position-startAddress),itemsToRead*sizeof(pool_element_t));

		count-=itemsToRead;
		position+=itemsToRead;
		if(count<=0)
			break;
		i+=itemsToRead;
	}
}

template <class pool_element_t,class pool_file_t> void TStaticPoolAccesser<pool_element_t,pool_file_t>::write(const pool_element_t buffer[],const l_addr_t count)
{
	overflowWrite(buffer,count,false);
}

template <class pool_element_t,class pool_file_t> void TStaticPoolAccesser<pool_element_t,pool_file_t>::overflowWrite(const pool_element_t buffer[],l_addr_t count,const bool append)
{
	if(count==0)
		return;
	if(position>getSize())
		throw(runtime_error(string(__func__)+" -- position parameter is out of bounds: "+istring(position)));

	if((getSize()-position)<count)
	{
		if(!append)
			throw(runtime_error(string(__func__)+" -- count parameter is out of bounds and not allowed to append"));

		const l_addr_t overflow=count-(getSize()-position);
		poolFile->insertSpace(poolId,getSize(),overflow);
	}

	for(l_addr_t i=0;;)
	{
		if(position>endAddress || position<startAddress)
			cacheBlock(position);
		l_addr_t itemsToWrite=endAddress-position+1;
		if(count<itemsToWrite)
			itemsToWrite=count;

		memcpy(cacheBuffer+(position-startAddress),buffer+i,itemsToWrite*sizeof(pool_element_t));

		dirty=true;
		count-=itemsToWrite;
		position+=itemsToWrite;
		if(count<=0)
			break;
		i+=itemsToWrite;
	}
}

template <class pool_element_t,class pool_file_t> void TStaticPoolAccesser<pool_element_t,pool_file_t>::copyData(const l_addr_t destWhere,const TStaticPoolAccesser<pool_element_t,pool_file_t> &src,const l_addr_t srcWhere,l_addr_t const length)
{
	if(srcWhere>src.getSize())
		throw(runtime_error(string(__func__)+" -- invalid srcWhere parameter: "+istring(srcWhere)));
	if((src.getSize()-srcWhere)<length)
		throw(runtime_error(string(__func__)+" -- invalid srcWhere/length parameters: "+istring(srcWhere)+"/"+istring(length)));

	if(destWhere>getSize())
		throw(runtime_error(string(__func__)+" -- invalid destWhere parameter: "+istring(destWhere)));
	if((getSize()-destWhere)<length)
		throw(runtime_error(string(__func__)+" -- invalid destWhere/length parameters: "+istring(destWhere)+"/"+istring(length)));
		
	// ??? change this to memmove later
	// 	- remember to set the dirty flag
	l_addr_t srcP=srcWhere;
	l_addr_t destP=destWhere;
	for(l_addr_t t=0;t<length;t++)
		operator[](destP++)=src[srcP++];
}

template <class pool_element_t,class pool_file_t> void TStaticPoolAccesser<pool_element_t,pool_file_t>::zeroData(const l_addr_t where,const l_addr_t length)
{
	if((getSize()-where)<length)
		throw(runtime_error(string(__func__)+" -- invalid where/length parameters: "+istring(where)+"/"+istring(length)));

	// ??? change this to memset later
	// 	- remember to set the dirty flag
	const l_addr_t l=where+length;
	for(l_addr_t t=where;t<l;t++)
		operator[](t)=0;
}

template <class pool_element_t,class pool_file_t> void TStaticPoolAccesser<pool_element_t,pool_file_t>::cacheBlock(const l_addr_t where) const
{
	poolFile->cacheBlock(where,this);
}


#endif
