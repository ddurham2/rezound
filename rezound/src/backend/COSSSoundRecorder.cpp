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

#ifdef ENABLE_OSS

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

#include "settings.h"

// ??? edit this to be able to detect necessary parameters from the typeof sample_t
// 	or I need to convert to 16bit 
// needs to match for what is above and type of sample_t ???
// ??? BTW- on a big endian machine, AFMT_S16_BE is available
// 	also now existing the OSS documenation I was reading are AFMT_MPEG 
// 	and AFMT_AC3 which would be nice for mroe than stereo
#define OSS_PCM_FORMAT AFMT_S16_LE

// ??? as the sample rate is lower these need to be lower so that onData is called more often and the view meters on the record dialog don't seem to lag

#define BUFFER_SIZE_BYTES 4096						// buffer size in bytes
#define BUFFER_SIZE_BYTES_LOG2 12					// log2(BUFFER_SIZE_BYTES) -- that is 2^this is BUFFER_SIZE_BYTES


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

void COSSSoundRecorder::initialize(CSound *sound)
{
	if(!initialized)
	{
		ASoundRecorder::initialize(sound);
		try
		{
			// open OSS device
			const string device=gOSSInputDevice;
			if((audio_fd=open(device.c_str(),O_RDONLY|O_NONBLOCK)) == -1) 
			{
				const string errString=strerror(errno);
				throw(runtime_error(string(__func__)+" -- error opening OSS device '"+device+" -- "+errString));
			}
			//printf("OSS: device: %s\n",device.c_str());

			// just did O_NONBLOCK so it wouldn't hang by waiting on the resource.. now set it back to blocking
			int flags=fcntl(audio_fd,F_GETFL,0);
			flags&=(~O_NONBLOCK); // turn off the O_NONBLOCK flag
			if(fcntl(audio_fd,F_SETFL,flags)!=0)
			{
				const string errString=strerror(errno);
				close(audio_fd);
				throw(runtime_error(string(__func__)+" -- error setting OSS device '"+device+" back to blocking I/O mode -- "+errString));
			}

			// set the bit rate and endianness
			int format=OSS_PCM_FORMAT; // signed 16-bit little endian
			if (ioctl(audio_fd, SNDCTL_DSP_SETFMT,&format)==-1)
			{
				const string errString=strerror(errno);
				close(audio_fd);
				throw(runtime_error(string(__func__)+" -- error setting the bit rate -- "+errString));
			}
			else if(format!=OSS_PCM_FORMAT)
			{
				close(audio_fd);
				throw(runtime_error(string(__func__)+" -- error setting the format -- the device does not support OSS_PCM_FORMAT format data"));
			}
			//printf("OSS: format: %d\n",format);


			// set number of channels 
			unsigned channelCount=sound->getChannelCount();
			if (ioctl(audio_fd, SNDCTL_DSP_CHANNELS, &channelCount)==-1)
			{
				const string errString=strerror(errno);
				close(audio_fd);
				throw(runtime_error(string(__func__)+" -- error setting the number of channels -- "+errString));
			}
			else if (channelCount!=sound->getChannelCount())
			{
				close(audio_fd);
				throw(runtime_error(string(__func__)+" -- error setting the number of channels -- the device does not support "+istring(sound->getChannelCount())+" channel recording"));
			}
			//printf("OSS: channel count: %d\n",sound->getChannelCount());


			// set the sample rate
			unsigned sampleRate=sound->getSampleRate();
			if (ioctl(audio_fd, SNDCTL_DSP_SPEED, &sampleRate)==-1) 
			{
				const string errString=strerror(errno);
				close(audio_fd);
				throw(runtime_error(string(__func__)+" -- error setting the sample rate -- "+errString));
			} 
			if (sampleRate!=sound->getSampleRate())
			{ 
				fprintf(stderr,("warning: OSS used a different sample rate ("+istring(sampleRate)+") than what was asked for ("+istring(sound->getSampleRate())+")\n").c_str());
				//close(audio_fd);
				//throw(runtime_error(string(__func__)+" -- error setting the sample rate -- the sample rate is not supported"));
			} 
			//printf("OSS: sample rate: %d\n",sampleRate);



			// set the buffering parameters
			//0x7fff means I don't need any limit
			int arg=(0x7fff<<16)|BUFFER_SIZE_BYTES_LOG2;  // 0xMMMMSSSS; where 0xMMMM is the number of buffers and 2^0xSSSS is the buffer size
			int parm=arg;
			if (ioctl(audio_fd, SNDCTL_DSP_SETFRAGMENT, &parm)==-1) 
			{
				const string errString=strerror(errno);
				close(audio_fd);
				throw(runtime_error(string(__func__)+" -- error setting the buffering parameters -- "+errString));
			}
			else if((arg&0xffff)!=(parm&0xffff))
			{
				close(audio_fd);
				throw(runtime_error(string(__func__)+" -- error setting the buffering parameters -- "+istring(BUFFER_SIZE_BYTES)+" bytes long, not supported"));
			}


			// get the fragment info
			audio_buf_info info;
			if(ioctl(audio_fd, SNDCTL_DSP_GETISPACE, &info)==-1)
			{
				const string errString=strerror(errno);
				close(audio_fd);
				throw(runtime_error(string(__func__)+" -- error getting the buffering parameters -- "+errString));
			}
			
			/*
			printf("OSS record: info.fragments: %d\n",info.fragments);
			printf("OSS record: info.fragstotal: %d\n",info.fragstotal);
			printf("OSS record: info.fragsize: %d\n",info.fragsize);
			printf("OSS record: info.bytes: %d\n",info.bytes);
			*/
			

			// start record thread
			recordThread.kill=false;
			recordThread.start();

			initialized=true;
		}
		catch(...)
		{
			ASoundRecorder::deinitialize();
			throw;
		}
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

		// close OSS audio device (which should cause the read to finish)
		close(audio_fd);

		ASoundRecorder::deinitialize();

		initialized=false;
	}
}

