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

#include <stdexcept>

#include "CMutex.h"
#include "CConditionVariable.h"

#define EOP (-1)

/*
 * This is a queue of memory which will do blocked reads and writes
 * I would prefer to use a the pipe() system call, but I need a specific
 * pipe size.  It is meant to be used by 2 threads, one reading from and
 * one writing to the pipe.  All the pertinent methods are thread-safe
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
	TMemoryPipe(int pipeSize = 1024); 
	virtual ~TMemoryPipe();

	 // an EPipeClosed is thrown if the read end of the pipe is closed (all 3 methods)
	int read(type *buffer,int size,bool block);
	int peek(type *buffer,int size,bool block);
	int skip(int size,bool block);

	 // the write method always blocks
	 // an EPipeClosed is thrown if the write end of the pipe is closed
	int write(const type *buffer,int size, bool block);

	void open(); // note, pipe is open after construction
	void closeRead();
	void closeWrite();

	bool isReadOpened() const;
	bool isWriteOpened() const;

	void setSize(int pipeSize); // set the amount of capacity, this clear all data in the pipe
	int getSize() const; // get available read space
	int clear(); // remove all data currently in pipe, returns the number of elements cleared

	class EPipeClosed : public runtime_error { public: EPipeClosed(const string msg) : runtime_error(msg) { } };

private:

	bool readOpened;
	bool writeOpened;
	int readPos;
	int writePos;

	type *buffer;
	int bufferSize;

	// protects the data-structure
	CMutex waitStateMutex;

	// use to ensure that only a single read() call is in progress
	CMutex readerMutex;
	// use to ensure that only a single write() call is in progress
	CMutex writerMutex;

	CConditionVariable fullCond;
	CConditionVariable emptyCond;

	int privateRead(type *buffer,int size,bool block,int &readPos);

};

#define __TMemoryPipe_H__CPP
#include "TMemoryPipe.cpp"
#undef __TMemoryPipe_H__CPP

#endif

