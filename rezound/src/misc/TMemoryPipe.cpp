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

#ifndef __TMemoryPipe_H__
#error this file should not be included or compiled except from within the TMemoryPipe.h header file
#endif

#include <string.h>
#include <stdio.h> // for fprintf(stderr,...)

#include <stdexcept>
#include <algorithm>

#include <istring>

#include <AThread.h>


template <class type> TMemoryPipe<type>::TMemoryPipe(int _pipeSize) :
	readOpened(false),
	writeOpened(false),
	buffer(NULL)
{
	if(_pipeSize<=0)
		throw(runtime_error(string(__func__)+" -- invalid pipeSize: "+istring(_pipeSize)));
	
	bufferSize=_pipeSize+1;
	buffer=new type[bufferSize];

	open();
}

template <class type> TMemoryPipe<type>::~TMemoryPipe()
{
	closeRead();
	closeWrite();
	delete [] buffer;
}

template <class type> void TMemoryPipe<type>::open()
{
	if(readOpened || writeOpened)
		throw(runtime_error(string(__func__)+" -- either the read or write end is already open"));

	readPos=writePos=0;
	readOpened=writeOpened=true;
}


/*
 * If this method is updated, update peek() and skip() as well
 */
template <class type> int TMemoryPipe<type>::read(type *dest,int size,bool block)
{
	if(size<=0)
		return(0);

	CMutexLocker l(readerMutex); // protect from more than one reader
	CMutexLocker wsl(waitStateMutex);

	int _writePos;

	int sizeNeeded=size;
	int totalRead=0;

	restart:
	
	if(!readOpened) // closed while we were in this method
		throw(runtime_error(string(__func__)+" -- read end is not open"));

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
			if(!block)	
				goto done;

			fullCond.signal();
			emptyCond.wait(waitStateMutex);
			goto restart;
		}

		const int readAmount=min(sizeNeeded,sizeAvailable);

		memcpy(dest+totalRead,buffer+readPos,readAmount*sizeof(type));

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
			if(!block)	
				goto done;

			fullCond.signal();
			emptyCond.wait(waitStateMutex);
			goto restart;
		}

		// copy from last part of buffer
		const int readAmount=min(sizeNeeded,sizeAvailable1);

		memcpy(dest+totalRead,buffer+readPos,readAmount*sizeof(type));

		totalRead+=readAmount;
		readPos=(readPos+readAmount)%bufferSize;
		sizeNeeded-=readAmount;

		if(sizeNeeded>0)
		{
			// copy from first part of buffer
			const int readAmount=min(sizeNeeded,sizeAvailable2);

			memcpy(dest+totalRead,buffer,readAmount*sizeof(type));

			totalRead+=readAmount;
			readPos+=readAmount;
			sizeNeeded-=readAmount;
		}
	}
	else // buffer is empty
	{
		if(!block)
			goto done;

		fullCond.signal();
		emptyCond.wait(waitStateMutex);
		goto restart;
	}

	if(sizeNeeded>0 && block)
	{
		fullCond.signal();
		emptyCond.wait(waitStateMutex);
		goto restart;
	}

	done:

	fullCond.signal();
	return(totalRead);
}

/*
 * This method is simply a duplicate of read() except at the beginning where it makes 
 * a copy of readPos and makes sure that the requested size isn't larger than the pipe 
 * was allocated for.   So if read() changes, update this method too.
 */
template <class type> int TMemoryPipe<type>::peek(type *dest,int size,bool block)
{
	if(readOpened && size>bufferSize-1)
		throw(runtime_error(string(__func__)+" -- cannot peek for more data than the pipe can hold: "+istring(size)+">"+istring(bufferSize-1)));
	int readPos=this->readPos;

	if(size<=0)
		return(0);

	CMutexLocker l(readerMutex); // protect from more than one reader
	CMutexLocker wsl(waitStateMutex);

	int _writePos;

	int sizeNeeded=size;
	int totalRead=0;

	restart:
	
	if(!readOpened) // closed while we were in this method
		throw(runtime_error(string(__func__)+" -- read end is not open"));

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
			if(!block)	
				goto done;

			fullCond.signal();
			emptyCond.wait(waitStateMutex);
			goto restart;
		}

		const int readAmount=min(sizeNeeded,sizeAvailable);

		memcpy(dest+totalRead,buffer+readPos,readAmount*sizeof(type));

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
			if(!block)	
				goto done;

			fullCond.signal();
			emptyCond.wait(waitStateMutex);
			goto restart;
		}

		// copy from last part of buffer
		const int readAmount=min(sizeNeeded,sizeAvailable1);

		memcpy(dest+totalRead,buffer+readPos,readAmount*sizeof(type));

		totalRead+=readAmount;
		readPos=(readPos+readAmount)%bufferSize;
		sizeNeeded-=readAmount;

		if(sizeNeeded>0)
		{
			// copy from first part of buffer
			const int readAmount=min(sizeNeeded,sizeAvailable2);

			memcpy(dest+totalRead,buffer,readAmount*sizeof(type));

			totalRead+=readAmount;
			readPos+=readAmount;
			sizeNeeded-=readAmount;
		}
	}
	else // buffer is empty
	{
		if(!block)
			goto done;

		fullCond.signal();
		emptyCond.wait(waitStateMutex);
		goto restart;
	}

	if(sizeNeeded>0 && block)
	{
		fullCond.signal();
		emptyCond.wait(waitStateMutex);
		goto restart;
	}

	done:

	fullCond.signal();
	return(totalRead);
}

