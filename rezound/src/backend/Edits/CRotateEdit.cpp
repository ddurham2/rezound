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

#include "CRotateEdit.h"

#include "../CActionParameters.h"

CRotateEdit::CRotateEdit(const AActionFactory *factory,const CActionSound *actionSound,const RotateTypes _rotateType,const double _amount) :
	AAction(factory,actionSound),
	rotateType(_rotateType),

    		// convert seconds to samples
    	amount((sample_pos_t)(_amount*actionSound->sound->getSampleRate()))
{
	if(_amount<0)
		throw(runtime_error(string(__func__)+_(" -- amount is negative")));
}

CRotateEdit::~CRotateEdit()
{
}

bool CRotateEdit::doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo)
{
	rotate(actionSound,rotateType,amount);
	return(true);
}

AAction::CanUndoResults CRotateEdit::canUndo(const CActionSound *actionSound) const
{
	return(curYes);
}

void CRotateEdit::undoActionSizeSafe(const CActionSound *actionSound)
{ 
	// do just as doActionSizeSafe except the other direction
	rotate(actionSound,rotateType==rtLeft ? rtRight : rtLeft,amount);
}


void CRotateEdit::rotate(const CActionSound *actionSound,const RotateTypes rotateType,const sample_pos_t _amount)
{
	const sample_pos_t selectionLength=actionSound->selectionLength();
	const sample_pos_t amount=_amount%selectionLength; // wrap the amount if its too large

	if(rotateType==rtLeft)
		actionSound->sound->rotateLeft(actionSound->doChannel,actionSound->start,actionSound->stop,amount);
	else // if(rotateType==rtRight)
		actionSound->sound->rotateRight(actionSound->doChannel,actionSound->start,actionSound->stop,amount);
}

// ---------------------------------------------


CRotateLeftEditFactory::CRotateLeftEditFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog) :
	AActionFactory(N_("<< Rotate Left"),"",channelSelectDialog,dialog)
{
}

CRotateLeftEditFactory::~CRotateLeftEditFactory()
{
}

CRotateEdit *CRotateLeftEditFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return(new CRotateEdit(
		this,
		actionSound,
		CRotateEdit::rtLeft,
		actionParameters->getDoubleParameter("Amount")
	));
}

// ---------------------------------------------


CRotateRightEditFactory::CRotateRightEditFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog) :
	AActionFactory(N_(">> Rotate Right"),"",channelSelectDialog,dialog)
{
}

CRotateRightEditFactory::~CRotateRightEditFactory()
{
}

CRotateEdit *CRotateRightEditFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return(new CRotateEdit(
		this,
		actionSound,
		CRotateEdit::rtRight,
		actionParameters->getDoubleParameter("Amount")
	));
}
