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

#include "CPortAudioSoundRecorder.h"

#ifdef ENABLE_PORTAUDIO

#include <stdexcept>
#include <typeinfo>

#include <istring>

#include "settings.h"

// ??? as the sample rate is lower these need to be lower so that onData is called more often and the view meters on the record dialog don't seem to lag
#define BUFFER_SIZE_FRAMES 1024


CPortAudioSoundRecorder::CPortAudioSoundRecorder() :
	ASoundRecorder(),

	stream(NULL),
	initialized(false)
{
	PaError err=Pa_Initialize();
	if(err!=paNoError) 
		throw(runtime_error(string(__func__)+" -- error initializing PortAudio -- "+Pa_GetErrorText(err)));
}

CPortAudioSoundRecorder::~CPortAudioSoundRecorder()
{
	deinitialize();
	Pa_Terminate();
}

void CPortAudioSoundRecorder::initialize(CSound *sound)
{
	if(!initialized)
	{
		PaError err;

		PaSampleFormat sampleFormat=0;
		if(typeid(sample_t)==typeid(int16_t))
			sampleFormat=paInt16;
		else if(typeid(sample_t)==typeid(float))
			sampleFormat=paFloat32;
		else
			throw(runtime_error(string(__func__)+" -- unhandled sample_t format"));
		
		ASoundRecorder::initialize(sound);

		// open a PortAudio stream
		err = Pa_OpenStream(
			&stream,
			gPortAudioOutputDevice,
			sound->getChannelCount(),
			sampleFormat,
			NULL,
			paNoDevice,			/* no output device */
			0,				/* no output */
			paInt16,			/* no output */
			NULL,
			sound->getSampleRate(),
			BUFFER_SIZE_FRAMES,
			12,				/* an arbitrary value I chose */
			paClipOff|paDitherOff,
			CPortAudioSoundRecorder::PortAudioCallback,
			this);

		if(err!=paNoError) 
			throw(runtime_error(string(__func__)+" -- error opening PortAudio stream -- "+Pa_GetErrorText(err)));
#warning test with some parameters that I know will fail

		// start the PortAudio stream
		err=Pa_StartStream(stream);
		if(err!=paNoError) 
			throw(runtime_error(string(__func__)+" -- error starting PortAudio stream -- "+Pa_GetErrorText(err)));

		initialized=true;
	}
	else
		throw(runtime_error(string(__func__)+" -- already initialized"));
}

void CPortAudioSoundRecorder::deinitialize()
{
	if(initialized)
	{
		PaError err;

		// stop the PortAudio stream
		err=Pa_StopStream(stream);
		if(err!=paNoError) 
			fprintf(stderr,"%s -- error starting PortAudio stream -- %s\n",__func__,Pa_GetErrorText(err));

		// close PortAudio stream
		err=Pa_CloseStream(stream);
		if(err!=paNoError) 
			fprintf(stderr,"%s -- error closing PortAudio stream -- %s\n",__func__,Pa_GetErrorText(err));

		ASoundRecorder::deinitialize();

		stream=NULL;
		initialized=false;
	}
}

void CPortAudioSoundRecorder::redo()
{
/* 
	   When recording, if the user hits redo, there may be 
	recorded buffers within PortAudio that we haven't read yet.
	So, this will hopefully clear them, and do it quickly.
*/
	Pa_AbortStream(stream);
	Pa_StartStream(stream);
}

int CPortAudioSoundRecorder::PortAudioCallback(void *inputBuffer,void *outputBuffer,unsigned long framesPerBuffer,PaTimestamp outTime,void *userData)
{
	try
	{
		((CPortAudioSoundRecorder *)userData)->onData((const sample_t *)inputBuffer,framesPerBuffer);
	}
	catch(exception &e)
	{
		fprintf(stderr,"exception caught in record thread: %s\n",e.what());
		abort();
	}
	catch(...)
	{
		fprintf(stderr,"unknown exception caught in record thread\n");
		abort();
	}
	return 0;
}

#endif // ENABLE_PORTAUDIO
