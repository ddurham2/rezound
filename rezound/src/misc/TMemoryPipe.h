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
#define __TMemoryPipe_H__

#include "../../config/common.h"

#include "CMutex.h"
#include "CConditionVariable.h"

#define EOP (-1)

/*
 * This is a queue of memory which will do blocked reads and writes
 * I would prefer to use a the pipe() system call, but I need a specific
 * pipe size.  It is meant to be used by 2 threads, one reading from and
 * one writing to the pipe.  All the pertainent methods are thread-safe
 *
 * The type template parameters must be a type who's instantiations can
 * be safely memcpy-ed
 *
 * This class has more functionality than is needed for 
 * ReZound, but I took it from another project I was working on a long
 * time ago, so I left the extra stuff in.
 *
 */

template <class type> class TMemoryPipe
{
public:

	TMemoryPipe(int pipeSize);
	virtual ~TMemoryPipe();

	int read(type *buffer,int size,bool block);
	int peek(type *buffer,int size,bool block);
	int skip(int size,bool block);

	/* 
	 * the write method always blocks, unless kickOutWriter is called
	 * in which it will return the number of elements that were written 
	 */
	int write(const type *buffer,int size);

	void closeRead();
	void closeWrite();

	bool isReadOpened() const;
	bool isWriteOpened() const;

	int getSize() const; // get available read space
	void clear(); // remove all data in pipe

private:

	void open();

	bool readOpened;
	bool writeOpened;
	int readPos;
	int writePos;

	type *buffer;
	int bufferSize;

	CMutex readerMutex; // use to prevent more than one reader at the same time
	CMutex writerMutex; // use to prevent more than one writer at the same time

	CMutex waitStateMutex;

	CConditionVariable fullCond;
	CConditionVariable emptyCond;

};

#include "TMemoryPipe.cpp"

#endif

