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

#include "CNoiseGateAction.h"

#include "../CActionSound.h"
#include "../CActionParameters.h"

#include "../DSP/NoiseGate.h"
#include "../unit_conv.h"

CNoiseGateAction::CNoiseGateAction(const CActionSound &actionSound,const float _windowTime,const float _threshold,const float _gainAttackTime,const float _gainReleaseTime) :
	AAction(actionSound),

	windowTime(_windowTime),
	threshold(_threshold),

	// attack and release are a bit backwards, since the attack time is how fast it cuts the gain 
	// below the threshold and release is how fast it restores the gain above the threshold
	gainAttackTime(_gainAttackTime),
	gainReleaseTime(_gainReleaseTime)
{
	if(windowTime<0.0)
		throw(runtime_error(string(__func__)+" -- windowTime is negative"));
	if(threshold<0.0)
		throw(runtime_error(string(__func__)+" -- threshold is negative"));
	if(gainAttackTime<0.0)
		throw(runtime_error(string(__func__)+" -- gainAttackTime is negative"));
	if(gainReleaseTime<0.0)
		throw(runtime_error(string(__func__)+" -- gainReleaseTime is negative"));
}

bool CNoiseGateAction::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	const sample_pos_t start=actionSound.start;
	const sample_pos_t stop=actionSound.stop;
	const sample_pos_t selectionLength=actionSound.selectionLength();

	if(prepareForUndo)
		moveSelectionToTempPools(actionSound,mmSelection,selectionLength);

	for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
	{
		if(actionSound.doChannel[i])
		{
			CRezPoolAccesser a=actionSound.sound->getAudio(i);
			const CRezPoolAccesser const_a=actionSound.sound->getAudio(i); // used to avoid writing cached block back to disk if unnecessary

			if(prepareForUndo)
				a.copyData(start,actionSound.sound->getTempAudio(tempAudioPoolKey,i),0,selectionLength);

			BEGIN_PROGRESS_BAR("Noise Gating -- Channel "+istring(i),start,stop);

			CDSPNoiseGate gate(
				ms_to_samples(windowTime,actionSound.sound->getSampleRate()),
				percent_to_amp(threshold),
				ms_to_samples(gainAttackTime,actionSound.sound->getSampleRate()),
				ms_to_samples(gainReleaseTime,actionSound.sound->getSampleRate())
				);

			// initialize the level detector within the noise gate block
			sample_pos_t t=start;
			for(;(t-start)<gate.getWindowTime() && t<=stop;t++)
				gate.initSample(const_a[t]);

			// now adjust the samples amplitudes by gain where gain bounces between 0 and 1 
			// depending on whether the level is above or below the threshold
			for(;t<=stop;t++)
			{
				const mix_sample_t s1=const_a[t];
				const mix_sample_t s2=gate.processSample(s1);
				if(s1!=s2) // avoid writing the sample to disk if it didn't change
					a[t]=s2;

				UPDATE_PROGRESS_BAR(t);
			}

			END_PROGRESS_BAR();

			actionSound.sound->invalidatePeakData(i,actionSound.start,actionSound.stop);
		}
	}

	return(true);
}

AAction::CanUndoResults CNoiseGateAction::canUndo(const CActionSound &actionSound) const
{
	return(curYes);
}

void CNoiseGateAction::undoActionSizeSafe(const CActionSound &actionSound)
{
	restoreSelectionFromTempPools(actionSound,actionSound.start,actionSound.selectionLength());
}


// --------------------------------------------------

CNoiseGateActionFactory::CNoiseGateActionFactory(AActionDialog *channelSelectDialog,AActionDialog *normalDialog) :
	AActionFactory("Noise Gate","Applies a gain [0,1] to the sound when the level becomes less than the threshold.\nThe gain changes with a velocity according to the attack and release times",false,channelSelectDialog,normalDialog,NULL)
{
}

CNoiseGateAction *CNoiseGateActionFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	return(new CNoiseGateAction(
		actionSound,
		actionParameters->getDoubleParameter("Window Time"),
		actionParameters->getDoubleParameter("Threshold"),
		actionParameters->getDoubleParameter("Gain Attack Time"),
		actionParameters->getDoubleParameter("Gain Release Time")
	));
}


