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

#include "CRemoveChannelsEdit.h"

#include "../CActionSound.h"
#include "../CActionParameters.h"


CRemoveChannelsEdit::CRemoveChannelsEdit(const CActionSound actionSound) :
	AAction(actionSound),
	tempAudioPoolKey(-1),
	sound(NULL)
{
}

CRemoveChannelsEdit::~CRemoveChannelsEdit()
{
	if(tempAudioPoolKey!=-1)
		sound->removeTempAudioPools(tempAudioPoolKey);
}

bool CRemoveChannelsEdit::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	if(prepareForUndo)
	{
		tempAudioPoolKey=actionSound.sound->moveChannelsToTemp(actionSound.doChannel);
		sound=actionSound.sound;
	}
	else
	{
		for(int t=actionSound.sound->getChannelCount()-1;t>=0;t--)
		{
			if(actionSound.doChannel[t])
				actionSound.sound->removeChannels((unsigned)t,1);
		}
	}
	

	return true;
}

AAction::CanUndoResults CRemoveChannelsEdit::canUndo(const CActionSound &actionSound) const
{
	return curYes;
}

void CRemoveChannelsEdit::undoActionSizeSafe(const CActionSound &actionSound)
{
	actionSound.sound->moveChannelsFromTemp(tempAudioPoolKey,actionSound.doChannel);
	tempAudioPoolKey=-1;
}



// ------------------------------

CRemoveChannelsEditFactory::CRemoveChannelsEditFactory(AActionDialog *dialog) :
	AActionFactory(N_("Remove Channels"),_("Remove Channels of Audio"),NULL,dialog,true,false)
{
}

CRemoveChannelsEditFactory::~CRemoveChannelsEditFactory()
{
}

CRemoveChannelsEdit *CRemoveChannelsEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return new CRemoveChannelsEdit(actionSound);
}

