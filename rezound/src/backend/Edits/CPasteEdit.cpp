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

#include "CPasteEdit.h"

#include <stdexcept>
#include <algorithm>

#include <istring>

#include "../AActionDialog.h"
#include "../ASoundClipboard.h"


/* 
 * - pasteChannels is a NxN matrix of bools that says how the channels are pasted into the sound
 *    for example  if pasteChannels[0][1]==true    then data from the left channel gets pasted into the right channel
 *    for example  if pasteChannels[1][1]==true    then data from the right channel gets pasted into the right channel
 *
 * ??? this should now be fixed with the abstraction of ASoundClipboard
 * - Although the data could represent it, I ignore if two channels from the source data is to be pasted into one channel;
 *   I just use the first one I come to... The GUI interface should have radio buttons to disallow the user creating that
 *   senario
 */

CPasteEdit::CPasteEdit(const CActionSound actionSound,const bool _pasteChannels[MAX_CHANNELS][MAX_CHANNELS],PasteTypes _pasteType,MixMethods _mixMethod) :
    AAction(actionSound),

    pasteType(_pasteType),
    mixMethod(_mixMethod),

    undoRemoveLength(0)
{
	for(unsigned y=0;y<MAX_CHANNELS;y++)
	for(unsigned x=0;x<MAX_CHANNELS;x++)
		pasteChannels[y][x]=_pasteChannels[y][x];

	// create whichChannels which is true for each channel that will have something going into it from the clipboard
	for(unsigned y=0;y<MAX_CHANNELS;y++)
	{
		whichChannels[y]=false;
		for(unsigned x=0;x<MAX_CHANNELS;x++)
			whichChannels[y]|=pasteChannels[y][x];
	}
}

CPasteEdit::~CPasteEdit()
{
}

bool CPasteEdit::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	for(unsigned y=0;y<MAX_CHANNELS;y++)
	{
		for(unsigned x=0;x<MAX_CHANNELS;x++)
		{
			printf("%d ",pasteChannels[y][x]);
		}
		printf("\n");
	}

	const ASoundClipboard *clipboard=clipboards[gWhichClipboard];
	const sample_pos_t clipboardLength=clipboard->getLength(actionSound.sound->getSampleRate());

	// save info for undo
	undoRemoveLength=clipboardLength;
	originalLength=actionSound.sound->getLength();

	switch(pasteType)
	{
	case ptInsert:
		// insert the space into the channels we need to
		actionSound.sound->addSpace(whichChannels,actionSound.start,clipboardLength);
		pasteData(clipboard,pasteChannels,actionSound,clipboardLength,false,mmOverwrite,mmAdd);
		break;

	case ptReplace:
		if(prepareForUndo)
		{
			CActionSound _actionSound(actionSound);
			for(unsigned t=0;t<MAX_CHANNELS;t++)
				_actionSound.doChannel[t]=whichChannels[t];

			moveSelectionToTempPools(_actionSound,mmSelection,clipboardLength);
		}
		else
		{
			actionSound.sound->removeSpace(whichChannels,actionSound.start,actionSound.selectionLength());
			actionSound.sound->addSpace(whichChannels,actionSound.start,clipboardLength);
		}

		pasteData(clipboard,pasteChannels,actionSound,clipboardLength,false,mmOverwrite,mmAdd);
		break;

	case ptMix:
	{
		/*                    v--- start selection position
		 * Either:            
		 * 	Sound:     [--|-----]
		 * 	Clipboard:    |---|
		 *
		 * 	or
		 *
		 * 	Sound:     [--|-----]
		 * 	Clipboard:    |---------|
		 *
		 * 	                    |___|
		 * 	                      ^-- extraLength
		 */

		// it is the amount that the clipboard, if pasted starting at the start position, overhangs the end of the sound
		sample_pos_t extraLength=0;
		if((actionSound.start+clipboardLength)>actionSound.sound->getLength())
			extraLength=(actionSound.start+clipboardLength)-actionSound.sound->getLength();

		if(prepareForUndo)
		{
			CActionSound _actionSound(actionSound);
			for(unsigned t=0;t<MAX_CHANNELS;t++)
				_actionSound.doChannel[t]=whichChannels[t];

			if(extraLength>0)
				_actionSound.stop=actionSound.sound->getLength()-1;
			else // doesn't overhang the end
				_actionSound.stop=_actionSound.start+clipboardLength-1;

			// move the data that is going to be affected into a temp pool and replace the space
			moveSelectionToTempPools(_actionSound,mmSelection,_actionSound.selectionLength());

			// copy the data back after moving it to the temp pool to mix on top of
			for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
			{
				if(whichChannels[i])
					actionSound.sound->getAudio(i).copyData(actionSound.start,actionSound.sound->getTempAudio(tempAudioPoolKey,i),0,_actionSound.selectionLength());
			}
		}

		// add silence at the end if the clipboard's data overhangs the end of the sound
		if(extraLength>0)
			actionSound.sound->addSpace(whichChannels,actionSound.sound->getLength(),extraLength,true);

		pasteData(clipboard,pasteChannels,actionSound,clipboardLength,prepareForUndo,mixMethod,mixMethod);

	}
		break;

	case ptLimitedMix:
		if(prepareForUndo)
		{
			CActionSound _actionSound(actionSound);
			for(unsigned t=0;t<MAX_CHANNELS;t++)
				_actionSound.doChannel[t]=whichChannels[t];

			_actionSound.stop=min(_actionSound.start+clipboardLength-1,_actionSound.stop);

			// reassign this value for undo
			undoRemoveLength=_actionSound.selectionLength();

			// move the data that is going to be affected into a temp pool and replace the space
			moveSelectionToTempPools(_actionSound,mmSelection,_actionSound.selectionLength());

			// copy the data back after moving it to the temp pool to mix on top of
			for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
			{
				if(whichChannels[i])
					actionSound.sound->getAudio(i).copyData(actionSound.start,actionSound.sound->getTempAudio(tempAudioPoolKey,i),0,_actionSound.selectionLength());
			}
		}

		pasteData(clipboard,pasteChannels,actionSound,undoRemoveLength,prepareForUndo,mixMethod,mixMethod);

		break;

	default:
		throw(runtime_error(string(__func__)+" -- unhandled pasteType: "+istring(pasteType)));
	}

	return(true);
}

