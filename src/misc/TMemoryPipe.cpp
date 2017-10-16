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

#ifndef __TMemoryPipe_H__CPP
#error this file should not be included or compiled except from within the TMemoryPipe.h header file
#endif

#include <string.h>
#include <sys/mman.h>

#include <stdexcept>
#include <algorithm>

#include <istring>


template <class type> TMemoryPipe<type>::TMemoryPipe(int pipeSize) :
	readOpened(false),
	writeOpened(false),
	buffer(NULL),
	bufferSize(0)
{
	setSize(pipeSize);
	open();
}

template <class type> TMemoryPipe<type>::~TMemoryPipe()
{
	closeRead();
	closeWrite();

	if(buffer) {
		munlock(buffer, bufferSize);
		delete [] buffer;
	}
}

template <class type> void TMemoryPipe<type>::setSize(int pipeSize)
{
	std::unique_lock<std::mutex> ml(waitStateMutex);

	if(pipeSize<=0)
		throw runtime_error(string(__func__)+" -- invalid pipeSize: "+istring(pipeSize));

	if(buffer) {
		// release lock on not-swapping memory
		munlock(buffer, bufferSize);
	}

	type *temp=new type[pipeSize+1];
	delete [] buffer;
	buffer=temp;

	bufferSize=pipeSize+1;

	// lock memory for being swapped (for JACK's sake)
	mlock(buffer, bufferSize);
}

template <class type> void TMemoryPipe<type>::open()
{
	if(readOpened || writeOpened)
		throw runtime_error(string(__func__)+" -- either the read or write end is already open");

	readPos=writePos=0;
	readOpened=writeOpened=true;
}


template <class type> int TMemoryPipe<type>::read(type *dest,int size,bool block)
{
	return privateRead(dest, size, block, readPos);
}

template <class type> int TMemoryPipe<type>::peek(type *dest,int size,bool block)
{
	if(readOpened && size>bufferSize-1)
			// ??? not really an error, just return what we can I suppose
		throw runtime_error(string(__func__)+" -- cannot peek for more data than the pipe can hold: "+istring(size)+">"+istring(bufferSize-1));

	int tmpReadPos=readPos;
	return privateRead(dest, size, block, tmpReadPos);
}

template <class type> int TMemoryPipe<type>::skip(int size,bool block)
{
	return privateRead(NULL, size, block, readPos);
}

// Private implemention of a read operation. 
// If dest is NULL, then no data is really copied (thus, making it a peek)
// readPos is passed to use the data-member or not (thus, making it a skip)
template <class type> int TMemoryPipe<type>::privateRead(type *dest,int size,bool block,int &readPos)
{
	if(size<=0)
		return 0;

	// prevent simultaneous read()s
	std::unique_lock<std::mutex> l(readerMutex, std::defer_lock);
	if (block) {
		l.lock();
	} else {
		if(! l.try_lock())
			return 0;
	}

	std::unique_lock<std::mutex> wsl(waitStateMutex);

	int _writePos;

	int sizeNeeded=size;
	int totalRead=0;

	restart:
	
	if(!readOpened) // closed while we were in this method
		throw EPipeClosed(string(__func__)+" -- read end is not open");

	if(!writeOpened)
	{
		if(getSize()<=0)
		{
			if(totalRead>0)
				goto done;
			else
			{
				totalRead=EOP;
				goto done;
			}
		}
	}

	_writePos=writePos;

	if(readPos<_writePos)
	{
		const int sizeAvailable=_writePos-readPos;

		if(sizeAvailable==0)
		{
			if(!block) {
				goto done;
			}

			fullCond.notify_one();
			emptyCond.wait(wsl);
			goto restart;
		}

		const int readAmount=min(sizeNeeded,sizeAvailable);

		if(dest) {
			memcpy(dest+totalRead,buffer+readPos,readAmount*sizeof(type));
		}

		totalRead+=readAmount;
		readPos+=readAmount;
		sizeNeeded-=readAmount;
	}
	else if(readPos>_writePos)
	{
		const int sizeAvailable1=bufferSize-readPos; // size available till end of buffer
		const int sizeAvailable2=_writePos; // size available from start of buffer

		if((sizeAvailable1+sizeAvailable2)==0)
		{
			if(!block) {
				goto done;
			}

			fullCond.notify_one();
			emptyCond.wait(wsl);
			goto restart;
		}

		// copy from last part of buffer
		const int readAmount=min(sizeNeeded,sizeAvailable1);

		if(dest) {
			memcpy(dest+totalRead,buffer+readPos,readAmount*sizeof(type));
		}

		totalRead+=readAmount;
		readPos=(readPos+readAmount)%bufferSize;
		sizeNeeded-=readAmount;

		if(sizeNeeded>0)
		{
			// copy from first part of buffer
			const int readAmount=min(sizeNeeded,sizeAvailable2);

			if(dest) {
				memcpy(dest+totalRead,buffer,readAmount*sizeof(type));
			}

			totalRead+=readAmount;
			readPos+=readAmount;
			sizeNeeded-=readAmount;
		}
	}
	else // buffer is empty
	{
		if(!block) {
			goto done;
		}

		fullCond.notify_one();
		emptyCond.wait(wsl);
		goto restart;
	}

	if(sizeNeeded>0 && block)
	{
		fullCond.notify_one();
		emptyCond.wait(wsl);
		goto restart;
	}

	done:

	fullCond.notify_one();
	return totalRead;
}

