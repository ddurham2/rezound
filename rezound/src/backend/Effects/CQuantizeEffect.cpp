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

#include "CQuantizeEffect.h"

#include <stdexcept>

#include <istring>

#include "../CActionParameters.h"
#include "../CActionSound.h"

#include "../DSP/Quantizer.h"


CQuantizeEffect::CQuantizeEffect(const CActionSound &actionSound,unsigned _quantumCount,float _inputGain,float _outputGain) :
	AAction(actionSound),

	quantumCount(_quantumCount),
	inputGain(_inputGain),
	outputGain(_outputGain)
{
	// ??? here I could warn about the quantumCount being greater than the number of discrete values that sample_t can hold
}

CQuantizeEffect::~CQuantizeEffect()
{
}

bool CQuantizeEffect::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	const sample_pos_t start=actionSound.start;
	const sample_pos_t stop=actionSound.stop;

	if(prepareForUndo)
		moveSelectionToTempPools(actionSound,mmSelection,actionSound.selectionLength());

	for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
	{
		if(actionSound.doChannel[i])
		{
			CStatusBar statusBar("Quantize -- Channel "+istring(i),start,stop,true); 

			CRezPoolAccesser dest=actionSound.sound->getAudio(i);
			const CRezPoolAccesser src=prepareForUndo ? actionSound.sound->getTempAudio(tempAudioPoolKey,i) : actionSound.sound->getAudio(i);

			TDSPQuantizer<mix_sample_t,MAX_SAMPLE> quantizer(quantumCount);

			sample_pos_t srcP=prepareForUndo ? 0 : start;
			for(sample_pos_t t=start;t<=stop;t++)
			{
				dest[t]=ClipSample(quantizer.processSample((mix_sample_t)(inputGain*src[srcP++]))*outputGain);
				
				if(statusBar.update(t))
				{ // cancelled
					if(prepareForUndo)
						undoActionSizeSafe(actionSound);
					else
						actionSound.sound->invalidatePeakData(i,actionSound.start,t);
					return false;
				}
			}

			if(!prepareForUndo)
				actionSound.sound->invalidatePeakData(i,actionSound.start,actionSound.stop);
		}
	}

	return(true);
}

AAction::CanUndoResults CQuantizeEffect::canUndo(const CActionSound &actionSound) const
{
	return(curYes);
}

void CQuantizeEffect::undoActionSizeSafe(const CActionSound &actionSound)
{
	restoreSelectionFromTempPools(actionSound,actionSound.start,actionSound.selectionLength());
}



// ---------------------------------------------

CQuantizeEffectFactory::CQuantizeEffectFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog) :
	AActionFactory("Quantize","Quantize the Number of Possible Sample Levels",channelSelectDialog,dialog)
{
}

CQuantizeEffectFactory::~CQuantizeEffectFactory()
{
}

CQuantizeEffect *CQuantizeEffectFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return(new CQuantizeEffect(
		actionSound,
		actionParameters->getUnsignedParameter("Quantum Count"),
		actionParameters->getDoubleParameter("Input Gain"),
		actionParameters->getDoubleParameter("Output Gain")
	));
}

