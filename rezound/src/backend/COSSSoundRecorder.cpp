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

#include "COSSSoundRecorder.h"

#include <errno.h>
#include <string.h>

#include <stdio.h> // for fprintf

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <sys/soundcard.h>

#include <stdexcept>
#include <string>
#include <iostream>

#include <istring>

// ??? edit this to be able to detect necessary parameters from the typeof sample_t
#define OSS_PCM_FORMAT AFMT_S16_LE // needs to match for what is above and type of sample_t ???

#define BUFFER_SIZE_BYTES 8192						// buffer size in bytes
#define BUFFER_SIZE_BYTES_LOG2 13					// log2(BUFFER_SIZE_BYTES) -- that is 2^this is BUFFER_SIZE_BYTES


COSSSoundRecorder::COSSSoundRecorder() :
	ASoundRecorder(),

	audio_fd(-1),
	initialized(false),

	recordThread(this),
	threadFinishedSem(1)
{
}

COSSSoundRecorder::~COSSSoundRecorder()
{
	deinitialize();
}

void COSSSoundRecorder::initialize(ASound *sound)
{
	if(!initialized)
	{
		ASoundRecorder::initialize(sound);

		// open OSS device
		if((audio_fd=open("/dev/dsp",O_RDONLY,0)) == -1) 
			throw(runtime_error(string(__func__)+" -- error opening OSS device -- "+strerror(errno)));
		//printf("OSS: device: %s\n","/dev/dsp");

		// set the bit rate and endianness
		int format=OSS_PCM_FORMAT; // signed 16-bit little endian
		if (ioctl(audio_fd, SNDCTL_DSP_SETFMT,&format)==-1)
		{
			close(audio_fd);
			throw(runtime_error(string(__func__)+" -- error setting the bit rate -- "+strerror(errno)));
		}
		else if(format!=OSS_PCM_FORMAT)
		{
			close(audio_fd);
			throw(runtime_error(string(__func__)+" -- error setting the format -- the device does not support OSS_PCM_FORMAT format data"));
		}
		//printf("OSS: format: %d\n",format);


		// set number of channels 
		unsigned stereo=sound->getChannelCount()-1;     /* 0=mono, 1=stereo */
		if (ioctl(audio_fd, SNDCTL_DSP_STEREO, &stereo)==-1)
		{
			close(audio_fd);
			throw(runtime_error(string(__func__)+" -- error setting the number of channels -- "+strerror(errno)));
		}
		else if (stereo!=1)
		{
			close(audio_fd);
			throw(runtime_error(string(__func__)+" -- error setting the number of channels -- the device does not support stereo"));
		}
		//printf("OSS: channel count: %d\n",sound->getChannelCount());


		// set the sample rate
		unsigned speed=sound->getSampleRate();
		if (ioctl(audio_fd, SNDCTL_DSP_SPEED, &speed)==-1) 
		{
			close(audio_fd);
			throw(runtime_error(string(__func__)+" -- error setting the sample rate -- "+strerror(errno)));
		} 
		if (speed!=sound->getSampleRate())
		{ 
			fprintf(stderr,("warning: OSS used a different sample rate ("+istring(speed)+") than what was asked for ("+istring(sound->getSampleRate())+")\n").c_str());
			//close(audio_fd);
			//throw(runtime_error(string(__func__)+" -- error setting the sample rate -- the sample rate is not supported"));
		} 
		//printf("OSS: sample rate: %d\n",speed);



		// set the buffering parameters
		//0x7fff means I don't need any limit
		int arg=(0x7fff<<16)|BUFFER_SIZE_BYTES_LOG2;  // 0xMMMMSSSS; where 0xMMMM is the number of buffers and 2^0xSSSS is the buffer size
		int parm=arg;
		if (ioctl(audio_fd, SNDCTL_DSP_SETFRAGMENT, &parm)==-1) 
		{
			close(audio_fd);
			throw(runtime_error(string(__func__)+" -- error setting the buffering parameters -- "+strerror(errno)));
		}
		else if(arg!=parm)
		{
			close(audio_fd);
			throw(runtime_error(string(__func__)+" -- error setting the buffering parameters -- "+istring(BUFFER_SIZE_BYTES)+" bytes long, not supported"));
		}


		// get the fragment info
		audio_buf_info info;
		if(ioctl(audio_fd, SNDCTL_DSP_GETISPACE, &info)==-1)
		{
			close(audio_fd);
			throw(runtime_error(string(__func__)+" -- error getting the buffering parameters -- "+strerror(errno)));
		}
		
		/*
		printf("OSS record: info.fragments: %d\n",info.fragments);
		printf("OSS record: info.fragstotal: %d\n",info.fragstotal);
		printf("OSS record: info.fragsize: %d\n",info.fragsize);
		printf("OSS record: info.bytes: %d\n",info.bytes);
		*/
		

		// start record thread
		threadFinishedSem.Wait(); // will immediatly return only used to decrement Sem
		recordThread.kill=false;
		if(recordThread.Start())
		{
			threadFinishedSem.Post();
			throw(runtime_error(string(__func__)+" -- error starting record thread"));
		}

		initialized=true;
	}
	else
		throw(runtime_error(string(__func__)+" -- already initialized"));
}

void COSSSoundRecorder::deinitialize()
{
	if(initialized)
	{
		// stop record thread
		recordThread.kill=true;

		// because recordThread.wait() did nothing expected, I use a semaphore to wait on the thread to finish
		threadFinishedSem.Wait();

		// close OSS audio device (which should cause the read to finish)
		close(audio_fd);

		ASoundRecorder::deinitialize();

		initialized=false;
	}
}

void COSSSoundRecorder::redo()
{
	ASoundRecorder::redo();

	// in OSS, do I want to clear out the buffers already recorded??? Cause it may cause a hiccup in the input containig some of the already buffered data
	// 	I could do non-blocking reads while info.fragments is >0 or something
	// there may be a way to do this with OSS

}

COSSSoundRecorder::CRecordThread::CRecordThread(COSSSoundRecorder *_parent) :
	Thread(),

	kill(false),
	parent(_parent)
{
}

COSSSoundRecorder::CRecordThread::~CRecordThread()
{
}

void COSSSoundRecorder::CRecordThread::Run()
{
	try
	{
		sample_t buffer[BUFFER_SIZE_BYTES/sizeof(sample_t)]; 
		while(!kill)
		{
			int len;
			if((len=read(parent->audio_fd,buffer,BUFFER_SIZE_BYTES))!=BUFFER_SIZE_BYTES)
				fprintf(stderr,"warning: didn't read whole buffer -- only read %d of %d bytes\n",len,BUFFER_SIZE_BYTES);

			parent->onData(buffer,len/(sizeof(sample_t)*parent->getChannelCount()));
		}

		parent->threadFinishedSem.Post();
	}
	catch(exception &e)
	{
		parent->threadFinishedSem.Post();
		cerr << "exception caught in record thread: " << e.what() << endl;
		abort();
	}
	catch(...)
	{
		parent->threadFinishedSem.Post();
		cerr << "unknown exception caught in record thread" << endl;
		abort();
	}
}

