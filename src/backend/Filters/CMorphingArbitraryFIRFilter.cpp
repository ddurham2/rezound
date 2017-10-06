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

#include "CMorphingArbitraryFIRFilter.h"

#include <algorithm>
#include <memory>

#include "../CActionParameters.h"

#include "../DSP/Convolver.h"
#include "../unit_conv.h"

#include "filters_util.h"

CMorphingArbitraryFIRFilter::CMorphingArbitraryFIRFilter(const AActionFactory *factory,const CActionSound *actionSound,const float _wetdryMix,const CGraphParamValueNodeList &_freqResponse1,const CGraphParamValueNodeList &_freqResponse2,const bool _useLFO,const CLFODescription &_sweepLFODesc,const unsigned _kernelLength,const bool _removeDelay) :
	AAction(factory,actionSound),

	wetdryMix(_wetdryMix),
	freqResponse1(_freqResponse1),
	freqResponse2(_freqResponse2),
	useLFO(_useLFO),
	sweepLFODesc(_sweepLFODesc),
	kernelLength(_kernelLength),
	removeDelay(_removeDelay)
{
	if(sweepLFODesc.amp<0.0)
		throw runtime_error(string(__func__)+_(" -- sweepLFODesc.amp is negative"));
	if(freqResponse1.size()!=freqResponse2.size())
		throw runtime_error(string(__func__)+_(" -- freqResponse1 and freqResonse2 do not contain the same number of nodes"));
}

CMorphingArbitraryFIRFilter::~CMorphingArbitraryFIRFilter()
{
}

bool CMorphingArbitraryFIRFilter::doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo)
{
#ifdef HAVE_FFTW
	const sample_pos_t start=actionSound->start;
	const sample_pos_t stop=actionSound->stop;
	const sample_pos_t selectionLength=actionSound->selectionLength();

	if(prepareForUndo)
		moveSelectionToTempPools(actionSound,mmSelection,actionSound->selectionLength());

	const float dryGain=(100.0-fabs(wetdryMix))/100.0 * (wetdryMix<0.0 ? -1.0 : 1.0);
	const float wetGain=wetdryMix/100.0;


	// given the user defined curve, now create the array of frequency component coefficients
	const sample_pos_t filterKernelLength=kernelLength+1;
	TAutoBuffer<float> filterKernel(filterKernelLength);


	unsigned channelsDoneCount=0;
	for(unsigned i=0;i<actionSound->sound->getChannelCount();i++)
	{
		if(actionSound->doChannel[i])
		{
										/* constructing with bogus kernel values right now */
			TFFTConvolverFrequencyDomainKernel<float,float> convolver(filterKernel,filterKernelLength);

			CRezPoolAccesser dest=actionSound->sound->getAudio(i);
			const CRezPoolAccesser src=prepareForUndo ? actionSound->sound->getTempAudio(tempAudioPoolKey,i) : actionSound->sound->getAudio(i);
			sample_pos_t srcOffset=prepareForUndo ? 0 : start;

			CStatusBar statusBar(_("Filtering -- Channel ")+istring(++channelsDoneCount)+"/"+istring(actionSound->countChannels()),start,stop,true); 

			auto_ptr<ALFO> sweepLFO(gLFORegistry.createLFO(sweepLFODesc,actionSound->sound->getSampleRate()));

			// #defined because this occurs between both the removeDelay and !removeDelay code
			#define RECALC_FILTER_KERNEL 										\
			{ 													\
				/* p: [0,1) of the morphing interpolation position */						\
				const double p=useLFO ? (double)(sweepLFO->getValue(destPos-start)) : (double)((sample_fpos_t)(destPos-start)/(sample_fpos_t)(stop-start)); \
 																\
				CGraphParamValueNodeList freqResponse; 								\
					/* NOTE: freqResponse1 and freqResponse2 contain exactly the same number of nodes */	\
				for(size_t t=0;t<freqResponse1.size();t++) 							\
				{ 												\
					const double x1=freqResponse1[t].x; 							\
					const double y1=freqResponse1[t].y; 							\
					const double x2=freqResponse2[t].x; 							\
					const double y2=freqResponse2[t].y; 							\
					freqResponse.push_back(CGraphParamValueNode( 						\
						x1+((x2-x1)*p), 								\
						y1+((y2-y1)*p) 									\
					)); 											\
				} 												\
 																\
				/* the iterator won't work unless the range of the .x components are 0 to 1, so I have to convert the frequencies (in .x) to a value from 0 to 1 */ \
				const CGraphParamValueNodeList normFreqResponse=normalizeFrequencyResponse(freqResponse,actionSound->sound->getSampleRate()); \
 																\
				CGraphParamValueIterator fr_i(normFreqResponse,filterKernelLength); 				\
				for(size_t t=0;t<filterKernelLength;t++) 							\
					filterKernel[t]=fr_i.next(); 								\
				convolver.setNewMagnitudeArray(filterKernel,filterKernelLength); 				\
			}

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
					const sample_pos_t count=min((sample_pos_t)convolver.getChunkSize(),stop-destPos+1); // amount to read/write to the convolver

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

					// update with a new set of frequency coefficients (morphing) between each frame
					RECALC_FILTER_KERNEL

					// read from the convolver
					{
						convolver.beginRead();

						// skip as much of the convolved output as we can and are supposed to
						const sample_pos_t maxToSkip=min(skipOutput,(sample_pos_t)convolver.getChunkSize());
						sample_pos_t skippedAmount=0;
						for(;skippedAmount<maxToSkip; skippedAmount++,skipOutput--)
							convolver.readSample();

						// now read from the convolver and write back to the dest as much as we can and as much as we're supposed to
						const sample_pos_t maxToRead=min(count,(sample_pos_t)convolver.getChunkSize()-skippedAmount);
						for(sample_pos_t t=0;t<maxToRead;t++)
							dest[destPos++]=ClipSample((src[srcReadPos++])*dryGain+convolver.readSample()*wetGain);
					}

					if(statusBar.update(destPos))
					{
						if(prepareForUndo)
							undoActionSizeSafe(actionSound);
						else
							actionSound->sound->invalidatePeakData(i,actionSound->start,destPos);
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
					const sample_pos_t count=min((sample_pos_t)convolver.getChunkSize(),stop-destPos+1);

					// write to the convolver
					convolver.beginWrite();
					for(sample_pos_t t=0;t<count;t++)
						convolver.writeSample(src[srcPos++]);
					srcPos-=count;

					// update with a new set of frequency coefficients (morphing) between each frame
					RECALC_FILTER_KERNEL

					// read from the convolver
					convolver.beginRead();
					for(sample_pos_t t=0;t<count;t++)
						dest[destPos++]=ClipSample((src[srcPos++])*dryGain+convolver.readSample()*wetGain);

					if(statusBar.update(destPos))
					{
						if(prepareForUndo)
							undoActionSizeSafe(actionSound);
						else
							actionSound->sound->invalidatePeakData(i,actionSound->start,destPos);
						return false;
					}
				}
			}

			if(!prepareForUndo)
				actionSound->sound->invalidatePeakData(i,actionSound->start,actionSound->stop);
		}
	}

	return true;
