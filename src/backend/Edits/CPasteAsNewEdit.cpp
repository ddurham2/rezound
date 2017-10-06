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

#include "CPasteAsNewEdit.h"

#include "../ASoundClipboard.h"
#include "../ASoundFileManager.h"
#include "../CActionParameters.h"
#include "../CLoadedSound.h"
#include "../settings.h"

CPasteAsNewEdit::CPasteAsNewEdit(const AActionFactory *factory,const CActionSound *actionSound,ASoundFileManager *_soundFileManager) :
    AAction(factory,actionSound),
    soundFileManager(_soundFileManager)
{
}

CPasteAsNewEdit::~CPasteAsNewEdit()
{
}

bool CPasteAsNewEdit::doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo)
{
	ASoundClipboard *clipboard=clipboards[gWhichClipboard];
	const sample_pos_t clipboardLength=clipboard->getLength(clipboard->getSampleRate());

	unsigned channelCount=0;
	for(unsigned t=0;t<MAX_CHANNELS;t++)
		channelCount+= clipboard->getWhichChannels()[t] ? 1 : 0;
	
	CLoadedSound *newSound=soundFileManager->createNew(
		soundFileManager->getUntitledFilename(gPromptDialogDirectory,"rez"),
		channelCount,
		clipboard->getSampleRate(),
		clipboardLength);

	try
	{
		CSoundLocker sl(newSound->sound, false);
		unsigned k=0;
		for(unsigned t=0;t<MAX_CHANNELS;t++)
		{
			if(clipboard->getWhichChannels()[t])
				clipboard->copyTo(newSound->sound,k++,t,0,clipboardLength,mmOverwrite,sftNone,false);
		}
	}
	catch(...)
	{
		soundFileManager->close(ASoundFileManager::ctSaveNone,newSound);
		throw;
	}

	return true;
}

AAction::CanUndoResults CPasteAsNewEdit::canUndo(const CActionSound *actionSound) const
{
	return curNA;
}

void CPasteAsNewEdit::undoActionSizeSafe(const CActionSound *actionSound)
{
}

bool CPasteAsNewEdit::doesWarrantSaving() const
{
	return false;
}


// ------------------------------


CPasteAsNewEditFactory::CPasteAsNewEditFactory() :
	AActionFactory(N_("Paste As New"),_("Paste the Clipboard's Contents into a Newly Created Sound Window"),NULL,NULL,false,false)
{
	requiresALoadedSound=false;
	selectionPositionsAreApplicable=false;
}

CPasteAsNewEditFactory::~CPasteAsNewEditFactory()
{
}

CPasteAsNewEdit *CPasteAsNewEditFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return new CPasteAsNewEdit(this,actionSound,actionParameters->getSoundFileManager());
}

bool CPasteAsNewEditFactory::doPreActionSetup(CLoadedSound *loadedSound)
{
	if(!AAction::clipboards[gWhichClipboard]->prepareForCopyTo())
		return false;

	if(AAction::clipboards[gWhichClipboard]->isEmpty())
	{
		Message(_("No data has been cut or copied to the selected clipboard yet."));
		return false;
	}
	return true;
}