void COSSSoundRecorder::redo()
{
/* 
	   When recording, if the user hits redo, there may be 
	recorded buffers within OSS that we haven't read yet, 
	so we need to flush them.
	   I do this by setting the file descripter to 
	non-blocking and then attempt read 1+the number of 
	fragments waiting to be read and protect the reads with 
	a mutex so the recording thread won't read and send the 
	data to the derivation
*/

/*
	*** However, I would have expected that this code was necessary, 
	but in many test records (without this code) I never found it to 
	be a problem, so I'm not going to enable the code which should 
	prevent the problem until I see it as a problem.  Mainly because 
	I can't test that it's doing what its supposed to do.  Maybe it 
	would show up as a problem on a sound card driver which would 
	buffer more than 64k, because that's all the emu10k1 drivers will 
	buffer.  Or perhaps I could test the recording under a heavily 
	loaded machine where the recording thread wouldn't be able to get
	around to reading as readily.

	redoMutex.lock();
	try
	{
*/
		ASoundRecorder::redo();
/*

		// I want to clear out the buffers already recorded that I haven't read yet
		fcntl(audio_fd,F_SETFL,O_NONBLOCK);

		audio_buf_info info;
		if(ioctl(audio_fd, SNDCTL_DSP_GETISPACE, &info)==-1)
		{
			close(audio_fd);
			throw(runtime_error(string(__func__)+" -- error getting the buffering parameters -- "+strerror(errno)));
		}
		printf("OSS record: info.fragments: %d\n",info.fragments);

		sample_t buffer[BUFFER_SIZE_BYTES/sizeof(sample_t)]; 
		for(int t=0;t<info.fragments+1;t++)
			read(audio_fd,buffer,BUFFER_SIZE_BYTES);

		int f=fcntl(audio_fd,F_GETFL);
		f&=~O_NONBLOCK;
		fcntl(audio_fd,F_SETFL,f);

		redoMutex.unlock();
	}
	catch(...)
	{
		redoMutex.unlock();
		throw;
	}

*/
}

COSSSoundRecorder::CRecordThread::CRecordThread(COSSSoundRecorder *_parent) :
	AThread(),

	kill(false),
	parent(_parent)
{
}

COSSSoundRecorder::CRecordThread::~CRecordThread()
{
}

void COSSSoundRecorder::CRecordThread::main()
{
/*
	bool redoMutexLocked=false;
*/
	try
	{
		sample_t buffer[BUFFER_SIZE_BYTES/sizeof(sample_t)]; 
		while(!kill)
		{
/*
			parent->redoMutex.lock();
			redoMutexLocked=true;
*/

			int len,err;
			if((len=read(parent->audio_fd,buffer,BUFFER_SIZE_BYTES))!=BUFFER_SIZE_BYTES)
			{
				if(len==-1)
					fprintf(stderr,"warning: error returned by read() function -- %s\n",strerror(errno));
				else
					fprintf(stderr,"warning: didn't read whole buffer -- only read %d of %d bytes\n",len,BUFFER_SIZE_BYTES);
			}

			if(len!=-1)
				parent->onData(buffer,len/(sizeof(sample_t)*parent->getChannelCount()));
			// else wait a few milliseconds?

/*
			redoMutexLocked=false;
			parent->redoMutex.unlock();
*/
		}
	}
	catch(exception &e)
	{
/*
		if(redoMutexLocked)
			parent->redoMutex.unlock();
*/
		cerr << "exception caught in record thread: " << e.what() << endl;

		abort();
	}
	catch(...)
	{
/*
		if(redoMutexLocked)
			parent->redoMutex.unlock();
*/
		cerr << "unknown exception caught in record thread" << endl;

		abort();
	}
}

#endif // ENABLE_OSS
