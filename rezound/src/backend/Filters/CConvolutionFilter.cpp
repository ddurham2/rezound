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

#include "CConvolutionFilter.h"

#include <CPath.h>

#include "../DSP/Convolver.h"
#include "../DSP/TSoundStretcher.h"
#include "../DSP/SinglePoleFilters.h"
#include "../DSP/Delay.h"

#include "../unit_conv.h"

#include "../ASoundTranslator.h"
#include "../CActionParameters.h"

#include "../settings.h"

CConvolutionFilter::CConvolutionFilter(const CActionSound &actionSound,const float _wetdryMix,const float _inputGain,const float _outputGain,const float _inputLowpassFreq,const float _predelay,const string _filterKernelFilename,const bool _openFilterKernelAsRaw,const float _filterKernelGain,const float _filterKernelLowpassFreq,const float _filterKernelRate,const bool _reverseFilterKernel,const bool _wrapDecay) :
	AAction(actionSound),

	wetdryMix(_wetdryMix),
	inputGain(_inputGain),
	outputGain(_outputGain),
	inputLowpassFreq(_inputLowpassFreq),
	predelay(_predelay),

	filterKernelFilename(_filterKernelFilename),
	openFilterKernelAsRaw(_openFilterKernelAsRaw),
	filterKernelGain(_filterKernelGain),
	filterKernelLowpassFreq(_filterKernelLowpassFreq),
	filterKernelRate(_filterKernelRate),
	reverseFilterKernel(_reverseFilterKernel),
	wrapDecay(_wrapDecay)
{
}

CConvolutionFilter::~CConvolutionFilter()
{
}

#include <unistd.h> // for symlink and unlink
bool CConvolutionFilter::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
#ifdef HAVE_LIBRFFTW
	const sample_pos_t start=actionSound.start;
	const sample_pos_t stop=actionSound.stop;
	const sample_pos_t selectionLength=actionSound.selectionLength();

	const string filename=filterKernelFilename;
	if(!CPath(filename).exists()) // ??? need a "canRead()" method
		throw EUserMessage(string(__func__)+_(" -- cannot read kernel filter file '")+filename+"'");

	const string tempFilename=gFallbackWorkDir+"/filter_kernel_"+CPath(filename).baseName();

	// for lack of a better way.. I just make a link of the file and load that temp file incase it's already opened.. perhaps I should use the loaded CSound object if it exists (but right now there's not an easy way to do that)
	// 	in which case I would need a pointer to the gSoundFileManager which could be passed to the factory object when it's constructed in CMainWindow.cpp
	symlink(filename.c_str(),tempFilename.c_str());
	try
	{
		const ASoundTranslator *translator=ASoundTranslator::findTranslator(tempFilename,openFilterKernelAsRaw);

		CSound filterKernelFile;
		try
		{
			translator->loadSound(tempFilename,&filterKernelFile);

			if(prepareForUndo)
				moveSelectionToTempPools(actionSound,mmSelection,actionSound.selectionLength());

			// ??? see if I can use the first sample of the kernelfilter to control the dry gain so I don't have to rewind src
			const float dryGain=(100.0-fabs(wetdryMix))/100.0 * (wetdryMix<0.0 ? -1.0 : 1.0);
			const float wetGain=wetdryMix/100.0;

			unsigned channelsDoneCount=0;
			for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
			{
				if(actionSound.doChannel[i])
				{
					/*
					 * read the data from filterKernelFile but also adjust the sample rate 
					 * of the filterKernel file to match the action sound's sample rate 
					 */
					const sample_fpos_t rateAdjustment=(sample_fpos_t)actionSound.sound->getSampleRate()/(sample_fpos_t)filterKernelFile.getSampleRate();

					const CRezPoolAccesser filterKernelAccesser=filterKernelFile.getAudio(i%filterKernelFile.getChannelCount());

					const sample_pos_t filterKernelLength=(sample_pos_t)(filterKernelAccesser.getSize()*rateAdjustment/filterKernelRate);
					TAutoBuffer<float> filterKernel(filterKernelLength);

					TSoundStretcher<const CRezPoolAccesser> filterKernelStretcher(filterKernelAccesser,0,filterKernelAccesser.getSize(),filterKernelLength);

					TDSPSinglePoleLowpassFilter<float,float> filterKernelLowpassFilter(freq_to_fraction(filterKernelLowpassFreq,filterKernelFile.getSampleRate()));
					for(sample_pos_t t=0;t<filterKernelLength;t++)
						filterKernel[t]=filterKernelLowpassFilter.processSample(convert_sample<sample_t,float>(filterKernelStretcher.getSample())*filterKernelGain);

					if(reverseFilterKernel)
					{
						const sample_pos_t d=filterKernelLength/2;
						sample_pos_t p1=0;
						sample_pos_t p2=filterKernelLength-1;
						for(sample_pos_t t=0;t<d;t++)
						{
							float temp=filterKernel[p1];
							filterKernel[p1++]=filterKernel[p2];
							filterKernel[p2--]=temp;
						}
					}

						// ??? this might be a bad thing.. cause a value in the filter kernel of 0.0000001 might be on purpose
					// trim silent samples from the end of the filter kernel after rate changing, gain, and filtering
					sample_pos_t filterKernelLengthSub=0;
					for(sample_pos_t t=filterKernelLength-1;t>0;t--)
					{
						if(fabs(filterKernel[t])>0.0000001)
							break;
						filterKernelLengthSub++;
					}

					//TSimpleConvolver<mix_sample_t,float> convolver(filterKernel,filterKernelLength-filterKernelLengthSub);
					TFFTConvolverTimeDomainKernel<float,float> convolver(filterKernel,filterKernelLength-filterKernelLengthSub);

					TDSPSinglePoleLowpassFilter<float,float> inputLowpassFilter(freq_to_fraction(inputLowpassFreq,actionSound.sound->getSampleRate()));

					TDSPDelay<float> predelayer(ms_to_samples(predelay,actionSound.sound->getSampleRate()));

					CRezPoolAccesser dest=actionSound.sound->getAudio(i);
					const CRezPoolAccesser src=prepareForUndo ? actionSound.sound->getTempAudio(tempAudioPoolKey,i) : actionSound.sound->getAudio(i);
					sample_pos_t srcOffset=prepareForUndo ? 0 : start;

					CStatusBar statusBar(_("Convolving -- Channel ")+istring(++channelsDoneCount)+"/"+istring(actionSound.countChannels()),start,stop,true); 

					sample_pos_t srcPos=srcOffset;
					sample_pos_t destPos=start;
					sample_pos_t prevCount=0; // only needed in the wrapDecay section to know what size the last chunk was
					while(destPos<=stop)
					{
						const sample_pos_t count=min((sample_pos_t)convolver.getChunkSize(),stop-destPos+1);
						prevCount=count;

						// write to the convolver
						convolver.beginWrite();
						if(predelay>0)
						{
								for(sample_pos_t t=0;t<count;t++)
									convolver.writeSample(predelayer.processSample(inputLowpassFilter.processSample(inputGain*src[srcPos++])));
						}
						else // avoid running thru the predelay delay object if there is no predelay
						{
								for(sample_pos_t t=0;t<count;t++)
									convolver.writeSample(inputLowpassFilter.processSample(inputGain*src[srcPos++]));
						}
						srcPos-=count;

						// read from the convolver
						convolver.beginRead();
						for(sample_pos_t t=0;t<count;t++)
							dest[destPos++]=ClipSample(outputGain*(src[srcPos++]*dryGain+convolver.readSample()*wetGain));

						// (if TSimpleConvolver were used)
						//dest[destPos++]=ClipSample(convolver.processSample(src[srcPos++]));

						if(statusBar.update(destPos))
						{
							if(prepareForUndo)
								undoActionSizeSafe(actionSound);
							else
								actionSound.sound->invalidatePeakData(i,actionSound.start,destPos);
							filterKernelFile.closeSound();
							unlink(tempFilename.c_str());
							return false;
						}
					}

					if(wrapDecay)
					{
							destPos=start;

							// finish reading from a possibly incomplete write of a whole chunk (yet it did pad with zeros up there)
							for(sample_pos_t t=prevCount;t<convolver.getChunkSize();t++)
							{
								dest[destPos]=ClipSample(dest[destPos]+(outputGain*convolver.readSample()*wetGain));
								destPos++;
								if(destPos>stop)
										destPos=start;
							}

							// finish reading the extra samples from the convolution
							const sample_pos_t M=(filterKernelLength-filterKernelLengthSub); // I really think I could also subtract the iteration count of the preceeding loop (essentially because trailing zeros were processed in the final chunk)
							for(sample_pos_t t=0;t<M-1;t++)
							{
								dest[destPos]=ClipSample(dest[destPos]+(outputGain*convolver.readEndingSample()*wetGain));
								destPos++;
								if(destPos>stop)
										destPos=start;
							}
					}

					if(!prepareForUndo)
						actionSound.sound->invalidatePeakData(i,actionSound.start,actionSound.stop);
				}
			}

			filterKernelFile.closeSound();
			unlink(tempFilename.c_str());

			return(true);
		}
		catch(...)
		{
			filterKernelFile.closeSound();
			throw;
		}
	}
	catch(...)
	{
		unlink(tempFilename.c_str());
		throw;
	}
