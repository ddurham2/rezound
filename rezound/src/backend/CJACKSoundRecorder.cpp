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

#include "CJACKSoundRecorder.h"

#ifdef ENABLE_JACK

#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <stdexcept>
#include <typeinfo>

#include <istring>

#include "settings.h"
#include "AFrontendHooks.h"


CJACKSoundRecorder::CJACKSoundRecorder() :
	ASoundRecorder(),

	initialized(false),
	client(NULL),

	tempBuffer(1)
{
}

CJACKSoundRecorder::~CJACKSoundRecorder()
{
	deinitialize();
}

void CJACKSoundRecorder::initialize(CSound *sound)
{
	if(!initialized)
	{
		ASoundRecorder::initialize(sound);
		try
		{
			// try to become a client of the JACK server
			if((client=jack_client_new((REZOUND_PACKAGE+string("_recording")).c_str()))==0) 
				throw runtime_error(string(__func__)+" -- error connecting to jack server -- jackd not running?");

			// tell the JACK server to call `processAudio()' whenever there is work to be done
			jack_set_process_callback(client,processAudio,this);

			// tell the JACK server to call `maxToProcessChanged()' whenever the maximum number of frames that 
			// will be passed to `processAudio()' changes
			jack_set_buffer_size_callback(client,maxToProcessChanged,this);

			// tell the JACK server to call `sampleRateChanged()' whenever the sample rate of the system changes
			jack_set_sample_rate_callback(client,sampleRateChanged,this);

			// tell the JACK server to call `jackShutdown()' if it ever shuts down, either entirely, 
			// or if it just decides to stop calling us
			jack_on_shutdown(client,jackShutdown,this);

			sampleRateChanged(jack_get_sample_rate(client),this); // make note of the sample rate for this device

			maxToProcessChanged(jack_get_buffer_size(client),this); // initially set tempBuffer to the right size


			// create two ports
			for(unsigned t=0;t<sound->getChannelCount();t++)
				input_ports[t]=jack_port_register(client,("input_"+istring(t+1)).c_str(),JACK_DEFAULT_AUDIO_TYPE,JackPortIsInput,0);


			// tell the JACK server that we are ready to roll
			if(jack_activate(client)) 
			{
				jack_client_close(client); client=NULL;
				throw runtime_error(string(__func__)+" -- cannot activate client");
			}
			

			// gather output port names in case we need to prompt for them
			vector<string> outputPortNames;
			const char **ports=jack_get_ports(client,NULL,NULL,JackPortIsOutput);
			while(ports && *ports)
				outputPortNames.push_back(*(ports++));
			ports-=outputPortNames.size();
			free(ports);


			// connect the ports
			for(unsigned t=0;t<sound->getChannelCount();t++)
			{
				if(gJACKInputPortNames[t]=="")
				{ // never asked before
					if(outputPortNames.size()<=0)
						throw runtime_error(string(__func__)+" -- no output ports are defined within the JACK server to connect to for audio recording");

					try
					{
						gJACKInputPortNames[t]=gFrontendHooks->promptForJACKPort("Choose Port for Input Channel "+istring(t+1),outputPortNames);
					}
					catch(...)
					{
						jack_client_close(client); client=NULL;
						throw;
					}
				}

				const string portName=gJACKInputPortNames[t];
				if(jack_connect(client,portName.c_str(),jack_port_name(input_ports[t]))) 
				{
					jack_client_close(client); client=NULL;
					throw runtime_error(string(__func__)+" -- cannot connect input port: "+portName);
				}
			}


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

void CJACKSoundRecorder::deinitialize()
{
	if(initialized)
	{
		jack_client_close(client); client=NULL;

		ASoundRecorder::deinitialize();

		initialized=false;
	}
}

void CJACKSoundRecorder::redo()
{
	// ??? clear JACK's recording buffers if possible/necessary?
	ASoundRecorder::redo();
}

int CJACKSoundRecorder::processAudio(jack_nframes_t nframes,void *arg)
{
	CJACKSoundRecorder *that=(CJACKSoundRecorder *)arg;

	try
	{
		if(typeid(sample_t)==typeid(jack_default_audio_sample_t *)) // no conversion necessary
		{
			throw(runtime_error(string(__func__)+" -- unhandled sample_t/jack_default_audio_sample_t combination (2)"));
		}
		else if(typeid(jack_default_audio_sample_t)==typeid(float) && typeid(sample_t)==typeid(int16_t))
		{
			sample_t *tempBuffer=that->tempBuffer;

			// convert the recorded buffer to the native type and give to ASoundRecorder::onData
			const unsigned channelCount=that->getSound()->getChannelCount();
			for(unsigned i=0;i<channelCount;i++)
			{
				const jack_default_audio_sample_t * const in=(jack_default_audio_sample_t *)jack_port_get_buffer(that->input_ports[i],nframes);

				sample_t *tt=tempBuffer+i;
				for(unsigned t=0;t<nframes;t++,tt+=channelCount)
					(*tt)=(sample_t)(in[t]*MAX_SAMPLE);
			}
		}
		else
			throw(runtime_error(string(__func__)+" -- unhandled sample_t/jack_default_audio_sample_t combination"));

		that->onData(that->tempBuffer,nframes);
	}
	catch(exception &e)
	{
		fprintf(stderr,"exception caught in record callback: %s\n",e.what());

	}
	catch(...)
	{
		fprintf(stderr,"unknown exception caught in record callback\n");
	}

	return 0;
}

int CJACKSoundRecorder::maxToProcessChanged(jack_nframes_t nframes,void *arg)
{
	CJACKSoundRecorder *that=(CJACKSoundRecorder *)arg;

	that->tempBuffer.setSize(nframes*that->getSound()->getChannelCount());
	return 0;
}

int CJACKSoundRecorder::sampleRateChanged(jack_nframes_t nframes,void *arg)
{
	return 0;
}

void CJACKSoundRecorder::jackShutdown(void *arg)
{
}

#endif // ENABLE_JACK
