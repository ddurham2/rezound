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

#include "CMakeSymetricAction.h"

CMakeSymetricAction::CMakeSymetricAction(const CActionSound &actionSound) :
	AAction(actionSound)
{
}

CMakeSymetricAction::~CMakeSymetricAction()
{
}

bool CMakeSymetricAction::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
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
			CRezPoolAccesser dest1=actionSound.sound->getAudio(i);
			CRezPoolAccesser dest2=actionSound.sound->getAudio(i);

			const CRezPoolAccesser a=prepareForUndo ? actionSound.sound->getTempAudio(tempAudioPoolKey,i) : actionSound.sound->getAudio(i);
			const CRezPoolAccesser b=prepareForUndo ? actionSound.sound->getTempAudio(tempAudioPoolKey,i) : actionSound.sound->getAudio(i);
			const sample_pos_t offset=prepareForUndo ? 0 : start;

			const sample_pos_t selectionLengthDiv2=selectionLength/2;
			const sample_pos_t selectionLengthSub1=selectionLength-1;

			CStatusBar statusBar("Make Symetric -- Channel "+istring(i),0,selectionLengthDiv2,true);

			for(sample_pos_t t=0;t<=selectionLengthDiv2;t++)
			{
				const mix_sample_t as=a[t+offset];
				const mix_sample_t bs=b[selectionLengthSub1-t+offset];
				const sample_t s=(as+bs)/2;

				dest1[t+start]=dest2[selectionLengthSub1-t+start]=s;

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

AAction::CanUndoResults CMakeSymetricAction::canUndo(const CActionSound &actionSound) const
{
	return(curYes);
}

void CMakeSymetricAction::undoActionSizeSafe(const CActionSound &actionSound)
{
	restoreSelectionFromTempPools(actionSound,actionSound.start,actionSound.selectionLength());
}


// --------------------------------------------------
//
CMakeSymetricActionFactory::CMakeSymetricActionFactory(AActionDialog *channelSelectDialog) :
	AActionFactory("Make Symetric","Make Symetric For Noise Loops",false,channelSelectDialog,NULL,NULL)
{
}

CMakeSymetricActionFactory::~CMakeSymetricActionFactory()
{
}

CMakeSymetricAction *CMakeSymetricActionFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	return(new CMakeSymetricAction(actionSound));
}


