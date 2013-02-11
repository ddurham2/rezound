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
#ifdef HAVE_FFTW
	analyzerPlan=NULL;
#endif
}

ASoundPlayer::~ASoundPlayer()
{
#ifdef HAVE_FFTW
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
	samplingForStereoPhaseMeters.setSize(gStereoPhaseMeterPointCount*devices[0].channelCount); // ??? only device zero

	// only handling the first device ???
	for(unsigned t=0;t<devices[0].channelCount;t++)
		RMSLevelDetectors[t].setWindowTime(ms_to_samples(gMeterRMSWindowTime,devices[0].sampleRate));



#ifdef HAVE_FFTW
	frequencyAnalysisBufferPrepared=false;
	for(size_t t=0;t<ASP_ANALYSIS_BUFFER_SIZE;t++)
		frequencyAnalysisBuffer[t]=0.0;

	analyzerPlan = fftw_plan_r2r_1d(ASP_ANALYSIS_BUFFER_SIZE, frequencyAnalysisBuffer, data, FFTW_HC2R, FFTW_ESTIMATE);

	// this causes calculateAnalyzerBandIndexRanges() to be called later when getting the analysis
	bandLowerIndexes.clear();
	bandUpperIndexes.clear();
#endif
}

void ASoundPlayer::deinitialize()
{
	stopAll();

#ifdef HAVE_FFTW
	if(analyzerPlan!=NULL)
	{
		fftw_destroy_plan(analyzerPlan);
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
	CMutexLocker ml(m);
	if(!soundPlayerChannels.insert(soundPlayerChannel).second)
		throw(runtime_error(string(__func__)+" -- sound player channel already in list"));
}

void ASoundPlayer::removeSoundPlayerChannel(CSoundPlayerChannel *soundPlayerChannel)
{
	CMutexLocker ml(m);

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

	{
		CMutexLocker ml(m);
		for(set<CSoundPlayerChannel *>::iterator i=soundPlayerChannels.begin();i!=soundPlayerChannels.end();i++)
			(*i)->mixOntoBuffer(nChannels,buffer,bufferSize);
	}


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

		// set peak levels to zero for values above the current number of channels
		for(unsigned i=nChannels;i<MAX_CHANNELS;i++)
			peakLevels[i]=0;
	}

	if(gStereoPhaseMetersEnabled)
		samplingForStereoPhaseMeters.write(buffer,bufferSize*nChannels, false); // NOTE: nChannels is supposed to equal devices[0].channelCount

#ifdef HAVE_FFTW
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
	CMutexLocker ml(m);
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


const size_t ASoundPlayer::getSamplingForStereoPhaseMeters(sample_t *buffer,size_t bufferSizeInSamples) const
{
	return samplingForStereoPhaseMeters.read(buffer,min(bufferSizeInSamples,(size_t)(gStereoPhaseMeterPointCount*devices[0].channelCount)), false)/devices[0].channelCount; // ??? only device zero
}

#ifdef HAVE_FFTW

// NOTE: when I say 'band', a band is expressed in terms of an 
// octave (where octave is a real number) and each band is a 
// multiple of deltaOctave (i.e when deltaOctave is 0.5, then 
// the bands are defined as octaves: 0, 0.5, 1.0, 1.5, 2.0, 2.5, etc)

// ??? these need to be settings in the registry and have enforced limits
static const float baseOctave=40;	// bottom frequency of analyzer  (actually the first band contains from 0Hz to upperFreqAtOctave(0) )
static const size_t octaveStride=6;	// 6 bands per octave
static const float deltaOctave=1.0f/octaveStride;

// returns the frequency (in Hz) given the octave
static float freqAtOctave(float octave)
{
	return baseOctave*powf(2.0f,octave);
}

// return middle of the previous band's frequency and our band's frequency
static float lowerFreqAtOctave(float octave)
{
	return (freqAtOctave(octave-deltaOctave)+freqAtOctave(octave))/2.0f;
}

// return middle of the our band's frequency and the next band's frequency
static float upperFreqAtOctave(float octave)
{
	return (freqAtOctave(octave)+freqAtOctave(octave+deltaOctave))/2.0f;
}

// returns the index (into an frequency domain array) given a frequency (but doesn't always return an integer, it returns what index we would wish to be there (perhaps between two elements))
static float indexAtFreq(float freq,unsigned sampleRate)
{
	return (2.0f*(ASP_ANALYSIS_BUFFER_SIZE/2)*freq)/(float)sampleRate;
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
	TAutoBuffer<fftw_real> *h=new TAutoBuffer<fftw_real>(windowSize, true);
	
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

#ifdef HAVE_FFTW
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

	fftw_execute(analyzerPlan);
	
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
#ifdef HAVE_FFTW
	return (size_t)freqAtOctave((float)index/(float)octaveStride);
#else
	return 0;
#endif
}

const size_t ASoundPlayer::getFrequencyAnalysisOctaveStride() const
{
#ifdef HAVE_FFTW
	return octaveStride;
#else
	return 1;
#endif
}


// ----------------------------

#include <stdio.h> // just for fprintf
#include <vector>
#include <string>

#include "CNULLSoundPlayer.h"
#include "COSSSoundPlayer.h"
#include "CALSASoundPlayer.h"
#include "CPortAudioSoundPlayer.h"
#include "CJACKSoundPlayer.h"
#include "CPulseSoundPlayer.h"

#include "AStatusComm.h"

#include <CNestedDataFile/CNestedDataFile.h>

ASoundPlayer *ASoundPlayer::createInitializedSoundPlayer()
{
	// if the registry doesn't already contain a methods setting, then create the default one
	if(gSettingsRegistry->keyExists("AudioOutputMethods")!=CNestedDataFile::ktValue)
	{
		vector<string> methods;
		methods.push_back("pulse");
		methods.push_back("oss");
		methods.push_back("alsa");
		methods.push_back("jack");
		methods.push_back("portaudio");
		gSettingsRegistry->setValue("AudioOutputMethods",methods);
	}


	bool initializeThrewException=false;
	ASoundPlayer *soundPlayer=NULL;

	vector<string> methods=gSettingsRegistry->getValue<vector<string> >("AudioOutputMethods");
	
	// add --audio-method=... to the beginning
	if(gDefaultAudioMethod!="")
		methods.insert(methods.begin(),gDefaultAudioMethod);

	// try this as a last resort (it just holds the pointer place (so it's not NULL throughout the rest of the code) but it is written to fail to initialize
	methods.push_back("null");

	// for each requested method in registry.AudioOutputMethods try each until one succeeds
	// 'suceeding' is true if the method was enabled at build time and it can initialize now at run-time
	for(size_t t=0;t<methods.size();t++)
	{
		const string method=methods[t];
		try
		{
#define INITIALIZE_PLAYER(ClassName)					\
			{						\
				initializeThrewException=false;		\
				delete soundPlayer;			\
				soundPlayer=new ClassName();		\
				soundPlayer->initialize();		\
				break; /* no exception thrown from initialize() so we're good to go */ \
			}

			if(method=="oss")
			{	
#ifdef ENABLE_OSS
				INITIALIZE_PLAYER(COSSSoundPlayer)
#endif
			}
			else if(method=="alsa")
			{	
#ifdef ENABLE_ALSA
				INITIALIZE_PLAYER(CALSASoundPlayer)
#endif
			}
			else if(method=="jack")
			{
#ifdef ENABLE_JACK
				INITIALIZE_PLAYER(CJACKSoundPlayer)
#endif
			}
			else if(method=="portaudio")
			{
#ifdef ENABLE_PORTAUDIO
				INITIALIZE_PLAYER(CPortAudioSoundPlayer)
#endif
			}
			else if(method=="pulse")
			{
#ifdef ENABLE_PULSE
				INITIALIZE_PLAYER(CPulseSoundPlayer)
#endif
			}
			else if(method=="null")
			{
				INITIALIZE_PLAYER(CNULLSoundPlayer)
			}
			else
			{
				Warning("unhandled method type in the registry:AudioOutputMethods[] '"+method+"'");
				continue;
			}
		}
		catch(exception &e)
		{ // now really give up
			fprintf(stderr,"Error occurred while initializing audio output method '%s' -- %s\n",method.c_str(),e.what());
			initializeThrewException=true;
		}
	}

	if(soundPlayer)
	{
		if(initializeThrewException)
			Error(_("No audio output method could be initialized -- Playing will be disabled."));

		return soundPlayer;
	}
	else	/* ??? this should never happen anymore now with CNULLSoundPlayer */
		throw runtime_error(string(__func__)+" -- "+_("Either no audio output method was enabled at configure-time, or no method was recognized in the registry:AudioOutputMethods[] setting"));
}