template <class type> int TMemoryPipe<type>::write(const type *_src,int size,bool block)
{
	if(size<=0)
		return 0;

	// prevent simultaneous write()s
	std::unique_lock<std::mutex> l(writerMutex, std::defer_lock);
	if (block) {
		l.lock();
	} else {
		if(! l.try_lock())
			return 0;
	}

	std::unique_lock<std::mutex> wsl(waitStateMutex);

	const type *src=(const type *)_src;
	int _readPos;

	int sizeToGive=size;
	int totalWritten=0;

	restart:

	if(!writeOpened)
		throw EPipeClosed(string(__func__)+" -- write end is not open");

	if(!readOpened)
	{
		totalWritten=EOP;
		goto done;
	}

	_readPos=readPos;

	if(writePos<_readPos)
	{
		const int spaceAvailable=(_readPos-writePos)-1;
		if(spaceAvailable<=0)
		{ // buffer is full
			if(!block) {
				goto done;
			} 
			emptyCond.notify_one();
			fullCond.wait(wsl);
			goto restart;
		}

		const int writeAmount=min(sizeToGive,spaceAvailable);

				// ??? I could save an add by altering src instead of using totalWritten... and know totalWritten at the end from src-_src
		memcpy(buffer+writePos,src+totalWritten,writeAmount*sizeof(type));

		totalWritten+=writeAmount;
		writePos+=writeAmount;
		sizeToGive-=writeAmount;
	}
	else if(writePos>=_readPos)
	{
		const int spaceAvailable1=(bufferSize-writePos)-(_readPos==0 ? 1 : 0); // space available till end of buffer
		const int spaceAvailable2=_readPos-(_readPos==0 ? 0 : 1); // space available from start of buffer

		const int spaceAvailable=spaceAvailable1+spaceAvailable2;

		if(spaceAvailable<=0)
		{ // buffer is full
			if(!block) {
				goto done;
			} 
			emptyCond.notify_one();
			fullCond.wait(wsl);
			goto restart;
		}
				
		// copy to last part of buffer
		const int writeAmount=min(sizeToGive,spaceAvailable1);

		memcpy(buffer+writePos,src+totalWritten,writeAmount*sizeof(type));

		totalWritten+=writeAmount;
		writePos=(writePos+writeAmount)%bufferSize;
		sizeToGive-=writeAmount;

		if(sizeToGive>0)
		{
			// copy to first part of buffer
			const int writeAmount=min(sizeToGive,spaceAvailable2);

			memcpy(buffer,src+totalWritten,writeAmount*sizeof(type));

			totalWritten+=writeAmount;
			writePos+=writeAmount;
			sizeToGive-=writeAmount;
		}
	}

	if(sizeToGive>0)
	{
		emptyCond.notify_one();
		goto restart;
	}

	done:

	emptyCond.notify_one();
	return totalWritten;
}

template <class type> void TMemoryPipe<type>::closeRead()
{
	if(readOpened)
	{
		std::unique_lock<std::mutex> ml(waitStateMutex);
		readOpened=false;
		emptyCond.notify_all();
		fullCond.notify_all();
	}
}

template <class type> void TMemoryPipe<type>::closeWrite()
{
	if(writeOpened)
	{
		std::unique_lock<std::mutex> ml(waitStateMutex);
		writeOpened=false;
		fullCond.notify_all();
		emptyCond.notify_all();
	}
}

template <class type> bool TMemoryPipe<type>::isReadOpened() const
{
	return readOpened;
}

template <class type> bool TMemoryPipe<type>::isWriteOpened() const
{
	return writeOpened;
}

template <class type> int TMemoryPipe<type>::getSize() const
{
	/*
	If we get interrupted between the time we get the value for 
	writePos and when we get the value for readPos this will be 
	screwed up, but it may not be that bad.. just gives the 
	wrong size.  It could be solved be leaving then private and 
	have one method for read to call and another for write to 
	call, then we know either the read or write pos variable 
	won't be changing.
	*/

	const int diff=writePos-readPos;
	if(diff<0) // if(writePos<readPos)
		return bufferSize+diff;
	else // if(writePos>=readPos);
		return diff;
}

template <class type> int TMemoryPipe<type>::clear()
{
	std::unique_lock<std::mutex> ml(waitStateMutex);
	const int len=getSize();
	readPos=writePos=0;
	fullCond.notify_one();
	return len;
}

