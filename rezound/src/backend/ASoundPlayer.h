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

#ifndef __ASoundPlayer_H__
#define __ASoundPlayer_H__

#include "../../config/common.h"

class ASoundPlayer;

#include <set>

#ifdef HAVE_LIBRFFTW
#include <vector>
#include <map>
#include <rfftw.h>
#include <CMutex.h>
#include <TAutoBuffer.h>
#endif


/*
 - This class should be derived from to create the actual player for a given
   operating system platform.

 - This class is not thread-safe, methods from two independent threads should 
   not call common methods for an ASoundPlayer object without a locking mechanism

 - if deinitialize is overridden, then it should not raise an exception if
   the player is not initialized.

 - overriding initialize and deinitialize methods should invoke the overridden method
 	- this class's initialize() method should be called after the derived class 
	has finished its initialization

 - the derived class's destructor needs to call deinitialize because if this base
   class did it, some stuff could be freed in the derived class before the base
   class's destructor ran

 - To play sounds, call createChannel to get a CSoundPlayerChannel object that
   you can use to play, stop, seek ...
*/

#include "CSound_defs.h"
class CSoundPlayerChannel;

#include "DSP/LevelDetector.h"
	
#define MAX_OUTPUT_DEVICES 16
class ASoundPlayer
{
public:
	struct RDeviceParams
	{
		unsigned channelCount;
		unsigned sampleRate;

		RDeviceParams() : channelCount(1),sampleRate(44100) { }
	} devices[MAX_OUTPUT_DEVICES];

	virtual ~ASoundPlayer();

	virtual void initialize();
	virtual void deinitialize();
	virtual bool isInitialized() const=0;

	void stopAll();

	/*
	 * These need to be implemented to know if the output device(s)
	 * support full duplex mode or not. If not, then when ReZound
	 * is about to record this method should call killAll() and 
	 * deinitialize the device(s).  When doneRecording() is called
	 * then the device(s) should be reinitialized, but the playing
	 * state of each channel does not have to be restored.
	 *
	 * It is the responsbility of the caller of aboutToRecord() to 
	 * make sure that doneRecording() is called after recording has
	 * ended or an error has occurred.  Otherwise, the audio output
	 * would never be restored.
	 *
	 * If these are called while the sound player is not initialized
	 * the results are undefined.
	 */
	virtual void aboutToRecord()=0;
	virtual void doneRecording()=0;

	CSoundPlayerChannel *newSoundPlayerChannel(CSound *sound);

	// gets the max RMS level since the last call to this method for the same channel (hence to sub-systems could not currently use this method at the same time sinc they would interfere with each other's last-time-this-method was called)
	const sample_t getRMSLevel(unsigned channel) const;

	// gets the maximum peak level since the last call to this method for the same channel (hence two sub-systems could not currently use this method at the same time since they would interfere with each other's last-time-this-method-was-called)
	const sample_t getPeakLevel(unsigned channel) const;

	// Returns the frequency analysis of the most recent buffer played (if ReZound was configured and found fftw installed)
	// It returns a result that is not useful for careful analysis because for 
	// the CPU's sake it only does analysis on part of the buffer and doesn't 
	// make any effort to track results from buffers that were processed between 
	// calls to this function.  
	// The result is a vector of float with values 0 to 1 
	// The frequency band of each element is:
	//     f(i)=20*2^(i/2)
	// which gives each octave at i=0,2,4,6,8... and an band within the octave at 1,3,5,7...
	// So the elements would measure 20Hz, ~30Hz, 40Hz, ~60Hz, 80Hz, ~120Hz, 160Hz, ~240Hz, 320Hz, ~480Hz, 640Hz, ...
	// The number of elements will be according to the sample rate of the output device since the 
	// higher the sample rate the higher the frequency that can be represented in the data.  The elements
	// will represent as many bands as will fit from 20Hz up to half the sample rate, not going over.
	// the labels on the bands corrispond to its being the center frequency in the band (except perhaps the first and last band)
	const vector<float> getFrequencyAnalysis() const;
	const size_t getFrequencyAnalysisOctaveStride() const; // return the number of bands per octave returned by getFrequencyAnalysis

protected:

	ASoundPlayer();

	// bufferSize is in sample frames (NOT BYTES)
	void mixSoundPlayerChannels(const unsigned nChannels,sample_t * const buffer,const size_t bufferSize);

private:

	friend class CSoundPlayerChannel;

	set<CSoundPlayerChannel *> soundPlayerChannels; // ??? might as well be a vector
	void addSoundPlayerChannel(CSoundPlayerChannel *soundPlayerChannel);
	void removeSoundPlayerChannel(CSoundPlayerChannel *soundPlayerChannel);


	CDSPRMSLevelDetector RMSLevelDetectors[MAX_CHANNELS];

	mutable sample_t maxRMSLevels[MAX_CHANNELS];
	mutable bool resetMaxRMSLevels[MAX_CHANNELS]; // a bool that is flagged if the next buffer processed should start with a new max or max with the current one (since it hasn't been obtained from the get method yet)

	mutable sample_t peakLevels[MAX_CHANNELS];
	mutable bool resetPeakLevels[MAX_CHANNELS]; // a bool that is flagged if the next buffer processed should start with a new max or max with the current one (since it hasn't been obtained from the get method yet)

#ifdef HAVE_LIBRFFTW
	#define ASP_ANALYSIS_BUFFER_SIZE 8192
	mutable CMutex frequencyAnalysisBufferMutex;
	mutable bool frequencyAnalysisBufferPrepared;
	mutable fftw_real frequencyAnalysisBuffer[ASP_ANALYSIS_BUFFER_SIZE];
	size_t frequencyAnalysisBufferLength; // the amount of data that mixSoundPlayerChannels copied into the buffer
	mutable map<size_t,TAutoBuffer<fftw_real> *> hammingWindows; // create and save Hamming windows for any length needed
	rfftw_plan analyzerPlan;
	mutable vector<size_t> bandLowerIndexes; // mutable because calculateAnalyzerBandIndexRanges is called from getFrequencyAnalysis
	mutable vector<size_t> bandUpperIndexes;

	void calculateAnalyzerBandIndexRanges() const; // const because it is called from getFrequencyAnalysis
	static TAutoBuffer<fftw_real> *createHammingWindow(size_t windowSize);
#endif
};



#endif
