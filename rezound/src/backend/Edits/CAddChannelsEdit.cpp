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


CAddChannelsEdit::CAddChannelsEdit(const CActionSound actionSound,unsigned _where,unsigned _count) :
	AAction(actionSound),

	where(_where),
	count(_count)
{
}

CAddChannelsEdit::~CAddChannelsEdit()
{
}

bool CAddChannelsEdit::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	actionSound.sound->addChannels(where,count);
	return(true);
}

AAction::CanUndoResults CAddChannelsEdit::canUndo(const CActionSound &actionSound) const
{
	return(curYes);
}

void CAddChannelsEdit::undoActionSizeSafe(const CActionSound &actionSound)
{
	actionSound.sound->removeChannels(where,count);
}



// ------------------------------

CAddChannelsEditFactory::CAddChannelsEditFactory(AActionDialog *normalDialog) :
	AActionFactory("Add Channels","Add New Channels of Audio",false,NULL,normalDialog,NULL,true,false)
{
}

CAddChannelsEdit *CAddChannelsEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	return(new CAddChannelsEdit(
		actionSound,
		actionParameters->getUnsignedParameter("Insert Where"),
		actionParameters->getUnsignedParameter("Insert Count")
	));
}

#include "../CLoadedSound.h"
#include "../CSoundPlayerChannel.h"
bool CAddChannelsEditFactory::doPreActionSetup(CLoadedSound *loadedSound)
{
	// ??? if there were many more actions that required this, I should have a flag set to the factory that told it to stop the channel if it was playing before doing the action
	loadedSound->channel->stop();
	return(true);
}

