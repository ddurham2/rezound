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

#include "CSwapChannelsEdit.h"

#include "../CLoadedSound.h"
#include "../CActionSound.h"
#include "../CActionParameters.h"

CSwapChannelsEdit::CSwapChannelsEdit(const CActionSound actionSound,unsigned _channelA,unsigned _channelB) :
	AAction(actionSound),

	channelA(_channelA),
	channelB(_channelB)
{
}

CSwapChannelsEdit::~CSwapChannelsEdit()
{
}

bool CSwapChannelsEdit::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	const sample_pos_t start=actionSound.start;
	const sample_pos_t stop=actionSound.stop;
	const sample_pos_t selectionLength=actionSound.selectionLength();

	actionSound.sound->swapChannels(channelA,channelB,start,selectionLength);
	return(true);
}

AAction::CanUndoResults CSwapChannelsEdit::canUndo(const CActionSound &actionSound) const
{
	return(curYes);
}

void CSwapChannelsEdit::undoActionSizeSafe(const CActionSound &actionSound)
{
	// undo is same as doing it in this case
	CActionSound a(actionSound);
	doActionSizeSafe(a,false);
}



// ------------------------------

CSwapChannelsEditFactory::CSwapChannelsEditFactory(AActionDialog *normalDialog) :
	AActionFactory("Swap Channels","Swap the Audio Between Two Channels",false,NULL,normalDialog,NULL)
{
}

CSwapChannelsEditFactory::~CSwapChannelsEditFactory()
{
}

CSwapChannelsEdit *CSwapChannelsEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	return(new CSwapChannelsEdit(
		actionSound,
		actionParameters->getUnsignedParameter("Channel A"),
		actionParameters->getUnsignedParameter("Channel B")
	));
}

bool CSwapChannelsEditFactory::doPreActionSetup(CLoadedSound *loadedSound)
{
	// don't bother doing this action if there is not more than 1 channel
	return(loadedSound->sound->getChannelCount()>1);
}

