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

#include "CAddChannelsEdit.h"

#include "../CActionSound.h"
#include "../CActionParameters.h"


CAddChannelsEdit::CAddChannelsEdit(const AActionFactory *factory,const CActionSound *actionSound,unsigned _where,unsigned _count) :
	AAction(factory,actionSound),

	where(_where),
	count(_count)
{
}

CAddChannelsEdit::~CAddChannelsEdit()
{
}

bool CAddChannelsEdit::doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo)
{
	actionSound->sound->addChannels(where,count,true);
	return true;
}

AAction::CanUndoResults CAddChannelsEdit::canUndo(const CActionSound *actionSound) const
{
	return curYes;
}

void CAddChannelsEdit::undoActionSizeSafe(const CActionSound *actionSound)
{
	actionSound->sound->removeChannels(where,count);
}



// ------------------------------

CAddChannelsEditFactory::CAddChannelsEditFactory(AActionDialog *dialog) :
	AActionFactory(N_("Add Channels"),_("Add New Channels of Audio"),NULL,dialog,true,false)
{
	selectionPositionsAreApplicable=false;
}

CAddChannelsEditFactory::~CAddChannelsEditFactory()
{
}

CAddChannelsEdit *CAddChannelsEditFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return new CAddChannelsEdit(
		this,
		actionSound,
		actionParameters->getValue<unsigned>("Insert Where"),
		actionParameters->getValue<unsigned>("Insert Count")
	);
}

