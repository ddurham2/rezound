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

CChangeAmplitudeEffect::CChangeAmplitudeEffect(const CActionSound actionSound,const CGraphParamValueNodeList &_volumeCurve) :
	AAction(actionSound),
	volumeCurve(_volumeCurve)
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
			BEGIN_PROGRESS_BAR("Changing Amplitude -- Channel "+istring(i),0,selectionLength); 

			sample_pos_t srcp=0;
			for(unsigned x=0;x<volumeCurve.size()-1;x++)
			{
				sample_pos_t segmentStartPosition,segmentStopPosition;
				double segmentStartValue,segmentStopValue;
				sample_pos_t segmentLength;

				interpretGraphNodes(volumeCurve,x,selectionLength,segmentStartPosition,segmentStartValue,segmentStopPosition,segmentStopValue,segmentLength);

				segmentStartPosition+=actionSound.start;
				segmentStopPosition+=actionSound.start;

				// ??? I could check if segmentStartValue and segmentStopValue were the same, if so, don't interpolate between them

				// ??? change to use the same implementation, but just get srcAccesser based off of prepare for undo

				// either get the data from the undo pool or get it right from the audio pool if we didn't prepare for undo
				if(prepareForUndo)
				{
					const CRezPoolAccesser src=actionSound.sound->getTempAudio(tempAudioPoolKey,i);
					CRezPoolAccesser dest=actionSound.sound->getAudio(i);
					for(sample_pos_t t=0;t<segmentLength;t++)
					{
						float scalar=(float)(segmentStartValue+(((segmentStopValue-segmentStartValue)*(double)(t))/(segmentLength-1)));
						dest[t+segmentStartPosition]=ClipSample((mix_sample_t)(src[srcp++]*scalar));

						UPDATE_PROGRESS_BAR(srcp);
					}
				}
				else
				{
					CRezPoolAccesser a=actionSound.sound->getAudio(i);
					for(sample_pos_t t=segmentStartPosition;t<=segmentStopPosition;t++)
					{
						float scalar=(float)(segmentStartValue+(((segmentStopValue-segmentStartValue)*(double)(t-segmentStartPosition))/(segmentLength-1)));
						a[t]=ClipSample((mix_sample_t)(a[t]*scalar));

						UPDATE_PROGRESS_BAR(t-start);
					}
					actionSound.sound->invalidatePeakData(i,actionSound.start,actionSound.stop);
				}
			}

			END_PROGRESS_BAR();
		}
	}

	return(true);
}

AAction::CanUndoResults CChangeAmplitudeEffect::canUndo(const CActionSound &actionSound) const
{
	return(curYes);
}

void CChangeAmplitudeEffect::undoActionSizeSafe(const CActionSound &actionSound)
{
	restoreSelectionFromTempPools(actionSound,actionSound.start,actionSound.selectionLength());
}





// ---------------------------------------------

CChangeAmplitudeEffectFactory::CChangeAmplitudeEffectFactory(AActionDialog *channelSelectDialog,AActionDialog *normalDialog,AActionDialog *advancedDialog) :
	AActionFactory("Change Amplitude","Change Amplitude",true,channelSelectDialog,normalDialog,advancedDialog)
{
}

CChangeAmplitudeEffect *CChangeAmplitudeEffectFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	if(actionParameters->getGraphParameter(0).size()<2)
		throw(runtime_error(string(__func__)+" -- nodes contains less than 2 nodes"));

	return(new CChangeAmplitudeEffect(actionSound,actionParameters->getGraphParameter(0)));
}


