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

#include "ASoundPlayer.h"

#include <stdexcept>
#include <algorithm>

#include <math.h>

#include "CSound.h"
#include "CSoundPlayerChannel.h"

#include "settings.h"

#include "unit_conv.h"

ASoundPlayer::ASoundPlayer()
{
#ifdef HAVE_LIBRFFTW
	analyzerPlan=NULL;
#endif
}

ASoundPlayer::~ASoundPlayer()
{
#ifdef HAVE_LIBRFFTW
	for(map<size_t,TAutoBuffer<fftw_real> *>::iterator i=hammingWindows.begin();i!=hammingWindows.end();i++)
		delete i->second;
#endif
}

void ASoundPlayer::initialize()
{
	for(unsigned t=0;t<MAX_CHANNELS;t++)
	{
		maxRMSLevels[t]=peakLevels[t]=0;
		resetMaxRMSLevels[t]=resetPeakLevels[t]=false;
	}

	// only handling the first device ???
	for(unsigned t=0;t<devices[0].channelCount;t++)
		RMSLevelDetectors[t].setWindowTime(ms_to_samples(gMeterRMSWindowTime,devices[0].sampleRate));



#ifdef HAVE_LIBRFFTW
	frequencyAnalysisBufferPrepared=false;
	for(size_t t=0;t<ASP_ANALYSIS_BUFFER_SIZE;t++)
		frequencyAnalysisBuffer[t]=0.0;

	analyzerPlan=rfftw_create_plan(ASP_ANALYSIS_BUFFER_SIZE, FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE);

	// this causes calculateAnalyzerBandIndexRanges() to be called later when getting the analysis
	bandLowerIndexes.clear();
	bandUpperIndexes.clear();
#endif
}

void ASoundPlayer::deinitialize()
{
	stopAll();

#ifdef HAVE_LIBRFFTW
	if(analyzerPlan!=NULL)
	{
		rfftw_destroy_plan(analyzerPlan);
		analyzerPlan=NULL;
	}
#endif
}

CSoundPlayerChannel *ASoundPlayer::newSoundPlayerChannel(CSound *sound)
{
	return(new CSoundPlayerChannel(this,sound));
}

void ASoundPlayer::addSoundPlayerChannel(CSoundPlayerChannel *soundPlayerChannel)
{
	if(!soundPlayerChannels.insert(soundPlayerChannel).second)
		throw(runtime_error(string(__func__)+" -- sound player channel already in list"));
}

void ASoundPlayer::removeSoundPlayerChannel(CSoundPlayerChannel *soundPlayerChannel)
{
	soundPlayerChannel->stop();
	
	set<CSoundPlayerChannel *>::const_iterator i=soundPlayerChannels.find(soundPlayerChannel);
	if(i!=soundPlayerChannels.end())
		soundPlayerChannels.erase(i);
}

