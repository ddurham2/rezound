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

#include "COSSSoundPlayer.h"

#ifdef ENABLE_OSS

#include <errno.h>
#include <string.h>

#include <stdio.h> // for fprintf
#include <math.h> // for log

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <sys/soundcard.h>


#include <stdexcept>
#include <string>

#include <istring>
#include <TAutoBuffer.h>

#include "settings.h"

#define BUFFER_COUNT gDesiredOutputBufferCount
#define BUFFER_SIZE_FRAMES gDesiredOutputBufferSize
#define BUFFER_SIZE_BYTES(sample_size) (BUFFER_SIZE_FRAMES*(sample_size)*gDesiredOutputChannelCount)		// buffer size in bytes
#define BUFFER_SIZE_BYTES_LOG2(sample_size) ((size_t)(log((double)BUFFER_SIZE_BYTES((sample_size)))/log(2.0)))	// log2(BUFFER_SIZE_BYTES) -- that is 2^this is BUFFER_SIZE_BYTES


COSSSoundPlayer::COSSSoundPlayer() :
	ASoundPlayer(),

	initialized(false),
	audio_fd(-1),
	supportsFullDuplex(false),
	wasInitializedBeforeRecording(false),

	playThread(this)
{
}

COSSSoundPlayer::~COSSSoundPlayer()
{
	deinitialize();
}

bool COSSSoundPlayer::isInitialized() const
{
	return initialized;
}

