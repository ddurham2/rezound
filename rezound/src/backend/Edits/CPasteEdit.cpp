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

#include <math.h>

#include <stdexcept>
#include <algorithm>
#include <utility>

#include <istring>

#include "../AActionDialog.h"
#include "../ASoundClipboard.h"
#include "../CActionParameters.h"


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

//                                                    const bool _pasteChannels[MAX_CHANNELS][MAX_CHANNELS]
CPasteEdit::CPasteEdit(const CActionSound actionSound,const vector<vector<bool> > &_pasteChannels,PasteTypes _pasteType,MixMethods _mixMethod,double _repeatCount) :
	AAction(actionSound),

	pasteType(_pasteType),
	mixMethod(_mixMethod),

	repeatCount(_repeatCount),

	pasteChannels(_pasteChannels),

	undoRemoveLength(0)
{
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

#include <stdio.h> // ??? just for the printf below
bool CPasteEdit::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	for(unsigned y=0;y<MAX_CHANNELS;y++)
	{
		for(unsigned x=0;x<MAX_CHANNELS;x++)
		{
			printf("%d ",(int)pasteChannels[y][x]);
		}
		printf("\n");
	}

	ASoundClipboard *clipboard=clipboards[gWhichClipboard];
	const sample_pos_t clipboardLength=clipboard->getLength(actionSound.sound->getSampleRate());
	
	// this is the length of the clipboard after we repeat it the number of times the user asked for
	const sample_pos_t repeatedClipboardLength=(sample_pos_t)sample_fpos_floor((sample_fpos_t)clipboardLength*repeatCount);

	if(repeatCount<0.0 || repeatedClipboardLength<1)
		return false;

	// save info for undo
	undoRemoveLength=repeatedClipboardLength;
	originalLength=actionSound.sound->getLength();

	switch(pasteType)
	{
	case ptInsert:
		// insert the space into the channels we need to
		actionSound.sound->addSpace(whichChannels,actionSound.start,repeatedClipboardLength);
		pasteData(clipboard,pasteChannels,actionSound,clipboardLength,repeatCount,false,mmOverwrite,mixMethod,sftNone);
		break;

	case ptReplace:
		if(prepareForUndo)
		{
			CActionSound _actionSound(actionSound);
			for(unsigned t=0;t<MAX_CHANNELS;t++)
				_actionSound.doChannel[t]=whichChannels[t];

			moveSelectionToTempPools(_actionSound,mmSelection,undoRemoveLength);
		}
		else
		{
			actionSound.sound->removeSpace(whichChannels,actionSound.start,actionSound.selectionLength());
			actionSound.sound->addSpace(whichChannels,actionSound.start,repeatedClipboardLength);
		}

		pasteData(clipboard,pasteChannels,actionSound,clipboardLength,repeatCount,false,mmOverwrite,mixMethod,sftNone);
		break;

	case ptOverwrite:
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
		if((actionSound.start+repeatedClipboardLength)>actionSound.sound->getLength())
			extraLength=(actionSound.start+repeatedClipboardLength)-actionSound.sound->getLength();

		if(prepareForUndo)
		{
			CActionSound _actionSound(actionSound);
			for(unsigned t=0;t<MAX_CHANNELS;t++)
				_actionSound.doChannel[t]=whichChannels[t];

			if(extraLength>0)
				_actionSound.stop=actionSound.sound->getLength()-1;
			else // doesn't overhang the end
				_actionSound.stop=_actionSound.start+repeatedClipboardLength-1;

			// move the data that is going to be affected into a temp pool and replace the space
			moveSelectionToTempPools(_actionSound,mmSelection,_actionSound.selectionLength());

			if(pasteType!=ptOverwrite && mixMethod!=mmOverwrite)
			{
				// copy the data back after moving it to the temp pool to mix on top of
				for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
				{
					if(whichChannels[i])
						actionSound.sound->getAudio(i).copyData(actionSound.start,actionSound.sound->getTempAudio(tempAudioPoolKey,i),0,_actionSound.selectionLength());
				}
			}
		}

		// add silence at the end if the clipboard's data overhangs the end of the sound
		if(extraLength>0)
			actionSound.sound->addSpace(whichChannels,actionSound.sound->getLength(),extraLength,true);

		pasteData(clipboard,pasteChannels,actionSound,clipboardLength,repeatCount,!prepareForUndo,pasteType==ptOverwrite ? mmOverwrite : mixMethod,mixMethod,sftNone);

	}
		break;

	case ptLimitedOverwrite:
	case ptLimitedMix:
		if(prepareForUndo)
		{
			CActionSound _actionSound(actionSound);
			for(unsigned t=0;t<MAX_CHANNELS;t++)
				_actionSound.doChannel[t]=whichChannels[t];

			_actionSound.stop=min(_actionSound.start+repeatedClipboardLength-1,_actionSound.stop);

			// reassign this value for undo
			undoRemoveLength=_actionSound.selectionLength();

			// move the data that is going to be affected into a temp pool and replace the space
			moveSelectionToTempPools(_actionSound,mmSelection,_actionSound.selectionLength());

			if(pasteType!=ptLimitedOverwrite && mixMethod!=mmOverwrite)
			{
				// copy the data back after moving it to the temp pool to mix on top of
				for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
				{
					if(whichChannels[i])
						actionSound.sound->getAudio(i).copyData(actionSound.start,actionSound.sound->getTempAudio(tempAudioPoolKey,i),0,_actionSound.selectionLength());
				}
			}
		}

		{
			// send pasteData an adjustedRepeatCount, if the limited length (undoRemoveLength) is less than what we would like to paste (repeatedClipboardLength), then calculate how many times we can actually repeat the clipboard (which could be <=1 or even >1)
			double adjustedRepeatCount;
			if(undoRemoveLength<repeatedClipboardLength)
				adjustedRepeatCount=(double)((sample_fpos_t)undoRemoveLength/(sample_fpos_t)clipboardLength);
			else
				adjustedRepeatCount=repeatCount;

			pasteData(clipboard,pasteChannels,actionSound,clipboardLength,adjustedRepeatCount,!prepareForUndo,pasteType==ptLimitedOverwrite ? mmOverwrite : mixMethod,mixMethod,sftNone);
		}
		break;

	case ptFitMix:
		if(prepareForUndo)
		{
			CActionSound _actionSound(actionSound);
			for(unsigned t=0;t<MAX_CHANNELS;t++)
				_actionSound.doChannel[t]=whichChannels[t];

			// reassign this value for undo
			undoRemoveLength=actionSound.selectionLength();

			// move the data that is going to be affected into a temp pool and replace the space
			moveSelectionToTempPools(_actionSound,mmSelection,actionSound.selectionLength());

			// copy the data back after moving it to the temp pool to mix on top of
			if(mixMethod!=mmOverwrite)
			{
				for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
				{
					if(whichChannels[i])
						actionSound.sound->getAudio(i).copyData(actionSound.start,actionSound.sound->getTempAudio(tempAudioPoolKey,i),0,_actionSound.selectionLength());
				}
			}
		}

		// instead of being able to simply pass repeatCount to pasteData, I have to break it into its multiple repeats here
		{
			CActionSound _actionSound(actionSound);
			double iRepeatCount;
			const double fRepeatCount=modf(repeatCount,&iRepeatCount);

			// do all the n iterations of repeatCount as n.xyz
			const sample_pos_t oneIterationLength=(sample_pos_t)sample_fpos_floor((sample_fpos_t)_actionSound.selectionLength()/(sample_fpos_t)repeatCount);
			for(unsigned k=0;k<(unsigned)iRepeatCount;k++)
			{
				pasteData(clipboard,pasteChannels,_actionSound,oneIterationLength,1,!prepareForUndo,mixMethod,mixMethod,sftChangeRate);
				_actionSound.start+=oneIterationLength;
			}


			// this would be the way to do it, but ...
			//sample_pos_t lastIterationLength=(sample_pos_t)sample_fpos_floor(((sample_fpos_t)actionSound.selectionLength()*(sample_fpos_t)fRepeatCount)/(sample_fpos_t)repeatCount);
		
			// ... this will be sure to cover the remainder left off from flooring errors in the n iterations above
				// perhaps there's a better way, maybe rounding instead of flooring when calculating oneIterationLength
			const sample_pos_t lastIterationLength=actionSound.selectionLength()-(oneIterationLength*(unsigned)iRepeatCount);

			if(lastIterationLength>0 && clipboardLength>1/*has a little bug from flooring if this is true*/)
			{
				const sample_pos_t shortenedClipboardLength=(sample_pos_t)sample_fpos_floor((sample_fpos_t)clipboard->getLength(_actionSound.sound->getSampleRate())*fRepeatCount);
				clipboard->temporarilyShortenLength(_actionSound.sound->getSampleRate(),max(shortenedClipboardLength,(sample_pos_t)1));
				try
				{
					pasteData(clipboard,pasteChannels,_actionSound,lastIterationLength,1,!prepareForUndo,mixMethod,mixMethod,sftChangeRate);

					clipboard->undoTemporaryShortenLength();
				}
				catch(...)
				{
					clipboard->undoTemporaryShortenLength();
					throw;
				}
			}
		}
		break;

	default:
		throw runtime_error(string(__func__)+" -- unhandled pasteType: "+istring(pasteType));
	}

	return true;
}

