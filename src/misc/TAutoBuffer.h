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
#pragma once

#include "../../config/common.h"

// I suppose these headers would be available on all posix platforms
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <stdexcept>
#include <string>

#ifndef __func__
	#ifdef _MSC_VER
		// Microsoft compiler specific
		#define __func__	__FUNCTION__
	#endif
#endif

/*
	This template class is used to allocate an buffer of memory that will automatically deallocate the
	memory when this object goes out of scope.

	The type used to instantiate the template must have a default constructor
*/

template <class type> class TAutoBuffer
{
public:
	// size is in elements, not bytes
	TAutoBuffer(size_t _size=0,bool zeroContents=false) : 
		size(0),
		buffer(NULL)
	{
		setSize(_size,zeroContents);
		/*
		if(_size>0)
		{
			buffer=(type *)malloc(_size*sizeof(type));
			if(buffer==NULL)
				throw runtime_error(string(__func__)+" -- error allocating memory -- "+strerror(errno));
			if(zeroContents)
				memset(buffer,0,_size*sizeof(type));
		}
		size=_size;
		*/
	}

	virtual ~TAutoBuffer()
	{
		if(buffer)
			free(buffer);
	}


	// cast to type *
	operator type * ()
	{
		return buffer;
	}

	operator const type * () const 
	{
		return buffer;
	}

	// get at internal buffer
	type *data()
	{
		return buffer;
	}

	const type *data() const
	{
		return buffer;
	}

	type *operator+(int offset)
	{
		return buffer+offset;
	}

	const type *operator+(int offset) const
	{
		return buffer+offset;
	}

#if 0 // I don't know why these cause so many warnings, but it should still be automatically castable to void* thru the type* operators
	// cast to void *
	operator void * const ()
	{
		return buffer;
	}

	operator const void * const () const 
	{
		return buffer;
	}
#endif


	/* for some reason this causes ambiguities in gcc 3.2 .. so I guess it will choose to cast to pointer then subscript if I don't define this but do subscript an object of this class 
	type &operator[](size_t i) const
	{
		return buffer[i];
	}
	*/

	size_t getSize() const
	{
		return size;
	}

	void setSize(size_t newSize,bool zeroAllContents=false) // newSize is in elements not bytes
	{
		if(newSize>0)
		{
			type *temp=(type *)realloc(buffer,newSize*sizeof(type));
			if(temp==NULL)
				throw std::runtime_error(std::string(__func__)+" -- error reallocating memory");
			buffer=temp;
			size=newSize;
			if(zeroAllContents)
				memset(buffer,0,size*sizeof(type));
		}
		else
		{
			if(buffer)
				free(buffer);
			buffer=NULL;
			size=0;
		}
	}

	void copyFrom(const TAutoBuffer<type> &source,size_t amount)
	{
		if(amount>source.getSize())
			amount=source.getSize();

		if(getSize()<amount)
			setSize(amount);

		memcpy(buffer,source.buffer,amount);
	}

private:

	size_t size;
	type *buffer;
};

#endif
