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

#include "CMuteEdit.h"

CMuteEdit::CMuteEdit(const AActionFactory *factory,const CActionSound *actionSound) :
	AAction(factory,actionSound)
{
}

CMuteEdit::~CMuteEdit()
{
}

bool CMuteEdit::doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo)
{
	if(prepareForUndo)
		moveSelectionToTempPools(actionSound,mmSelection,actionSound->selectionLength());

	for(unsigned i=0;i<actionSound->sound->getChannelCount();i++)
	{
		if(actionSound->doChannel[i])
			actionSound->sound->silenceSound(i,actionSound->start,actionSound->selectionLength(),true,true);
	}

	return(true);
}

AAction::CanUndoResults CMuteEdit::canUndo(const CActionSound *actionSound) const
{
	// should check some size constraint
	return(curYes);
}

void CMuteEdit::undoActionSizeSafe(const CActionSound *actionSound)
{
	restoreSelectionFromTempPools(actionSound,actionSound->start,actionSound->selectionLength());
}



// ------------------------------

CMuteEditFactory::CMuteEditFactory(AActionDialog *channelSelectDialog) :
	AActionFactory(N_("Mute"),"",channelSelectDialog,NULL)
{
}

CMuteEditFactory::~CMuteEditFactory()
{
}

CMuteEdit *CMuteEditFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return(new CMuteEdit(this,actionSound));
}

