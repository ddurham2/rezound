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

#include "CChangeTempoAction.h"

#include "../CActionSound.h"
#include "../CActionParameters.h"

#include "../DSP/TTempoChanger.h"

CChangeTempoAction::CChangeTempoAction(const CActionSound &actionSound,const double _tempoChange,bool _useAntiAliasFilter,unsigned _antiAliasFilterLength,bool _useQuickSeek,unsigned _sequenceLength,unsigned _seekingWindowLength,unsigned _overlapLength) :
	AAction(actionSound),
	undoRemoveLength(0),
	tempoChange(_tempoChange),

	useAntiAliasFilter(_useAntiAliasFilter),
	antiAliasFilterLength(_antiAliasFilterLength),
	useQuickSeek(_useQuickSeek),
	sequenceLength(_sequenceLength),
	seekingWindowLength(_seekingWindowLength),
	overlapLength(_overlapLength)
{
}

CChangeTempoAction::~CChangeTempoAction()
{
}

bool CChangeTempoAction::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
#ifdef AVAIL_PITCH_CHANGE
	const sample_pos_t start=actionSound.start;
	const sample_pos_t stop=actionSound.stop;
	const sample_pos_t oldLength=actionSound.selectionLength();
	const sample_pos_t newLength=(sample_pos_t)(oldLength/tempoChange);

	if(tempoChange==0.0)
		return false;

	// we are gonna use the undo selection copy as the source buffer while changing the tempo
	moveSelectionToTempPools(actionSound,mmSelection,newLength);

	undoRemoveLength=newLength;

	unsigned channelsDoneCount=0;
	for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
	{
		if(actionSound.doChannel[i])
		{
			CStatusBar statusBar(_("Changing Tempo -- Channel ")+istring(++channelsDoneCount)+"/"+istring(actionSound.countChannels()),0,newLength,true); 
	
			// here, we're using the undo data as a source from which to calculate the new data
			const CRezPoolAccesser src=actionSound.sound->getTempAudio(tempAudioPoolKey,i);
			CRezPoolAccesser dest=actionSound.sound->getAudio(i);

			TTempoChanger<CRezPoolAccesser> stretcher(src,0,oldLength,newLength,actionSound.sound->getSampleRate(),1,0);

			// these are libSoundTouch specific
			stretcher.setSetting(SETTING_USE_AA_FILTER,useAntiAliasFilter);
			stretcher.setSetting(SETTING_AA_FILTER_LENGTH,antiAliasFilterLength);
			stretcher.setSetting(SETTING_USE_QUICKSEEK,useQuickSeek);
			stretcher.setSetting(SETTING_SEQUENCE_MS,sequenceLength);
			stretcher.setSetting(SETTING_SEEKWINDOW_MS,seekingWindowLength);
			stretcher.setSetting(SETTING_OVERLAP_MS,overlapLength);

			for(sample_pos_t t=0;t<newLength;t++)
			{
				dest[t+start]=stretcher.getSample();

				if(statusBar.update(t))
				{ // cancelled
					restoreSelectionFromTempPools(actionSound,actionSound.start,undoRemoveLength);
					return false;
				}
			}
		}
	}

	// adjust the cue positions that were within the selection (and shift over the ones past the selection) and don't touch anchored cues
	for(size_t t=0;t<actionSound.sound->getCueCount();t++)
	{
		if(actionSound.sound->isCueAnchored(t) || actionSound.sound->getCueTime(t)<=start)
			continue;

		if(actionSound.sound->getCueTime(t)>=stop)
			actionSound.sound->setCueTime(t,actionSound.sound->getCueTime(t)-oldLength+newLength);
		else
			actionSound.sound->setCueTime(t,(sample_pos_t)sample_fpos_round((sample_fpos_t)(actionSound.sound->getCueTime(t)-start)/tempoChange)+start);
	}
	
	// adjust start and stop positions
	actionSound.stop=actionSound.start+newLength;

	if(!prepareForUndo)
		freeAllTempPools();

	return true;
#else
	throw EUserMessage(_("No library was found at configure time to enable tempo changing"));
#endif
}

AAction::CanUndoResults CChangeTempoAction::canUndo(const CActionSound &actionSound) const
{
	return curYes;
}


void CChangeTempoAction::undoActionSizeSafe(const CActionSound &actionSound)
{
	restoreSelectionFromTempPools(actionSound,actionSound.start,undoRemoveLength);
}



// ---------------------------------------------

CChangeTempoActionFactory::CChangeTempoActionFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog) :
	AActionFactory(N_("Change Tempo"),_("Change Tempo"),channelSelectDialog,dialog)
{
}

CChangeTempoActionFactory::~CChangeTempoActionFactory()
{
}

CChangeTempoAction *CChangeTempoActionFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return new CChangeTempoAction(
		actionSound,
		actionParameters->getDoubleParameter("Tempo Change"),
		actionParameters->getBoolParameter("Use Anti-alias Filter"),
		actionParameters->getUnsignedParameter("Anti-alias Filter Length"),
		actionParameters->getBoolParameter("Use Quick Seek"),
		actionParameters->getUnsignedParameter("Sequence Length"),
		actionParameters->getUnsignedParameter("Seek Window Length"),
		actionParameters->getUnsignedParameter("Overlap Length")
	);
}