/*
 * writes the data from the clipboard onto the sound...
 * this method assumes there is already room to write the data
 * 	whichChannels is true for each which which is going to have data written to, calculated from pasteChannels
 * 	pasteChannels came from the user
 */
//                                                          const bool pasteChannels[MAX_CHANNELS][MAX_CHANNELS]
void CPasteEdit::pasteData(const ASoundClipboard *clipboard,const vector<vector<bool> > &pasteChannels,const CActionSound &actionSound,const sample_pos_t srcToUse,const double repeatCount,bool invalidatePeakData,MixMethods initialMixMethod,MixMethods nonInitialMixMethod,SourceFitTypes fitSrc)
{
	for(unsigned y=0;y<actionSound.sound->getChannelCount();y++)
	{
		bool first=true;
		for(unsigned x=0;x<MAX_CHANNELS;x++)
		{
			if(pasteChannels[y][x])
			{
				sample_pos_t start=actionSound.start;
				double iRepeatCount;
				const double fRepeatCount=modf(repeatCount,&iRepeatCount);

				// do all the n iterations of repeatCount as n.xyz
				for(unsigned k=0;k<(unsigned)iRepeatCount;k++)
				{
					clipboards[gWhichClipboard]->copyTo(actionSound.sound,y,x,start,srcToUse,first ? initialMixMethod : nonInitialMixMethod,fitSrc,first && invalidatePeakData);
					start+=srcToUse;
				}

				// do the .xyz remaining part of the paste
				if(fRepeatCount>0.0 && (srcToUse*fRepeatCount)>=1.0)
					clipboards[gWhichClipboard]->copyTo(actionSound.sound,y,x,start,(sample_pos_t)((sample_fpos_t)srcToUse*fRepeatCount),first ? initialMixMethod : nonInitialMixMethod,fitSrc,first && invalidatePeakData);

				first=false;
			}
		}
	}

	const sample_pos_t repeatedSrcToUse=(sample_pos_t)sample_fpos_floor((sample_fpos_t)srcToUse*repeatCount);
	actionSound.stop=actionSound.start+repeatedSrcToUse-1;
}

