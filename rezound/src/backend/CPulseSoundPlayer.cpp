/* 
 * Copyright (C) 2010 - David W. Durham
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

#include "CPulseSoundPlayer.h"

#include <stdexcept>
using namespace std;

#ifdef ENABLE_PULSE

#include <stdio.h>

#include <pulse/simple.h>
#include <pulse/error.h>

#include "settings.h"

#define BUFFER_COUNT gDesiredOutputBufferCount
#define BUFFER_SIZE_FRAMES gDesiredOutputBufferSize
#define BUFFER_SIZE_BYTES(sample_size) (BUFFER_SIZE_FRAMES*(sample_size)*gDesiredOutputChannelCount)		// buffer size in bytes

CPulseSoundPlayer::CPulseSoundPlayer() :
	ASoundPlayer(),

	initialized(false),
	stream(NULL),
	//supportsFullDuplex(false),
	//wasInitializedBeforeRecording(false),

	playThread(this)
{
}

CPulseSoundPlayer::~CPulseSoundPlayer()
{
	deinitialize();
}

bool CPulseSoundPlayer::isInitialized() const
{
	return initialized;
}

void CPulseSoundPlayer::initialize()
{
	if(!initialized)
	{
		try
		{
			pa_sample_spec ss;
#ifndef WORDS_BIGENDIAN
	#if defined(SAMPLE_TYPE_S16)
			ss.format = PA_SAMPLE_S16LE;
	#elif defined(SAMPLE_TYPE_FLOAT)
			ss.format = PA_SAMPLE_FLOAT32LE;
	#else
		#error unhandled SAMPLE_TYPE_xxx define
	#endif
#else
	#if defined(SAMPLE_TYPE_S16)
			ss.format = PA_SAMPLE_S16BE;
	#elif defined(SAMPLE_TYPE_FLOAT)
			ss.format = PA_SAMPLE_FLOAT32BE;
	#else
		#error unhandled SAMPLE_TYPE_xxx define
	#endif
#endif
			ss.rate = gDesiredOutputSampleRate;
			ss.channels = gDesiredOutputChannelCount;


			// Create a new playback stream

			pa_buffer_attr buffattr;
			//below buffattr.maxlength=
			buffattr.tlength=BUFFER_SIZE_BYTES(sizeof(sample_t));
			buffattr.prebuf=(uint32_t)-1;
			buffattr.minreq=(uint32_t)-1;
			buffattr.fragsize=(uint32_t)-1;
			buffattr.maxlength=2*buffattr.tlength;

			int error;
			if (!(stream = pa_simple_new(NULL, PACKAGE_STRING, PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, &buffattr, &error))) {
				throw runtime_error(string(__func__)+" -- error opening Pulse device -- " + pa_strerror(error));
			}

			devices[0].channelCount=ss.channels; // make note of the number of channels for this device (??? which is only device zero for now)
			devices[0].sampleRate=ss.rate; // make note of the sample rate for this device (??? which is only device zero for now)


			// set parameters and latency???

			// start play thread
			playThread.kill=false;

			ASoundPlayer::initialize();
			playThread.start();
			initialized=true;
			fprintf(stderr, "PulseAudio player initialized\n");
		}
		catch(...)
		{
			if(playThread.isRunning())
			{
				playThread.kill=true;
				playThread.wait();
			}
			if(stream)
			{
				pa_simple_free(stream);
				stream=NULL;
			}

			ASoundPlayer::deinitialize();
			throw;
		}
	}
	else
		throw runtime_error(string(__func__)+" -- already initialized");
}

void CPulseSoundPlayer::deinitialize()
{
	if(initialized)
	{
		ASoundPlayer::deinitialize();

		// stop play thread
		playThread.kill=true;
		playThread.wait();
		
		// close Pulse audio device
		if(stream)
		{
			pa_simple_free(stream);
			stream=NULL;
		}

		initialized=false;

		fprintf(stderr, "PulseAudio player deinitialized\n");
	}
}

void CPulseSoundPlayer::aboutToRecord()
{
/*
	wasInitializedBeforeRecording=isInitialized(); // avoid attempting to initialize when done if we're not already initialized now
	if(!supportsFullDuplex)
		deinitialize();
*/
}

void CPulseSoundPlayer::doneRecording()
{
/*
	if(wasInitializedBeforeRecording && !initialized && !supportsFullDuplex)
		initialize();
*/
}


CPulseSoundPlayer::CPlayThread::CPlayThread(CPulseSoundPlayer *_parent) :
	AThread(),

	kill(false),
	parent(_parent)
{
}

CPulseSoundPlayer::CPlayThread::~CPlayThread()
{
}

void CPulseSoundPlayer::CPlayThread::main()
{
	try
	{
		// ??? when I start to more fully support multiple devices I need to read from the devices array and not from the global settings here

		TAutoBuffer<sample_t> buffer(BUFFER_SIZE_FRAMES*gDesiredOutputChannelCount*2/*<--padding*/, true); 

		while(!kill)
		{
			// can mixChannels throw any exception?
			parent->mixSoundPlayerChannels(gDesiredOutputChannelCount,buffer,BUFFER_SIZE_FRAMES);


			int error;
			if(pa_simple_write(parent->stream, buffer, BUFFER_SIZE_BYTES(sizeof(sample_t)), &error)) {
				fprintf(stderr,"warning: error writing buffer to pulse audio -- %s\n", pa_strerror(error));
			}
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

#endif // ENABLE_PULSE