void COSSSoundPlayer::initialize()
{
	if(!initialized)
	{
		try
		{
			int sampleFormat=0;
			int sampleSize=0;
			string sSampleFormat="none";
#warning need to change this to try several formats for a supported implementation because float is being reduced to 16bit right now
#ifndef WORDS_BIGENDIN
			// little endian platform
	#if defined(SAMPLE_TYPE_S16)
			sampleFormat=AFMT_S16_LE;
			sampleSize=2;
			sSampleFormat="little endian 16bit signed";
	#elif defined(SAMPLE_TYPE_FLOAT)
			sampleFormat=AFMT_S16_LE; // closest thing
			sampleSize=2;
			sSampleFormat="little endian 24bit signed";
	#else
			#error unhandled SAMPLE_TYPE_xxx define
	#endif
#else
			// big endian platform
	#if defined(SAMPLE_TYPE_S16)
			sampleFormat=AFMT_S16_BE;
			sampleSize=2;
			sSampleFormat="big endian 16bit signed";
	#elif defined(SAMPLE_TYPE_FLOAT)
			sampleFormat=AFMT_S16_BE; // closest thing
			sampleSize=2;
			sSampleFormat="big endian 24bit signed";
	#else
			#error unhandled SAMPLE_TYPE_xxx define
	#endif
#endif

			// open OSS device
			const string device=gOSSOutputDevice;
			if((audio_fd=open(device.c_str(),O_WRONLY|O_NONBLOCK)) == -1) 
			{
				const string errString=strerror(errno);
				throw runtime_error(string(__func__)+" -- error opening OSS device '"+device+" -- "+errString);
			}
			//printf("OSS: device: %s\n",device.c_str());

			// just did O_NONBLOCK so it wouldn't hang by waiting on the resource.. now set it back to blocking
			int flags=fcntl(audio_fd,F_GETFL,0);
			flags&=(~O_NONBLOCK); // turn off the O_NONBLOCK flag
			if(fcntl(audio_fd,F_SETFL,flags)!=0)
			{
				const string errString=strerror(errno);
				close(audio_fd);
				throw runtime_error(string(__func__)+" -- error setting OSS device '"+device+" back to blocking I/O mode -- "+errString);
			}

			// set the bit rate and endianness
			int format=sampleFormat;
			if (ioctl(audio_fd, SNDCTL_DSP_SETFMT,&format)==-1)
			{
				const string errString=strerror(errno);
				close(audio_fd);
				throw runtime_error(string(__func__)+" -- error setting the format -- "+errString);
			}
			else if(format!=sampleFormat)
			{
				close(audio_fd);
				throw runtime_error(string(__func__)+" -- error setting the format -- the device does not support '"+sSampleFormat+"' (OSS format #"+istring(sampleFormat)+") formatted data");
			}
			//printf("OSS: sampleFormat: %d\n",sampleFormat);


			// set number of channels 
			unsigned channelCount=gDesiredOutputChannelCount;
			if (ioctl(audio_fd, SNDCTL_DSP_CHANNELS, &channelCount)==-1)
			{
				const string errString=strerror(errno);
				close(audio_fd);
				throw runtime_error(string(__func__)+" -- error setting the number of channels -- "+errString);
			}
			else if (channelCount!=gDesiredOutputChannelCount)
			{
				close(audio_fd);
				throw runtime_error(string(__func__)+" -- error setting the number of channels -- the device does not support "+istring(gDesiredOutputChannelCount)+" channel playback");
			}
			//printf("OSS: channel count: %d\n",gDesiredOutputChannelCount);

			devices[0].channelCount=channelCount; // make note of the number of channels for this device (??? which is only device zero for now)

			// set the sample rate
			unsigned sampleRate=gDesiredOutputSampleRate;
			if (ioctl(audio_fd, SNDCTL_DSP_SPEED, &sampleRate)==-1) 
			{
				const string errString=strerror(errno);
				close(audio_fd);
				throw runtime_error(string(__func__)+" -- error setting the sample rate -- "+errString);
			} 
			if (sampleRate!=gDesiredOutputSampleRate)
			{ 
				fprintf(stderr,("warning: OSS used a different sample rate ("+istring(sampleRate)+") than what was asked for ("+istring(gDesiredOutputSampleRate)+"); will have to do extra calculations to compensate\n").c_str());
				//close(audio_fd);
				//throw runtime_error(string(__func__)+" -- error setting the sample rate -- the sample rate is not supported");
			} 
			//printf("OSS: sample rate: %d\n",sampleRate);

			devices[0].sampleRate=sampleRate; // make note of the sample rate for this device (??? which is only device zero for now)


			// set the buffering parameters
			const int arg=((BUFFER_COUNT)<<16)|BUFFER_SIZE_BYTES_LOG2(sampleSize);  // 0xMMMMSSSS; where 0xMMMM is the number of buffers and 2^0xSSSS is the buffer size
			int parm=arg;
			if (ioctl(audio_fd, SNDCTL_DSP_SETFRAGMENT, &parm)==-1) 
			{
				const string errString=strerror(errno);
				close(audio_fd);
				throw runtime_error(string(__func__)+" -- error setting the buffering parameters -- "+errString);
			}
			else if(arg!=parm)
			{
				// check buffer count
				const int actualBufferCount=parm>>16;
				if(actualBufferCount<2 || actualBufferCount>(2*BUFFER_COUNT))
				{ // don't really complain unless it comes back unreasonably different
					close(audio_fd);
					throw runtime_error(string(__func__)+" -- error setting the buffering parameters -- asking for "+istring(BUFFER_COUNT)+" buffers not supported");
				}

				// check fragment size
				const unsigned actualBufferSize=parm&0xffff;
				if(actualBufferSize!=BUFFER_SIZE_BYTES_LOG2(sampleSize))
				{
					close(audio_fd);
					throw runtime_error(string(__func__)+" -- error setting the buffering parameters -- asking for "+istring(BUFFER_COUNT)+" buffers, each "+istring(BUFFER_SIZE_BYTES(sampleSize))+" bytes long, not supported");
				}
			}


			// get the fragment info
			audio_buf_info info;
			if(ioctl(audio_fd, SNDCTL_DSP_GETOSPACE, &info)==-1)
			{
				const string errString=strerror(errno);
				close(audio_fd);
				throw runtime_error(string(__func__)+" -- error getting the buffering parameters -- "+errString);
			}


			// get the device's capabilities bit mask
			int caps;
			if(ioctl(audio_fd, SNDCTL_DSP_GETCAPS, &caps)==-1)
				caps=0;

			// determine if the device supports full duplex mode
			// 	??? http://www.4front-tech.com/pguide/audio2.html#fulldup says that this should be checked AFTER attempting to put the card into full duplex mode... Shouldn't I be able to check the for ability before attempting to use it?
			supportsFullDuplex= (caps&DSP_CAP_DUPLEX) ? true : false;

			/*
			printf("OSS player: info.fragments: %d\n",info.fragments);
			printf("OSS player: info.fragstotal: %d\n",info.fragstotal);
			printf("OSS player: info.fragsize: %d\n",info.fragsize);
			printf("OSS player: info.bytes: %d\n",info.bytes);
			printf("OSS player: supportsFullDuplex: %d\n",supportsFullDuplex);
			*/

			// start play thread
			playThread.kill=false;
			playThread.start();

			ASoundPlayer::initialize();
			initialized=true;
		}
		catch(...)
		{
			ASoundPlayer::deinitialize();
			throw;
		}
	}
	else
		throw runtime_error(string(__func__)+" -- already initialized");
}

