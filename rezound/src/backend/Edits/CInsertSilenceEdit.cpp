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

#include "CInsertSilenceEdit.h"

#include "../CActionParameters.h"

CInsertSilenceEdit::CInsertSilenceEdit(const CActionSound actionSound,const double _silenceLength) :
	AAction(actionSound),
	silenceLength(_silenceLength),
	origLength(0)
{
	if(silenceLength<0)
		throw(runtime_error(string(__func__)+_(" -- silenceLength is less than zero: ")+istring(silenceLength)));
}

CInsertSilenceEdit::~CInsertSilenceEdit()
{
}

bool CInsertSilenceEdit::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	origLength=actionSound.sound->getLength();
	const sample_pos_t sampleCount=(sample_pos_t)(silenceLength*actionSound.sound->getSampleRate());
	if(sampleCount>0)
	{
		if(actionSound.start==actionSound.sound->getLength()-1)
			actionSound.start++; // FIXUP: if the start is at the end, then actually insert AFTER the last sample 
		actionSound.sound->addSpace(actionSound.doChannel,actionSound.start,sampleCount,true);
		actionSound.stop=(actionSound.start+sampleCount)-1;
	}
	else
		actionSound.stop=actionSound.start;

	return(true);
}

AAction::CanUndoResults CInsertSilenceEdit::canUndo(const CActionSound &actionSound) const
{
	return(curYes);
}

void CInsertSilenceEdit::undoActionSizeSafe(const CActionSound &actionSound)
{
	const sample_pos_t sampleCount=(sample_pos_t)(silenceLength*actionSound.sound->getSampleRate());
	actionSound.sound->removeSpace(actionSound.doChannel,actionSound.start,sampleCount,origLength);
}



// ------------------------------

CInsertSilenceEditFactory::CInsertSilenceEditFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog) :
	AActionFactory(N_("Insert Silence"),"",channelSelectDialog,dialog)
{
}

CInsertSilenceEditFactory::~CInsertSilenceEditFactory()
{
}

CInsertSilenceEdit *CInsertSilenceEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return(new CInsertSilenceEdit(
		actionSound,
		actionParameters->getDoubleParameter("Length")
	));
}

