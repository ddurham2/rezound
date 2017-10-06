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

static const char *gSelectionNames[]=
{
	N_("Select All"),
	N_("Select to Beginning"),
	N_("Select to End"),
	N_("Flip to Beginning"),
	N_("Flip to End"),
	N_("Move Stop to Start Position"),
	N_("Move Start to Stop Position")	
};

static const char *gSelectionDescriptions[]=
{
	"",
	N_("Move Start Position to Beginning"),
	N_("Move Stop Position to End"),
	N_("Move Stop Position to Beginning"),
	N_("Move Start Position to End"),
	N_("Move Stop Position Backward to Start Position"),
	N_("Move Start Position Forward to Stop Position")	
};


CSelectionEdit::CSelectionEdit(const AActionFactory *factory,const CActionSound *actionSound,Selections _selection) :
	AAction(factory,actionSound),
	selection(_selection),
	amount(-1.0),
	selectStart(NIL_SAMPLE_POS),
	selectStop(NIL_SAMPLE_POS)
{
}

CSelectionEdit::CSelectionEdit(const AActionFactory *factory,const CActionSound *actionSound,Selections _selection,const double _amount) :
	AAction(factory,actionSound),
	selection(_selection),
	amount(_amount),
	selectStart(NIL_SAMPLE_POS),
	selectStop(NIL_SAMPLE_POS)
{
}

CSelectionEdit::CSelectionEdit(const AActionFactory *factory,const CActionSound *actionSound,sample_pos_t _selectStart,sample_pos_t _selectStop) :
	AAction(factory,actionSound),
	selection(sSetSelectionPositions),
	amount(-1.0),
	selectStart(_selectStart),
	selectStop(_selectStop)
{
}


CSelectionEdit::~CSelectionEdit()
{
}

AAction::CanUndoResults CSelectionEdit::canUndo(const CActionSound *actionSound) const
{
	return curYes;
}

bool CSelectionEdit::doesWarrantSaving() const
{
	return false;
}

bool CSelectionEdit::doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo)
{
	sample_pos_t i;
	sample_pos_t start=actionSound->start;
	sample_pos_t stop=actionSound->stop;

	// convert amount (in seconds) to a number of samples
	const sample_pos_t samplesAmount=(sample_pos_t)sample_fpos_floor((sample_fpos_t)amount*actionSound->sound->getSampleRate());

	switch(selection)
	{
	case sSelectAll:
		start=0;
		stop=actionSound->sound->getLength()-1;
		break;

	case sSelectToBeginning:
		start=0;
		break;

	case sSelectToEnd:
		stop=actionSound->sound->getLength()-1;
		break;

	case sFlopToBeginning:
		i=start;
		start=0;
		stop=(i - (i>0 ? 1 : 0));
		break;

	case sFlopToEnd:
		i=stop;
		stop=(actionSound->sound->getLength()-1);
		start=(i+ (i<actionSound->sound->getLength() ? 1 : 0));
		break;

	case sSelectToSelectStart:
		stop=start;
		break;

	case sSelectToSelectStop:
		start=stop;
		break;

	// ----------------------------------------

	case sSetSelectionPositions:
		start=selectStart;
		stop=selectStop;
		break;

	// ----------------------------------------

	case sGrowSelectionToTheLeft:
		if(start>samplesAmount)
			start-=samplesAmount;
		else
			start=0;
		break;

	case sGrowSelectionToTheRight:
		if((actionSound->sound->getLength()-stop)>samplesAmount)
			stop+=samplesAmount;
		else
			stop=actionSound->sound->getLength()-1;
		break;

	case sGrowSelectionInBothDirections:
		if(start>samplesAmount)
			start-=samplesAmount;
		else
			start=0;

		if((actionSound->sound->getLength()-stop)>samplesAmount)
			stop+=samplesAmount;
		else
			stop=actionSound->sound->getLength()-1;
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
		if((actionSound->sound->getLength()-stop)>samplesAmount)
		{
			stop+=samplesAmount;
			start+=samplesAmount;
		}
		else
		{
			if((actionSound->sound->getLength()-start)>samplesAmount)
			{
				stop=actionSound->sound->getLength()-1;
				start+=samplesAmount;
			}
			else
				start=stop=actionSound->sound->getLength()-1;
		}
		break;

	default:
		throw runtime_error(string(__func__)+" -- invalid selection type: "+istring(selection));
	}

	actionSound->start=start;
	actionSound->stop=stop;

	return true;
}

void CSelectionEdit::undoActionSizeSafe(const CActionSound *actionSound)
{
	/* the undo logic should take care of this
	actionSound->channel->setStartPosition(oldStart);
	actionSound->channel->setStopPosition(oldStop);
	*/
}




// -----------------------------------

CSelectionEditFactory::CSelectionEditFactory(Selections _selection) :
	AActionFactory(gSelectionNames[_selection],_(gSelectionDescriptions[_selection]),NULL,NULL,false,false),

	selection(_selection)
{
	selectionPositionsAreApplicable=false;
}

CSelectionEditFactory::~CSelectionEditFactory()
{
}

CSelectionEdit *CSelectionEditFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return new CSelectionEdit(this,actionSound,selection);
}



// -----------------------------------

CSelectionEditPositionFactory::CSelectionEditPositionFactory() :
	AActionFactory(N_("Selection Change"),_("Selection Change From Mouse"),NULL,NULL,false,false),

	selectStart(0),
	selectStop(0)
{
	selectionPositionsAreApplicable=false;
}

CSelectionEditPositionFactory::~CSelectionEditPositionFactory()
{
}

CSelectionEdit *CSelectionEditPositionFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return new CSelectionEdit(this,actionSound,selectStart,selectStop);
}


// -----------------------------------

CGrowOrSlideSelectionEditFactory::CGrowOrSlideSelectionEditFactory(AActionDialog *normalDialog) :
	AActionFactory(N_("Grow or Slide Selection"),"",NULL,normalDialog,false,false)
{
	selectionPositionsAreApplicable=false;
}

CGrowOrSlideSelectionEditFactory::~CGrowOrSlideSelectionEditFactory()
{
}

CSelectionEdit *CGrowOrSlideSelectionEditFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	Selections selection;
	switch(actionParameters->getValue<unsigned>("How"))
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
		throw runtime_error(string(__func__)+" -- unhandled How value: "+istring(actionParameters->getValue<unsigned>("How")));
	};
	
	return new CSelectionEdit(this,actionSound,selection,actionParameters->getValue<double>("Amount"));
}



