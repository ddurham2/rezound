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
CAddCueAction::CAddCueAction(const CActionSound actionSound,const string _cueName,const sample_pos_t _cueTime,const bool _isAnchored) :
	AAction(actionSound),
	
// ??? maybe change this to an ASound::RCue object
	cueName(_cueName),
	cueTime(_cueTime),
	isAnchored(_isAnchored)
{
}

CAddCueAction::~CAddCueAction()
{
}


bool CAddCueAction::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	actionSound.sound->addCue(cueName,cueTime,isAnchored);
	return true;
}

void CAddCueAction::undoActionSizeSafe(const CActionSound &actionSound)
{
	// it is not necessary to do anything here because AAction handles restoring all the cues
}

AAction::CanUndoResults CAddCueAction::canUndo(const CActionSound &actionSound) const
{
	return curYes;
}


// -----------------------------------

CAddCueActionFactory::CAddCueActionFactory(AActionDialog *dialog) :
	AActionFactory(N_("Add Cue"),"",NULL,dialog,false,false)
{
}

CAddCueActionFactory::~CAddCueActionFactory()
{
}

CAddCueAction *CAddCueActionFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return new CAddCueAction(
		actionSound,
		actionParameters->getStringParameter("name"),
		actionParameters->getSamplePosParameter("position"),
		actionParameters->getBoolParameter("isAnchored")
	);
}


// ----------------------------------- 

CRemoveCueAction::CRemoveCueAction(const CActionSound actionSound,const size_t cueIndex) :
	AAction(actionSound),
	removeCueIndex(cueIndex)
{
}

CRemoveCueAction::~CRemoveCueAction()
{
}

bool CRemoveCueAction::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	actionSound.sound->removeCue(removeCueIndex);
	return true;
}

void CRemoveCueAction::undoActionSizeSafe(const CActionSound &actionSound)
{
	// it is not necessary to do anything here because AAction handles restoring all the cues
}

AAction::CanUndoResults CRemoveCueAction::canUndo(const CActionSound &actionSound) const
{
	return curYes;
}


// ----------------------------------- 

CRemoveCueActionFactory::CRemoveCueActionFactory() :
	AActionFactory(N_("Remove Cue"),"",NULL,NULL,false,false)
{
}

CRemoveCueActionFactory::~CRemoveCueActionFactory()
{
}

CRemoveCueAction *CRemoveCueActionFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return new CRemoveCueAction(
		actionSound,
		actionParameters->getUnsignedParameter("index")
	);
}



// -----------------------------------
CReplaceCueAction::CReplaceCueAction(const CActionSound actionSound,const size_t _cueIndex,const string _cueName,const sample_pos_t _cueTime,const bool _isAnchored) :
	AAction(actionSound),
	
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


bool CReplaceCueAction::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	actionSound.sound->removeCue(cueIndex);
	actionSound.sound->insertCue(cueIndex,cueName,cueTime,isAnchored);
	return true;
}

void CReplaceCueAction::undoActionSizeSafe(const CActionSound &actionSound)
{
	// it is not necessary to do anything here because AAction handles restoring all the cues
}

AAction::CanUndoResults CReplaceCueAction::canUndo(const CActionSound &actionSound) const
{
	return curYes;
}


// -----------------------------------

CReplaceCueActionFactory::CReplaceCueActionFactory(AActionDialog *dialog) :
	AActionFactory(N_("Replace Cue"),"",NULL,dialog,false,false)
{
}

CReplaceCueActionFactory::~CReplaceCueActionFactory()
{
}

CReplaceCueAction *CReplaceCueActionFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return new CReplaceCueAction(
		actionSound,
		actionParameters->getUnsignedParameter("index"),
		actionParameters->getStringParameter("name"),
		actionParameters->getSamplePosParameter("position"),
		actionParameters->getBoolParameter("isAnchored")
	);
}


// -----------------------------------
CMoveCueAction::CMoveCueAction(const CActionSound actionSound,const size_t _cueIndex,const sample_pos_t _cueTime) :
	AAction(actionSound),
	
	cueIndex(_cueIndex),
	cueTime(_cueTime)
{
}

CMoveCueAction::~CMoveCueAction()
{
}


bool CMoveCueAction::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	actionSound.sound->setCueTime(cueIndex,cueTime);
	return true;
}

void CMoveCueAction::undoActionSizeSafe(const CActionSound &actionSound)
{
	// it is not necessary to do anything here because AAction handles restoring all the cues
}

AAction::CanUndoResults CMoveCueAction::canUndo(const CActionSound &actionSound) const
{
	return curYes;
}


// -----------------------------------

CMoveCueActionFactory::CMoveCueActionFactory() :
	AActionFactory(N_("Move Cue"),"",NULL,NULL,false,false)
{
}

CMoveCueActionFactory::~CMoveCueActionFactory()
{
}

CMoveCueAction *CMoveCueActionFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return new CMoveCueAction(
		actionSound,
		actionParameters->getUnsignedParameter("index"),
		actionParameters->getSamplePosParameter("position")
	);
}