AAction::CanUndoResults CPasteEdit::canUndo(const CActionSound &actionSound) const
{
	// should check some size constraint
	return curYes;
}

void CPasteEdit::undoActionSizeSafe(const CActionSound &actionSound)
{
	switch(pasteType)
	{
	case ptInsert:
		actionSound.sound->removeSpace(whichChannels,actionSound.start,undoRemoveLength,originalLength);
		break;

	case ptReplace:
	case ptOverwrite:
	case ptLimitedOverwrite:
	case ptMix:
	case ptLimitedMix:
	case ptFitMix:
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
		throw runtime_error(string(__func__)+" -- unhandled pasteType: "+istring(pasteType));
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
	case ptFitMix:
		stop=min(actionSound.stop+1,actionSound.sound->getLength()-1);
		break;

	case ptOverwrite:
	case ptMix:
		stop=min(start+clipboardLength,actionSound.sound->getLength()-1);
		break;

	case ptLimitedOverwrite:
	case ptLimitedMix:
		stop=min(start+clipboardLength,start+actionSound.selectionLength());
		break;

	default:
		throw runtime_error(string(__func__)+" -- unhandled pasteType: "+istring(pasteType));
	}
	return true;
}



// ------------------------------


#define CHECK_FOR_DATA(ClassName) 						\
	bool ClassName::doPreActionSetup(CLoadedSound *loadedSound)		\
	{ 									\
		if(!AAction::clipboards[gWhichClipboard]->prepareForCopyTo())	\
			return false;						\
										\
		if(AAction::clipboards[gWhichClipboard]->isEmpty())		\
		{ 								\
			Message("No data has been cut or copied to the selected clipboard yet."); \
			return false; 						\
		} 								\
		return true; 							\
	}