void ASoundPlayer::mixSoundPlayerChannels(const unsigned nChannels,sample_t * const buffer,const size_t bufferSize)
{
	memset(buffer,0,bufferSize*sizeof(*buffer)*nChannels);

	// ??? it might be nice that if no sound player channel object is playing that this method would not return
	// so that the caller wouldn't eat any CPU time doing anything with the silence returned

	for(set<CSoundPlayerChannel *>::iterator i=soundPlayerChannels.begin();i!=soundPlayerChannels.end();i++)
		(*i)->mixOntoBuffer(nChannels,buffer,bufferSize);


// ??? could just schedule this to occur (by making a copy of the buffer) the next time getLevel or getAnalysis is called rather than doing it here in the callback for mixing audio
	// calculate the peak levels and max RMS levels for this chunk
	if(gLevelMetersEnabled)
	{
		for(unsigned i=0;i<nChannels;i++)
		{
			size_t p=i;
			sample_t peak=resetPeakLevels[i] ? 0 : peakLevels[i];
			resetPeakLevels[i]=false;
			sample_t maxRMSLevel=resetMaxRMSLevels[i] ? 0 : maxRMSLevels[i];
			resetMaxRMSLevels[i]=false;
			for(size_t t=0;t<bufferSize;t++)
			{
				// m = max(m,abs(buffer[p]);
				sample_t s=buffer[p];

				s= s<0 ? -s : s; // s=abs(s)

				// peak=max(peak,s)
				if(peak<s)
					peak=s;

				// update the RMS level detectors
				sample_t RMSLevel=RMSLevelDetectors[i].readLevel(s);

				// RMSLevel=max(maxRMSLevel,RMSLevel)
				if(maxRMSLevel<RMSLevel)
					maxRMSLevel=RMSLevel;

				p+=nChannels;
			}
		
			maxRMSLevels[i]=maxRMSLevel;
			peakLevels[i]=peak;
		}
		for(unsigned i=nChannels;i<MAX_CHANNELS;i++)
			peakLevels[i]=0;
	}

#ifdef HAVE_LIBRFFTW
	if(gFrequencyAnalyzerEnabled)
	{
		CMutexLocker l(frequencyAnalysisBufferMutex);

		// put data into the frequency analysis buffer
		const size_t copyAmount=min((size_t)ASP_ANALYSIS_BUFFER_SIZE,(size_t)bufferSize); // only copy part of the output buffer (whichever is smaller)
		
		// mix all the output channels onto the buffer (copying the first channel and adding the others
		size_t q=0;
		for(size_t t=0;t<copyAmount;t++)
		{
			frequencyAnalysisBuffer[t]=buffer[q];
			q+=nChannels;
		}
		for(size_t i=1;i<nChannels;i++) 
		{
			q=i;
			for(size_t t=0;t<copyAmount;t++)
			{
				frequencyAnalysisBuffer[t]+=buffer[q];
				q+=nChannels;
			}
		}

		// I would normalize and apply the Hamming window here, but I'll defer that until the call to getFrequencyAnalysis()
		frequencyAnalysisBufferPrepared=false;
		frequencyAnalysisBufferLength=copyAmount;
	}
#endif
	
}

void ASoundPlayer::stopAll()
{
	for(set<CSoundPlayerChannel *>::iterator i=soundPlayerChannels.begin();i!=soundPlayerChannels.end();i++)
		(*i)->stop();
}

const sample_t ASoundPlayer::getRMSLevel(unsigned channel) const
{
	if(!isInitialized())
		return 0;

	//return RMSLevelDetectors[channel].readCurrentLevel();
	const sample_t r=maxRMSLevels[channel];
	resetMaxRMSLevels[channel]=true;
	return r;
}

const sample_t ASoundPlayer::getPeakLevel(unsigned channel) const
{
	if(!isInitialized())
		return 0;

	const sample_t p=peakLevels[channel];
	resetPeakLevels[channel]=true;
	return p;
}

#ifdef HAVE_LIBRFFTW

// NOTE: when I say 'band', a band is expressed in terms of an 
// octave (where octave is a real number) and each band is a 
// multiple of deltaOctave (i.e when deltaOctave is 0.5, then 
// the bands are defined as octaves: 0, 0.5, 1.0, 1.5, 2.0, 2.5, etc)

// ??? these need to be settings in the registry and have enforced limits
static const float baseOctave=40;	// bottom frequency of analyzer  (actually the first band contains from 0Hz to upperFreqAtOctave(0) )
static const size_t octaveStride=6;	// 6 bands per octave
static const float deltaOctave=1.0/octaveStride;

// returns the frequency (in Hz) given the octave
static float freqAtOctave(float octave)
{
	return baseOctave*pow((float)2.0,octave);
}

// return middle of the previous band's frequency and our band's frequency
static float lowerFreqAtOctave(float octave)
{
	return (freqAtOctave(octave-deltaOctave)+freqAtOctave(octave))/2.0;
}

// return middle of the our band's frequency and the next band's frequency
static float upperFreqAtOctave(float octave)
{
	return (freqAtOctave(octave)+freqAtOctave(octave+deltaOctave))/2.0;
}

// returns the index (into an frequency domain array) given a frequency (but doesn't always return an integer, it returns what index we would wish to be there (perhaps between two elements))
static float indexAtFreq(float freq,unsigned sampleRate)
{
	return (2.0*(ASP_ANALYSIS_BUFFER_SIZE/2)*freq)/(float)sampleRate;
}

// returns the (integer) lower index of the given band (expressed as an octave) into a frequency domain array
static size_t lowerIndexAtOctave(float octave,unsigned sampleRate)
{
	return (size_t)floor(indexAtFreq(lowerFreqAtOctave(octave),sampleRate));
}

