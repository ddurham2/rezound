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

 - The derived class should not cause onData() to be called before the base 
 class's initialize() method has be called and not call onData() method after
 deinitialize() has been called (or until initialize is called again).

 - This class appends the recorded data to the given ASound object

 - It should not be an error to deinitialize() while not initialize()

*/

#include <cc++/thread.h>
#include <string>

class ASoundRecorder
{
public:

	ASoundRecorder();
	virtual ~ASoundRecorder();

	void setStatusTrigger(TriggerFunc triggerFunc,void *data);
	void removeStatusTrigger(TriggerFunc triggerFunc,void *data);
	float getAndResetLastPeakValue(unsigned channel);

	virtual void initialize(ASound *sound);
	virtual void deinitialize();


	void start();
	void stop();
	virtual void redo();
	void done(); // should be called while initialized, but after stop to do extra-space-cleanup



	size_t clipCount;
	unsigned getChannelCount() const;
	unsigned getSampleRate() const;

	bool isStarted() const;
	const sample_pos_t getRecordedLength() const;
	string getRecordedLengthS() const;
	string getRecordedSizeS() const;

	// This function should be called when a data chunk is recorded
	// data should come in an interlaced format that is [sL1sR1 sL2sR2 sL3sR3 ...]
	//$$$$$Davy This wasn't working on my gcc as a protected member, 
	//so I just moved it into the public
	void onData(const sample_t *samples,const size_t sampleFramesRecorded);
protected:


private:
	ASound *sound;
	bool started;
	sample_pos_t prealloced;
	sample_pos_t origLength;
	sample_pos_t writePos;

	CTrigger statusTrigger;

	float lastPeakValues[MAX_CHANNELS];

	ost::Mutex mutex;

};

#endif
