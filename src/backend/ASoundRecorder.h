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

#include "CSound.h" // have to include here because we need CSound::RCue
#include "CTrigger.h"

/*

 - This class should be derived from to do sound recording for a specific
 platform.  The platform specific code should call the onData
 protected method when data is recorded.

 - The implementation should not cause initialize() to wait until recording stops
 but should return control to the caller while it records so the that the caller
 can also call redo and deinitialize to affect the recording process.

 - The derived class should NOT forget to call this base class's initialize and
 deinitialize methods.  initialize() should be called at the top of the derived 
 class's implementation, and deinitialize() should be called at the botton of
 the derived class's implementation.

 - The derived class should not cause onData() to be called before the base 
 class's initialize() method has be called and not call onData() method after
 deinitialize() has been called (or until initialize is called again).

 - This class appends the recorded data to the given CSound object
	- perhaps later a moveData will be issued at the end to move to where it was inserted

 - It should not be an error to deinitialize() while not initialize()

*/

#include <string>

#include <CMutex.h>

class CSound;

class ASoundRecorder
{
public:

	ASoundRecorder();
	virtual ~ASoundRecorder();

	static ASoundRecorder *createInitializedSoundRecorder(CSound *sound);

	void setStatusTrigger(TriggerFunc triggerFunc,void *data);
	void removeStatusTrigger(TriggerFunc triggerFunc,void *data);
	float getAndResetLastPeakValue(unsigned channel);

	virtual void initialize(CSound *sound);
	virtual void deinitialize();


				// startThreshold is in dBFS
	void start(const double startThreshold=1,const sample_pos_t maxDuration=NIL_SAMPLE_POS);
	void stop();
	virtual void redo(const sample_pos_t maxDuration=NIL_SAMPLE_POS);


	size_t clipCount;
	unsigned getChannelCount() const;
	unsigned getSampleRate() const;

	bool isStarted() const;
	bool isWaitingForThreshold() const; // may be started, but is still waiting for a certain threshold to actually start saving data
	const sample_pos_t getRecordedLength() const;
	string getRecordedLengthS() const;
	string getRecordedSizeS() const;
	string getRecordLimitS() const;

	const CSound *getSound() const { return(sound); }

	void addCueNow(const string name,bool isAnchored);

	sample_t getDCOffset(unsigned channel) const;

	// compensates for the current reported DC Offset
	void compensateForDCOffset();

protected:

	// This function should be called when a data chunk is recorded
	// data should come in an interlaced format that is [sL1sR1 sL2sR2 sL3sR3 ...]
	void onData(sample_t *samples,const size_t sampleFramesRecorded);

private:
	CSound *sound;
	bool started;
	sample_t startThreshold;
	sample_pos_t prealloced;
	sample_pos_t origLength;
	sample_pos_t writePos;
	sample_pos_t stopPosition;

	vector<CSound::RCue> addedCues;

	CTrigger statusTrigger;

	float lastPeakValues[MAX_CHANNELS];

	sample_t DCOffset[MAX_CHANNELS];
	double DCOffsetSum[MAX_CHANNELS];
	double DCOffsetCount;

	sample_t DCOffsetCompensation[MAX_CHANNELS];

	CMutex mutex;

	// deinitialize invokes this to cleanup any unnecessary prealloced space
	// and actually adds the cues to the sound if any were requested
	void done();

	void prvStop();

	bool cueNameExists(const string name) const;
};

#endif
