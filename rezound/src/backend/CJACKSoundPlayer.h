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

#ifndef __CJACKSoundPlayer_H__
#define __CJACKSoundPlayer_H__

#include "../../config/common.h"

#ifdef ENABLE_JACK

#include "ASoundPlayer.h"

#include <jack/jack.h>

#include <TAutoBuffer.h>

class CJACKSoundPlayer : public ASoundPlayer
{
public:

	CJACKSoundPlayer();
	virtual ~CJACKSoundPlayer();

	void initialize();
	void deinitialize();
	bool isInitialized() const;

	void aboutToRecord();
	void doneRecording();

	// used only in CRecordClipBoard to know what sampleRate to initialize the new CSound object to
	static unsigned hack_sampleRate;

private:
	bool initialized;
	jack_client_t *client;
	jack_port_t *output_ports[MAX_CHANNELS];

	TAutoBuffer<sample_t> tempBuffer;

	static int processAudio(jack_nframes_t nframes,void *arg);
	static int sampleRateChanged(jack_nframes_t nframes,void *arg);
	static void jackShutdown(void *arg);
};

#endif // ENABLE_JACK

#endif
