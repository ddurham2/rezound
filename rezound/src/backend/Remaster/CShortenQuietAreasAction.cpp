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

#include "CShortenQuietAreasAction.h"

#include "../CActionParameters.h"
#include "../unit_conv.h"
#include "../DSP/LevelDetector.h"

#include <istring>

#include <utility>
#include <algorithm>


CShortenQuietAreasAction::CShortenQuietAreasAction(const AActionFactory *factory,const CActionSound *actionSound,const float _quietThreshold,const float _quietTime,const float _unquietTime,const float _detectorWindow,const float _shortenFactor) :
	AAction(factory,actionSound),
	quietThreshold(_quietThreshold),
	quietTime(_quietTime),
	unquietTime(_unquietTime),
	detectorWindow(_detectorWindow),
	shortenFactor(_shortenFactor),

	undoRemoveLength(0)
{
	if(shortenFactor<0.0 || shortenFactor>1.0)
		throw runtime_error(string(__func__)+" shortenFactor is out of range: "+istring(shortenFactor));
}

CShortenQuietAreasAction::~CShortenQuietAreasAction()
{
}

bool CShortenQuietAreasAction::doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo)
{
	if(shortenFactor==1.0)
		return true;

	const sample_pos_t start=actionSound->start;
	const sample_pos_t stop=actionSound->stop;
	const sample_pos_t selectionLength=actionSound->selectionLength();

	undoRemoveLength=selectionLength; // we shrink this each time space is removed

	moveSelectionToTempPools(actionSound,mmSelection,actionSound->selectionLength(),(sample_pos_t)quietTime); // fudge by quietTime.. we might read past the end for crossfading

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
	 *            ---     <=quiet threshold      ---     >=quiet time      ---    >quiet treshold      ---
	 *  -------> | 0 | -----------------------> | 1 | ------------------> | 2 | --------------------> | 3 |
	 *            ---                            ---   [position noted     ---                         ---
	 *             ^                                     within audio]                                  |
	 *             |                                                                                    |
	 *             |                                                                                    |
	 *             |                         shorten area between noted position and here               |
	 *              ------------------------------------------------------------------------------------
	 *                                                    >=unquiet time                           
	 */

	int state=0;
	sample_pos_t dQuietBeginPos=start;
	sample_pos_t dQuietEndPos;
	sample_pos_t sQuietBeginPos=0;
	sample_pos_t sQuietEndPos;
	sample_pos_t quietCounter;
	sample_pos_t unquietCounter;
	CDSPRMSLevelDetector levelDetector(max((sample_pos_t)1,ms_to_samples(detectorWindow,actionSound->sound->getSampleRate())));

	auto_array<const CRezPoolAccesser> srces(MAX_CHANNELS);
	auto_array<const CRezPoolAccesser> alt_srces(MAX_CHANNELS); // used in order to be more efficient.. the crossfade code reads from two positions in the src data
	
	sample_pos_t destPos=start;
	auto_array<CRezPoolAccesser> dests(MAX_CHANNELS);

	// create accessors to write to
	const unsigned channelCount=actionSound->sound->getChannelCount();
	for(unsigned i=0;i<channelCount;i++)
	{
		srces[i]=new CRezPoolAccesser(actionSound->sound->getTempAudio(tempAudioPoolKey,i));
		alt_srces[i]=new CRezPoolAccesser(actionSound->sound->getTempAudio(tempAudioPoolKey,i));
		dests[i]=new CRezPoolAccesser(actionSound->sound->getAudio(i));
	}

	CStatusBar statusBar(_("Shortening Quiet Areas"),0,selectionLength,true);
	sample_pos_t srcPos;
	for(srcPos=0;srcPos<selectionLength;srcPos++,destPos++)
	{
		if(statusBar.update(srcPos))
		{ // cancelled
			if(prepareForUndo)
				undoActionSizeSafe(actionSound);
			else
			{
				for(unsigned i=0;i<channelCount;i++)
					actionSound->sound->invalidatePeakData(i,actionSound->start,destPos);
			}

			return false;
		}

		const sample_t s=(*(srces[0]))[srcPos]; // ??? which channel to use or both? (prompt the user!)
		const mix_sample_t l=levelDetector.readLevel(s);

		switch(state)
		{
		case 0: // waiting for level to fall below threshold
			if(l<=quietThreshold)
			{
				state=1;
				quietCounter=0;
				dQuietBeginPos=destPos;
				sQuietBeginPos=srcPos;
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
				// position of quiet areas start is noted because dQuietBeginPos has been updated in state 0
			}
			else
				quietCounter++;

			break;

		case 2: // level has been below the threshold for quiet time samples, now waiting for it to go back above the threshold
			if(l>quietThreshold)
			{
				state=3;
				unquietCounter=0;
				dQuietEndPos=destPos;
				sQuietEndPos=srcPos;
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

				alterQuietAreaLength(actionSound,destPos,unquietCounter,dQuietBeginPos,dQuietEndPos,sQuietBeginPos,sQuietEndPos,srces,alt_srces,dests);
			}
			else
				unquietCounter++;

			break;
		}

		// copying the data as we go
		(*(dests[0]))[destPos]=s;
		for(unsigned i=1;i<channelCount;i++)
			(*(dests[i]))[destPos]=(*(srces[i]))[srcPos];
	}

	if(state==2)
	{ // were left within quiet area but it never remained unquiet long enough to take the arc back to state 0, but we still should shorten the quiet area

		// this is what happens wend going from state 2 -> 3 .. except we subtract 1 because we don't want to overstep the boundary
		unquietCounter=0;
		dQuietEndPos=destPos-1;
		sQuietEndPos=srcPos-1;

		alterQuietAreaLength(actionSound,destPos,unquietCounter,dQuietBeginPos,dQuietEndPos,sQuietBeginPos,sQuietEndPos,srces,alt_srces,dests);
	}

	if(!prepareForUndo)
		freeAllTempPools(); // free the temp pools if we don't need to keep the backup copy around

	// new selectStop position
	actionSound->stop=destPos-1;

	return true;
}

