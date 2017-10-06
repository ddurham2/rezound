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

#include "CMarkQuietAreasAction.h"

#include "../CActionParameters.h"
#include "../unit_conv.h"
#include "../DSP/LevelDetector.h"

#include <utility>

CMarkQuietAreasAction::CMarkQuietAreasAction(const AActionFactory *factory,const CActionSound *actionSound,const float _quietThreshold,const float _quietTime,const float _unquietTime,const float _detectorWindow,const string _quietBeginCue,const string _quietEndCue) :
	AAction(factory,actionSound),
	quietThreshold(_quietThreshold),
	quietTime(_quietTime),
	unquietTime(_unquietTime),
	detectorWindow(_detectorWindow),
	quietBeginCue(_quietBeginCue),
	quietEndCue(_quietEndCue)
{
}

CMarkQuietAreasAction::~CMarkQuietAreasAction()
{
}

bool CMarkQuietAreasAction::doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo)
{
	const sample_pos_t start=actionSound->start;
	const sample_pos_t stop=actionSound->stop;
	const sample_t quietThreshold=dBFS_to_amp(this->quietThreshold);
	const sample_pos_t quietTime=max((sample_pos_t)1,ms_to_samples(this->quietTime,actionSound->sound->getSampleRate()));
	const sample_pos_t unquietTime=ms_to_samples(this->unquietTime,actionSound->sound->getSampleRate());


	/*
	 * The algorithm follows this state machine:
	 *
	 *                     >quiet threshold                                      <=quiet threshold
	 *              ------------------------------                           ---------------------------
	 *             |                              |                         |                           |
	 *             |                              |                         |                           |
	 *             V                              |                         V                           |
	 *
	 * ---     <=quiet threshold      ---     >=quiet time      ---    >quiet treshold      ---
	 *  -------> | 0 | -----------------------> | 1 | ------------------> | 2 | --------------------> | 3 |
	 *            ---                            ---     add begin cue     ---                         ---
	 *             ^                                                                                    |
	 *             |                                                                                    |
	 *             |                                        add end cue                                 |
	 *              ------------------------------------------------------------------------------------
	 *                                                    >=unquiet time                           
	 */

	// ??? might want to limit the number of cues added by checking for a max before each add

	int state=0;
	sample_pos_t quietBeginPos=start;
	sample_pos_t quietEndPos;
	sample_pos_t quietCounter;
	sample_pos_t unquietCounter;
	CDSPRMSLevelDetector levelDetector(max((sample_pos_t)1,ms_to_samples(detectorWindow,actionSound->sound->getSampleRate())));
	const CRezPoolAccesser src=actionSound->sound->getAudio(0); // ??? which channel to use or both? (prompt the user!)
	for(sample_pos_t pos=start;pos<=stop;pos++)
	{
		const mix_sample_t l=levelDetector.readLevel(src[pos]);

		switch(state)
		{
		case 0: // waiting for level to fall below threshold
			if(l<=quietThreshold)
			{
				state=1;
				quietCounter=0;
				quietBeginPos=pos;
			}

			break;

		case 1: // waiting for level that is below threshold to remain that way for quiet time
			if(l>quietThreshold)
			{	// level wasn't below threshold for long enough
				state=0;
			}
			else if(quietCounter>=quietTime)
			{ 	// level has now been below the threshold for long enough
				state=2;
				actionSound->sound->addCue(quietBeginCue,quietBeginPos,false);
			}
			else
				quietCounter++;

			break;

		case 2: // level has been below the threshold for quiet time samples, now waiting for it to go back above the threshold
			if(l>quietThreshold)
			{
				state=3;
				unquietCounter=0;
				quietEndPos=pos;
			}

			break;

		case 3: // waiting for level to remain above the quiet threshold for unquiet time samples
			if(l<=quietThreshold)
			{ // level has fallen back below the treshold, start over waiting for unquiet time
				state=2;
			}
			else if(unquietCounter>=unquietTime)
			{ // level has now been above the threshold for unquiet time samples
				state=0;
				actionSound->sound->addCue(quietEndCue,quietEndPos,false);
			}
			else
				unquietCounter++;

			break;
		}

	}

	return true;
}

AAction::CanUndoResults CMarkQuietAreasAction::canUndo(const CActionSound *actionSound) const
{
	return curYes;
}

void CMarkQuietAreasAction::undoActionSizeSafe(const CActionSound *actionSound)
{
	// it is not necessary to do anything here because AAction handles restoring all the cues
}


// --------------------------------------------------

CMarkQuietAreasActionFactory::CMarkQuietAreasActionFactory(AActionDialog *normalDialog) :
	AActionFactory(N_("Mark Quiet Areas"),_("Mark Quiet Areas with Cues"),NULL,normalDialog)
{
}

CMarkQuietAreasActionFactory::~CMarkQuietAreasActionFactory()
{
}

CMarkQuietAreasAction *CMarkQuietAreasActionFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return new CMarkQuietAreasAction(
		this,
		actionSound,
		actionParameters->getValue<float>("Threshold for Quiet"),
		actionParameters->getValue<float>("Must Remain Quiet for"),
		actionParameters->getValue<float>("Must Remain Unquiet for"),
		actionParameters->getValue<float>("Level Detector Window Time"),
		actionParameters->getValue<string>("Quiet Begin Cue Name"),
		actionParameters->getValue<string>("Quiet End Cue Name")
		);
}


