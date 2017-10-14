/* 
 * Copyright (C) 2010 - David W. Durham
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

#ifndef __CPulseSoundPlayer_H__
#define __CPulseSoundPlayer_H__

#include "../../config/common.h"

#ifdef ENABLE_PULSE

#include <memory>
#include "stdx/thread"

#include "ASoundPlayer.h"

struct pa_simple;

class CPulseSoundPlayer : public ASoundPlayer
{
public:

	CPulseSoundPlayer();
	virtual ~CPulseSoundPlayer();

	void initialize();
	void deinitialize();
	bool isInitialized() const;

	void aboutToRecord();
	void doneRecording();
private:

	bool initialized;
	pa_simple *stream;
	//bool supportsFullDuplex;
	//bool wasInitializedBeforeRecording;

	std::unique_ptr<stdx::thread> playThread;

	void threadWork();
};

#endif // ENABLE_PULSE

#endif
