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
#include "CTrigger.h"

/*
 - This class should be derived from to do sound recording for a specific
 platform.  The platform specific code should call the onData
 protected method when data is recorded.

 - The implementation should not cause initialize() to wait until recording stops
 but should return control to the caller while it records so the that the caller
 can also call redo and deinitialize to affect the recording process.

 - The derived class should NOT forget to call this base class's initialize and
 deinitialize methods.  initialize() should be called at the top of the dirved 
 class's implementation, and deinitialize() should be called at the botton of
 the derived class's implementation.

 - This class appends the recorded data to the given ASound object

*/
class ASoundRecorder
{
public:

	ASoundRecorder();
	virtual ~ASoundRecorder();

	void setPeakReadTrigger(TriggerFunc triggerFunc,void *data);
	void removePeakReadTrigger(TriggerFunc triggerFunc,void *data);
	float getLastPeakValue(unsigned channel) const;

	virtual void initialize(ASound *sound);
	// it should not be an error to deinitialize while not initialize
	virtual void deinitialize();

	void start();
	virtual void redo();

	unsigned getChannelCount() const;
	unsigned getSampleRate() const;

protected:

	// This function should be called when a data chunk is recorded
	// data should come in an interlaced format that is [sL1sR1 sL2sR2 sL3sR3 ...]
	void onData(const sample_t *samples,const size_t sampleFramesRecorded);

private:
	ASound *sound;
	bool started;
	sample_pos_t prealloced;
	sample_pos_t origLength;
	sample_pos_t writePos;

	CTrigger peakReadTrigger;

	float lastPeakValues[MAX_CHANNELS];

};

#endif
