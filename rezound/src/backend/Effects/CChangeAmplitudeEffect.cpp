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

#include "CChangeAmplitudeEffect.h"

#include <stdexcept>

#include "../CActionSound.h"
#include "../CActionParameters.h"

CChangeAmplitudeEffect::CChangeAmplitudeEffect(const CActionSound &actionSound,const CGraphParamValueNodeList &_volumeCurve) :
	AAction(actionSound),
	volumeCurve(_volumeCurve)
{
}

CChangeAmplitudeEffect::~CChangeAmplitudeEffect()
{
}

bool CChangeAmplitudeEffect::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	if(prepareForUndo)
		moveSelectionToTempPools(actionSound,mmSelection,actionSound.selectionLength());

	const sample_pos_t start=actionSound.start;
	const sample_pos_t selectionLength=actionSound.selectionLength();

	for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
	{
		if(actionSound.doChannel[i])
		{
			CStatusBar statusBar("Changing Amplitude -- Channel "+istring(i),0,selectionLength,true);

			sample_pos_t srcPos=prepareForUndo ? 0 : start;
			const CRezPoolAccesser src=prepareForUndo ? actionSound.sound->getTempAudio(tempAudioPoolKey,i) : actionSound.sound->getAudio(i);

			sample_pos_t destPos=start;
			CRezPoolAccesser dest=actionSound.sound->getAudio(i);
		
			CGraphParamValueIterator iter(volumeCurve,selectionLength);
			for(sample_pos_t t=0;t<selectionLength;t++)
			{
				dest[destPos++]=ClipSample(src[srcPos++]*iter.next());

				if(statusBar.update(t))
				{ // cancelled
					if(prepareForUndo)
						undoActionSizeSafe(actionSound);
					else
						actionSound.sound->invalidatePeakData(i,actionSound.start,t);
					return false;
				}
			}
		}
	}

	return true;
}

AAction::CanUndoResults CChangeAmplitudeEffect::canUndo(const CActionSound &actionSound) const
{
	return curYes;
}

void CChangeAmplitudeEffect::undoActionSizeSafe(const CActionSound &actionSound)
{
	restoreSelectionFromTempPools(actionSound,actionSound.start,actionSound.selectionLength());
}





// ---------------------------------------------

CChangeVolumeEffectFactory::CChangeVolumeEffectFactory(AActionDialog *channelSelectDialog,AActionDialog *normalDialog) :
	AActionFactory("Change Volume","Change Volume",false,channelSelectDialog,normalDialog,NULL)
{
}

CChangeVolumeEffectFactory::~CChangeVolumeEffectFactory()
{
}

CChangeAmplitudeEffect *CChangeVolumeEffectFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	if(actionParameters->getGraphParameter("Volume Change").size()<2)
		throw runtime_error(string(__func__)+" -- graph parameter 0 contains less than 2 nodes");

	return new CChangeAmplitudeEffect(actionSound,actionParameters->getGraphParameter("Volume Change"));
}


// ---------------------------------------------

CGainEffectFactory::CGainEffectFactory(AActionDialog *channelSelectDialog,AActionDialog *normalDialog,AActionDialog *advancedDialog) :
	AActionFactory("Gain","Gain",true,channelSelectDialog,normalDialog,advancedDialog)
{
}

CGainEffectFactory::~CGainEffectFactory()
{
}

CChangeAmplitudeEffect *CGainEffectFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	string parameterName;
	if(actionParameters->containsParameter("Gain")) // it's just that the frontend uses two different names for the same parameter because the dialog is different
		parameterName="Gain";
	else
		parameterName="Gain Curve";

	if(actionParameters->getGraphParameter(parameterName).size()<2)
		throw runtime_error(string(__func__)+" -- graph parameter 0 contains less than 2 nodes");

	return new CChangeAmplitudeEffect(actionSound,actionParameters->getGraphParameter(parameterName));
}