static const vector<vector<bool> > getPasteChannels(AActionDialog *pasteChannelsDialog)
{
	static vector<vector<bool> > pasteChannels;
	pasteChannels.clear();

	if(pasteChannelsDialog->wasShown)
	{
		const vector<vector<bool> > &m= *reinterpret_cast<const vector<vector<bool> > *>(pasteChannelsDialog->getUserData());
		for(unsigned y=0;y<MAX_CHANNELS;y++)
		{
			pasteChannels.push_back(vector<bool>() );
			for(unsigned x=0;x<MAX_CHANNELS;x++)
				pasteChannels[y].push_back(m[y][x]);
		}
	}
	else
	{ // if the dialog was not shown for this action, then make channel1 -> channel1, channel2 -> channel2, ...
		const ASoundClipboard *clipboard=AAction::clipboards[gWhichClipboard];

		for(unsigned y=0;y<MAX_CHANNELS;y++)
		{
			pasteChannels.push_back(vector<bool>() );
			for(unsigned x=0;x<MAX_CHANNELS;x++)
			{
				if(y==x && clipboard->getWhichChannels()[y])
					pasteChannels[y].push_back(true);
				else
					pasteChannels[y].push_back(false);
			}
		}
	}

	return pasteChannels;
}

static const MixMethods getMixMethod(const CActionParameters *actionParameters)
{
	return actionParameters->containsParameter("MixMethod") ? (MixMethods)actionParameters->getUnsignedParameter("MixMethod") : mmAdd;
}

static const double getRepeatCount(const CActionParameters *actionParameters)
{
	return actionParameters->containsParameter("Repeat Count") ? actionParameters->getDoubleParameter("Repeat Count") : 1.0;
}


// ------------------------------

CInsertPasteEditFactory::CInsertPasteEditFactory(AActionDialog *channelSelectDialog) :
	AActionFactory("Insert Paste","Insert at the Start Position",channelSelectDialog,NULL)
{
}

CInsertPasteEditFactory::~CInsertPasteEditFactory()
{
}

CPasteEdit *CInsertPasteEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return new CPasteEdit(actionSound,getPasteChannels(channelSelectDialog),CPasteEdit::ptInsert,getMixMethod(actionParameters),getRepeatCount(actionParameters));
}

CHECK_FOR_DATA(CInsertPasteEditFactory)


// -----------------------------------


CReplacePasteEditFactory::CReplacePasteEditFactory(AActionDialog *channelSelectDialog) :
	AActionFactory("Replace Paste","Replace the Selection",channelSelectDialog,NULL)
{
}

