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

#ifndef __CJACKSoundRecorder
#define __CJACKSoundRecorder

#include "../../config/common.h"

#ifdef ENABLE_JACK

#include "ASoundRecorder.h"

#include <jack/jack.h>

#include <TAutoBuffer.h>

class CJACKSoundRecorder : public ASoundRecorder
{
public:
	CJACKSoundRecorder();
	virtual ~CJACKSoundRecorder();

	void initialize(CSound *sound);
	void deinitialize();

	void redo();

private:
	bool initialized;
	jack_client_t *client;
	jack_port_t *input_ports[MAX_CHANNELS];
	unsigned channelCount;

	TAutoBuffer<sample_t> tempBuffer;

	static int processAudio(jack_nframes_t nframes,void *arg);
	static int maxToProcessChanged(jack_nframes_t nframes,void *arg);
	static int sampleRateChanged(jack_nframes_t nframes,void *arg);
	static void jackShutdown(void *arg);
};

#endif // ENABLE_JACK

#endif
