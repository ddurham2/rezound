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

#ifndef __TPoolAccesser_H__
#error This file must be included through TPoolAccesser.h NOT compiled with a project file or a makefile.
#endif


#ifndef __TPoolAccesser_CPP__
#define __TPoolAccesser_CPP__

#include <stdexcept>

#include <istring>

template <class pool_element_t,class pool_file_t> TPoolAccesser<pool_element_t,pool_file_t>::TPoolAccesser(const TStaticPoolAccesser<pool_element_t,pool_file_t> &src) :
	TStaticPoolAccesser<pool_element_t,pool_file_t>(src)
{
}


template <class pool_element_t,class pool_file_t> TPoolAccesser<pool_element_t,pool_file_t> &TPoolAccesser<pool_element_t,pool_file_t>::operator=(const TPoolAccesser<pool_element_t,pool_file_t> &rhs)
{
	throw runtime_error(string(__func__)+" -- it is invalid to assign TPoolAccesser<pool_element_t,pool_file_t> objects; the copy constructor must be used");
}

template <class pool_element_t,class pool_file_t> void TPoolAccesser<pool_element_t,pool_file_t>::insert(const l_addr_t where,const l_addr_t count)
{
	poolFile->insertSpace(poolId,where,count);
}

template <class pool_element_t,class pool_file_t> void TPoolAccesser<pool_element_t,pool_file_t>::append(const l_addr_t count)
{
	poolFile->insertSpace(poolId,getSize(),count);
}

template <class pool_element_t,class pool_file_t> void TPoolAccesser<pool_element_t,pool_file_t>::prepend(const l_addr_t count)
{
	poolFile->insertSpace(poolId,0,count);
}

template <class pool_element_t,class pool_file_t> void TPoolAccesser<pool_element_t,pool_file_t>::copyData(const l_addr_t destWhere,const TStaticPoolAccesser<pool_element_t,pool_file_t> &src,const l_addr_t srcWhere,const l_addr_t length,const bool appendIfShort)
{
	if(destWhere>getSize())
		throw runtime_error(string(__func__)+" -- out of range destWhere parameter: "+istring(destWhere));
		
	if((getSize()-destWhere)<length)
	{
		if(appendIfShort)
			append(length-(getSize()-destWhere));
		else 
			throw runtime_error(string(__func__)+" -- invalid destWhere/length parameters: "+istring(destWhere)+"/"+istring(length));
	}
		
	TStaticPoolAccesser<pool_element_t,pool_file_t>::copyData(destWhere,src,srcWhere,length);
}

template <class pool_element_t,class pool_file_t> void TPoolAccesser<pool_element_t,pool_file_t>::moveData(const l_addr_t destWhere,TPoolAccesser<pool_element_t,pool_file_t> &srcPool,const l_addr_t srcWhere,const l_addr_t count)
{
	if(srcPool.poolFile!=poolFile) // ??? perhaps I could do a data copy if they weren't the same... but I should probably create another function for that, although the name wouldn't imply using a different method as is named
		throw runtime_error(string(__func__)+" -- srcPool's poolFile is not the same as this accesser's poolFile");
	poolFile->moveData(poolId,destWhere,srcPool.poolId,srcWhere,count);
}



template <class pool_element_t,class pool_file_t> void TPoolAccesser<pool_element_t,pool_file_t>::remove(const l_addr_t where,const l_addr_t count)
{
	poolFile->removeSpace(poolId,where,count);
	if(position>getSize())
		position=getSize();
}

template <class pool_element_t,class pool_file_t> void TPoolAccesser<pool_element_t,pool_file_t>::clear()
{
	poolFile->clearPool(poolId);
}


template <class pool_element_t,class pool_file_t> void TPoolAccesser<pool_element_t,pool_file_t>::write(const pool_element_t buffer[],const l_addr_t count,const bool append)
{
	overflowWrite(buffer,count,append);
}

#endif