/*
 * This is simply a copy of the read() method with the memcpy()'s take out
 * So if read() changes update this method too.
 */
template <class type> int TMemoryPipe<type>::skip(int size,bool block)
{
	if(size<=0)
		return(0);

	CMutexLocker l(readerMutex); // protect from more than one reader
	CMutexLocker wsl(waitStateMutex);

	int _writePos;

	int sizeNeeded=size;
	int totalRead=0;

	restart:
	
	if(!readOpened) // closed while we were in this method
		throw(runtime_error(string(__func__)+" -- read end is not open"));

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
			if(!block)	
				goto done;

			fullCond.signal();
			emptyCond.wait(waitStateMutex);
			goto restart;
		}

		const int readAmount=min(sizeNeeded,sizeAvailable);

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
			if(!block)	
				goto done;

			fullCond.signal();
			emptyCond.wait(waitStateMutex);
			goto restart;
		}

		// copy from last part of buffer
		const int readAmount=min(sizeNeeded,sizeAvailable1);

		totalRead+=readAmount;
		readPos=(readPos+readAmount)%bufferSize;
		sizeNeeded-=readAmount;

		if(sizeNeeded>0)
		{
			// copy from first part of buffer
			const int readAmount=min(sizeNeeded,sizeAvailable2);

			totalRead+=readAmount;
			readPos+=readAmount;
			sizeNeeded-=readAmount;
		}
	}
	else // buffer is empty
	{
		if(!block)
			goto done;

		fullCond.signal();
		emptyCond.wait(waitStateMutex);
		goto restart;
	}

	if(sizeNeeded>0 && block)
	{
		fullCond.signal();
		emptyCond.wait(waitStateMutex);
		goto restart;
	}

	done:

	fullCond.signal();
	return(totalRead);
}


template <class type> int TMemoryPipe<type>::write(const type *_src,int size)
{
	if(size<=0)
		return(0);

	CMutexLocker l(writerMutex); // protect from more than one writer
	CMutexLocker wsl(waitStateMutex);

	const type *src=(const type *)_src;
	int _readPos;

	int sizeToGive=size;
	int totalWritten=0;

	restart:

	if(!writeOpened)
		throw(runtime_error(string(__func__)+" -- write end is not open"));

	if(!readOpened)
	{
		totalWritten=EOP;
		goto done;
	}

	_readPos=readPos;

	if(writePos<_readPos)
	{
		const int spaceAvailable=(_readPos-writePos)-1;
		if(spaceAvailable==0)
		{ // buffer is full
			emptyCond.signal();
			fullCond.wait(waitStateMutex);
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

		if(spaceAvailable==0)
		{ // buffer is full
			emptyCond.signal();
			fullCond.wait(waitStateMutex);
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
		emptyCond.signal();
		goto restart;
	}

	done:

	emptyCond.signal();
	return(totalWritten);
}

template <class type> void TMemoryPipe<type>::closeRead()
{
	if(readOpened)
	{
		CMutexLocker l(waitStateMutex);
		readOpened=false;
		emptyCond.broadcast();
		fullCond.broadcast();
	}
}

template <class type> void TMemoryPipe<type>::closeWrite()
{
	if(writeOpened)
	{
		CMutexLocker l(waitStateMutex);
		writeOpened=false;
		fullCond.broadcast();
		emptyCond.broadcast();
	}
}

template <class type> bool TMemoryPipe<type>::isReadOpened() const
{
	return(readOpened);
}

template <class type> bool TMemoryPipe<type>::isWriteOpened() const
{
	return(writeOpened);
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
		return(bufferSize+diff);
	else // if(writePos>=readPos);
		return(diff);
}

template <class type> void TMemoryPipe<type>::clear()
{
	//skip(bufferSize,false);
	CMutexLocker l(readerMutex);
	CMutexLocker l2(waitStateMutex);
	readPos=writePos=0;
	fullCond.signal();
}

