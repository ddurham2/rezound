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

#include "CCopyCutDeleteEdit.h"

#include <istring>

CCopyCutDeleteEdit::CCopyCutDeleteEdit(const CActionSound actionSound,CCDType _type) :
    AAction(actionSound),

    type(_type)
{
}

CCopyCutDeleteEdit::~CCopyCutDeleteEdit()
{
}

bool CCopyCutDeleteEdit::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	// ??? cut could be optimized if the clipboard were abstracted in such a way that I could use moveData to move to a pool in the same pool file as the sound
	if(type==ccdtCopy || type==ccdtCut)
	{
		const sample_pos_t start=actionSound.start;
		const sample_pos_t selectionLength=actionSound.selectionLength();

		clearClipboard();

		for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
		{
			if(actionSound.doChannel[i])
			{
				const string poolName="Clipboard Channel "+istring(i);
				CRezClipboardPoolAccesser dest=clipboardPoolFile->createPool<sample_t>(poolName);
				// ??? progress bar.. either call this in 100 chunks or have a call back function
				dest.copyData(0,actionSound.sound->getAudio(i),start,selectionLength,true);
			}
		}
	}
	
	if(type==ccdtCut || type==ccdtDelete)
	{
		if(prepareForUndo)
			moveSelectionToTempPools(actionSound,mmSelection);
		else
			actionSound.sound->removeSpace(actionSound.doChannel,actionSound.start,actionSound.selectionLength());

		actionSound.stop=actionSound.start;
	}

	return(true);
}

AAction::CanUndoResults CCopyCutDeleteEdit::canUndo(const CActionSound &actionSound) const
{
	if(type==ccdtCut || type==ccdtDelete)
	{
		// should check some size constraint?
		return(curYes);
	}
	else
		return(curNA);
}

void CCopyCutDeleteEdit::undoActionSizeSafe(const CActionSound &actionSound)
{
	restoreSelectionFromTempPools(actionSound);
}

bool CCopyCutDeleteEdit::doesWarrantSaving() const
{
	return type!=ccdtCopy;
}


// ---------------------------------------------

CCopyEditFactory::CCopyEditFactory(AActionDialog *channelSelectDialog) :
	AActionFactory("Copy","Copy",false,channelSelectDialog,NULL,NULL,false)
{
}

CCopyCutDeleteEdit *CCopyEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	return(new CCopyCutDeleteEdit(actionSound,CCopyCutDeleteEdit::ccdtCopy));
}



// -----------------------------------


CCutEditFactory::CCutEditFactory(AActionDialog *channelSelectDialog) :
	AActionFactory("Cut","Cut",false,channelSelectDialog,NULL,NULL)
{
}

CCopyCutDeleteEdit *CCutEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	return(new CCopyCutDeleteEdit(actionSound,CCopyCutDeleteEdit::ccdtCut));
}



// -----------------------------------


CDeleteEditFactory::CDeleteEditFactory(AActionDialog *channelSelectDialog) :
	AActionFactory("Delete","Delete",false,channelSelectDialog,NULL,NULL)
{
}

CCopyCutDeleteEdit *CDeleteEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	return(new CCopyCutDeleteEdit(actionSound,CCopyCutDeleteEdit::ccdtDelete));
}


