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

#include "../CActionParameters.h"

#include <stdexcept>

#include <istring>

static const char *gSelectionNames[]=
{
	"Select All",
	"Select to Beginning",
	"Select to End",
	"Flip to Beginning",
	"Flip to End",
	"Move Stop to Start Position",
	"Move Start to Stop Position"
};

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
	amount(-1.0),
	selectStart(NIL_SAMPLE_POS),
	selectStop(NIL_SAMPLE_POS)
{
}

CSelectionEdit::CSelectionEdit(const CActionSound actionSound,Selections _selection,const double _amount) :
	AAction(actionSound),
	selection(_selection),
	amount(_amount),
	selectStart(NIL_SAMPLE_POS),
	selectStop(NIL_SAMPLE_POS)
{
}

CSelectionEdit::CSelectionEdit(const CActionSound actionSound,sample_pos_t _selectStart,sample_pos_t _selectStop) :
	AAction(actionSound),
	selection((Selections)-1),
	amount(-1.0),
	selectStart(_selectStart),
	selectStop(_selectStop)
{
}


CSelectionEdit::~CSelectionEdit()
{
}

AAction::CanUndoResults CSelectionEdit::canUndo(const CActionSound &actionSound) const
{
	return curYes;
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

	// convert amount (in seconds) to a number of samples
	const sample_pos_t samplesAmount=(sample_pos_t)sample_fpos_floor((sample_fpos_t)amount*actionSound.sound->getSampleRate());

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

		// ----------------------------------------

		case sGrowSelectionToTheLeft:
			if(start>samplesAmount)
				start-=samplesAmount;
			else
				start=0;
			break;

		case sGrowSelectionToTheRight:
			if((actionSound.sound->getLength()-stop)>samplesAmount)
				stop+=samplesAmount;
			else
				stop=actionSound.sound->getLength()-1;
			break;

		case sGrowSelectionInBothDirections:
			if(start>samplesAmount)
				start-=samplesAmount;
			else
				start=0;

			if((actionSound.sound->getLength()-stop)>samplesAmount)
				stop+=samplesAmount;
			else
				stop=actionSound.sound->getLength()-1;
			break;

		case sSlideSelectionToTheLeft:
			if(start>samplesAmount)
			{
				start-=samplesAmount;
				stop-=samplesAmount;
			}
			else
			{
				if(stop>samplesAmount)
				{
					start=0;
					stop-=samplesAmount;
				}
				else
					start=stop=0;
			}
			break;

		case sSlideSelectionToTheRight:
			if((actionSound.sound->getLength()-stop)>samplesAmount)
			{
				stop+=samplesAmount;
				start+=samplesAmount;
			}
			else
			{
				if((actionSound.sound->getLength()-start)>samplesAmount)
				{
					stop=actionSound.sound->getLength()-1;
					start+=samplesAmount;
				}
				else
					start=stop=actionSound.sound->getLength()-1;
			}
			break;

		default:
			throw runtime_error("select -- invalid selection type: "+istring(selection));
		}
	}
	else
	{
		start=selectStart;
		stop=selectStop;
	}

	actionSound.start=start;
	actionSound.stop=stop;

	return true;
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
	AActionFactory(gSelectionNames[_selection],gSelectionDescriptions[_selection],NULL,NULL,false,false),

	selection(_selection)
{
}

CSelectionEditFactory::~CSelectionEditFactory()
{
}

CSelectionEdit *CSelectionEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return new CSelectionEdit(actionSound,selection);
}



// -----------------------------------

CSelectionEditPositionFactory::CSelectionEditPositionFactory() :
	AActionFactory("Selection Change","Selection Change From Mouse",NULL,NULL,false,false),

	selectStart(0),
	selectStop(0)
{
}

CSelectionEditPositionFactory::~CSelectionEditPositionFactory()
{
}

CSelectionEdit *CSelectionEditPositionFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return new CSelectionEdit(actionSound,selectStart,selectStop);
}


// -----------------------------------

CGrowOrSlideSelectionEditFactory::CGrowOrSlideSelectionEditFactory(AActionDialog *normalDialog) :
	AActionFactory("Grow or Slide Selection","Grow or Slide Selection",NULL,normalDialog,false,false)
{
}

CGrowOrSlideSelectionEditFactory::~CGrowOrSlideSelectionEditFactory()
{
}

CSelectionEdit *CGrowOrSlideSelectionEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	Selections selection;
	switch(actionParameters->getUnsignedParameter("How"))
	{
	case 0:
		selection=sGrowSelectionToTheLeft;
		break;
	case 1:
		selection=sGrowSelectionToTheRight;
		break;
	case 2:
		selection=sGrowSelectionInBothDirections;
		break;
	case 3:
		selection=sSlideSelectionToTheLeft;
		break;
	case 4:
		selection=sSlideSelectionToTheRight;
		break;
	default:
		throw runtime_error("unhandled How value: "+istring(actionParameters->getUnsignedParameter("How")));
	};
	
	return new CSelectionEdit(actionSound,selection,actionParameters->getDoubleParameter("Amount"));
}



