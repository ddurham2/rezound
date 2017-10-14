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

#ifndef __CALSASoundRecorder_H__
#define __CALSASoundRecorder_H__

#include "../../config/common.h"

#ifdef ENABLE_ALSA

#include <memory>
#include "stdx/thread"

#include <alsa/asoundlib.h>

#include "ASoundRecorder.h"

class CALSASoundRecorder : public ASoundRecorder
{
public:
	CALSASoundRecorder();
	virtual ~CALSASoundRecorder();

	void initialize(CSound *sound);
	void deinitialize();

	void redo();

private:
	bool initialized;
	snd_pcm_t *capture_handle;
	snd_pcm_format_t capture_format;

	std::unique_ptr<stdx::thread> recordThread;

	void threadWork();
};

#endif // ENABLE_ALSA

#endif
