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

#include "CPortAudioSoundPlayer.h"

#ifdef ENABLE_PORTAUDIO

#include <stdio.h>
#include <string.h>

#include <stdexcept>

#include "settings.h"



CPortAudioSoundPlayer::CPortAudioSoundPlayer() :
	ASoundPlayer(),

	initialized(false),
	stream(NULL),
	supportsFullDuplex(true)
{
	PaError err=Pa_Initialize();
	if(err!=paNoError) 
		throw runtime_error(string(__func__)+" -- error initializing PortAudio -- "+Pa_GetErrorText(err));
}

CPortAudioSoundPlayer::~CPortAudioSoundPlayer()
{
	deinitialize();
	Pa_Terminate();
}

bool CPortAudioSoundPlayer::isInitialized() const
{
	return initialized;
}

void CPortAudioSoundPlayer::initialize()
{
	if(!initialized)
	{
		PaError err;

		PaSampleFormat sampleFormat;
#if defined(SAMPLE_TYPE_S16)
			sampleFormat=paInt16;
#elif defined(SAMPLE_TYPE_FLOAT)
			sampleFormat=paFloat32;
#else
			#error unhandled SAMPLE_TYPE_xxx define
#endif
		

		// open a PortAudio stream
#ifdef ENABLE_PORTAUDIO_V19
		PaStreamParameters output = { gPortAudioOutputDevice, 
			static_cast<int>(gDesiredOutputChannelCount),
			sampleFormat,
			Pa_GetDeviceInfo(gPortAudioOutputDevice)->defaultLowOutputLatency ,
			NULL};

		err = Pa_OpenStream(
				&stream,
				NULL,
				&output,
				gDesiredOutputSampleRate,
				gDesiredOutputBufferSize * gDesiredOutputBufferCount,
				paClipOff|paDitherOff,
				CPortAudioSoundPlayer::PortAudioCallback,
				this);

#else
		err = Pa_OpenStream(
			&stream,
			paNoDevice,			/* recording parameter, we're not recording */
			0,				/* recording parameter, we're not recording */
			paInt16,			/* recording parameter, we're not recording */
			NULL,
			gPortAudioOutputDevice,
			gDesiredOutputChannelCount,
			sampleFormat,
			NULL,
			gDesiredOutputSampleRate,
			gDesiredOutputBufferSize,
			gDesiredOutputBufferCount,
			paClipOff|paDitherOff,
			CPortAudioSoundPlayer::PortAudioCallback,
			this);
#endif

		if(err!=paNoError) 
			throw runtime_error(string(__func__)+" -- error opening PortAudio stream -- "+Pa_GetErrorText(err));
#warning test with some parameters that I know will fail

				// ??? is PortAudio is not going to make the conversion if it cannot get this exact sample reate
				// and right now, there is no way in libportaudio to get the sample rate that was obtained... So 
				// when they do implement being able to know at what sample rate the device was opened, I will query here
		devices[0].sampleRate=gDesiredOutputSampleRate; // make note of the sample rate for this device (??? which is only device zero for now)
		devices[0].channelCount=gDesiredOutputChannelCount; // make note of the number of channels for this device (??? which is only device zero for now)

		// start the PortAudio stream
		err=Pa_StartStream(stream);
		if(err!=paNoError) 
			throw runtime_error(string(__func__)+" -- error starting PortAudio stream -- "+Pa_GetErrorText(err));

		supportsFullDuplex=false;
/* ??? implement this when/if possible
		supportsFullDuplex= query libportaudio if the device supports fullduplex mode ... perhaps write a method that runs prior to this point that tried to open the device with input and output parameters to Pa_OpenStream
*/

		ASoundPlayer::initialize();
		initialized=true;
		fprintf(stderr, "PortAudio player initialized\n");
	}
	else
		throw runtime_error(string(__func__)+" -- already initialized");
}

void CPortAudioSoundPlayer::deinitialize()
{
	if(initialized)
	{
		PaError err;

		ASoundPlayer::deinitialize();

		// stop the PortAudio stream
		err=Pa_StopStream(stream);
		if(err!=paNoError) 
			fprintf(stderr,"%s -- error starting PortAudio stream -- %s\n",__func__,Pa_GetErrorText(err));

		// close PortAudio stream
		err=Pa_CloseStream(stream);
		if(err!=paNoError) 
			fprintf(stderr,"%s -- error closing PortAudio stream -- %s\n",__func__,Pa_GetErrorText(err));

		stream=NULL;
		initialized=false;
		fprintf(stderr, "PortAudio player deinitialized\n");
	}
}

void CPortAudioSoundPlayer::aboutToRecord()
{
	// ??? Of course it would be nice to know if the record device about to be used is the same as this playback device...
	// I would just use the port audio device and make it a parameter to this method, except port audio may not been the API being
	// used... Perhaps what I should do is commit to a rule that if I'm using an API for playback, that I have to use the same API
	// for recording.. then I could use a void * parameter to this method and I would know it's an int index into PortAudio's known devices
	//  BUT... if I commited to this rule, then perhaps it would make sense to just merge ASoundPlayer and ASoundRecorder together
	// or maybe just into the same file so then only one file compiles and I could know that the device identification method was consistant
	if(!supportsFullDuplex)
		deinitialize();
}

void CPortAudioSoundPlayer::doneRecording()
{
	if(!initialized && !supportsFullDuplex)
		initialize();
}


#ifdef ENABLE_PORTAUDIO_V19
int CPortAudioSoundPlayer::PortAudioCallback(const void *inputBuffer,void *outputBuffer,unsigned long framesPerBuffer,const PaStreamCallbackTimeInfo* outTime, PaStreamCallbackFlags statusFlags, void *userData)
#else
int CPortAudioSoundPlayer::PortAudioCallback(void *inputBuffer,void *outputBuffer,unsigned long framesPerBuffer,PaTimestamp outTime,void *userData)
#endif
{
	CPortAudioSoundPlayer* that = (CPortAudioSoundPlayer *)userData;
	if(!that->initialized) {
		return 0;
	}

	try
	{
		// no conversion necessary because we initialized it with the type of sample_t (if portaudio didn't support our sample_t type natively then we would need to do conversion here)
		that->mixSoundPlayerChannels(gDesiredOutputChannelCount,(sample_t *)outputBuffer,framesPerBuffer);
	}
	catch(exception &e)
	{
		fprintf(stderr,"exception caught in play callback: %s\n",e.what());
	}
	catch(...)
	{
		fprintf(stderr,"unknown exception caught in play callback\n");
	}
	return 0;
}

#endif // ENABLE_PORTAUDIO
