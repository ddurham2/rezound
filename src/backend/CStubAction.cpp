/* 
 * Copyright (C) 2005 - David W. Durham
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

#include "CStubAction.h"

CStubAction::CStubAction(const AActionFactory *factory,const CActionSound *actionSound) :
	AAction(factory,actionSound)
{
}

CStubAction::~CStubAction()
{
}

bool CStubAction::doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo)
{
	const sample_pos_t start=actionSound->start;
	const sample_pos_t stop=actionSound->stop;
	const sample_pos_t selectionLength=actionSound->selectionLength();

	if(prepareForUndo)
		moveSelectionToTempPools(actionSound,mmSelection,actionSound->selectionLength());

	unsigned channelsDoneCount=0;
	for(unsigned i=0;i<actionSound->sound->getChannelCount();i++)
	{
		if(actionSound->doChannel[i])
		{
			CStatusBar statusBar(_("Stub Action -- Channel ")+istring(++channelsDoneCount)+"/"+istring(actionSound->countChannels()),start,stop,true);

			sample_pos_t srcPos=prepareForUndo ? 0 : start;
			const CRezPoolAccesser src=prepareForUndo ? actionSound->sound->getTempAudio(tempAudioPoolKey,i) : actionSound->sound->getAudio(i);
			// now 'src' is an accessor either directly into the sound or into the temp pool created for undo
			// so it's range of indexes is either [start,stop] or [0,selectionLength) respectively

			sample_pos_t destPos=start;
			CRezPoolAccesser dest=actionSound->sound->getAudio(i);
			


// --- Insert your test effect here -- BEGIN --------------------------------------------
			while(destPos<=stop)
			{

				if(statusBar.update(destPos))
				{ // cancelled
					if(prepareForUndo)
						undoActionSizeSafe(actionSound);
					else
						actionSound->sound->invalidatePeakData(i,actionSound->start,destPos);
					return false;
				}

				destPos++;
			}
// --- Insert your test effect here -- END ----------------------------------------------


			if(!prepareForUndo)
				actionSound->sound->invalidatePeakData(i,actionSound->start,actionSound->stop);
		}
	}

	// set the new selection points (only necessary if the length of the sound has changed)
	//actionSound->stop=actionSound->start+selectionLength-1;

	return true;
}

AAction::CanUndoResults CStubAction::canUndo(const CActionSound *actionSound) const
{
	return curYes;
}

void CStubAction::undoActionSizeSafe(const CActionSound *actionSound)
{
	restoreSelectionFromTempPools(actionSound,actionSound->start,actionSound->selectionLength());
}


// --------------------------------------------------

CStubActionFactory::CStubActionFactory(AActionDialog *channelSelectDialog) :
	AActionFactory(N_("Stub"),"",channelSelectDialog,NULL)
{
}

CStubActionFactory::~CStubActionFactory()
{
}

CStubAction *CStubActionFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return new CStubAction(this,actionSound);
}


