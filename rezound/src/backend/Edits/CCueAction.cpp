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

#include "CCueAction.h"

#include "../CActionParameters.h"



// -----------------------------------
CAddCueAction::CAddCueAction(const AActionFactory *factory,const CActionSound *actionSound,const string _cueName,const sample_pos_t _cueTime,const bool _isAnchored) :
	AAction(factory,actionSound),
	
// ??? maybe change this to an ASound::RCue object
	cueName(_cueName),
	cueTime(_cueTime),
	isAnchored(_isAnchored)
{
}

CAddCueAction::~CAddCueAction()
{
}


bool CAddCueAction::doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo)
{
	actionSound->sound->addCue(cueName,cueTime,isAnchored);
	return true;
}

void CAddCueAction::undoActionSizeSafe(const CActionSound *actionSound)
{
	// it is not necessary to do anything here because AAction handles restoring all the cues
}

AAction::CanUndoResults CAddCueAction::canUndo(const CActionSound *actionSound) const
{
	return curYes;
}


// -----------------------------------

CAddCueActionFactory::CAddCueActionFactory(AActionDialog *dialog) :
	AActionFactory(N_("Add Cue"),"",NULL,dialog,false,false)
{
	selectionPositionsAreApplicable=false;
}

CAddCueActionFactory::~CAddCueActionFactory()
{
}

CAddCueAction *CAddCueActionFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return new CAddCueAction(
		this,
		actionSound,
		actionParameters->getValue<string>("name"),
		actionParameters->getValue<sample_pos_t>("position"),
		actionParameters->getValue<bool>("isAnchored")
	);
}


// ----------------------------------- 

CRemoveCueAction::CRemoveCueAction(const AActionFactory *factory,const CActionSound *actionSound,const size_t cueIndex) :
	AAction(factory,actionSound),
	removeCueIndex(cueIndex)
{
}

CRemoveCueAction::~CRemoveCueAction()
{
}

bool CRemoveCueAction::doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo)
{
	actionSound->sound->removeCue(removeCueIndex);
	return true;
}

void CRemoveCueAction::undoActionSizeSafe(const CActionSound *actionSound)
{
	// it is not necessary to do anything here because AAction handles restoring all the cues
}

AAction::CanUndoResults CRemoveCueAction::canUndo(const CActionSound *actionSound) const
{
	return curYes;
}


// ----------------------------------- 

CRemoveCueActionFactory::CRemoveCueActionFactory() :
	AActionFactory(N_("Remove Cue"),"",NULL,NULL,false,false)
{
	selectionPositionsAreApplicable=false;
}

CRemoveCueActionFactory::~CRemoveCueActionFactory()
{
}

CRemoveCueAction *CRemoveCueActionFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return new CRemoveCueAction(
		this,
		actionSound,
		actionParameters->getValue<unsigned>("index")
	);
}



// -----------------------------------
CReplaceCueAction::CReplaceCueAction(const AActionFactory *factory,const CActionSound *actionSound,const size_t _cueIndex,const string _cueName,const sample_pos_t _cueTime,const bool _isAnchored) :
	AAction(factory,actionSound),
	
	cueIndex(_cueIndex),

// ??? maybe change this to an ASound::RCue object
	cueName(_cueName),
	cueTime(_cueTime),
	isAnchored(_isAnchored)
{
}

CReplaceCueAction::~CReplaceCueAction()
{
}


bool CReplaceCueAction::doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo)
{
	actionSound->sound->removeCue(cueIndex);
	actionSound->sound->insertCue(cueIndex,cueName,cueTime,isAnchored);
	return true;
}

void CReplaceCueAction::undoActionSizeSafe(const CActionSound *actionSound)
{
	// it is not necessary to do anything here because AAction handles restoring all the cues
}

AAction::CanUndoResults CReplaceCueAction::canUndo(const CActionSound *actionSound) const
{
	return curYes;
}


// -----------------------------------

CReplaceCueActionFactory::CReplaceCueActionFactory(AActionDialog *dialog) :
	AActionFactory(N_("Replace Cue"),"",NULL,dialog,false,false)
{
	selectionPositionsAreApplicable=false;
}

CReplaceCueActionFactory::~CReplaceCueActionFactory()
{
}

CReplaceCueAction *CReplaceCueActionFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return new CReplaceCueAction(
		this,
		actionSound,
		actionParameters->getValue<unsigned>("index"),
		actionParameters->getValue<string>("name"),
		actionParameters->getValue<sample_pos_t>("position"),
		actionParameters->getValue<bool>("isAnchored")
	);
}


// -----------------------------------
CMoveCueAction::CMoveCueAction(const AActionFactory *factory,const CActionSound *actionSound,const size_t _cueIndex,const sample_pos_t _cueTime,const sample_pos_t _restoreStartPosition,const sample_pos_t _restoreStopPosition) :
	AAction(factory,actionSound),
	
	cueIndex(_cueIndex),
	cueTime(_cueTime),

	restoreStartPosition(_restoreStartPosition),
	restoreStopPosition(_restoreStopPosition)
{
}

CMoveCueAction::~CMoveCueAction()
{
}


bool CMoveCueAction::doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo)
{
	actionSound->sound->setCueTime(cueIndex,cueTime);
	return true;
}

void CMoveCueAction::undoActionSizeSafe(const CActionSound *actionSound)
{
	// it is not necessary to do anything with cues here because AAction handles restoring all the cues
	
	clearSavedSelectionPositions(); // so it won't restore them after this function is called

	if(restoreStartPosition!=NIL_SAMPLE_POS)
		actionSound->start=restoreStartPosition;

	if(restoreStopPosition!=NIL_SAMPLE_POS)
		actionSound->stop=restoreStopPosition;
}

AAction::CanUndoResults CMoveCueAction::canUndo(const CActionSound *actionSound) const
{
	return curYes;
}


// -----------------------------------

CMoveCueActionFactory::CMoveCueActionFactory() :
	AActionFactory(N_("Move Cue"),"",NULL,NULL,false,false)
{
	selectionPositionsAreApplicable=false;
}

CMoveCueActionFactory::~CMoveCueActionFactory()
{
}

CMoveCueAction *CMoveCueActionFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return new CMoveCueAction(
		this,
		actionSound,
		actionParameters->getValue<unsigned>("index"),
		actionParameters->getValue<sample_pos_t>("position"),
		
		(actionParameters->keyExists("restoreStartPosition") ? 
			actionParameters->getValue<sample_pos_t>("restoreStartPosition") : NIL_SAMPLE_POS),

		(actionParameters->keyExists("restoreStopPosition") ? 
			actionParameters->getValue<sample_pos_t>("restoreStopPosition") : NIL_SAMPLE_POS)
	);
}


