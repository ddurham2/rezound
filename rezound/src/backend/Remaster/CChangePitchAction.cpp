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

#include "CChangePitchAction.h"

#include "../CActionSound.h"
#include "../CActionParameters.h"

#include "../DSP/TPitchChanger.h"

CChangePitchAction::CChangePitchAction(const AActionFactory *factory,const CActionSound *actionSound,const float _deltaSemitones,bool _useAntiAliasFilter,unsigned _antiAliasFilterLength,bool _useQuickSeek,unsigned _sequenceLength,unsigned _seekingWindowLength,unsigned _overlapLength) :
	AAction(factory,actionSound),
	deltaSemitones(_deltaSemitones),

	useAntiAliasFilter(_useAntiAliasFilter),
	antiAliasFilterLength(_antiAliasFilterLength),
	useQuickSeek(_useQuickSeek),
	sequenceLength(_sequenceLength),
	seekingWindowLength(_seekingWindowLength),
	overlapLength(_overlapLength)
{
}

CChangePitchAction::~CChangePitchAction()
{
}

bool CChangePitchAction::doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo)
{
#ifdef AVAIL_PITCH_CHANGE
	const sample_pos_t start=actionSound->start;
	const sample_pos_t stop=actionSound->stop;
	const sample_pos_t selectionLength=actionSound->selectionLength();

	if(deltaSemitones==0.0)
		return false;

	if(prepareForUndo)
		moveSelectionToTempPools(actionSound,mmSelection,actionSound->selectionLength());

	unsigned channelsDoneCount=0;
	for(unsigned i=0;i<actionSound->sound->getChannelCount();i++)
	{
		if(actionSound->doChannel[i])
		{
			CStatusBar statusBar(_("Changing Pitch -- Channel ")+istring(++channelsDoneCount)+"/"+istring(actionSound->countChannels()),0,selectionLength,true); 
	
			sample_pos_t srcPos=prepareForUndo ? 0 : start;
			const CRezPoolAccesser src=prepareForUndo ? actionSound->sound->getTempAudio(tempAudioPoolKey,i) : actionSound->sound->getAudio(i);

			CRezPoolAccesser dest=actionSound->sound->getAudio(i);

			TPitchChanger<CRezPoolAccesser> changer(src,srcPos,selectionLength,deltaSemitones,actionSound->sound->getSampleRate(),1,0);

			// these are libSoundTouch specific
			changer.setSetting(SETTING_USE_AA_FILTER,useAntiAliasFilter);
			changer.setSetting(SETTING_AA_FILTER_LENGTH,antiAliasFilterLength);
			changer.setSetting(SETTING_USE_QUICKSEEK,useQuickSeek);
			changer.setSetting(SETTING_SEQUENCE_MS,sequenceLength);
			changer.setSetting(SETTING_SEEKWINDOW_MS,seekingWindowLength);
			changer.setSetting(SETTING_OVERLAP_MS,overlapLength);

			for(sample_pos_t t=0;t<selectionLength;t++)
			{
				dest[t+start]=changer.getSample();

				if(statusBar.update(t))
				{ // cancelled
					if(prepareForUndo)
						undoActionSizeSafe(actionSound);
					else
						actionSound->sound->invalidatePeakData(i,actionSound->start,t+start);
					return false;
				}
			}
		}
	}

	return true;
#else
	throw EUserMessage(_("No library was found at configure time to enable pitch changing"));
#endif
}

AAction::CanUndoResults CChangePitchAction::canUndo(const CActionSound *actionSound) const
{
	return curYes;
}

void CChangePitchAction::undoActionSizeSafe(const CActionSound *actionSound)
{
	restoreSelectionFromTempPools(actionSound,actionSound->start,actionSound->selectionLength());
}



// ---------------------------------------------

CChangePitchActionFactory::CChangePitchActionFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog) :
	AActionFactory(N_("Change Pitch"),_("Change Pitch"),channelSelectDialog,dialog)
{
}

CChangePitchActionFactory::~CChangePitchActionFactory()
{
}

CChangePitchAction *CChangePitchActionFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return new CChangePitchAction(
		this,
		actionSound,
		actionParameters->getValue<float>("Semitones"),
		actionParameters->getValue<bool>("Use Anti-alias Filter"),
		actionParameters->getValue<unsigned>("Anti-alias Filter Length"),
		actionParameters->getValue<bool>("Use Quick Seek"),
		actionParameters->getValue<unsigned>("Sequence Length"),
		actionParameters->getValue<unsigned>("Seek Window Length"),
		actionParameters->getValue<unsigned>("Overlap Length")
	);
}

