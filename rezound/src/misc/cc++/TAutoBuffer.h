// Copyright (C) 2002 Open Source Telecom Corporation. (David W. Durham)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
// As a special exception to the GNU General Public License, permission is
// granted for additional uses of the text contained in its release
// of Common C++.
//
// The exception is that, if you link the Common C++ library with other
// files to produce an executable, this does not by itself cause the
// resulting executable to be covered by the GNU General Public License.
// Your use of that executable is in no way restricted on account of
// linking the Common C++ library code into it.
//
// This exception does not however invalidate any other reasons why
// the executable file might be covered by the GNU General Public License.
//
// This exception applies only to the code released under the
// name Common C++.  If you copy code from other releases into a copy of
// Common C++, as the General Public License permits, the exception does
// not apply to the code that you add in this way.  To avoid misleading
// anyone as to the status of such modified files, you must delete
// this exception notice from them.
//
// If you write modifications of your own for Common C++, it is your
// choice whether to permit this exception to apply to your modifications.
// If you do not wish that, delete this exception notice.

#ifndef CCXX_TAutoBuffer_H_
#define CCXX_TAutoBuffer_H_


#include "../../../config/common.h"

#ifndef CCXX_CONFIG_H_
	#include <cc++/config.h>
#endif

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

#ifdef  CCXX_NAMESPACES
namespace ost {
#endif

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

#ifdef  CCXX_NAMESPACES
}
#endif

#endif
