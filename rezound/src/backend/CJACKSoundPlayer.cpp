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

#include "CJACKSoundPlayer.h"

#ifdef ENABLE_JACK

#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <stdexcept>
#include <typeinfo>

#include <istring>

#include "settings.h"
#include "AFrontendHooks.h"
#include "AStatusComm.h"

/*
 * Ask the mailing list about the hangup signal and I probably want to catch it and not die
 * 
 * Further testing needs to be done about the jackd process dieing while ReZound is running and what to do in that situation
 *
 */ 


unsigned CJACKSoundPlayer::hack_sampleRate=44100;

CJACKSoundPlayer::CJACKSoundPlayer() :
	ASoundPlayer(),

	initialized(false),
	client(NULL),

	tempBuffer(1)
{
	for(unsigned t=0;t<MAX_CHANNELS;t++)
		output_ports[t]=NULL;
}

CJACKSoundPlayer::~CJACKSoundPlayer()
{
	deinitialize();
}

bool CJACKSoundPlayer::isInitialized() const
{
	return initialized;
}

void CJACKSoundPlayer::initialize()
{
	if(!initialized)
	{
		try
		{
			// try to become a client of the JACK server
			if((client=jack_client_new(REZOUND_PACKAGE))==0) 
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
			devices[0].channelCount=gDesiredOutputChannelCount;

			maxToProcessChanged(jack_get_buffer_size(client),this); // initially set tempBuffer to the right size


			// create two ports
			for(unsigned t=0;t<devices[0].channelCount;t++)
				output_ports[t]=jack_port_register(client,("output_"+istring(t+1)).c_str(),JACK_DEFAULT_AUDIO_TYPE,JackPortIsOutput,0);


			// tell the JACK server that we are ready to roll
			if(jack_activate(client)) 
				throw runtime_error(string(__func__)+" -- cannot activate client");


			// gather input port names in case we need to prompt for them
			vector<string> inputPortNames;
			const char **ports=jack_get_ports(client,NULL,NULL,JackPortIsInput);
			while(ports && *ports)
				inputPortNames.push_back(*(ports++));
			ports-=inputPortNames.size();
			free(ports);


			// connect the ports
			for(unsigned t=0;t<devices[0].channelCount;t++)
			{
				askAgain:
				if(gJACKOutputPortNames[t]=="")
				{ // never asked before
					if(inputPortNames.size()<=0)
						throw runtime_error(string(__func__)+" -- no input ports are defined within the JACK server to connect to for audio playback");

					gJACKOutputPortNames[t]=gFrontendHooks->promptForJACKPort("Choose Port for Output Channel "+istring(t+1),inputPortNames);
				}

				const string portName=gJACKOutputPortNames[t];
				if(jack_connect(client,jack_port_name(output_ports[t]),portName.c_str())) 
				{
					Warning("Cannot connect to JACK port, "+portName+", please choose a different one");
					gJACKOutputPortNames[t]="";
					goto askAgain;
				}
			}

			ASoundPlayer::initialize();
			initialized=true;
		}
		catch(...)
		{
			deinitialize();
			throw;
		}
	}
	else
		throw(runtime_error(string(__func__)+" -- already initialized"));
}

void CJACKSoundPlayer::deinitialize()
{
	ASoundPlayer::deinitialize();

	if(client!=NULL)
	{
		for(unsigned t=0;t<devices[0].channelCount;t++) // device zero only for now
		{
			if(output_ports[t]!=NULL)
				jack_port_unregister(client,output_ports[t]);
			output_ports[t]=NULL;
		}
		jack_deactivate(client);
		jack_client_close(client); 
		client=NULL;
	}

	initialized=false;
}

void CJACKSoundPlayer::aboutToRecord()
{
}

void CJACKSoundPlayer::doneRecording()
{
}


int CJACKSoundPlayer::processAudio(jack_nframes_t nframes,void *arg)
{
/*
 * I know the documentation said to avoid any indeterminate system calles (which would include file i/o or dynamic allocation)
 * But the way I read sound data is by keeping it always on disk and caching some in memory.  And the way I see it, any access
 * to memory could page fault and cause disk access.  (same goes for the recorder too)
 */

	CJACKSoundPlayer *that=(CJACKSoundPlayer *)arg;

	try
	{
		
		if(typeid(sample_t)==typeid(jack_default_audio_sample_t *)) // no conversion necessary
		{
			// ??? implement this to write to a temp buffer and then copy to out1 and out2
			throw(runtime_error(string(__func__)+" -- unhandled sample_t/jack_default_audio_sample_t combination (2)"));
			//that->mixSoundPlayerChannels(gDesiredOutputChannelCount,(sample_t *)out,(unsigned)nframes);
		}
		else if(typeid(jack_default_audio_sample_t)==typeid(float) && typeid(sample_t)==typeid(int16_t))
		{
			sample_t *tempBuffer=that->tempBuffer;

			that->mixSoundPlayerChannels(gDesiredOutputChannelCount,tempBuffer,(unsigned)nframes);

			// copy back to 'out1 & 2' but normalizing it to [-1,1]
			const unsigned channelCount=that->devices[0].channelCount;
			for(unsigned i=0;i<channelCount;i++)
			{
				jack_default_audio_sample_t *out=(jack_default_audio_sample_t *)jack_port_get_buffer(that->output_ports[i],nframes);

				sample_t const *tt=tempBuffer+i;
				for(unsigned t=0;t<nframes;t++,tt+=channelCount)
					out[t]=(jack_default_audio_sample_t)(*tt)/(jack_default_audio_sample_t)MAX_SAMPLE;
			}
		}
		else
			throw(runtime_error(string(__func__)+" -- unhandled sample_t/jack_default_audio_sample_t combination"));
	}
	catch(exception &e)
	{
		fprintf(stderr,"exception caught in play callback: %s\n",e.what());
		// ??? I used to abort() here, but I should probably count so something a 
		// little more careful like count the exceptions and after 25, abort so 
		// that it doesn't endlessly happen, but doesn't die on the off-chance that 
		// something might happen that can be recovered from ... change all the 
		// other's too (including Recorder classes)
	}
	catch(...)
	{
		fprintf(stderr,"unknown exception caught in play callback\n");
	}
	return 0;
}

int CJACKSoundPlayer::maxToProcessChanged(jack_nframes_t nframes,void *arg)
{
	CJACKSoundPlayer *that=(CJACKSoundPlayer *)arg;

	that->tempBuffer.setSize(nframes*that->devices[0].channelCount); // ??? this is simply always device zero for now
	return 0;
}

int CJACKSoundPlayer::sampleRateChanged(jack_nframes_t nframes,void *arg)
{
	CJACKSoundPlayer *that=(CJACKSoundPlayer *)arg;

	that->devices[0].sampleRate=nframes; // ??? this is simply always device zero for now
	CJACKSoundPlayer::hack_sampleRate=nframes;
	return 0;
}

void CJACKSoundPlayer::jackShutdown(void *arg)
{
}

#endif // ENABLE_JACK
