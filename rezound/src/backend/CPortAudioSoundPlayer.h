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

#ifndef __CPortAudioSoundPlayer_H__
#define __CPortAudioSoundPlayer_H__

#include "../../config/common.h"

#ifdef HAVE_LIBPORTAUDIO

#include "ASoundPlayer.h"

#include <portaudio.h>

class CPortAudioSoundPlayer : public ASoundPlayer
{
public:

	CPortAudioSoundPlayer();
	virtual ~CPortAudioSoundPlayer();

	void initialize();
	void deinitialize();
	bool isInitialized() const;

	void aboutToRecord();
	void doneRecording();

private:
	bool initialized;
	PortAudioStream *stream;
	bool supportsFullDuplex;

	static int PortAudioCallback(void *inputBuffer,void *outputBuffer,unsigned long framesPerBuffer,PaTimestamp outTime,void *userData);

};

#endif // HAVE_LIBPORTAUDIO

#endif
