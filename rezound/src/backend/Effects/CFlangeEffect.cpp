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

#include "CFlangeEffect.h"

#include <stdexcept>
#include <memory>

#include "../DSP/FlangeEffect.h"
#include "../unit_conv.h"

#include "../CActionParameters.h"
#include "../CActionSound.h"

// LFO Speed is in Hertz, LFO Depth is in ?,LFO Phase is in degrees, Delay is in milliseconds
CFlangeEffect::CFlangeEffect(const CActionSound &actionSound,float _delayTime,float _wetGain,float _dryGain,const CLFODescription &_flangeLFO,float _feedback) :
	AAction(actionSound),

	// ??? perhaps I should do these conversions down in the code because I might someday be able to stream the action to disk for later use and the sampleRate would not necessarily be the same

	delayTime(_delayTime),

	wetGain(_wetGain),
	dryGain(_dryGain),

	flangeLFO(_flangeLFO),

	feedback(_feedback)
{
	if(_delayTime<0.0)
		throw(runtime_error(string(__func__)+" -- _delayTime is negative"));
	if(flangeLFO.amp<0.0)
		throw(runtime_error(string(__func__)+" -- flangeLFO.amp is negative"));
}

bool CFlangeEffect::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	const sample_pos_t start=actionSound.start;
	const sample_pos_t stop=actionSound.stop;

	if(prepareForUndo)
		moveSelectionToTempPools(actionSound,mmSelection,actionSound.selectionLength());

	for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
	{
		if(actionSound.doChannel[i])
		{
			CStatusBar statusBar("Flange -- Channel "+istring(i),start,stop); 

			CRezPoolAccesser dest=actionSound.sound->getAudio(i);
			const CRezPoolAccesser src=prepareForUndo ? actionSound.sound->getTempAudio(tempAudioPoolKey,i) : actionSound.sound->getAudio(i);

			auto_ptr<ALFO> LFO(gLFORegistry.createLFO(flangeLFO,actionSound.sound->getSampleRate()));

			CDSPFlangeEffect flangeEffect(
				ms_to_samples(delayTime,actionSound.sound->getSampleRate()),
				wetGain,
				dryGain,
				LFO.get(),
				ms_to_samples(flangeLFO.amp,actionSound.sound->getSampleRate()),
				feedback
				);

			sample_pos_t srcP=prepareForUndo ? 0 : start;
			for(sample_pos_t t=start;t<=stop;t++)
			{
				dest[t]=ClipSample(flangeEffect.processSample(src[srcP++]));
				statusBar.update(t);
			}
			
/* This code can be used to test what the LFOs actually look like
			for(sample_pos_t t=start;t<=stop;t++)
			{
				dest[t]=ClipSample(LFO->nextValue()*10000);
				UPDATE_PROGRESS_BAR(t);
			}
*/
		}
	}

	return(true);
}

AAction::CanUndoResults CFlangeEffect::canUndo(const CActionSound &actionSound) const
{
	return(curYes);
}

void CFlangeEffect::undoActionSizeSafe(const CActionSound &actionSound)
{
	restoreSelectionFromTempPools(actionSound,actionSound.start,actionSound.selectionLength());
}


// ---------------------------------------------

CFlangeEffectFactory::CFlangeEffectFactory(AActionDialog *channelSelectDialog,AActionDialog *normalDialog) :
	AActionFactory("Flange","Flange Effect",false,channelSelectDialog,normalDialog,NULL)
{
}

CFlangeEffect *CFlangeEffectFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	return(new CFlangeEffect(actionSound,
		(float)actionParameters->getDoubleParameter("Delay"),
		(float)actionParameters->getDoubleParameter("Wet Gain"),
		(float)actionParameters->getDoubleParameter("Dry Gain"),
		actionParameters->getLFODescription("Flange LFO"),
		(float)actionParameters->getDoubleParameter("Feedback")
	));
}

