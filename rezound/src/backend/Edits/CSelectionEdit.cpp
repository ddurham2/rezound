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

#include "CSelectionEdit.h"

#include <stdexcept>

#include <istring>

static const char *gSelectionDescriptions[]=
{
	"Select All",
	"Move Start Position to Beginning",
	"Move Stop Position to End",
	"Move Stop Position to Beginning",
	"Move Start Position to End",
	"Move Stop Position Backward to Start Position",
	"Move Start Position Forward to Stop Position"
};


CSelectionEdit::CSelectionEdit(const CActionSound actionSound,Selections _selection) :
	AAction(actionSound),
	selection(_selection),
	selectStart(NIL_SAMPLE_POS),
	selectStop(NIL_SAMPLE_POS)
{
}

CSelectionEdit::CSelectionEdit(const CActionSound actionSound,sample_pos_t _selectStart,sample_pos_t _selectStop) :
	AAction(actionSound),
	selection((Selections)-1),
	selectStart(_selectStart),
	selectStop(_selectStop)
{
}


CSelectionEdit::~CSelectionEdit()
{
}

AAction::CanUndoResults CSelectionEdit::canUndo(const CActionSound &actionSound) const
{
	return(curYes);
}

bool CSelectionEdit::doesWarrantSaving() const
{
	return false;
}

bool CSelectionEdit::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	sample_pos_t i;
	sample_pos_t start=actionSound.start;
	sample_pos_t stop=actionSound.stop;

	if(selection!=-1)
	{
		switch(selection)
		{
		case sSelectAll:
			start=0;
			stop=actionSound.sound->getLength()-1;
			break;

		case sSelectToBeginning:
			start=0;
			break;

		case sSelectToEnd:
			stop=actionSound.sound->getLength()-1;
			break;

		case sFlopToBeginning:
			i=start;
			start=0;
			stop=(i - (i>0 ? 1 : 0));
			break;

		case sFlopToEnd:
			i=stop;
			stop=(actionSound.sound->getLength()-1);
			start=(i+ (i<actionSound.sound->getLength() ? 1 : 0));
			break;

		case sSelectToSelectStart:
			stop=start;
			break;

		case sSelectToSelectStop:
			start=stop;
			break;

		default:
			throw(runtime_error("select -- invalid selection type: "+istring(selection)));
		}
	}
	else
	{
		start=selectStart;
		stop=selectStop;
	}

	actionSound.start=start;
	actionSound.stop=stop;

	return(true);
}

void CSelectionEdit::undoActionSizeSafe(const CActionSound &actionSound)
{
	/* the undo logic should take care of this
	actionSound.channel->setStartPosition(oldStart);
	actionSound.channel->setStopPosition(oldStop);
	*/
}




// -----------------------------------

CSelectionEditFactory::CSelectionEditFactory(Selections _selection) :
		// ??? set this name from the _selection parameter
	AActionFactory("Selection Change",gSelectionDescriptions[_selection],false,NULL,NULL,NULL,false,false),

	selection(_selection)
{
}

CSelectionEdit *CSelectionEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	return(new CSelectionEdit(actionSound,selection));
}



// -----------------------------------

CSelectionEditPositionFactory::CSelectionEditPositionFactory() :
	AActionFactory("Selection Change","Selection Change From Mouse",false,NULL,NULL,NULL,false,false),

	selectStart(0),
	selectStop(0)
{
}

CSelectionEdit *CSelectionEditPositionFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	return(new CSelectionEdit(actionSound,selectStart,selectStop));
}


