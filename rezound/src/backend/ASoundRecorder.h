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

#ifndef __ASoundRecorder_H__
#define __ASoundRecorder_H__


#include "../../config/common.h"

#include "ASound.h"

/*
 - This class should be derived from to do sound recording for a specific
 platform.  The platform specific code should call the onFunc protected 
 methods on each's appropriate event.
 - The implementation should not cause start() to wait until recording stops
 but should return control to the caller while it records so the that the caller
 can also call redo and deinit to affect the recording process.

 - This class appends the recorded data to the given pool file whose format must 
 match the parmaters also given at init time

 - If the derived class calls the onFunc events inappropriatly, that is, out of
 order, the behavior is undefined

 - We will eventually need a way to get a realtime audio status
	- probably could use a CTrigger object to do this

*/
class ASoundRecorder
{
public:

	ASoundRecorder();
	virtual ~ASoundRecorder();

	virtual void initialize(ASound *sound,const unsigned channelCount,const unsigned sampleRate)=0;
	virtual void deinitialize()=0;

	virtual void start()=0;
	virtual void redo()=0;

protected:

	// These functions should return true on success and false on failure

	// the implementation of init should call this 
	void onInit(ASound *sound,const unsigned channelCount,const unsigned sampleRate);

	// This function should be called before any data will be recorded
	void onStart();
	// This function should be called when a data chunk is recorded
	// data should come in an interlaced format that is [sL1sR1 sL2sR2 sL3sR3 ...]
	void onData(const sample_t *samples,const size_t sampleFramesRecorded);
	// This function should be called after all data is recorded (pass true if there was an error that caused the stop)
	void onStop(bool error);
	// This function should be called upon redo request
	void onRedo();

	unsigned channelCount;
	unsigned sampleRate;

private:
	ASound *sound;
	sample_pos_t prealloced;
	sample_pos_t origLength;

};

#endif