CReplacePasteEditFactory::~CReplacePasteEditFactory()
{
}

CPasteEdit *CReplacePasteEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return new CPasteEdit(actionSound,getPasteChannels(channelSelectDialog),CPasteEdit::ptReplace,getMixMethod(actionParameters),getRepeatCount(actionParameters));
}

CHECK_FOR_DATA(CReplacePasteEditFactory)

// -----------------------------------


COverwritePasteEditFactory::COverwritePasteEditFactory(AActionDialog *channelSelectDialog) :
	AActionFactory("Overwrite Paste","Overwrite Starting at the Start Position",channelSelectDialog,NULL)
{
}

COverwritePasteEditFactory::~COverwritePasteEditFactory()
{
}

CPasteEdit *COverwritePasteEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return new CPasteEdit(actionSound,getPasteChannels(channelSelectDialog),CPasteEdit::ptOverwrite,getMixMethod(actionParameters),getRepeatCount(actionParameters));
}

CHECK_FOR_DATA(COverwritePasteEditFactory)


// -----------------------------------


CLimitedOverwritePasteEditFactory::CLimitedOverwritePasteEditFactory(AActionDialog *channelSelectDialog) :
	AActionFactory("Limited Overwrite Paste","Overwrite Starting at the Start Position But Not Beyond the Stop Position",channelSelectDialog,NULL)
{
}

CLimitedOverwritePasteEditFactory::~CLimitedOverwritePasteEditFactory()
{
}

CPasteEdit *CLimitedOverwritePasteEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return new CPasteEdit(actionSound,getPasteChannels(channelSelectDialog),CPasteEdit::ptLimitedOverwrite,getMixMethod(actionParameters),getRepeatCount(actionParameters));
}

CHECK_FOR_DATA(CLimitedOverwritePasteEditFactory)


// -----------------------------------


CMixPasteEditFactory::CMixPasteEditFactory(AActionDialog *channelSelectDialog) :
	AActionFactory("Mix Paste","Mix Starting at the Start Position",channelSelectDialog,NULL)
{
}

CMixPasteEditFactory::~CMixPasteEditFactory()
{
}

CPasteEdit *CMixPasteEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return new CPasteEdit(actionSound,getPasteChannels(channelSelectDialog),CPasteEdit::ptMix,getMixMethod(actionParameters),getRepeatCount(actionParameters));
}

CHECK_FOR_DATA(CMixPasteEditFactory)

// -----------------------------------


CLimitedMixPasteEditFactory::CLimitedMixPasteEditFactory(AActionDialog *channelSelectDialog) :
	AActionFactory("Limited Mix Paste","Mix Starting at the Start Position But Not Beyond the Stop Position",channelSelectDialog,NULL)
{
}

CLimitedMixPasteEditFactory::~CLimitedMixPasteEditFactory()
{
}

CPasteEdit *CLimitedMixPasteEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return new CPasteEdit(actionSound,getPasteChannels(channelSelectDialog),CPasteEdit::ptLimitedMix,getMixMethod(actionParameters),getRepeatCount(actionParameters));
}

CHECK_FOR_DATA(CLimitedMixPasteEditFactory)

// -----------------------------------


CFitMixPasteEditFactory::CFitMixPasteEditFactory(AActionDialog *channelSelectDialog) :
	AActionFactory("Fit Mix Paste","Mix Starting at the Start Position But Change the Rate of the Clipboard to Fit Within the Selection",channelSelectDialog,NULL)
{
}

CFitMixPasteEditFactory::~CFitMixPasteEditFactory()
{
}

CPasteEdit *CFitMixPasteEditFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return new CPasteEdit(actionSound,getPasteChannels(channelSelectDialog),CPasteEdit::ptFitMix,getMixMethod(actionParameters),getRepeatCount(actionParameters));
}

CHECK_FOR_DATA(CFitMixPasteEditFactory)