#endif
	throw(EUserMessage(string(__func__)+_(" -- feature disabled because the fftw/rfftw library was not installed or detected when configure was run")));
}

AAction::CanUndoResults CConvolutionFilter::canUndo(const CActionSound &actionSound) const
{
	return(curYes);
}

void CConvolutionFilter::undoActionSizeSafe(const CActionSound &actionSound)
{
	restoreSelectionFromTempPools(actionSound,actionSound.start,actionSound.selectionLength());
}


// --------------------------------------------------

CConvolutionFilterFactory::CConvolutionFilterFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog) :
	AActionFactory(N_("Convolution Filter"),_("Convolve One Audio File with this One"),channelSelectDialog,dialog)
{
}

CConvolutionFilterFactory::~CConvolutionFilterFactory()
{
}

CConvolutionFilter *CConvolutionFilterFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return(new CConvolutionFilter(
		actionSound,
		actionParameters->getDoubleParameter("Wet/Dry Mix"),

		actionParameters->getDoubleParameter("Input Gain"),
		actionParameters->getDoubleParameter("Output Gain"),
		actionParameters->getDoubleParameter("Input Lowpass"),
		actionParameters->getDoubleParameter("Predelay"),

		actionParameters->getStringParameter("Filter Kernel"),
		actionParameters->getBoolParameter("Filter Kernel OpenAsRaw"),
		actionParameters->getDoubleParameter("FK Gain"),
		actionParameters->getDoubleParameter("FK Lowpass"),
		actionParameters->getDoubleParameter("FK Rate"),
		actionParameters->getBoolParameter("Reverse"),
		actionParameters->getBoolParameter("Wrap Decay back to Beginning")
	));
}


