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

#include "CBiquadResFilter.h"

#include <stdexcept>

#include "../CActionSound.h"
#include "../CActionParameters.h"

#include "../DSP/BiquadResFilters.h"
#include "../unit_conv.h"

CBiquadResFilter::CBiquadResFilter(const CActionSound &actionSound,FilterTypes _filterType,float _gain,float _frequency,float _resonance) :
	AAction(actionSound),
	filterType(_filterType),
	gain(_gain),
	frequency(_frequency),
	resonance(_resonance)
{
}

CBiquadResFilter::~CBiquadResFilter()
{
}

bool CBiquadResFilter::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	const sample_pos_t start=actionSound.start;
	const sample_pos_t stop=actionSound.stop;
	const sample_pos_t selectionLength=actionSound.selectionLength();

	if(prepareForUndo)
		moveSelectionToTempPools(actionSound,mmSelection,actionSound.selectionLength());

	unsigned channelsDoneCount=0;
	for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
	{
		if(actionSound.doChannel[i])
		{
			CRezPoolAccesser dest=actionSound.sound->getAudio(i);
			const CRezPoolAccesser src=prepareForUndo ? actionSound.sound->getTempAudio(tempAudioPoolKey,i) : actionSound.sound->getAudio(i);
			sample_pos_t srcOffset=prepareForUndo ? start : 0;
			const register float gain=this->gain;

			#define STATUS_BAR_UPDATE(t) \
				if(statusBar.update(t)) \
				{ /* cancelled */ \
					if(prepareForUndo) \
						undoActionSizeSafe(actionSound); \
					else \
						actionSound.sound->invalidatePeakData(i,actionSound.start,t); \
					return false; \
				}

			switch(filterType)
			{
			case ftLowpass:
			{
				CStatusBar statusBar("Lowpass Filter -- Channel "+istring(++channelsDoneCount)+"/"+istring(actionSound.countChannels()),start,stop,true);

				TDSPBiquadResLowpassFilter<mix_sample_t> filter(freq_to_fraction(frequency,actionSound.sound->getSampleRate()),resonance);
				for(sample_pos_t t=start;t<=stop;t++)
				{
					dest[t]=ClipSample(filter.processSample((mix_sample_t)(gain*src[t-srcOffset])));
					STATUS_BAR_UPDATE(t)
				}
			break;
			}

			case ftHighpass:
			{
				CStatusBar statusBar("Highpass Filter -- Channel "+istring(++channelsDoneCount)+"/"+istring(actionSound.countChannels()),start,stop,true);

				TDSPBiquadResHighpassFilter<mix_sample_t> filter(freq_to_fraction(frequency,actionSound.sound->getSampleRate()),resonance);
				for(sample_pos_t t=start;t<=stop;t++)
				{
					dest[t]=ClipSample(filter.processSample((mix_sample_t)(gain*src[t-srcOffset])));
					STATUS_BAR_UPDATE(t)
				}
			break;
			}

			case ftBandpass:
			{
				CStatusBar statusBar("Bandpass Filter -- Channel "+istring(++channelsDoneCount)+"/"+istring(actionSound.countChannels()),start,stop,true); 

				TDSPBiquadResBandpassFilter<mix_sample_t> filter(freq_to_fraction(frequency,actionSound.sound->getSampleRate()),resonance);
				for(sample_pos_t t=start;t<=stop;t++)
				{
					dest[t]=ClipSample(filter.processSample((mix_sample_t)(gain*src[t-srcOffset])));
					STATUS_BAR_UPDATE(t)
				}
			break;
			}

			default:
				throw(runtime_error(string(__func__)+" -- invalid filterType: "+istring(filterType)));
			}


			if(!prepareForUndo)
				actionSound.sound->invalidatePeakData(i,actionSound.start,actionSound.stop);
		}
	}

	return(true);
}

AAction::CanUndoResults CBiquadResFilter::canUndo(const CActionSound &actionSound) const
{
	return(curYes);
}

void CBiquadResFilter::undoActionSizeSafe(const CActionSound &actionSound)
{
	restoreSelectionFromTempPools(actionSound,actionSound.start,actionSound.selectionLength());
}


// --------------------------------------------------

CBiquadResLowpassFilterFactory::CBiquadResLowpassFilterFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog) :
	AActionFactory("Biquad Resonant Lowpass Filter","Single Pole Lowpass Filter",channelSelectDialog,dialog)
{
}

CBiquadResLowpassFilterFactory::~CBiquadResLowpassFilterFactory()
{
}

CBiquadResFilter *CBiquadResLowpassFilterFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return(new CBiquadResFilter(
		actionSound,
		CBiquadResFilter::ftLowpass,
		(float)actionParameters->getDoubleParameter("Gain"),
		(float)actionParameters->getDoubleParameter("Cutoff Frequency"),
		(float)actionParameters->getDoubleParameter("Resonance")
	));
}

// --------------------------------------------------

CBiquadResHighpassFilterFactory::CBiquadResHighpassFilterFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog) :
	AActionFactory("Biquad Resonant Highpass Filter","Single Pole Highpass Filter",channelSelectDialog,dialog)
{
}

CBiquadResHighpassFilterFactory::~CBiquadResHighpassFilterFactory()
{
}

CBiquadResFilter *CBiquadResHighpassFilterFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return(new CBiquadResFilter(
		actionSound,
		CBiquadResFilter::ftHighpass,
		(float)actionParameters->getDoubleParameter("Gain"),
		(float)actionParameters->getDoubleParameter("Cutoff Frequency"),
		(float)actionParameters->getDoubleParameter("Resonance")
	));
}

// --------------------------------------------------

CBiquadResBandpassFilterFactory::CBiquadResBandpassFilterFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog) :
	AActionFactory("Biquad Resonant Bandpass Filter","Bandpass Filter",channelSelectDialog,dialog)
{
}

CBiquadResBandpassFilterFactory::~CBiquadResBandpassFilterFactory()
{
}

CBiquadResFilter *CBiquadResBandpassFilterFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return(new CBiquadResFilter(
		actionSound,
		CBiquadResFilter::ftBandpass,
		(float)actionParameters->getDoubleParameter("Gain"),
		(float)actionParameters->getDoubleParameter("Center Frequency"),
		(float)actionParameters->getDoubleParameter("Resonance")
	));
}



