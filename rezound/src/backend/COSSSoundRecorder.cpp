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
#define BITS 16
#define OSS_PCM_FORMAT AFMT_S16_LE // needs to match for what is above and type of sample_t ???

#define BUFFER_COUNT 64
#define BUFFER_SIZE_BYTES 4096						// buffer size in bytes
#define BUFFER_SIZE_BYTES_LOG2 12					// log2(BUFFER_SIZE_BYTES) -- that is 2^this is BUFFER_SIZE_BYTES
#define BUFFER_SIZE_SAMPLES(channelCount) (BUFFER_SIZE_BYTES/(BITS/8)/channelCount) 	// in samples


COSSSoundRecorder::COSSSoundRecorder() :
	ASoundRecorder(),

	audio_fd(-1),
	initialized(false),

	recordThread(this)
{
}

COSSSoundRecorder::~COSSSoundRecorder()
{
	deinitialize();
}

void COSSSoundRecorder::initialize(ASound *sound,const unsigned channelCount,const unsigned sampleRate)
{
	if(!initialized)
	{
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
			throw(runtime_error(string(__func__)+" -- error setting the bit rate -- the device does not support "+istring(BITS)+" bit little endian data"));
		}
		//printf("OSS: format: %d\n",format);


		// set number of channels 
		unsigned stereo=channelCount-1;     /* 0=mono, 1=stereo */
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
		//printf("OSS: channel count: %d\n",channelCount);


		// set the sample rate
		unsigned speed=sampleRate;
		if (ioctl(audio_fd, SNDCTL_DSP_SPEED, &speed)==-1) 
		{
			close(audio_fd);
			throw(runtime_error(string(__func__)+" -- error setting the sample rate -- "+strerror(errno)));
		} 
		if (speed!=sampleRate)
		{ 
			fprintf(stderr,("warning: OSS used a different sample rate ("+istring(speed)+") than what was asked for ("+istring(sampleRate)+")\n").c_str());
			//close(audio_fd);
			//throw(runtime_error(string(__func__)+" -- error setting the sample rate -- the sample rate is not supported"));
		} 
		//printf("OSS: sample rate: %d\n",speed);



		// set the buffering parameters
				// ??? I'm no quite sure why I have to -1 here... when I would print the info below, it just always had 1 more than I asked for
		int arg=((BUFFER_COUNT-1)<<16)+BUFFER_SIZE_BYTES_LOG2;  // 0xMMMMSSSS; where 0xMMMM is the number of buffers and 2^0xSSSS is the buffer size
		int parm=arg;
		if (ioctl(audio_fd, SNDCTL_DSP_SETFRAGMENT, &parm)==-1) 
		{
			close(audio_fd);
			throw(runtime_error(string(__func__)+" -- error setting the buffering parameters -- "+strerror(errno)));
		}
		else if(arg!=parm)
		{
			close(audio_fd);
			throw(runtime_error(string(__func__)+" -- error setting the buffering parameters -- "+istring(BUFFER_COUNT)+" buffers, each "+istring(BUFFER_SIZE_BYTES)+" bytes long, not supported"));
		}


		// get the fragment info
		audio_buf_info info;
		if(ioctl(audio_fd, SNDCTL_DSP_GETISPACE, &info)==-1)
		{
			close(audio_fd);
			throw(runtime_error(string(__func__)+" -- error getting the buffering parameters -- "+strerror(errno)));
		}
		
		printf("OSS record: info.fragments: %d\n",info.fragments);
		printf("OSS record: info.fragstotal: %d\n",info.fragstotal);
		printf("OSS record: info.fragsize: %d\n",info.fragsize);
		printf("OSS record: info.bytes: %d\n",info.bytes);
		

		onInit(sound,channelCount,sampleRate);

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
		recordThread.wait();

		onStop(false);

		// close OSS audio device
		close(audio_fd);

		initialized=false;
	}
}

void COSSSoundRecorder::start()
{
	onStart();

	// start record thread
	recordThread.Start();

}

void COSSSoundRecorder::redo()
{
	// do I want to clear out the buffers already recorded??? Cause it may cause a hiccup in the input containig some of the already buffered data
	// there may be a way to do this with OSS
	onRedo();
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
		sample_t buffer[BUFFER_SIZE_SAMPLES(parent->channelCount)*parent->channelCount]; 
		while(!kill)
		{
			int len;
			if((len=read(parent->audio_fd,buffer,BUFFER_SIZE_BYTES))!=BUFFER_SIZE_BYTES)
				fprintf(stderr,"warning: didn't read whole buffer -- only read %d of %d bytes\n",len,BUFFER_SIZE_BYTES);

			parent->onData(buffer,len/(sizeof(sample_t)*parent->channelCount));
		}
	}
	catch(exception &e)
	{
		cerr << "exception caught in record thread: " << e.what() << endl;
		abort();
	}
	catch(...)
	{
		cerr << "unknown exception caught in record thread" << endl;
		abort();
	}
}

