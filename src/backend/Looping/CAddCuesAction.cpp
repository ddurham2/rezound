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

#include "CAddCuesAction.h"

#include "../CActionParameters.h"

CAddCuesAction::CAddCuesAction(const AActionFactory *factory,const CActionSound *actionSound,const string _cueName,const unsigned _cueCount,const bool _anchoredInTime) :
	AAction(factory,actionSound),
	cueName(_cueName),
	cueCount(_cueCount),
	timeInterval(0.0),
	anchoredInTime(_anchoredInTime)
{
}

CAddCuesAction::CAddCuesAction(const AActionFactory *factory,const CActionSound *actionSound,const string _cueName,const double _timeInterval,const bool _anchoredInTime) :
	AAction(factory,actionSound),
	cueName(_cueName),
	cueCount(0),
	timeInterval(_timeInterval),
	anchoredInTime(_anchoredInTime)
{
}

CAddCuesAction::~CAddCuesAction()
{
}

bool CAddCuesAction::doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo)
{
	const sample_pos_t start=actionSound->start;
	const sample_pos_t stop=actionSound->stop;
	const sample_pos_t selectionLength=actionSound->selectionLength();

	if(cueCount==0)
	{ // add a cue every X seconds within the selection
		const sample_pos_t interval=(sample_pos_t)sample_fpos_round(timeInterval*actionSound->sound->getSampleRate());

		if(interval<=0)
			throw runtime_error(_("The time interval was zero"));
		if((selectionLength/interval)>10000)
			throw runtime_error(_("The time interval was so small for the selected area that an unlikely desired amount of cues would be added"));

		sample_pos_t pos=start;
		do
		{
			actionSound->sound->addCue(cueName,pos,anchoredInTime);
			pos+=interval;
		} while(pos<stop);
	}
	else
	{ // add N cues within the selection
		sample_fpos_t interval=(sample_fpos_t)selectionLength/cueCount;
		sample_fpos_t position=start;
		for(size_t t=0;t<cueCount;t++)
		{
			actionSound->sound->addCue(cueName,(sample_pos_t)sample_fpos_round(position),anchoredInTime);
			position+=interval;
		}
	}

	return true;
}

AAction::CanUndoResults CAddCuesAction::canUndo(const CActionSound *actionSound) const
{
	return curYes;
}

void CAddCuesAction::undoActionSizeSafe(const CActionSound *actionSound)
{
	// it is not necessary to do anything here because AAction handles restoring all the cues
}


// --------------------------------------------------

CAddNCuesActionFactory::CAddNCuesActionFactory(AActionDialog *normalDialog) :
	AActionFactory(N_("Add N Cues"),_("Add N Cues at Evenly Spaced Intervals"),NULL,normalDialog)
{
}

CAddNCuesActionFactory::~CAddNCuesActionFactory()
{
}

CAddCuesAction *CAddNCuesActionFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return new CAddCuesAction(
		this,
		actionSound,
		actionParameters->getValue<string>("Cue Name"),
		actionParameters->getValue<unsigned>("Cue Count"),
		actionParameters->getValue<bool>("Anchor Cues in Time")
		);
}


// --------------------------------------------------

CAddTimedCuesActionFactory::CAddTimedCuesActionFactory(AActionDialog *normalDialog) :
	AActionFactory(N_("Add Timed Cues"),_("Add a Cue Every X Seconds"),NULL,normalDialog)
{
}

CAddTimedCuesActionFactory::~CAddTimedCuesActionFactory()
{
}

CAddCuesAction *CAddTimedCuesActionFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return new CAddCuesAction(
		this,
		actionSound,
		actionParameters->getValue<string>("Cue Name"),
		actionParameters->getValue<double>("Time Interval"),
		actionParameters->getValue<bool>("Anchor Cues in Time")
		);
}


