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

#include "CALSASoundRecorder.h"

#ifdef ENABLE_ALSA

#include <stdexcept>
#include <string>

#include <istring>
#include <TAutoBuffer.h>

#include "settings.h"
#include "AStatusComm.h"

// ??? edit this to be able to detect necessary parameters from the typeof sample_t
// 	or I need to convert to 16bit 
// needs to match for what is above and type of sample_t ???
#ifdef WORDS_BIGENDIAN
	#define ALSA_PCM_FORMAT SND_PCM_FORMAT_S16_BE
#else
	#define ALSA_PCM_FORMAT SND_PCM_FORMAT_S16_LE
#endif

// ??? as the sample rate is lower these need to be lower so that onData is called more often and the view meters on the record dialog don't seem to lag

#define PERIOD_SIZE_FRAMES 8192
#define PERIOD_COUNT 2


CALSASoundRecorder::CALSASoundRecorder() :
	ASoundRecorder(),

	initialized(false),
	capture_handle(NULL),

	recordThread(this)
{
}

CALSASoundRecorder::~CALSASoundRecorder()
{
	deinitialize();
}

void CALSASoundRecorder::initialize(CSound *sound)
{
	if(!initialized)
	{
		ASoundRecorder::initialize(sound);
		try
		{
			snd_pcm_hw_params_t *hw_params;
			snd_pcm_sw_params_t *sw_params;
			int err;

			if((err = snd_pcm_open(&capture_handle, gALSAInputDevice.c_str(), SND_PCM_STREAM_CAPTURE, 0)) < 0) 
				throw runtime_error(string(__func__)+" -- cannot open audio device: "+gALSAInputDevice+" -- "+snd_strerror(err));

			if((err = snd_pcm_hw_params_malloc(&hw_params)) < 0)
				throw runtime_error(string(__func__)+" -- cannot allocate hardware parameter structure -- "+snd_strerror(err));

			if((err = snd_pcm_hw_params_any(capture_handle, hw_params)) < 0)
				throw runtime_error(string(__func__)+" -- cannot initialize hardware parameter structure -- "+snd_strerror(err));

			// set sample rate
			unsigned int sampleRate=sound->getSampleRate();
			unsigned int inSampleRate=sampleRate;
			if((err = snd_pcm_hw_params_set_rate_near(capture_handle, hw_params, &inSampleRate, 0)) < 0)
				throw runtime_error(string(__func__)+" -- cannot set sample rate -- "+snd_strerror(err));
			if(sampleRate!=inSampleRate)
			{
				// ??? need to translate
				Message("ALSA used a different sample rate ("+istring(inSampleRate)+") than what was asked for ("+istring(sound->getSampleRate())+")");
				sound->setSampleRate(sampleRate);
			}


			// set number of channels
			unsigned channelCount=sound->getChannelCount();
				// might need to use the near function
			if((err = snd_pcm_hw_params_set_channels(capture_handle, hw_params, channelCount)) < 0)
				throw runtime_error(string(__func__)+" -- cannot set channel count -- "+snd_strerror(err));


			// set to interleaved access
			if((err = snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
				throw runtime_error(string(__func__)+" -- cannot set access type -- "+snd_strerror(err));


			// set periods to integer sizes only
			if((err = snd_pcm_hw_params_set_periods_integer(capture_handle, hw_params)) < 0)
				throw runtime_error(string(__func__)+" -- cannot set periods to integer only -- "+snd_strerror(err));


			// set period size
			snd_pcm_uframes_t period_size=PERIOD_SIZE_FRAMES;
			int dir=0;
			if((err = snd_pcm_hw_params_set_period_size_near(capture_handle, hw_params, &period_size, &dir)) < 0)
				throw runtime_error(string(__func__)+" -- cannot set period size -- "+snd_strerror(err));


			// set number of periods 
			unsigned int periods=PERIOD_COUNT;
			if((err = snd_pcm_hw_params_set_periods(capture_handle, hw_params, periods, 0)) < 0)
				throw runtime_error(string(__func__)+" -- cannot set periods -- "+snd_strerror(err));


			// and then set the buffer size (what .. what's this difference between buffers and periods?)
			if((err = snd_pcm_hw_params_set_buffer_size(capture_handle, hw_params, (PERIOD_COUNT*PERIOD_SIZE_FRAMES) )) < 0)
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
				if((err = snd_pcm_hw_params_set_format(capture_handle, hw_params, formatsToTry[t])) < 0)
					continue; // try the next one
				else
				{
					found=true;
					capture_format=formatsToTry[t];
					break;
				}
			}
			if(!found)
				throw runtime_error(string(__func__)+" -- cannot set sample format -- "+snd_strerror(err));


			// set the hardware parameters
			if((err = snd_pcm_hw_params(capture_handle, hw_params)) < 0)
				throw runtime_error(string(__func__)+" -- cannot set parameters -- "+snd_strerror(err));

			snd_pcm_hw_params_free(hw_params);


			/* tell ALSA to wake us up whenever PERIOD_SIZE_FRAMES or more frames of captured data can be delivered. Also, tell ALSA that we'll start the device ourselves. */

			if((err = snd_pcm_sw_params_malloc(&sw_params)) < 0)
				throw runtime_error(string(__func__)+" -- cannot allocate software parameters structure -- "+snd_strerror(err));

			if((err = snd_pcm_sw_params_current(capture_handle, sw_params)) < 0)
				throw runtime_error(string(__func__)+" -- cannot initialize software parameters structure -- "+snd_strerror(err));

			if((err = snd_pcm_sw_params_set_avail_min(capture_handle, sw_params, PERIOD_SIZE_FRAMES)) < 0)
				throw runtime_error(string(__func__)+" -- cannot set minimum available count -- "+snd_strerror(err));

			if((err = snd_pcm_sw_params_set_start_threshold(capture_handle, sw_params, 0)) < 0)
				throw runtime_error(string(__func__)+" -- cannot set start threshold -- "+snd_strerror(err));

			// make xruns not interrupt anything
			snd_pcm_uframes_t boundary;
			if((err = snd_pcm_sw_params_get_boundary(sw_params, &boundary)) < 0)
				throw runtime_error(string(__func__)+" -- cannot get boundary -- "+snd_strerror(err));
			if((err = snd_pcm_sw_params_set_stop_threshold(capture_handle, sw_params, boundary)) < 0)
				throw runtime_error(string(__func__)+" -- cannot set stop threshold -- "+snd_strerror(err));

			if((err = snd_pcm_sw_params(capture_handle, sw_params)) < 0)
				throw runtime_error(string(__func__)+" -- cannot set software parameters -- "+snd_strerror(err));

			snd_pcm_sw_params_free(sw_params);

			if((err = snd_pcm_prepare(capture_handle)) < 0)
				throw runtime_error(string(__func__)+" -- cannot prepare audio interface for use -- "+snd_strerror(err));


			// start record thread
			recordThread.kill=false;
			recordThread.start();

			initialized=true;
		}
		catch(...)
		{
			if(recordThread.isRunning())
			{
				recordThread.kill=true;
				recordThread.wait();
			}
			snd_pcm_close(capture_handle);
			capture_handle=NULL;

			ASoundRecorder::deinitialize();
			throw;
		}
	}
	else
		throw(runtime_error(string(__func__)+" -- already initialized"));
}

void CALSASoundRecorder::deinitialize()
{
	if(initialized)
	{
		// stop record thread
		recordThread.kill=true;
		recordThread.wait();

		// close ALSA audio device
		if(capture_handle!=NULL)
			snd_pcm_close(capture_handle);
		capture_handle=NULL;

		ASoundRecorder::deinitialize();

		initialized=false;
	}
}

void CALSASoundRecorder::redo()
{
/* 
	   When recording, if the user hits redo, there may be 
	recorded buffers within ALSA that we haven't read yet, 
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
		printf("ALSA record: info.fragments: %d\n",info.fragments);

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

CALSASoundRecorder::CRecordThread::CRecordThread(CALSASoundRecorder *_parent) :
	AThread(),

	kill(false),
	parent(_parent)
{
}

CALSASoundRecorder::CRecordThread::~CRecordThread()
{
}

void CALSASoundRecorder::CRecordThread::main()
{
	try
	{
		const unsigned channelCount=parent->getSound()->getChannelCount();

		int err;
		TAutoBuffer<sample_t> buffer(PERIOD_SIZE_FRAMES*channelCount*2); 
			// these are possibly used if sample format conversion is required
		TAutoBuffer<int16_t> buffer__int16_t(PERIOD_SIZE_FRAMES*channelCount);
		TAutoBuffer<int32_t> buffer__int32_t(PERIOD_SIZE_FRAMES*channelCount);
		TAutoBuffer<float> buffer__float(PERIOD_SIZE_FRAMES*channelCount);

		snd_pcm_format_t format=parent->capture_format;

		// must do conversion of initialized format -> sample_t type type if necessary
		int conv=-1;
#if defined(SAMPLE_TYPE_S16)
		if(format==SND_PCM_FORMAT_S16_LE || format==SND_PCM_FORMAT_S16_BE)
			conv=0;
		else if(format==SND_PCM_FORMAT_S32_LE || format==SND_PCM_FORMAT_S32_BE)
			conv=2;
		else if(format==SND_PCM_FORMAT_FLOAT_LE || format==SND_PCM_FORMAT_FLOAT_BE)
			conv=3;
#elif defined(SAMPLE_TYPE_FLOAT)
		if(format==SND_PCM_FORMAT_S16_LE || format==SND_PCM_FORMAT_S16_BE)
			conv=1;
		else if(format==SND_PCM_FORMAT_S32_LE || format==SND_PCM_FORMAT_S32_BE)
			conv=2;
		else if(format==SND_PCM_FORMAT_FLOAT_LE || format==SND_PCM_FORMAT_FLOAT_BE)
			conv=0;
#else
		#error unhandled SAMPLE_TYPE_xxx define
#endif

		if((err = snd_pcm_start(parent->capture_handle)) < 0)
			throw runtime_error(string(__func__)+" -- snd_pcm_start failed -- "+snd_strerror(err));

		while(!kill)
		{
			// wait till the interface has data ready, or 1 second has elapsed.
			if((err = snd_pcm_wait(parent->capture_handle, 1000)) < 0)
				throw runtime_error(string(__func__)+" -- snd_pcm_wait failed -- "+snd_strerror(err));

			// find out how much data is available to read
			snd_pcm_sframes_t frames_ready;
			if((frames_ready = snd_pcm_avail_update(parent->capture_handle)) < 0) 
			{
				if (frames_ready == -EPIPE) 
				{
					fprintf (stderr, "an xrun occured\n");
				} 
				else 
					throw runtime_error(string(__func__)+" -- unknown ALSA avail update return value: "+istring(frames_ready));
			}

				// not absolutely necessary to wait for the exact amount I don't believe ??? but I don't think it hurts.. but code below assumes it's PERIOD_SIZE frames that are ready to read
			if(frames_ready<PERIOD_SIZE_FRAMES)
			{
				if(frames_ready==0)
				{
					printf("zero\n");
					continue;
				}
				else
					throw runtime_error(string(__func__)+" -- frames_to_deliver is less than PERIOD_SIZE_FRAMES: "+istring(frames_ready)+"<"+istring(PERIOD_SIZE_FRAMES));
			}

/*
			CMutexLocker redoMutexLocker(parent->redoMutex);
*/

			snd_pcm_sframes_t len;
#warning probably should abstract this into a since it could be used several places
			switch(conv)
			{
				case 0:	// no conversion necessary
				{
					if((len=snd_pcm_readi(parent->capture_handle, buffer, PERIOD_SIZE_FRAMES)) < 0)
						fprintf(stderr, "ALSA read failed (%s)\n", snd_strerror(len));
				}
				break;

				case 1:	// we need to convert int16_t->sample_t
				{ 
					unsigned l=PERIOD_SIZE_FRAMES*channelCount;
					if((err=snd_pcm_readi(parent->capture_handle, buffer__int16_t, PERIOD_SIZE_FRAMES)) < 0)
						fprintf(stderr, "ALSA read failed (%s)\n", snd_strerror(err));
					for(unsigned t=0;t<l;t++)
						buffer[t]=convert_sample<int16_t,sample_t>(buffer__int16_t[t]);
				}
				break;

				case 2:	// we need to convert int32_t->sample_t
				{ 
					unsigned l=PERIOD_SIZE_FRAMES*channelCount;
					if((err=snd_pcm_readi(parent->capture_handle, buffer__int32_t, PERIOD_SIZE_FRAMES)) < 0)
						fprintf(stderr, "ALSA read failed (%s)\n", snd_strerror(err));
					for(unsigned t=0;t<l;t++)
						buffer[t]=convert_sample<int32_t,sample_t>(buffer__int32_t[t]);
				}
				break;

				case 3:	// we need to convert float->sample_t
				{ 
					unsigned l=PERIOD_SIZE_FRAMES*channelCount;
					if((err=snd_pcm_readi(parent->capture_handle, buffer__float, PERIOD_SIZE_FRAMES)) < 0)
						fprintf(stderr, "ALSA read failed (%s)\n", snd_strerror(err));
					for(unsigned t=0;t<l;t++)
						buffer[t]=convert_sample<float,sample_t>(buffer__float[t]);
				}
				break;

				default:
					throw runtime_error(string(__func__)+" -- no conversion determined");
			}

			parent->onData(buffer,PERIOD_SIZE_FRAMES);

		}
	}
	catch(exception &e)
	{
		fprintf(stderr,"exception caught in record thread: %s\n",e.what());
	}
	catch(...)
	{
		fprintf(stderr,"unknown exception caught in record thread\n");
	}
}

#endif // ENABLE_ALSA
