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

#include "CCopyCutToNewEdit.h"

#include "../ASoundFileManager.h"
#include "../CActionParameters.h"
#include "../CLoadedSound.h"

CCopyCutToNewEdit::CCopyCutToNewEdit(const CActionSound actionSound,CCType _type,ASoundFileManager *_soundFileManager) :
    AAction(actionSound),

    type(_type),
    soundFileManager(_soundFileManager)
{
}

CCopyCutToNewEdit::~CCopyCutToNewEdit()
{
}

bool CCopyCutToNewEdit::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	if(actionSound.countChannels()<=0)
		return false; // no channels to do

	const sample_pos_t start=actionSound.start;
	const sample_pos_t selectionLength=actionSound.selectionLength();

	CLoadedSound *newSound=soundFileManager->createNew(
		soundFileManager->getUntitledFilename(gPromptDialogDirectory,"rez"),
		actionSound.countChannels(),
		actionSound.sound->getSampleRate(),
		selectionLength);

	newSound->sound->lockSize();
	try
	{
		unsigned k=0;
		for(unsigned t=0;t<actionSound.sound->getChannelCount();t++)
		{
			if(actionSound.doChannel[t])
				newSound->sound->mixSound(k++,0,actionSound.sound->getAudio(t),start,actionSound.sound->getSampleRate(),selectionLength,mmOverwrite,sftNone,false,true);
		}
		newSound->sound->unlockSize();
	}
	catch(...)
	{
		newSound->sound->unlockSize();
		throw;
	}

	if(type==cctCutToNew)
	{
		if(prepareForUndo)
			moveSelectionToTempPools(actionSound,mmSelection);
		else
			actionSound.sound->removeSpace(actionSound.doChannel,actionSound.start,actionSound.selectionLength());

		actionSound.stop=actionSound.start=actionSound.start-1;
	}

	return true;
}

AAction::CanUndoResults CCopyCutToNewEdit::canUndo(const CActionSound &actionSound) const
{
	if(type==cctCutToNew)
		return curYes;
	else
		return curNA;
}

void CCopyCutToNewEdit::undoActionSizeSafe(const CActionSound &actionSound)
{
	restoreSelectionFromTempPools(actionSound);
}

bool CCopyCutToNewEdit::doesWarrantSaving() const
{
	return type==cctCutToNew;
}


// ---------------------------------------------

CCopyToNewEditFactory::CCopyToNewEditFactory(AActionDialog *channelSelectDialog) :
	AActionFactory(N_("Copy to New"),_("Copy to Selection to a Newly Created Sound Window"),channelSelectDialog,NULL,false,false)
{
}

CCopyToNewEditFactory::~CCopyToNewEditFactory()
{
}

CCopyCutToNewEdit *CCopyToNewEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return new CCopyCutToNewEdit(actionSound,CCopyCutToNewEdit::cctCopyToNew,actionParameters->getSoundFileManager());
}



// -----------------------------------


CCutToNewEditFactory::CCutToNewEditFactory(AActionDialog *channelSelectDialog) :
	AActionFactory(N_("Cut to New"),_("Cut and Move the Selection to a Newly Created Sound Window"),channelSelectDialog,NULL)
{
}

CCutToNewEditFactory::~CCutToNewEditFactory()
{
}

CCopyCutToNewEdit *CCutToNewEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return new CCopyCutToNewEdit(actionSound,CCopyCutToNewEdit::cctCutToNew,actionParameters->getSoundFileManager());
}


