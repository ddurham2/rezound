/* 
 * Copyright (C) 2003 - Marc Brevoort
 * Modified David W. Durham
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

#include "CGenerateNoiseAction.h"

#include "../CActionParameters.h"

CGenerateNoiseAction::CGenerateNoiseAction(const CActionSound actionSound,const double _noiseLength,const double _volume,const NoiseTypes _noiseType,const StereoImage _stereoImage,const double _maxParticleVelocity):
	AAction(actionSound),
	noiseLength(_noiseLength),	// seconds
	volume(_volume),		// 0 to 1 (a multiplier)
	noiseType(_noiseType),		// enum
	stereoImage(_stereoImage),	// enum
	origLength(0),

	maxParticleSpeed(_maxParticleVelocity),
	oldRandL(0),oldRandR(0)
{
	if(noiseLength<0)
		throw runtime_error(string(__func__)+" -- noiseLength is less than zero: "+istring(noiseLength));
}

CGenerateNoiseAction::~CGenerateNoiseAction()
{
}

void CGenerateNoiseAction::initializePinkNoise(PinkNoise *pink, int numRows)
{
	pink->pink_Index=0;
	pink->pink_IndexMask=(1<<numRows)-1;
	const long pmax=(numRows+1) * (1<<(PINK_RANDOM_BITS-1));
	pink->pink_Scalar=1.0f/pmax;

	for(int i=0;i<numRows;i++) 
		pink->pink_Rows[i]=0;
	pink->pink_RunningSum=0;
}

static unsigned long generateRandomNumber()
{
	// ??? this ain't too random is it?
	static unsigned long randSeed=time(NULL);
	randSeed=(randSeed*196314165+907633515);
	return randSeed;
}

// return a pink noise value from -1 to 1
double CGenerateNoiseAction::generatePinkNoise(PinkNoise *pink)
{
	long newRandom;
	pink->pink_Index=(pink->pink_Index+1) & pink->pink_IndexMask;

	if(pink->pink_Index!=0)
	{
		int numZeros=0;
		int n=pink->pink_Index;
		while((n&1)==0)
		{
			n=n>>1;
			numZeros++;
		}
		pink->pink_RunningSum-=pink->pink_Rows[numZeros];
		newRandom=((long)generateRandomNumber())>>PINK_RANDOM_SHIFT;
		pink->pink_RunningSum+=newRandom;
		pink->pink_Rows[numZeros]=newRandom;
	}

	// Add extra white noise value
	newRandom=((long)generateRandomNumber())>>PINK_RANDOM_SHIFT;
	long sum=pink->pink_RunningSum+newRandom;
	return 2.0*pink->pink_Scalar*sum;  // *2 because it's actually returning half the range we want (fix nature of algorithm?)
}

static double gaussrand(int n)
{
	double X=0.0;
	for (int i=0;i<n;i++)
		X+=((rand()/(RAND_MAX+1.0))-.5)*2.0;
	return X/n;
}

double CGenerateNoiseAction::generateBrownNoise(double *_oldRand,const double maxParticleSpeed)
{
	double oldRand=*_oldRand;
	double randval=gaussrand(20)*maxParticleSpeed/100.0;

	if( ((oldRand+randval)>=1.0) || (oldRand+randval<=-1.0)) 
		randval=-randval;

	oldRand+=randval;
	oldRand=min(1.0,max(-1.0,oldRand)); // limit to [-1.0, 1.0]

	*_oldRand=oldRand;
	return oldRand;
}


// return a noise value from -1 to 1
double CGenerateNoiseAction::getRandNoiseVal(const int noiseChannel)
{
	// This will return a random noise value, based on the type of noise that was requested.
	switch(noiseType)
	{
		case ntWhite:
			return ((double)rand()/RAND_MAX)*2.0-1.0;
		case ntPink:
			return generatePinkNoise(noiseChannel ? &pinkR : &pinkL);
		case ntBrown:
			return generateBrownNoise(noiseChannel ? &oldRandR : &oldRandL, maxParticleSpeed);
		case ntBlack:
			return 0.0; // :)
		default:
			throw runtime_error(string(__func__)+" -- internal error -- unimplemented noiseType: "+istring(noiseType));
	}
}

#warning add status bars
bool CGenerateNoiseAction::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	origLength=actionSound.sound->getLength();
	const sample_pos_t start=actionSound.start;
	const sample_pos_t sampleCount=(sample_pos_t)(noiseLength*actionSound.sound->getSampleRate());

	if(sampleCount==0) 
	{
		actionSound.stop=actionSound.start;
		return true;
	}
	
	actionSound.sound->addSpace(actionSound.doChannel,actionSound.start,sampleCount,true);
	actionSound.stop=(actionSound.start+sampleCount)-1;

	// new space is made for the noise, now calculate the values.

	switch(noiseType)
	{
		case ntWhite:
			srand(time(NULL));
			break;

		case ntPink:
			initializePinkNoise(&pinkL,16);
			initializePinkNoise(&pinkR,16);
			break;

		case ntBrown:
			oldRandL=0;
			oldRandR=0;
			break;

		case ntBlack:
			break;

		default:
			throw runtime_error(string(__func__)+" -- internal error -- unimplemented noiseType: "+istring(noiseType));
	}

	srand(time(NULL));

	// build a vector of indexes to the only channels that should be affected
	vector<unsigned> doChannels;
        for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
	{
                if(actionSound.doChannel[i]) 
			doChannels.push_back(i);
	}

        for(unsigned i=0;i<doChannels.size();i++)
        {
                CRezPoolAccesser destL=actionSound.sound->getAudio(doChannels[i]);

		if((i+1) < doChannels.size()) 
		{	// process a pair of channels
	        	CRezPoolAccesser destR=actionSound.sound->getAudio(doChannels[i+1]);
			sample_pos_t destPos=start;		
			for(sample_pos_t t=0;t<sampleCount;t++)
			{
				double randval=getRandNoiseVal(0);

				destL[destPos]=ClipSample(randval*volume*MAX_SAMPLE);

				switch(stereoImage) 
				{
					case siMono: 
						randval=randval; 
						break;

					case siInverse: 
						randval=-randval; 
						break;

					case siIndependent:
					case siSpatialStereo:
						randval=getRandNoiseVal(1);
						break;

					default: 
						throw runtime_error(string(__func__)+" -- internal error -- unimplemented stereoImage: "+istring(stereoImage));
				}
				destR[destPos]=ClipSample(randval*volume*MAX_SAMPLE);

				destPos++;
			}

			i++; // skip the next channel because we just did it
		}
		else
		{	// process a single channel
			sample_pos_t destPos=start;		
			for(sample_pos_t t=0;t<sampleCount;t++)
			{
				const double randval=getRandNoiseVal(0);
				destL[destPos]=ClipSample(randval*volume*MAX_SAMPLE);
				destPos++;
			}
		}
	}
	return true;
}

AAction::CanUndoResults CGenerateNoiseAction::canUndo(const CActionSound &actionSound) const
{
	return curYes;
}

void CGenerateNoiseAction::undoActionSizeSafe(const CActionSound &actionSound)
{
	const sample_pos_t sampleCount=(sample_pos_t)(noiseLength*actionSound.sound->getSampleRate());
	actionSound.sound->removeSpace(actionSound.doChannel,actionSound.start,sampleCount,origLength);
}



// ------------------------------

CGenerateNoiseActionFactory::CGenerateNoiseActionFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog) :
	AActionFactory(N_("Generate Noise"),_("Generate Various Colors of Noise"),channelSelectDialog,dialog)
{
}

CGenerateNoiseActionFactory::~CGenerateNoiseActionFactory()
{
}

// ??? t'would be nice if I had two factories: 1 for insert noise and 1 for replace selection with noise
CGenerateNoiseAction *CGenerateNoiseActionFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return new CGenerateNoiseAction(
		actionSound,
		actionParameters->getDoubleParameter("Length"),
		actionParameters->getDoubleParameter("Volume"),
		(CGenerateNoiseAction::NoiseTypes)actionParameters->getUnsignedParameter("Noise Color"),
		(CGenerateNoiseAction::StereoImage)actionParameters->getUnsignedParameter("Stereo Image"),
		actionParameters->getDoubleParameter("Max Particle Velocity")
	);
}	

