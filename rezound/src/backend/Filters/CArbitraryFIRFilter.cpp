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

#include "CArbitraryFIRFilter.h"

#include <algorithm>

#include "../CActionParameters.h"

#include "../DSP/Convolver.h"
#include "../unit_conv.h"

CArbitraryFIRFilter::CArbitraryFIRFilter(const CActionSound &actionSound,const float _wetdryMix,const CGraphParamValueNodeList &_freqResponse,const unsigned _firstOctaveFrequency,const unsigned _numberOfOctaves,const unsigned _kernelLength,const bool _removeDelay) :
	AAction(actionSound),

	wetdryMix(_wetdryMix),
		// these define how to interpret the values freqReponse[*].x and covert them to 0->1
	firstOctaveFrequency(_firstOctaveFrequency),
	numberOfOctaves(_numberOfOctaves),
	kernelLength(_kernelLength),
	removeDelay(_removeDelay)
{
	// the iterator won't work unless the range of the .x components are 0 to 1, so I have to convert the frequencies (in .x) to a value from 0 to 1
	// The range of .x is firstOctaveFrequency -> firstOctaveFrequency*pow(2,numberOfOctaves) so I have to fabricate values from 0 to baseFrequency
	// and eoither fabricate values above the max (if the sampleRate/2 is higher), or ignore some of the values  (if the sampleRate/2 is less than the max)
	freqResponse.push_back(CGraphParamValueNode(0,_freqResponse[0].y)); // fabricate the values from 0 to the base the same as the first element defined
	for(size_t t=0;t<_freqResponse.size();t++)
	{
		//printf("%d: %f, %f\n",t,_freqResponse[t].x,_freqResponse[t].y);

		if(_freqResponse[t].x>(actionSound.sound->getSampleRate()/2))
		{
			if(t<1)
			{ // shouldn't happen, but I suppose it could
				freqResponse.push_back(CGraphParamValueNode(1.0, _freqResponse[t].y ));
			}
			else
			{
				const double x1=_freqResponse[t-1].x;
				const double x2=_freqResponse[t].x;
				const double y1=_freqResponse[t-1].y;
				const double y2=_freqResponse[t].y;
				const double x=actionSound.sound->getSampleRate()/2;
									// interpolate where the truncation point to fall on the last line segment
				const double y=((y2-y1)/(x2-x1))*(x-x1)+y1;
				freqResponse.push_back(CGraphParamValueNode(1.0, y ));
			}
			break;
		}
		else
			freqResponse.push_back(CGraphParamValueNode(_freqResponse[t].x/(actionSound.sound->getSampleRate()/2),_freqResponse[t].y));
	}

	// fabricate values above the highest defined frequency in the given response if the sampleRate/2 is higher than it
	if(_freqResponse[_freqResponse.size()-1].x<=actionSound.sound->getSampleRate()/2)
		freqResponse.push_back(CGraphParamValueNode(1.0,_freqResponse[_freqResponse.size()-1].y));

/* might need to print the kernel nodes in debugging
	for(size_t t=0;t<freqResponse.size();t++)
		printf("%d: %f, %f\n",t,freqResponse[t].x,freqResponse[t].y);
*/
}

CArbitraryFIRFilter::~CArbitraryFIRFilter()
{
}

bool CArbitraryFIRFilter::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
#ifdef HAVE_LIBRFFTW
	const sample_pos_t start=actionSound.start;
	const sample_pos_t stop=actionSound.stop;
	const sample_pos_t selectionLength=actionSound.selectionLength();

	if(prepareForUndo)
		moveSelectionToTempPools(actionSound,mmSelection,actionSound.selectionLength());

	// given the user defined curve, now create the array of frequency component coefficients
		/* ???
		   the actual size of the fft that gets used is about 4 times the value below.. it should be at least twice as big bcause
		   we're only giving half the kernel (the magnitude, not the phase) but I'm not sure (without looking more) why it's doubling
		   it again.. someday I should look into this and either explain why it's happening or stop it from happening.  It might be 
		   because we're doing the +1 here.. but I do that to speed up the one-time fft it has to do to convert the given freq kernel
		   time domain kernel.. perhaps I could fudge the plus one value in convolver instead of here and not cause the 4x issue 
		*/
	const sample_pos_t filterKernelLength=kernelLength+1; // needs to be a parameter ... and I need to avoid tha horrible delay at the beginning
	TAutoBuffer<float> filterKernel(filterKernelLength);
	CGraphParamValueIterator fr_i(freqResponse,filterKernelLength);
	for(size_t t=0;t<filterKernelLength;t++)
		filterKernel[t]=fr_i.next();
	//filterKernel[0]=0.0; // I guess I should do this


