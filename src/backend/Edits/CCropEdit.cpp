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

#include "CCropEdit.h"

CCropEdit::CCropEdit(const AActionFactory *factory,const CActionSound *actionSound) :
    AAction(factory,actionSound),

    oldLength(0)
{
}

CCropEdit::~CCropEdit()
{
}

bool CCropEdit::doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo)
{
	if(prepareForUndo)
		moveSelectionToTempPools(actionSound,mmAllButSelection);
	else
	{
		sample_pos_t selectionLength=actionSound->selectionLength();
		oldLength=actionSound->sound->getLength();

		if(actionSound->start>0)
			actionSound->sound->removeSpace(actionSound->doChannel,0,actionSound->start);
		if(selectionLength<actionSound->sound->getLength())
			actionSound->sound->removeSpace(actionSound->doChannel,selectionLength,actionSound->sound->getLength()-selectionLength);
	}
	
	actionSound->selectAll();
		
	return(true);
}

AAction::CanUndoResults CCropEdit::canUndo(const CActionSound *actionSound) const
{
	// should check some size constraint?
	return(curYes);
}

void CCropEdit::undoActionSizeSafe(const CActionSound *actionSound)
{
	restoreSelectionFromTempPools(actionSound);
}



// ---------------------------------------------

CCropEditFactory::CCropEditFactory(AActionDialog *channelSelectDialog) :
	AActionFactory(N_("Crop"),"",channelSelectDialog,NULL)
{
}

CCropEditFactory::~CCropEditFactory()
{
}

CCropEdit *CCropEditFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return(new CCropEdit(this,actionSound));
}