#endif
	throw EUserMessage(string(__func__)+_(" -- feature disabled because the fftw/rfftw library was not installed or detected when configure was run"));
}

AAction::CanUndoResults CMorphingArbitraryFIRFilter::canUndo(const CActionSound *actionSound) const
{
	return curYes;
}

void CMorphingArbitraryFIRFilter::undoActionSizeSafe(const CActionSound *actionSound)
{
	restoreSelectionFromTempPools(actionSound,actionSound->start,actionSound->selectionLength());
}

const string CMorphingArbitraryFIRFilter::getExplanation()
{
	return _("\n\
This action will begin filtering with the first frequency response and morph the filter to the second frequency response.\n\
Both frequency reponses must have the same number of points.\n\
The morphing is calculated as each point in the first frequency response moving toward the position of the corrisponding point in the second frequency response.  A point corrisponds to another point if it is in the same order left to right.\n\
If 'Use LFO' is not checked, then the filter will use the entire selection's time to morph from the first to the second frequency reponse.\n\
If 'Use LFO' is checked, then the morphing will oscillate according to the LFO.\n\
A longer Kernel Length will have better filter frequency resolution, but too long of a Kernel Length will cause `steps` in the morphing to be audible depending on how different the two responses are.\n\
");
}

// --------------------------------------------------

CMorphingArbitraryFIRFilterFactory::CMorphingArbitraryFIRFilterFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog) :
	AActionFactory(N_("Morphing Arbitrary FIR Filter"),_("Animated Finite Impulse Response Filter Given two Custom/User-Defined Frequency Responses"),channelSelectDialog,dialog)
{
}

CMorphingArbitraryFIRFilterFactory::~CMorphingArbitraryFIRFilterFactory()
{
}

CMorphingArbitraryFIRFilter *CMorphingArbitraryFIRFilterFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return new CMorphingArbitraryFIRFilter(
		this,
		actionSound,
		actionParameters->getValue<float>("Wet/Dry Mix"),
		actionParameters->getValue<CGraphParamValueNodeList>("Frequency Response 1"),
		actionParameters->getValue<CGraphParamValueNodeList>("Frequency Response 2"),
		actionParameters->getValue<bool>("Use LFO"),
		actionParameters->getValue<CLFODescription>("Sweep LFO"),
		actionParameters->getValue<unsigned>("Kernel Length"),
		actionParameters->getValue<bool>("Undelay")
	);
}

