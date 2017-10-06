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

#include "CResampleAction.h"

#include "../CActionSound.h"
#include "../CActionParameters.h"

#include "../DSP/TSoundStretcher.h"

CResampleAction::CResampleAction(const AActionFactory *factory,const CActionSound *actionSound,const unsigned _newSampleRate) :
	AAction(factory,actionSound),
	undoRemoveLength(0),
	newSampleRate(_newSampleRate),
	oldSampleRate(0)
{
	if(newSampleRate<100 || newSampleRate>500000)
		throw runtime_error(string(__func__)+" -- an unlikely sample rate: +"+istring(newSampleRate));
}

CResampleAction::~CResampleAction()
{
}

bool CResampleAction::doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo)
{
	const sample_pos_t oldLength=actionSound->sound->getLength();
	oldSampleRate=actionSound->sound->getSampleRate();
	const sample_pos_t newLength=(sample_pos_t)((sample_fpos_t)oldLength/oldSampleRate*newSampleRate);
	// ??? need to make sure this doesn't put it past the max length.. should be handled by when CSound::addSpace eventually gets called

	if(newSampleRate==oldSampleRate)
		return false;

	// we are gonna use the undo selection copy as the source buffer while changing the rate
	// we have a fudgeFactor of 1 because we may read ahead up to 1 when changing the rate
	moveSelectionToTempPools(actionSound,mmAll,newLength,1);

	undoRemoveLength=newLength;

	unsigned channelsDoneCount=0;
	for(unsigned i=0;i<actionSound->sound->getChannelCount();i++)
	{
		if(actionSound->doChannel[i])
		{
			CStatusBar statusBar(_("Changing Sample Rate -- Channel ")+istring(++channelsDoneCount)+"/"+istring(actionSound->countChannels()),0,newLength,true); 
	
			// here, we're using the undo data as a source from which to calculate the new data
			const CRezPoolAccesser src=actionSound->sound->getTempAudio(tempAudioPoolKey,i);
			CRezPoolAccesser dest=actionSound->sound->getAudio(i);

			TSoundStretcher<const CRezPoolAccesser> stretcher(src,0,oldLength,newLength,1,0,true);
			for(sample_pos_t t=0;t<newLength;t++)
			{
				dest[t]=stretcher.getSample();

				if(statusBar.update(t))
				{ // cancelled
					restoreSelectionFromTempPools(actionSound,0,undoRemoveLength);
					return false;
				}
			}
		}
	}

	// adjust all cue positions (even anchored ones)
	for(size_t t=0;t<actionSound->sound->getCueCount();t++)
		actionSound->sound->setCueTime(t,(sample_pos_t)sample_fpos_round((sample_fpos_t)actionSound->sound->getCueTime(t)/oldSampleRate*newSampleRate));
	
	// adjust start and stop positions
	actionSound->stop=(sample_pos_t)sample_fpos_round((sample_fpos_t)actionSound->stop/oldSampleRate*newSampleRate);
	actionSound->start=(sample_pos_t)sample_fpos_round((sample_fpos_t)actionSound->start/oldSampleRate*newSampleRate);

	actionSound->sound->setSampleRate(newSampleRate);

	if(!prepareForUndo)
		freeAllTempPools();

	return true;
}

AAction::CanUndoResults CResampleAction::canUndo(const CActionSound *actionSound) const
{
	return curYes;
}

void CResampleAction::undoActionSizeSafe(const CActionSound *actionSound)
{
	restoreSelectionFromTempPools(actionSound,0,undoRemoveLength);
	actionSound->sound->setSampleRate(oldSampleRate);
}



// ---------------------------------------------

CResampleActionFactory::CResampleActionFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog) :
	AActionFactory(N_("Resample"),_("Change Sample Rate"),channelSelectDialog,dialog,true,false)
{
	selectionPositionsAreApplicable=false;
}

CResampleActionFactory::~CResampleActionFactory()
{
}

CResampleAction *CResampleActionFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return new CResampleAction(this,actionSound,actionParameters->getValue<unsigned>("New Sample Rate"));
}

