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
#define __TPoolAccesser_H__

#include "../../config/common.h"

#include "TStaticPoolAccesser.h"

template <class pool_element_t,class pool_file_t> class TPoolAccesser : public TStaticPoolAccesser<pool_element_t,pool_file_t>
{
public:
	// ??? I REALLY don't think that I should have to re-typedef these from the base class... but it suppresses the warnings I get with gcc3.1.1
	typedef typename TStaticPoolAccesser<pool_element_t,pool_file_t>::l_addr_t l_addr_t;
	typedef typename TStaticPoolAccesser<pool_element_t,pool_file_t>::p_addr_t p_addr_t;
	typedef typename TStaticPoolAccesser<pool_element_t,pool_file_t>::poolId_t poolId_t;

	TPoolAccesser(const TStaticPoolAccesser<pool_element_t,pool_file_t> &src);

	// space modifier methods
	void insert(const l_addr_t where,const l_addr_t count);
	void append(const l_addr_t count);
	void prepend(const l_addr_t count);

	void remove(const l_addr_t where,const l_addr_t count);
	void clear();


	// bulk transfer methods
	void copyData(const l_addr_t destWhere,const TStaticPoolAccesser<pool_element_t,pool_file_t> &src,const l_addr_t srcWhere,const l_addr_t length,const bool appendIfShort=false);
	void moveData(const l_addr_t destWhere,TPoolAccesser<pool_element_t,pool_file_t> &srcPool,const l_addr_t srcWhere,const l_addr_t count);


	// stream-like access methods
	void write(const pool_element_t buffer[],const l_addr_t count,const bool append=true);


	// invalid operations
	TPoolAccesser<pool_element_t,pool_file_t> &operator=(const TPoolAccesser<pool_element_t,pool_file_t> &rhs);

};

#include "TPoolAccesser.cpp"

#endif