void COSSSoundPlayer::deinitialize()
{
	if(initialized)
	{
		ASoundPlayer::deinitialize();

		// stop play thread
		playThread.kill=true;
		playThread.wait();
		
		// close OSS audio device
		close(audio_fd);

		initialized=false;

		//printf("OSS player: deinitialized\n");
	}
}

void COSSSoundPlayer::aboutToRecord()
{
	wasInitializedBeforeRecording=isInitialized(); // avoid attempting to initialize when done if we're not already initialized now
	if(!supportsFullDuplex)
		deinitialize();
}

void COSSSoundPlayer::doneRecording()
{
	if(wasInitializedBeforeRecording && !initialized && !supportsFullDuplex)
		initialize();
}


COSSSoundPlayer::CPlayThread::CPlayThread(COSSSoundPlayer *_parent) :
	AThread(),

	kill(false),
	parent(_parent)
{
}

COSSSoundPlayer::CPlayThread::~CPlayThread()
{
}

void COSSSoundPlayer::CPlayThread::main()
{
	try
	{
		// ??? when I start to more fully support multiple devices I need to read from the devices array and not from the global settings here

		/*
		 * NOTE: in linux, during testing when this thread was overruning this
		 * 	mix buffer, the thread would die, but the process would hang.
		 * 	I think it was sending a signal that perhaps I should catch.
		 *      So, I'm just making it bigger than necessary
		 */
		TAutoBuffer<sample_t> buffer(BUFFER_SIZE_FRAMES*gDesiredOutputChannelCount*2); 
			// these are possibly used if sample format conversion is required
		TAutoBuffer<int16_t> buffer__int16_t(BUFFER_SIZE_FRAMES*gDesiredOutputChannelCount);
		TAutoBuffer<int32_t> buffer__int32_t(BUFFER_SIZE_FRAMES*gDesiredOutputChannelCount);
		TAutoBuffer<float> buffer__float(BUFFER_SIZE_FRAMES*gDesiredOutputChannelCount);

		while(!kill)
		{
			// can mixChannels throw any exception?
			parent->mixSoundPlayerChannels(gDesiredOutputChannelCount,buffer,BUFFER_SIZE_FRAMES);

			int len;

			// must do conversion of sample_t type -> initialized format type if necessary
#if defined(SAMPLE_TYPE_S16)
			// if(initialize to S16) ???
			{ // no conversion necessary
				if((len=write(parent->audio_fd,buffer,BUFFER_SIZE_FRAMES*sizeof(sample_t)*gDesiredOutputChannelCount))!=(int)BUFFER_SIZE_BYTES(sizeof(int16_t)))
					fprintf(stderr,"warning: didn't write whole buffer -- only wrote %d of %d bytes\n",len,BUFFER_SIZE_BYTES(sizeof(int16_t)));
			}

#elif defined(SAMPLE_TYPE_FLOAT)
			
			// if(initialized to S16) ???
			{ // we need to convert float->int16_t
				unsigned l=BUFFER_SIZE_FRAMES*gDesiredOutputChannelCount;
				for(unsigned t=0;t<l;t++)
					buffer__int16_t[t]=convert_sample<float,int16_t>(buffer[t]);
				if((len=write(parent->audio_fd,buffer__int16_t,BUFFER_SIZE_FRAMES*sizeof(int16_t)*gDesiredOutputChannelCount))!=(int)BUFFER_SIZE_BYTES(sizeof(int16_t)))
					fprintf(stderr,"warning: didn't write whole buffer -- only wrote %d of %d bytes\n",len,BUFFER_SIZE_BYTES(sizeof(int16_t)));
			}

#else
			#error unhandled SAMPLE_TYPE_xxx define
#endif
		}
	}
	catch(exception &e)
	{
		fprintf(stderr,"exception caught in play thread: %s\n",e.what());
	}
	catch(...)
	{
		fprintf(stderr,"unknown exception caught in play thread\n");
	}
}

#endif // ENABLE_OSS
