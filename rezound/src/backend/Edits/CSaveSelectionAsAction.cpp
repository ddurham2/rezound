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

#include "CSaveSelectionAsAction.h"

#include "../ASoundFileManager.h"
#include "../CActionParameters.h"

CSaveSelectionAsAction::CSaveSelectionAsAction(const CActionSound actionSound,ASoundFileManager *_soundFileManager) :
	AAction(actionSound),
	soundFileManager(_soundFileManager)
{
}

CSaveSelectionAsAction::~CSaveSelectionAsAction()
{
}

bool CSaveSelectionAsAction::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	soundFileManager->savePartial(actionSound.sound,"",actionSound.start,actionSound.selectionLength());
	return true;
}

AAction::CanUndoResults CSaveSelectionAsAction::canUndo(const CActionSound &actionSound) const
{
	return curNA;
}

void CSaveSelectionAsAction::undoActionSizeSafe(const CActionSound &actionSound)
{
	// not applicable
}

bool CSaveSelectionAsAction::doesWarrantSaving() const
{
	return false;
}


// ------------------------------

CSaveSelectionAsActionFactory::CSaveSelectionAsActionFactory() :
	AActionFactory("Save Selection As","Save Selection As",false,NULL,NULL,NULL,false,false)
{
}

CSaveSelectionAsActionFactory::~CSaveSelectionAsActionFactory()
{
}

CSaveSelectionAsAction *CSaveSelectionAsActionFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	return new CSaveSelectionAsAction(actionSound,actionParameters->getSoundFileManager());
}

