/* 
 * Copyright (C) 2005 - David W. Durham
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

#include "CSaveAsAudioFileAction.h"

#include "../CActionParameters.h"
#include "../ASoundFileManager.h"

CSaveAsAudioFileAction::CSaveAsAudioFileAction(const AActionFactory *factory,const CActionSound *actionSound,const CActionParameters *_actionParameters) :
	AAction(factory,actionSound),
	actionParameters(_actionParameters)
{
}

CSaveAsAudioFileAction::~CSaveAsAudioFileAction()
{
}

bool CSaveAsAudioFileAction::doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo)
{
	return actionParameters->getSoundFileManager()->saveAs(actionParameters->getValue<string>("filename"),actionParameters->getValue<bool>("saveAsRaw"));
}


// ------------------------------

CSaveAsAudioFileActionFactory::CSaveAsAudioFileActionFactory(AActionDialog *dialog) :
	AActionFactory(N_("Save As"),_("Save an Audio File as a Chosen Filename"),NULL,dialog,false,false)
{
	selectionPositionsAreApplicable=false;
}

CSaveAsAudioFileActionFactory::~CSaveAsAudioFileActionFactory()
{
}

CSaveAsAudioFileAction *CSaveAsAudioFileActionFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return new CSaveAsAudioFileAction(this,actionSound,actionParameters);
}

