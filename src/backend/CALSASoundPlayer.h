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

#ifndef __CALSASoundPlayer_H__
#define __CALSASoundPlayer_H__

#include "../../config/common.h"

#ifdef ENABLE_ALSA

#include <alsa/asoundlib.h>

#include "ASoundPlayer.h"

#include <AThread.h>

class CALSASoundPlayer : public ASoundPlayer
{
public:

	CALSASoundPlayer();
	virtual ~CALSASoundPlayer();

	void initialize();
	void deinitialize();
	bool isInitialized() const;

	void aboutToRecord();
	void doneRecording();
private:

	bool initialized;
	snd_pcm_t *playback_handle;
	snd_pcm_format_t playback_format;

	class CPlayThread : public AThread
	{
	public:
		CPlayThread(CALSASoundPlayer *parent);
		virtual ~CPlayThread();

		bool kill;

	protected:
		void main();

		CALSASoundPlayer *parent;

	};

	CPlayThread playThread;
	friend class CPlayThread;
};

#endif // ENABLE_ALSA

#endif
