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

#include "CRemoveDCAction.h"

#include <math.h>

CRemoveDCAction::CRemoveDCAction(const CActionSound actionSound) :
	AAction(actionSound)
{
}

CRemoveDCAction::~CRemoveDCAction()
{
}

bool CRemoveDCAction::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	const sample_pos_t start=actionSound.start;
	const sample_pos_t stop=actionSound.stop;
	const sample_pos_t selectionLength=actionSound.selectionLength();

	if(prepareForUndo)
		moveSelectionToTempPools(actionSound,mmSelection,actionSound.selectionLength());

	for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
	{
		if(actionSound.doChannel[i])
		{
			const CRezPoolAccesser src=prepareForUndo ? actionSound.sound->getTempAudio(tempAudioPoolKey,i) : actionSound.sound->getAudio(i);
			CRezPoolAccesser dest=actionSound.sound->getAudio(i);

			const sample_pos_t srcStart=prepareForUndo ? 0 : start;
			const sample_pos_t srcStop=prepareForUndo ? selectionLength-1 : stop;

			#define SAMPLES_TO_CHECK (32768*1000)

			// find the DC component (only look at SAMPLE_TO_CHECK samples)
			size_t step=((srcStop-srcStart)+1)/SAMPLES_TO_CHECK;
			if(step<1)
				step=1;
			double avgValue=0;
			size_t count=0;
			for(sample_pos_t t=srcStart;t<=srcStop;t+=step)
			{
				avgValue+=src[t];
				count++;
			}
			avgValue/=(double)count;

			const mix_sample_t DCOffset=(mix_sample_t)round(avgValue);

			BEGIN_PROGRESS_BAR("Remove DC Component -- Channel "+istring(i),start,stop);

			sample_pos_t srcPos=srcStart;
			for(sample_pos_t t=start;t<=stop;t++)
			{
				dest[t]=ClipSample(src[srcPos++]-DCOffset);

				UPDATE_PROGRESS_BAR(t);
			}

			END_PROGRESS_BAR();

			// invalid if we didn't prepare for undo (which created new and invalidated space)
			if(!prepareForUndo)
				actionSound.sound->invalidatePeakData(i,start,stop);
		}
	}

	return(true);
}

AAction::CanUndoResults CRemoveDCAction::canUndo(const CActionSound &actionSound) const
{
	// should check some size constraint
	return(curYes);
}

void CRemoveDCAction::undoActionSizeSafe(const CActionSound &actionSound)
{
	restoreSelectionFromTempPools(actionSound,actionSound.start,actionSound.selectionLength());
}



// ------------------------------

CRemoveDCActionFactory::CRemoveDCActionFactory(AActionDialog *channelSelectDialog) :
	AActionFactory("Remove DC Component","Removes a DC offset from the audio, recentering it around zero",false,channelSelectDialog,NULL,NULL)
{
}

CRemoveDCAction *CRemoveDCActionFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	return(new CRemoveDCAction(actionSound));
}