/* might need to print the kernel in debugging
	for(size_t t=0;t<filterKernelLength;t++)
		printf("%d: %f\n",t,filterKernel[t]);
*/
			
	const float dryGain=(100.0-fabs(wetdryMix))/100.0 * (wetdryMix<0.0 ? -1.0 : 1.0);
	const float wetGain=wetdryMix/100.0;

	for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
	{
		if(actionSound.doChannel[i])
		{
			TFFTConvolverFrequencyDomainKernel<float,float> convolver(filterKernel,filterKernelLength);

			CRezPoolAccesser dest=actionSound.sound->getAudio(i);
			const CRezPoolAccesser src=prepareForUndo ? actionSound.sound->getTempAudio(tempAudioPoolKey,i) : actionSound.sound->getAudio(i);
			sample_pos_t srcOffset=prepareForUndo ? 0 : start;

			CStatusBar statusBar("Filtering -- Channel "+istring(i),start,stop,true); 

			if(removeDelay)
			{
				// the filter kernel (necessarily) causes a delay of filterKernelLength/2 samples in the filtered output.
				// so, here, I counter-act that effect by skipping the first filterKernelLength/2 samples

				sample_pos_t srcFeedPos=srcOffset; // where in src to feed to the convolver
				sample_pos_t srcReadPos=srcOffset; // where in src to read and mix with the convolver's output
				const sample_pos_t srcStop=srcOffset+src.getSize();
				sample_pos_t destPos=start;
				sample_pos_t skipOutput=filterKernelLength/2;

				while(destPos<=stop)
				{
					const sample_pos_t count=min(convolver.getChunkSize(),stop-destPos+1); // amount to read/write to the convolver

					// write to the convolver
					{
						convolver.beginWrite();
						if(count>(srcStop-srcFeedPos)) // <-- optimization
						{ // check for srcFeedPos overflow each time
							for(sample_pos_t t=0;t<count && srcFeedPos<srcStop;t++)
								convolver.writeSample(src[srcFeedPos++]);
						}
						else
						{ // avoid that check above
							for(sample_pos_t t=0;t<count;t++)
								convolver.writeSample(src[srcFeedPos++]);
						}
					}

					// read from the convolver
					{
						convolver.beginRead();

						// skip as much of the convolved output as we can and are supposed to
						const sample_pos_t maxToSkip=min(skipOutput,convolver.getChunkSize());
						sample_pos_t skippedAmount=0;
						for(;skippedAmount<maxToSkip; skippedAmount++,skipOutput--)
							convolver.readSample();

						// now read from the convolver and write back to the dest as much as we can and as much as we're supposed to
						const sample_pos_t maxToRead=min(count,convolver.getChunkSize()-skippedAmount);
						for(sample_pos_t t=0;t<maxToRead;t++)
							dest[destPos++]=ClipSample((src[srcReadPos++])*dryGain+convolver.readSample()*wetGain);
					}

					if(statusBar.update(destPos))
					{
						if(prepareForUndo)
							undoActionSizeSafe(actionSound);
						else
							actionSound.sound->invalidatePeakData(i,actionSound.start,destPos);
						return false;
					}
				}
			}
			else
			{
				sample_pos_t srcPos=srcOffset;
				sample_pos_t destPos=start;
				while(destPos<=stop)
				{
					const sample_pos_t count=min(convolver.getChunkSize(),stop-destPos+1);

					// write to the convolver
					convolver.beginWrite();
					for(sample_pos_t t=0;t<count;t++)
						convolver.writeSample(src[srcPos++]);
					srcPos-=count;

					// read from the convolver
					convolver.beginRead();
					for(sample_pos_t t=0;t<count;t++)
						dest[destPos++]=ClipSample((src[srcPos++])*dryGain+convolver.readSample()*wetGain);

					if(statusBar.update(destPos))
					{
						if(prepareForUndo)
							undoActionSizeSafe(actionSound);
						else
							actionSound.sound->invalidatePeakData(i,actionSound.start,destPos);
						return false;
					}
				}
			}

			if(!prepareForUndo)
				actionSound.sound->invalidatePeakData(i,actionSound.start,actionSound.stop);
		}
	}

	return(true);
#endif
	throw(EUserMessage(string(__func__)+" -- feature disabled because the fftw/rfftw library was not installed or detected when configure was run"));
}

AAction::CanUndoResults CArbitraryFIRFilter::canUndo(const CActionSound &actionSound) const
{
	return(curYes);
}

void CArbitraryFIRFilter::undoActionSizeSafe(const CActionSound &actionSound)
{
	restoreSelectionFromTempPools(actionSound,actionSound.start,actionSound.selectionLength());
}


// --------------------------------------------------

CArbitraryFIRFilterFactory::CArbitraryFIRFilterFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog) :
	AActionFactory("Arbitrary FIR Filter","Finite Impulse Response Filter Given a Custom/User-Defined Frequency Response",channelSelectDialog,dialog)
{
}

CArbitraryFIRFilterFactory::~CArbitraryFIRFilterFactory()
{
}

CArbitraryFIRFilter *CArbitraryFIRFilterFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return(new CArbitraryFIRFilter(
		actionSound,
		actionParameters->getDoubleParameter("Wet/Dry Mix"),
		actionParameters->getGraphParameter("Frequency Response"),
		actionParameters->getUnsignedParameter("Base Frequency"),
		actionParameters->getUnsignedParameter("Number of Octaves"),
		actionParameters->getUnsignedParameter("Kernel Length"),
		actionParameters->getBoolParameter("Undelay")
	));
}