// returns the (integer) upper index of the given band (expressed as an octave) into a frequency domain array
static size_t upperIndexAtOctave(float octave,unsigned sampleRate)
{
	return (size_t)(floor(indexAtFreq(upperFreqAtOctave(octave),sampleRate)));
}

void ASoundPlayer::calculateAnalyzerBandIndexRanges() const
{
	bandLowerIndexes.clear();
	bandUpperIndexes.clear();

	float octave=0;
	while(freqAtOctave(octave)<(devices[0].sampleRate/2))
	{
		bandLowerIndexes.push_back(lowerIndexAtOctave(octave,devices[0].sampleRate));
		bandUpperIndexes.push_back(upperIndexAtOctave(octave,devices[0].sampleRate));

		octave+=deltaOctave;
	}
	if(bandLowerIndexes.size()>0)
	{
		// make sure the first band includes the first element (but not actually 0Hz because that's just the DC offset)
		bandLowerIndexes[0]=1;

		// make sure all the indexes are in range
		for(size_t t=0;t<bandUpperIndexes.size();t++)
		{
			bandLowerIndexes[t]=max((int)1,min((int)ASP_ANALYSIS_BUFFER_SIZE/2,(int)bandLowerIndexes[t]));
			bandUpperIndexes[t]=max((int)1,min((int)ASP_ANALYSIS_BUFFER_SIZE/2,(int)bandUpperIndexes[t]));
		}
	}
	else
	{
		// shouldn't happen, but if it does, do this to avoid calling this method over and over by adding soemthing to the vectors
		bandLowerIndexes.push_back(0);
		bandUpperIndexes.push_back(0);
	}

	/*
	for(size_t t=0;t<bandLowerIndexes.size();t++)
		printf("%d .. %d\n",bandLowerIndexes[t],bandUpperIndexes[t]);
	*/
}

TAutoBuffer<fftw_real> *ASoundPlayer::createHammingWindow(size_t windowSize)
{
//printf("creating for length %d\n",windowSize);
	TAutoBuffer<fftw_real> *h=new TAutoBuffer<fftw_real>(windowSize);
	
	for(size_t t=0;t<windowSize;t++)
		(*h)[t]=0.54-0.46*cos(2.0*M_PI*t/windowSize);

	return h;
}

#endif


#include <stdlib.h>
const vector<float> ASoundPlayer::getFrequencyAnalysis() const
{ 
	vector<float> v;

	if(!isInitialized())
		return v;

#ifdef HAVE_LIBRFFTW
	CMutexLocker l(frequencyAnalysisBufferMutex);

	if(!frequencyAnalysisBufferPrepared)
	{
		// now divide by MAX_SAMPLE to normalize the values
		// and multiply by the Hamming window
		const fftw_real k=1.0/MAX_SAMPLE;

		if(hammingWindows.find(frequencyAnalysisBufferLength)==hammingWindows.end())
			hammingWindows[frequencyAnalysisBufferLength]=createHammingWindow(frequencyAnalysisBufferLength);
		const fftw_real *hammingWindow=*(hammingWindows[frequencyAnalysisBufferLength]);

		for(size_t t=0;t<frequencyAnalysisBufferLength;t++)
			frequencyAnalysisBuffer[t]*=k*hammingWindow[t];

			
		// in case bufferSize was less than ASP_ANALYSIS_BUFFER_SIZE pad the data with zeros
		for(size_t t=frequencyAnalysisBufferLength;t<ASP_ANALYSIS_BUFFER_SIZE;t++)
			frequencyAnalysisBuffer[t]=0.0;

		frequencyAnalysisBufferPrepared=true;
	}

/*
for(size_t t=0;t<ASP_ANALYSIS_BUFFER_SIZE;t++)
	((fftw_real *)frequencyAnalysisBuffer)[t]=((float)(rand()*2.0)/RAND_MAX)-1.0;
*/

	if(bandLowerIndexes.size()<=0) // calculate the index ranges for each band in the returned analysis since we haven't done it yet (I would do it on initialization, but I might not know the sampling rate at that point)
		calculateAnalyzerBandIndexRanges();
	

/*
for(int t=0;t<ASP_ANALYSIS_BUFFER_SIZE;t++)
{
	frequencyAnalysisBuffer[t]=sin(5.0*(t*M_PI*2.0/(ASP_ANALYSIS_BUFFER_SIZE)));
	frequencyAnalysisBuffer[t]+=0.5*sin(15.0*(t*M_PI*2.0/(ASP_ANALYSIS_BUFFER_SIZE)));
}
*/

	fftw_real data[ASP_ANALYSIS_BUFFER_SIZE];
	rfftw_one(analyzerPlan,(fftw_real *)frequencyAnalysisBuffer,data);
	
	for(size_t t=0;t<bandLowerIndexes.size();t++)
	{
		const size_t l=bandLowerIndexes[t];
		const size_t u=bandUpperIndexes[t];

		float sum=0;
		for(size_t i=l;i<u;i++)
		{
			const fftw_real re=data[i];
			const fftw_real im= (i==ASP_ANALYSIS_BUFFER_SIZE/2) ? 0 : data[ASP_ANALYSIS_BUFFER_SIZE-i];

			const fftw_real power=sqrt(re*re+im*im)/(ASP_ANALYSIS_BUFFER_SIZE/2); 

			//sum+=power*power;
			
			// find max magnitude of a frequency within band i
			if(sum<power)
				sum=power;
		}
		//sum/=(u-l);
		//sum=sqrt(sum);
		//v.push_back(sum);
		
		v.push_back(sum);
	}

#endif


#if 1 
	// this enables the returned values to be averaged with the previous NUM_AVG-1 analysies
	#define NUM_AVG 2
	static vector<float> prevVs[NUM_AVG];
	static size_t currentPrevV=0;

	// overwrite the oldest prev vector
	prevVs[currentPrevV]=v;
	
	currentPrevV++;
	currentPrevV%=NUM_AVG;

	vector<float> temp;

	if(prevVs[NUM_AVG-1].size()>0)
	{
		// create initial values in temp
		temp=prevVs[0];

		// add all over prev vectors to temp
		for(size_t i=1;i<NUM_AVG;i++)
		{
			for(size_t t=0;t<v.size();t++)
				temp[t]+=prevVs[i][t];
		}

		// divide by number of sums for an average
		for(size_t t=0;t<v.size();t++)
			temp[t]/=NUM_AVG;
	}

	return temp;
#else
	return v;
#endif
}

