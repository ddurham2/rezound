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

#ifndef __TAutoBuffer_H__
#define __TAutoBuffer_H__


#include "../../../config/common.h"

// I suppose these headers would be available on all posix platforms
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <stdexcept>
#include <string>

/*
	This template class is used to allocate an buffer of memory that will automatically deallocate the
	memory when this object goes out of scope.

	The type used to instansiate the template must have a default constructor
*/

template <class type> class TAutoBuffer
{
public:
	TAutoBuffer(const size_t _size,const bool zeroContents=false) : 
		buffer(NULL)
	{
		if(_size>0)
		{
			buffer=(type *)malloc(_size*sizeof(type));
			if(buffer==NULL)
				throw(runtime_error(string(__func__)+" -- error allocating memory -- "+strerror(errno)));
			if(zeroContents)
				memset(buffer,0,size*sizeof(type));
		}
		size=_size;
	}

	virtual ~TAutoBuffer()
	{
		free(buffer);
	}

	// cast to type *
	operator type * const ()
	{
		return(buffer);
	}

	operator const type * const () const 
	{
		return(buffer);
	}

	// cast to void *
	operator void * const ()
	{
		return(buffer);
	}

	operator const void * const () const 
	{
		return(buffer);
	}



	type &operator[](const size_t i) const
	{
		return(buffer[i]);
	}

	const size_t getSize() const
	{
		return(size);
	}

private:

	size_t size;
	type *buffer;
};

#endif