void CShortenQuietAreasAction::alterQuietAreaLength(CActionSound *actionSound,sample_pos_t &destPos,const sample_pos_t unquietCounter,const sample_pos_t dQuietBeginPos,const sample_pos_t dQuietEndPos,const sample_pos_t sQuietBeginPos,const sample_pos_t sQuietEndPos,auto_array<const CRezPoolAccesser> &srces,auto_array<const CRezPoolAccesser> &alt_srces,auto_array<CRezPoolAccesser> &dests)
{
	// it's time to alter length of quiet area 
	// 	- remove an appropriately sized section between the dQuietBeginPos and dQuietEndPos
	// 	- create a crossfade using data from srces

	// the crossfade looks something like:
	//
	//   before delete region          |    within delete region     |    after delete region
	//                          \      |                            /|                         
	//                            \    |                          /  |                         
	//                              \  |                        /    |                         
	//                                \|                      /      |                         
	//
	//                         |_______|                     |_______|
	//                       fade 1 region                 fade 2 region
	//                        fading out                     fading in
	//
	// when the region to be deleted is deleted, then the two points in time at the two vertical positions become the same point in time
	// and the crossfade is the result of adding the signals from fade 1 and fade 2 together
	// fade 1's and fade 2's covered region may overlap if the region to be deleted is smaller than half the fade
	//
	// the implementation does the delete first, then goes back to the src to calculate the data surounding the point of deletion

	const unsigned channelCount=actionSound->sound->getChannelCount();

	const sample_pos_t dQuietAreaLength=(dQuietEndPos-dQuietBeginPos)+1;

	if(dQuietAreaLength>1)
	{ // don't do anything that at least doesn't affect two samples

		const sample_pos_t dDeleteLength=(sample_pos_t)sample_fpos_round((sample_fpos_t)dQuietAreaLength*(1.0-shortenFactor));
		const sample_pos_t dDeletePos=dQuietBeginPos+((dQuietAreaLength-dDeleteLength)/2); // split the different
		sample_pos_t sDeletePos=sQuietBeginPos+((dQuietAreaLength-dDeleteLength)/2); // same as dDeletePos except mapped into src space instead of dest space

		actionSound->sound->removeSpace(dDeletePos,dDeleteLength);
		undoRemoveLength-=dDeleteLength;
		destPos-=dDeleteLength;

		// the crossfade time is based on the "Must Remain Quiet For" parameter (but not more than we have remained unquiet, otherwise we'll write into a previous crossfade) and a minimum of 1ms (but an even smaller minimum if we don't have enough data in srces to do 1ms of crossfade)
		const sample_pos_t crossfadeTime=min(sDeletePos, max((sample_pos_t)(actionSound->sound->getSampleRate()/1000), min(unquietCounter,(sample_pos_t)quietTime) ));
		if(crossfadeTime>0)
		{

			sample_pos_t fade1Pos=sDeletePos-crossfadeTime;
			sample_pos_t fade2Pos=(sDeletePos+dDeleteLength)-crossfadeTime;
			sample_pos_t writePos=dDeletePos-crossfadeTime;

			for(sample_pos_t t=0;t<crossfadeTime;t++)
			{
				const float g=(float)t/(float)crossfadeTime;

				for(unsigned i=0;i<channelCount;i++)
					(*(dests[i]))[writePos]=ClipSample( (1.0-g)*( (*(srces[i]))[fade1Pos]) + g*( (*(alt_srces[i]))[fade2Pos]) );

				fade1Pos++;
				fade2Pos++;
				writePos++;
			}
		}
	}
}

AAction::CanUndoResults CShortenQuietAreasAction::canUndo(const CActionSound *actionSound) const
{
	return curYes;
}

void CShortenQuietAreasAction::undoActionSizeSafe(const CActionSound *actionSound)
{
	if(shortenFactor==1.0)
		return;

	restoreSelectionFromTempPools(actionSound,actionSound->start,undoRemoveLength);
}


// --------------------------------------------------

CShortenQuietAreasActionFactory::CShortenQuietAreasActionFactory(AActionDialog *normalDialog) :
	AActionFactory(N_("Shorten Quiet Areas"),_("Shorten Quiet Areas with Cues"),NULL,normalDialog)
{
}

CShortenQuietAreasActionFactory::~CShortenQuietAreasActionFactory()
{
}

CShortenQuietAreasAction *CShortenQuietAreasActionFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return new CShortenQuietAreasAction(
		this,
		actionSound,
		actionParameters->getValue<float>("Threshold for Quiet"),
		actionParameters->getValue<float>("Must Remain Quiet for"),
		actionParameters->getValue<float>("Must Remain Unquiet for"),
		actionParameters->getValue<float>("Level Detector Window Time"),
		actionParameters->getValue<float>("Shorten Found Area To")
		);
}