/*
 * writes the data from the clipboard onto the sound...
 * this method assumes there is already room to write the data
 * 	whichChannels is true for each which which is going to have data written to, calculated from pasteChannels
 * 	pasteChannels came from the user
 */
void CPasteEdit::pasteData(const ASoundClipboard *clipboard,const bool pasteChannels[MAX_CHANNELS][MAX_CHANNELS],const CActionSound &actionSound,const sample_pos_t srcLength,bool invalidatePeakData,MixMethods initialMixMethod,MixMethods nonInitialMixMethod)
{
	for(unsigned y=0;y<actionSound.sound->getChannelCount();y++)
	{
		bool first=true;
		for(unsigned x=0;x<MAX_CHANNELS;x++)
		{
			if(pasteChannels[y][x])
			{
				clipboards[gWhichClipboard]->copyTo(actionSound.sound,y,x,actionSound.start,srcLength,first ? initialMixMethod : nonInitialMixMethod,first && invalidatePeakData);
				first=false;
			}
		}
	}

	actionSound.stop=actionSound.start+srcLength-1;
}

AAction::CanUndoResults CPasteEdit::canUndo(const CActionSound &actionSound) const
{
	// should check some size constraint
	return(curYes);
}

void CPasteEdit::undoActionSizeSafe(const CActionSound &actionSound)
{
	switch(pasteType)
	{
	case ptInsert:
		actionSound.sound->removeSpace(whichChannels,actionSound.start,undoRemoveLength,originalLength);
		break;

	case ptReplace:
	case ptMix:
	case ptLimitedMix:
		{
		// I have to create undoActionSound (a copy of actionSound) and change its doChannel according 
		// to pasteChannels (actually from whichChannels which came originally from pasteChannels)
		CActionSound undoActionSound(actionSound);
		for(unsigned t=0;t<MAX_CHANNELS;t++)
			undoActionSound.doChannel[t]=whichChannels[t];

		restoreSelectionFromTempPools(undoActionSound,undoActionSound.start,undoRemoveLength);
		}
		break;

	default:
		throw(runtime_error(string(__func__)+" -- unhandled pasteType: "+istring(pasteType)));
	}
}

bool CPasteEdit::getResultingCrossfadePoints(const CActionSound &actionSound,sample_pos_t &start,sample_pos_t &stop)
{
	const ASoundClipboard *clipboard=clipboards[gWhichClipboard];
	const sample_pos_t clipboardLength=clipboard->getLength(actionSound.sound->getSampleRate());

	start=actionSound.start;

	switch(pasteType)
	{
	case ptInsert:
		stop=start;
		break;

	case ptReplace:
		stop=min(actionSound.stop+1,actionSound.sound->getLength()-1);
		break;

	case ptMix:
		stop=min(start+clipboardLength,actionSound.sound->getLength()-1);
		break;

	case ptLimitedMix:
		stop=min(start+clipboardLength,start+actionSound.selectionLength());
		break;

	default:
		throw(runtime_error(string(__func__)+" -- unhandled pasteType: "+istring(pasteType)));
	}
	return(true);
}



// ------------------------------


