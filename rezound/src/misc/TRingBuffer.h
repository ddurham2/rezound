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

/*
 * This template implements a ring-buffer
 *
 * It is by not means thread-safe because a process could get interupted between
 * saving the readPos and writePos variables at the beginning of the read/write
 * methods.  Other than that, I think they would be.
 *
 * It is also not thread-safe in the sense that there is nothing to protect from
 * two writers or two reads being active at the same time; this would cause
 * undefined behavior.
 *
 * memcpy is used to copy the instances of type in the read and write methods,
 * so this would have to be modified if an object requiring a copy constructor
 * were used.  It would also have to be determined when to destruct objects
 * requiring a destructor.. All possible, but not implemented at this point.
 */

#ifndef __TRingBuffer_H__
#define __TRingBuffer_H__

#include "../../config/common.h"

#include <stddef.h>	// for NULL
#include <stdlib.h> 	// for memcpy()
#include <algorithm>	// for min()
template<class type> class TRingBuffer
{
public:
	TRingBuffer(const size_t _maxSize=0) :
		maxSize(0),
		buffer(NULL),
		readPos(0),
		writePos(0)
	{
		setSize(_maxSize);
	}

	virtual ~TRingBuffer()
	{
		delete [] buffer;
	}

	void setSize(const size_t _maxSize)
	{
		clear();
		delete [] buffer;
		maxSize=_maxSize+1;
		buffer=new type[maxSize];
	}

	void clear()
	{
		readPos=writePos=0;
	}

	const size_t getUsedSpace() const
	{
		return calcUsedSpace(readPos,writePos);
	}

	const size_t getFreeSpace() const
	{
		return calcFreeSpace(readPos,writePos);
	}

	const size_t read(const type *&buffer1,size_t &buffer1Length,const type *&buffer2,size_t &buffer2Length,size_t lengthToRead)
	{
		const size_t readPos=this->readPos;
		const size_t writePos=this->writePos;

		const size_t amountToRead=min(lengthToRead, calcUsedSpace(readPos,writePos));
		if(amountToRead<=0)
		{
			buffer1=buffer2=NULL;
			buffer1Length=buffer2Length=0;
			return 0;
		}

		if(readPos<=writePos)
		{
			buffer1=buffer+readPos;
			buffer1Length=min(amountToRead,writePos-readPos);

			buffer2=NULL;
			buffer2Length=0;
		}
		else
		{
			buffer1=buffer+readPos;
			buffer1Length=min(amountToRead,maxSize-readPos);

			buffer2=buffer;
			buffer2Length=min(amountToRead-buffer1Length,writePos);
		}

		// update real value
		this->readPos=(readPos+(buffer1Length+buffer2Length))%maxSize;

		return buffer1Length+buffer2Length;
	}

	const size_t read(type *readInto,const size_t length)
	{
		const type *buffer1, *buffer2;
		size_t buffer1Length,buffer2Length;

		const size_t amountRead=read(buffer1,buffer1Length,buffer2,buffer2Length,length);

		// should do a for-loop if I know type is a class and needs to have its copy constructor called.. and I would need call the copy constructor on this very memory not allocating any new memory
		memcpy(readInto,buffer1,buffer1Length*sizeof(type));
		memcpy(readInto+buffer1Length,buffer2,buffer2Length*sizeof(type));

		return amountRead;
	}

	const size_t write(const type *writeFrom,const size_t length,const size_t stride=1)
	{
		const size_t readPos=this->readPos;
		const size_t writePos=this->writePos;

		const size_t amountToWrite=min(length, calcFreeSpace(readPos,writePos));
		if(amountToWrite<=0)
			return 0;

		const size_t partToWrite=min(amountToWrite, (readPos<=writePos) ? (maxSize-writePos) : (readPos-writePos) );
		if(stride==1)
		{
			for(size_t t=0;t<partToWrite;t++)
				buffer[writePos+t]=writeFrom[t];
		}
		else
		{
			for(size_t t=0;t<partToWrite;t++)
				buffer[writePos+t]=writeFrom[t*stride];
		}

		// update real value
		this->writePos=(writePos+partToWrite)%maxSize;

		if(partToWrite<amountToWrite)
			return partToWrite+write(writeFrom+(partToWrite*stride),length-partToWrite,stride);
		else
			return partToWrite;
	}

private:
	size_t maxSize;
	type *buffer;
	size_t readPos,writePos;

	const size_t calcUsedSpace(const size_t readPos,const size_t writePos) const
	{
		return (readPos<=writePos) ? writePos-readPos : maxSize-(readPos-writePos);
	}

	const size_t calcFreeSpace(const size_t readPos,const size_t writePos) const
	{
		return (maxSize-calcUsedSpace(readPos,writePos))-1;
	}
};

#endif
