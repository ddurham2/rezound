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

#include "../ASoundClipboard.h"

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
	if(actionSound.countChannels()<=0)
		return(false); // no channels to do

	// ??? cut could be optimized if the clipboard were abstracted in such a way that I could use moveData to move to a pool in the same pool file as the sound
	if(type==ccdtCopy || type==ccdtCut)
	{
		const sample_pos_t start=actionSound.start;
		const sample_pos_t selectionLength=actionSound.selectionLength();

		if(AAction::clipboards[gWhichClipboard]->isReadOnly())
			throw(EUserMessage((_("cannot copy/cut to clipboard: ")+AAction::clipboards[gWhichClipboard]->getDescription()).c_str()));

		AAction::clipboards[gWhichClipboard]->copyFrom(actionSound.sound,actionSound.doChannel,start,selectionLength);
	}
	
	if(type==ccdtCut || type==ccdtDelete)
	{
		if(prepareForUndo)
			moveSelectionToTempPools(actionSound,mmSelection);
		else
			actionSound.sound->removeSpace(actionSound.doChannel,actionSound.start,actionSound.selectionLength());

		actionSound.stop=actionSound.start=actionSound.start-1;
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
	AActionFactory(N_("Copy"),"",channelSelectDialog,NULL,false,false)
{
}

CCopyEditFactory::~CCopyEditFactory()
{
}

CCopyCutDeleteEdit *CCopyEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return(new CCopyCutDeleteEdit(actionSound,CCopyCutDeleteEdit::ccdtCopy));
}



// -----------------------------------


CCutEditFactory::CCutEditFactory(AActionDialog *channelSelectDialog) :
	AActionFactory(N_("Cut"),"",channelSelectDialog,NULL)
{
}

CCutEditFactory::~CCutEditFactory()
{
}

CCopyCutDeleteEdit *CCutEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return(new CCopyCutDeleteEdit(actionSound,CCopyCutDeleteEdit::ccdtCut));
}



// -----------------------------------


CDeleteEditFactory::CDeleteEditFactory(AActionDialog *channelSelectDialog) :
	AActionFactory(N_("Delete"),"",channelSelectDialog,NULL)
{
}

CDeleteEditFactory::~CDeleteEditFactory()
{
}

CCopyCutDeleteEdit *CDeleteEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return(new CCopyCutDeleteEdit(actionSound,CCopyCutDeleteEdit::ccdtDelete));
}