#define CHECK_FOR_DATA(ClassName) 						\
	bool ClassName::doPreActionSetup() 					\
	{ 									\
		if(!AAction::clipboards[gWhichClipboard]->prepareForCopyTo())	\
			return(false);						\
										\
		if(AAction::clipboards[gWhichClipboard]->isEmpty())		\
		{ 								\
			Message("No data has been cut or copied yet"); 		\
			return(false); 						\
		} 								\
		return(true); 							\
	}

typedef bool PasteChannels_t[MAX_CHANNELS][MAX_CHANNELS];

static bool **getPasteChannels(AActionDialog *pasteChannelsDialog)
{
	if(pasteChannelsDialog->wasShown)
		return((bool **)pasteChannelsDialog->getUserData());
	else
	{ // if the dialog was not shown for this action, then make channel1 -> channel1, channel2 -> channel2, ...

		const ASoundClipboard *clipboard=AAction::clipboards[gWhichClipboard];

		static PasteChannels_t pasteChannels;

		for(unsigned y=0;y<MAX_CHANNELS;y++)
		for(unsigned x=0;x<MAX_CHANNELS;x++)
		{
			if(y==x && clipboard->getWhichChannels()[y])
				pasteChannels[x][y]=true;
			else
				pasteChannels[x][y]=false;
		}

		return((bool **)pasteChannels);
	}
}


// ------------------------------

CInsertPasteEditFactory::CInsertPasteEditFactory(AActionDialog *channelSelectDialog) :
	AActionFactory("Insert Paste","Insert at the Start Position",false,channelSelectDialog,NULL,NULL)
{
}

CPasteEdit *CInsertPasteEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	return(new CPasteEdit(actionSound,(PasteChannels_t)getPasteChannels(channelSelectDialog),CPasteEdit::ptInsert));
}

CHECK_FOR_DATA(CInsertPasteEditFactory)


// -----------------------------------


CReplacePasteEditFactory::CReplacePasteEditFactory(AActionDialog *channelSelectDialog) :
	AActionFactory("Replace Paste","Replace the Selection",false,channelSelectDialog,NULL,NULL)
{
}

CPasteEdit *CReplacePasteEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	return(new CPasteEdit(actionSound,(PasteChannels_t)getPasteChannels(channelSelectDialog),CPasteEdit::ptReplace));
}

CHECK_FOR_DATA(CReplacePasteEditFactory)

// -----------------------------------


COverwritePasteEditFactory::COverwritePasteEditFactory(AActionDialog *channelSelectDialog) :
	AActionFactory("Overwrite Paste","Overwrite Starting at the Start Position",false,channelSelectDialog,NULL,NULL)
{
}

CPasteEdit *COverwritePasteEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	return(new CPasteEdit(actionSound,(PasteChannels_t)getPasteChannels(channelSelectDialog),CPasteEdit::ptMix,mmOverwrite));
}

CHECK_FOR_DATA(COverwritePasteEditFactory)


// -----------------------------------


CLimitedOverwritePasteEditFactory::CLimitedOverwritePasteEditFactory(AActionDialog *channelSelectDialog) :
	AActionFactory("Limited Overwrite Paste","Overwrite Starting at the Start Position But Not Beyond the Stop Position",false,channelSelectDialog,NULL,NULL)
{
}

CPasteEdit *CLimitedOverwritePasteEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	return(new CPasteEdit(actionSound,(PasteChannels_t)getPasteChannels(channelSelectDialog),CPasteEdit::ptLimitedMix,mmOverwrite));
}

CHECK_FOR_DATA(CLimitedOverwritePasteEditFactory)


// -----------------------------------


CMixPasteEditFactory::CMixPasteEditFactory(AActionDialog *channelSelectDialog) :
	AActionFactory("Mix Paste","Mix Starting at the Start Position",false,channelSelectDialog,NULL,NULL)
{
}

CPasteEdit *CMixPasteEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
		// ??? get mix method from dialog
	return(new CPasteEdit(actionSound,(PasteChannels_t)getPasteChannels(channelSelectDialog),CPasteEdit::ptMix,mmAdd));
}

CHECK_FOR_DATA(CMixPasteEditFactory)

// -----------------------------------


CLimitedMixPasteEditFactory::CLimitedMixPasteEditFactory(AActionDialog *channelSelectDialog) :
	AActionFactory("Limited Mix Paste","Mix Starting at the Start Position But Not Beyond the Stop Position",false,channelSelectDialog,NULL,NULL)
{
}

CPasteEdit *CLimitedMixPasteEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
		// ??? get mix method from dialog
	return(new CPasteEdit(actionSound,(PasteChannels_t)getPasteChannels(channelSelectDialog),CPasteEdit::ptLimitedMix,mmAdd));
}

CHECK_FOR_DATA(CLimitedMixPasteEditFactory)

