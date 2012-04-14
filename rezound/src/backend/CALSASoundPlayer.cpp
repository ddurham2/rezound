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
 *
 * This code is based on examples in an ALSA lib tutorial given at: 
 * 	http://equalarea.com/paul/alsa-audio.html
 * Thanks to Paul Davis (except for directing me to JACK at the bottom and getting me WAY off track with my goals! :) )
 */

#include "CALSASoundPlayer.h"

#ifdef ENABLE_ALSA


#include <stdexcept>
#include <string>

#include <istring>
#include <TAutoBuffer.h>

#include "settings.h"

#warning use these values from the registry globals
#define PERIOD_SIZE_FRAMES 1024
#define PERIOD_COUNT 2


CALSASoundPlayer::CALSASoundPlayer() :
	ASoundPlayer(),

	initialized(false),
	playback_handle(NULL),

	playThread(this)
{
}

CALSASoundPlayer::~CALSASoundPlayer()
{
	deinitialize();
}

bool CALSASoundPlayer::isInitialized() const
{
	return initialized;
}

void CALSASoundPlayer::initialize()
{
	if(!initialized)
	{
		try
		{
			snd_pcm_hw_params_t *hw_params;
			snd_pcm_sw_params_t *sw_params;
			int err;

			if((err = snd_pcm_open(&playback_handle, gALSAOutputDevice.c_str(), SND_PCM_STREAM_PLAYBACK, 0)) < 0) 
				throw runtime_error(string(__func__)+" -- cannot open audio device: "+gALSAOutputDevice+" -- "+snd_strerror(err));

			if((err = snd_pcm_hw_params_malloc(&hw_params)) < 0)
				throw runtime_error(string(__func__)+" -- cannot allocate hardware parameter structure -- "+snd_strerror(err));

			if((err = snd_pcm_hw_params_any(playback_handle, hw_params)) < 0)
				throw runtime_error(string(__func__)+" -- cannot initialize hardware parameter structure -- "+snd_strerror(err));

			// set sample rate
			unsigned int sampleRate=gDesiredOutputSampleRate;
			unsigned int outSampleRate=sampleRate;
			if((err = snd_pcm_hw_params_set_rate_near(playback_handle, hw_params, &outSampleRate, 0)) < 0)
				throw runtime_error(string(__func__)+" -- cannot set sample rate -- "+snd_strerror(err));
			if(sampleRate!=outSampleRate)
				fprintf(stderr,"warning: ALSA used a different sample rate (%d) than what was asked for (%d); will have to do extra calculations to compensate\n", (int)outSampleRate, (int)sampleRate);
			devices[0].sampleRate=outSampleRate; // make note of the sample rate for this device (??? which is only device zero for now)


			// set number of channels
			unsigned channelCount=gDesiredOutputChannelCount;
				// might need to use the near function
			if((err = snd_pcm_hw_params_set_channels(playback_handle, hw_params, channelCount)) < 0)
				throw runtime_error(string(__func__)+" -- cannot set channel count -- "+snd_strerror(err));
			devices[0].channelCount=channelCount; // make note of the number of channels for this device (??? which is only device zero for now)


			// set to interleaved access
			if((err = snd_pcm_hw_params_set_access(playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
				throw runtime_error(string(__func__)+" -- cannot set access type -- "+snd_strerror(err));


			// set periods to integer sizes only
			if((err = snd_pcm_hw_params_set_periods_integer(playback_handle, hw_params)) < 0)
				throw runtime_error(string(__func__)+" -- cannot set periods to integer only -- "+snd_strerror(err));


			// set period size
			snd_pcm_uframes_t period_size=PERIOD_SIZE_FRAMES;
			int dir=0;
			if((err = snd_pcm_hw_params_set_period_size_near(playback_handle, hw_params, &period_size, &dir)) < 0)
				throw runtime_error(string(__func__)+" -- cannot set period size -- "+snd_strerror(err));


			// set number of periods 
			unsigned int periods=PERIOD_COUNT;
			if((err = snd_pcm_hw_params_set_periods(playback_handle, hw_params, periods, 0)) < 0)
				throw runtime_error(string(__func__)+" -- cannot set periods -- "+snd_strerror(err));


			// and then set the buffer size (what .. what's this difference between buffers and periods?)
			if((err = snd_pcm_hw_params_set_buffer_size(playback_handle, hw_params, (PERIOD_COUNT*PERIOD_SIZE_FRAMES) )) < 0)
				throw runtime_error(string(__func__)+" -- cannot set buffer size -- "+snd_strerror(err));


			// set sample format
			vector<snd_pcm_format_t> formatsToTry;
#ifndef WORDS_BIGENDIAN
	#if defined(SAMPLE_TYPE_S16)
			formatsToTry.push_back(SND_PCM_FORMAT_S16_LE);
			formatsToTry.push_back(SND_PCM_FORMAT_S32_LE);
			formatsToTry.push_back(SND_PCM_FORMAT_FLOAT_LE);
	#elif defined(SAMPLE_TYPE_FLOAT)
			formatsToTry.push_back(SND_PCM_FORMAT_FLOAT_LE);
			formatsToTry.push_back(SND_PCM_FORMAT_S32_LE);
			formatsToTry.push_back(SND_PCM_FORMAT_S16_LE);
	#else
			#error unhandled SAMPLE_TYPE_xxx define
	#endif
#else
	#if defined(SAMPLE_TYPE_S16)
			formatsToTry.push_back(SND_PCM_FORMAT_S16_BE);
			formatsToTry.push_back(SND_PCM_FORMAT_S32_BE);
			formatsToTry.push_back(SND_PCM_FORMAT_FLOAT_BE);
	#elif defined(SAMPLE_TYPE_FLOAT)
			formatsToTry.push_back(SND_PCM_FORMAT_FLOAT_BE);
			formatsToTry.push_back(SND_PCM_FORMAT_S32_BE);
			formatsToTry.push_back(SND_PCM_FORMAT_S16_BE);
	#else
			#error unhandled SAMPLE_TYPE_xxx define
	#endif
#endif
			bool found=false;
			for(size_t t=0;t<formatsToTry.size();t++)
			{
				if((err = snd_pcm_hw_params_set_format(playback_handle, hw_params, formatsToTry[t])) < 0)
					continue; // try the next one
				else
				{
					found=true;
					playback_format=formatsToTry[t];
					break;
				}
			}
			if(!found)
				throw runtime_error(string(__func__)+" -- cannot set sample format -- "+snd_strerror(err));


			// set the hardware parameters
			if((err = snd_pcm_hw_params(playback_handle, hw_params)) < 0)
				throw runtime_error(string(__func__)+" -- cannot set parameters -- "+snd_strerror(err));

			snd_pcm_hw_params_free(hw_params);



			/* tell ALSA to wake us up whenever PERIOD_SIZE_FRAMES or more frames of playback data can be delivered. Also, tell ALSA that we'll start the device ourselves. */

			if((err = snd_pcm_sw_params_malloc(&sw_params)) < 0)
				throw runtime_error(string(__func__)+" -- cannot allocate software parameters structure -- "+snd_strerror(err));

			if((err = snd_pcm_sw_params_current(playback_handle, sw_params)) < 0)
				throw runtime_error(string(__func__)+" -- cannot initialize software parameters structure -- "+snd_strerror(err));

			if((err = snd_pcm_sw_params_set_avail_min(playback_handle, sw_params, PERIOD_SIZE_FRAMES)) < 0)
				throw runtime_error(string(__func__)+" -- cannot set minimum available count -- "+snd_strerror(err));

			if((err = snd_pcm_sw_params_set_start_threshold(playback_handle, sw_params, 0)) < 0)
				throw runtime_error(string(__func__)+" -- cannot set start threshold -- "+snd_strerror(err));

			// make xruns not interrupt anything
			snd_pcm_uframes_t boundary;
			if((err = snd_pcm_sw_params_get_boundary(sw_params, &boundary)) < 0)
				throw runtime_error(string(__func__)+" -- cannot get boundary -- "+snd_strerror(err));
			if((err = snd_pcm_sw_params_set_stop_threshold(playback_handle, sw_params, boundary)) < 0)
				throw runtime_error(string(__func__)+" -- cannot set stop threshold -- "+snd_strerror(err));

			if((err = snd_pcm_sw_params_set_silence_threshold(playback_handle, sw_params, 0)) < 0) 
				throw runtime_error(string(__func__)+" -- cannot set silence threshold -- "+snd_strerror(err));


			if((err = snd_pcm_sw_params(playback_handle, sw_params)) < 0)
				throw runtime_error(string(__func__)+" -- cannot set software parameters -- "+snd_strerror(err));

			snd_pcm_sw_params_free(sw_params);


			if((err = snd_pcm_prepare(playback_handle)) < 0)
				throw runtime_error(string(__func__)+" -- cannot prepare audio interface for use -- "+snd_strerror(err));



			// start play thread
			playThread.kill=false;
			playThread.start();

			ASoundPlayer::initialize();
			initialized=true;
			fprintf(stderr, "ALSA player initialized\n");
		}
		catch(...)
		{
			if(playThread.isRunning())
			{
				playThread.kill=true;
				playThread.wait();
			}
			if(playback_handle)
				snd_pcm_close(playback_handle);
			playback_handle=NULL;

			ASoundPlayer::deinitialize();
			throw;
		}
	}
	else
		throw runtime_error(string(__func__)+" -- already initialized");
}

void CALSASoundPlayer::deinitialize()
{
	if(initialized)
	{
		ASoundPlayer::deinitialize();

		// stop play thread
		playThread.kill=true;
		playThread.wait();
		
		// close ALSA audio device
		if(playback_handle!=NULL)
			snd_pcm_close(playback_handle);
		playback_handle=NULL;

		initialized=false;
		fprintf(stderr, "ALSA player deinitialized\n");
	}
}

void CALSASoundPlayer::aboutToRecord()
{
}

void CALSASoundPlayer::doneRecording()
{
}


CALSASoundPlayer::CPlayThread::CPlayThread(CALSASoundPlayer *_parent) :
	AThread(),

	kill(false),
	parent(_parent)
{
}

CALSASoundPlayer::CPlayThread::~CPlayThread()
{
}

void CALSASoundPlayer::CPlayThread::main()
{
	try
	{
		int err;
		TAutoBuffer<sample_t> buffer(PERIOD_SIZE_FRAMES*parent->devices[0].channelCount*2); 
			// these are possibly used if sample format conversion is required
		TAutoBuffer<int16_t> buffer__int16_t(PERIOD_SIZE_FRAMES*parent->devices[0].channelCount);
		TAutoBuffer<int32_t> buffer__int32_t(PERIOD_SIZE_FRAMES*parent->devices[0].channelCount);
		TAutoBuffer<float> buffer__float(PERIOD_SIZE_FRAMES*parent->devices[0].channelCount);

		snd_pcm_format_t format=parent->playback_format;

		// must do conversion of sample_t type -> initialized format type if necessary
		int conv=-1;
#ifndef WORDS_BIGENDIAN
	#if defined(SAMPLE_TYPE_S16)
		if(format==SND_PCM_FORMAT_S16_LE)
			conv=0;
		else if(format==SND_PCM_FORMAT_S32_LE)
			conv=4;
		else if(format==SND_PCM_FORMAT_FLOAT_LE)
			conv=2;
	#elif defined(SAMPLE_TYPE_FLOAT)
		if(format==SND_PCM_FORMAT_S16_LE)
			conv=1;
		else if(format==SND_PCM_FORMAT_S32_LE)
			conv=3;
		else if(format==SND_PCM_FORMAT_FLOAT_LE)
			conv=0;
	#else
		#error unhandled SAMPLE_TYPE_xxx define
	#endif
#else
	#if defined(SAMPLE_TYPE_S16)
		if(format==SND_PCM_FORMAT_S16_BE)
			conv=0;
		else if(format==SND_PCM_FORMAT_S32_BE)
			conv=4;
		else if(format==SND_PCM_FORMAT_FLOAT_BE)
			conv=2;
	#elif defined(SAMPLE_TYPE_FLOAT)
		if(format==SND_PCM_FORMAT_S16_BE)
			conv=1;
		else if(format==SND_PCM_FORMAT_S32_BE)
			conv=3;
		else if(format==SND_PCM_FORMAT_FLOAT_BE)
			conv=0;
	#else
		#error unhandled SAMPLE_TYPE_xxx define
	#endif
#endif

		while(!kill)
		{
			// can mixChannels throw any exception???
			parent->mixSoundPlayerChannels(parent->devices[0].channelCount,buffer,PERIOD_SIZE_FRAMES);

			// wait till the interface is ready for data, or 1 second has elapsed.
			if((err = snd_pcm_wait (parent->playback_handle, 1000)) < 0)
				throw runtime_error(string(__func__)+" -- snd_pcm_wait failed -- "+snd_strerror(err));

			// find out how much space is available for playback data 
			snd_pcm_sframes_t frames_to_deliver;
			if((frames_to_deliver = snd_pcm_avail_update(parent->playback_handle)) < 0) 
			{
				if (frames_to_deliver == -EPIPE) 
				{
					fprintf (stderr, "an xrun occured\n");
				} 
				else 
					throw runtime_error(string(__func__)+" -- unknown ALSA avail update return value: "+istring(frames_to_deliver));
			}

			if(frames_to_deliver<PERIOD_SIZE_FRAMES)
				throw runtime_error(string(__func__)+" -- frames_to_deliver is less than PERIOD_SIZE_FRAMES: "+istring(frames_to_deliver)+"<"+istring(PERIOD_SIZE_FRAMES));

#warning probably should abstract this into a since it could be used several places
			switch(conv)
			{
				case 0:	// no conversion necessary
				{
					if((err=snd_pcm_writei(parent->playback_handle, buffer, PERIOD_SIZE_FRAMES)) < 0)
						fprintf(stderr, "ALSA write failed (%s)\n", snd_strerror(err));
				}
				break;

				case 1:	// we need to convert float->int16_t
				{ 
					unsigned l=PERIOD_SIZE_FRAMES*parent->devices[0].channelCount;
					for(unsigned t=0;t<l;t++)
						buffer__int16_t[t]=convert_sample<float,int16_t>((float)buffer[t]);
					if((err=snd_pcm_writei(parent->playback_handle, buffer__int16_t, PERIOD_SIZE_FRAMES)) < 0)
						fprintf(stderr, "ALSA write failed (%s)\n", snd_strerror(err));
				}
				break;

				case 2:	// we need to convert int16_t->float
				{ 
					unsigned l=PERIOD_SIZE_FRAMES*parent->devices[0].channelCount;
					for(unsigned t=0;t<l;t++)
						buffer__float[t]=convert_sample<int16_t,float>((int16_t)buffer[t]);
					if((err=snd_pcm_writei(parent->playback_handle, buffer__float, PERIOD_SIZE_FRAMES)) < 0)
						fprintf(stderr, "ALSA write failed (%s)\n", snd_strerror(err));
				}
				break;

				case 3:	// we need to convert float->int32_t
				{ 
					unsigned l=PERIOD_SIZE_FRAMES*parent->devices[0].channelCount;
					for(unsigned t=0;t<l;t++)
						buffer__int32_t[t]=convert_sample<float,int32_t>((float)buffer[t]);
					if((err=snd_pcm_writei(parent->playback_handle, buffer__int32_t, PERIOD_SIZE_FRAMES)) < 0)
						fprintf(stderr, "ALSA write failed (%s)\n", snd_strerror(err));
				}
				break;

				case 4:	// we need to convert int16_t->int32_t
				{ 
					unsigned l=PERIOD_SIZE_FRAMES*parent->devices[0].channelCount;
					for(unsigned t=0;t<l;t++)
						buffer__int32_t[t]=convert_sample<int16_t,int32_t>((int16_t)buffer[t]);
					if((err=snd_pcm_writei(parent->playback_handle, buffer__int32_t, PERIOD_SIZE_FRAMES)) < 0)
						fprintf(stderr, "ALSA write failed (%s)\n", snd_strerror(err));
				}
				break;

				default:
					throw runtime_error(string(__func__)+" -- no conversion determined");
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

#endif // ENABLE_ALSA