const size_t ASoundPlayer::getFrequency(size_t index) const
{
#ifdef HAVE_LIBRFFTW
	return (size_t)freqAtOctave((float)index/(float)octaveStride);
#else
	return 0;
#endif
}

const size_t ASoundPlayer::getFrequencyAnalysisOctaveStride() const
{
#ifdef HAVE_LIBRFFTW
	return octaveStride;
#else
	return 1;
#endif
}


// ----------------------------

#include "COSSSoundPlayer.h"
#include "CPortAudioSoundPlayer.h"
#include "CJACKSoundPlayer.h"

#include "AStatusComm.h"

#include <stdio.h> // just for fprintf

ASoundPlayer *ASoundPlayer::createInitializedSoundPlayer()
{
	ASoundPlayer *soundPlayer=NULL;

#if defined(ENABLE_PORTAUDIO)
	soundPlayer=new CPortAudioSoundPlayer();
#elif defined(ENABLE_JACK)
	soundPlayer=new CJACKSoundPlayer();
#elif defined(ENABLE_OSS)
	soundPlayer=new COSSSoundPlayer();
#endif

	try
	{
		soundPlayer->initialize();
	}
	catch(exception &e)
	{
#if !defined(ENABLE_PORAUDIO) && !defined(ENABLE_JACK)
		// OSS was the only defined method
		Error(string(e.what())+"\nPlaying will be disabled.");
#else
		// OSS was not the original method chosen at configure time so now fall back to using OSS if it wasn't disabled
	#ifdef ENABLE_OSS
		fprintf(stderr,"%s\n",(string(e.what())+"\nAttempting to fall back to using OSS for audio output.").c_str());
		//Warning(string(e.what())+"\nAttempting to fall back to using OSS for audio output.");

		// try OSS
		delete soundPlayer;
		soundPlayer=new COSSSoundPlayer();
		try
		{
			soundPlayer->initialize();
		}
		catch(exception &e)
		{ // now really give up
			Error(string("Error occurred after trying to fall back to OSS\n")+e.what()+"\nPlaying will be disabled.");
		}
	#else
		Error(string(e.what())+"\nPlaying will be disabled.");
	#endif
#endif
	}
	return soundPlayer;
}
