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

#include "AAction.h"

#include <math.h>

#include <stdexcept>

#include <istring>

#include "CLoadedSound.h"
#include "AStatusComm.h"
#include "CSoundPlayerChannel.h"

#include "CActionSound.h"
#include "AActionDialog.h"

#include "settings.h"




// ----------------------------------------------------------------------
// -- AActionFactory ----------------------------------------------------
// ----------------------------------------------------------------------

AActionFactory::AActionFactory(const string _actionName,const string _actionDescription,const bool __hasAdvancedMode,AActionDialog *_channelSelectDialog,AActionDialog *_normalDialog,AActionDialog *_advancedDialog,bool _willResize,bool _crossfadeEdgesIsApplicable) :
	actionName(_actionName),
	actionDescription(_actionDescription),

	channelSelectDialog(_channelSelectDialog),
	normalDialog(_normalDialog),
	advancedDialog(_advancedDialog),

	_hasAdvancedMode(__hasAdvancedMode),
	willResize(_willResize),
	crossfadeEdgesIsApplicable(_crossfadeEdgesIsApplicable)
{
}

// ??? need to pass a flag for 'allowUndo'
bool AActionFactory::performAction(CLoadedSound *loadedSound,CActionParameters *actionParameters,bool showChannelSelectDialog,bool advancedMode)
{
	bool ret=true;
	try
	{
		// set wasShown to false for all dialogs
		if(channelSelectDialog!=NULL)
			channelSelectDialog->wasShown=false;
		if(normalDialog!=NULL)
			normalDialog->wasShown=false;
		if(advancedDialog!=NULL)
			advancedDialog->wasShown=false;

		// if this action doesn't have an advanced mode, we don't do anything
		// this is mainly an interface issue... If the user say right-clicks a
		// button expecting advanced mode and it doesn't have one, but does do
		// something, then they'll always be guessing if that did anything 
		// different than left-click
		if(advancedMode && !_hasAdvancedMode)
		{
			Message("This action does not have an advanced mode");
			return(false);
		}

		if(!doPreActionSetup(loadedSound))
			return(false);

		CActionSound actionSound(loadedSound->channel,gCrossfadeEdges);
		if(!showChannelSelectDialog || channelSelectDialog==NULL || (channelSelectDialog->wasShown=true,channelSelectDialog->show(&actionSound,actionParameters)))
		{
			AAction *action=NULL;

			if(advancedMode)
			{

				if(advancedDialog!=NULL)
				{
					if(!(advancedDialog->wasShown=true,advancedDialog->show(&actionSound,actionParameters)))
						return(false);
				}

				action=manufactureAction(actionSound,actionParameters,true);
				ret&=action->doAction(loadedSound->channel,true,willResize,crossfadeEdgesIsApplicable);
			}
			else
			{
				if(normalDialog!=NULL)
				{
					if(!(normalDialog->wasShown=true,normalDialog->show(&actionSound,actionParameters)))
						return(false);
				}

				action=manufactureAction(actionSound,actionParameters,false);
				ret&=action->doAction(loadedSound->channel,true,willResize,crossfadeEdgesIsApplicable);
			}

			/*
			if(prepareForUndo)
			{
			*/
				AAction::CanUndoResults result=action->canUndo();
				if(ret && result==AAction::curYes)
					loadedSound->actions.push(action);
				else 
				{
					if(result==AAction::curNo)
						Warning("not handling non-undoable actions currently.... should I clear the actions stack?");

					delete action;
				}
			/*
			}
			*/

			return(ret);
		}

		return(false);
	}
	catch(exception &e)
	{
		Error(e.what());
		return(false);
	}
}

const string &AActionFactory::getName() const
{
	return(actionName);
}

const string &AActionFactory::getDescription() const
{
	return(actionDescription);
}

const bool AActionFactory::hasAdvancedMode() const
{
	return(_hasAdvancedMode);
}






// ----------------------------------------------------------------------
// -- AAction -----------------------------------------------------------
// ----------------------------------------------------------------------

vector<ASoundClipboard *> AAction::clipboards;

// ??? perhaps we could hold on to a pointer to our factory to easily be able to rePEAT the action

AAction::AAction(const CActionSound &_actionSound) :
	tempAudioPoolKey(-1),
	tempAudioPoolKey2(-1),

	actionSound(_actionSound),
	willResize(false),
	done(false),
	oldSelectStart(NIL_SAMPLE_POS),
	oldSelectStop(NIL_SAMPLE_POS),

	origIsModified(true),

	restoreMoveMode(mmInvalid),
	restoreWhere(0),restoreLength(0),
	restoreLength2(0),
	restoreTotalLength(0),


	tempCrossfadePoolKeyStart(-1),
	tempCrossfadePoolKeyStop(-1),
	crossfadeStart(0),
	crossfadeStartLength(0),
	crossfadeStop(0),
	crossfadeStopLength(0)
{
}

AAction::~AAction()
{
	freeAllTempPools();
}

bool AAction::doesWarrantSaving() const
{
	return true; // by default
}

bool AAction::getResultingCrossfadePoints(const CActionSound &actionSound,sample_pos_t &start,sample_pos_t &stop)
{
	start=actionSound.start;
	stop=actionSound.stop;
	return(true);
}

bool AAction::doAction(CSoundPlayerChannel *channel,bool prepareForUndo,bool _willResize,bool crossfadeEdgesIsApplicable)
{
	if(done)
		throw(runtime_error(string(__func__)+" -- action has already been done"));

	done=true;

	origIsModified=actionSound.sound->isModified();
	actionSound.sound->setIsModified(actionSound.sound->isModified() || doesWarrantSaving());

	willResize=_willResize;

	// even though the AAction derived class might not resize the sound, we will if crossfading is to be done
	willResize|= (actionSound.doCrossfadeEdges!=cetNone && crossfadeEdgesIsApplicable);

	if(willResize)
		actionSound.sound->lockForResize();
	else
		actionSound.sound->lockSize();

	try
	{
		// save the cues so that if they are modified by the action, then they can be restored at undo
		if(prepareForUndo)
		{
			restoreCues.clear();
			for(size_t t=0;t<actionSound.sound->getCueCount();t++)
				restoreCues.push_back(CSound::RCue(actionSound.sound->getCueName(t).c_str(),actionSound.sound->getCueTime(t),actionSound.sound->isCueAnchored(t)));
		}

		if(channel!=NULL)
		{
			// save the selection positions
			oldSelectStart=channel->getStartPosition();
			oldSelectStop=channel->getStopPosition();

			// save the output routing informations
			if(prepareForUndo)
				restoreOutputRoutes=channel->getOutputRoutes();
		}

		CActionSound _actionSound(actionSound);

		if(crossfadeEdgesIsApplicable)
			prepareForInnerCrossfade(_actionSound);

	
		bool ret=doActionSizeSafe(_actionSound,prepareForUndo && canUndo()==curYes);

		// make sure that the start and stop positions are in range after the action
		if(_actionSound.start<0)
			_actionSound.start=0;
		else if(_actionSound.start>=_actionSound.sound->getLength())
			_actionSound.start=_actionSound.sound->getLength()-1;
	
		if(_actionSound.stop<0)
			_actionSound.stop=0;
		else if(_actionSound.stop>=_actionSound.sound->getLength())
			_actionSound.stop=_actionSound.sound->getLength()-1;
	
		if(crossfadeEdgesIsApplicable)
		{
			crossfadeEdges(_actionSound);
	
			// again, make sure that the start and stop positions are in range after the crossfade
			if(_actionSound.start<0)
				_actionSound.start=0;
			else if(_actionSound.start>=_actionSound.sound->getLength())
				_actionSound.start=_actionSound.sound->getLength()-1;
		
			if(_actionSound.stop<0)
				_actionSound.stop=0;
			else if(_actionSound.stop>=_actionSound.sound->getLength())
				_actionSound.stop=_actionSound.sound->getLength()-1;
		}
	
		if(channel!=NULL)
		{
			setSelection(_actionSound.start,_actionSound.stop,channel);

			vector<int16_t> dummy;
			channel->updateAfterEdit(dummy);
		}

		if(willResize)
			actionSound.sound->unlockForResize();
		else
			actionSound.sound->unlockSize();

		actionSound.sound->flush();

		return(ret);
	}
	catch(EUserMessage &e)
	{
		if(willResize)
			actionSound.sound->unlockForResize();
		else
			actionSound.sound->unlockSize();

		Message(e.what());
		return(false);
	}
	catch(...)
	{
		if(willResize)
			actionSound.sound->unlockForResize();
		else
			actionSound.sound->unlockSize();

		throw;
	}
}

// ??? probably need to do what we can to be able to redo the action

void AAction::undoAction(CSoundPlayerChannel *channel)
{
	if(!done)
		throw(runtime_error(string(__func__)+" -- action has not yet been done"));

	// willResize is set from doAction, and it's needed value should be consistant with the way the action will be undo
	if(willResize)
		actionSound.sound->lockForResize();
	else
		actionSound.sound->lockSize();

	try
	{
		if(canUndo()!=curYes)
			throw(runtime_error(string(__func__)+" -- cannot undo action"));

		uncrossfadeEdges();

		const CActionSound _actionSound(actionSound);

		undoActionSizeSafe(_actionSound);

		// make sure that the start and stop positions are in range after undoing the action
		if(_actionSound.start<0)
			_actionSound.start=0;
		else if(_actionSound.start>=_actionSound.sound->getLength())
			_actionSound.start=_actionSound.sound->getLength()-1;
	
		if(_actionSound.stop<0)
			_actionSound.stop=0;
		else if(_actionSound.stop>=_actionSound.sound->getLength())
			_actionSound.stop=_actionSound.sound->getLength()-1;
	

		// restore the cues
		actionSound.sound->clearCues();
		for(size_t t=0;t<restoreCues.size();t++)
			actionSound.sound->addCue(restoreCues[t].name,restoreCues[t].time,restoreCues[t].isAnchored);
		restoreCues.clear();


		// one thing this will do is restore the output routing information
		if(channel!=NULL)
			channel->updateAfterEdit(restoreOutputRoutes);


		// restore the selection position
		if(channel!=NULL && oldSelectStart!=NIL_SAMPLE_POS && oldSelectStop!=NIL_SAMPLE_POS)
			setSelection(oldSelectStart,oldSelectStop,channel);


		if(willResize)
			actionSound.sound->unlockForResize();
		else
			actionSound.sound->unlockSize();


		actionSound.sound->flush();

		done=false;
	}
	catch(EUserMessage &e)
	{
		if(willResize)
			actionSound.sound->unlockForResize();
		else
			actionSound.sound->unlockSize();

		Message(e.what());
	}
	catch(...)
	{
		if(willResize)
			actionSound.sound->unlockForResize();
		else
			actionSound.sound->unlockSize();

		throw;
	}
	actionSound.sound->setIsModified(origIsModified);
}

AAction::CanUndoResults AAction::canUndo() const
{
	return(canUndo(actionSound));
}

void AAction::setSelection(sample_pos_t start,sample_pos_t stop,CSoundPlayerChannel *channel)
{
	channel->setStartPosition(start);
	channel->setStopPosition(stop);
}

/*
	This method is given the sound after the derived class has
	performed the action.  Then we crossfade what's just before
	and what's just after the start and stop edges according
	to the global crossfade time(s)
*/
void AAction::crossfadeEdges(const CActionSound &actionSound)
{
	didCrossfadeEdges=cetNone;

	// might have the user specify some other reasons not to do the crossfade if say, the selection length is below the crossfade times or something
	if(actionSound.doCrossfadeEdges==cetInner)
		crossfadeEdgesInner(actionSound);
	else if(actionSound.doCrossfadeEdges==cetOuter)
		crossfadeEdgesOuter(actionSound);
}


/*
	An inner crossfade on the edges means that before the action is done
	data is saved from the inner edges of the selection, then after the 
	action is done, that saved data is then blended with a crossfade on
	top of the new selection.

	Except the way I implemented it was so that even if a delete is done
	the first part of the deleted selection will get crossfaded after what
	is beyond the new start==stop selection positions.  So, data outside
	the selection can still be modified, but not shortened
*/

void AAction::prepareForInnerCrossfade(const CActionSound &actionSound)
{
	if(actionSound.doCrossfadeEdges==cetInner)
	{
		bool allChannels[MAX_CHANNELS];
		for(size_t t=0;t<MAX_CHANNELS;t++)
			allChannels[t]=true;

		sample_pos_t startPos,stopPos;
		if(!getResultingCrossfadePoints(actionSound,startPos,stopPos))
			throw(EUserMessage("Cannot anticipate certain information necessary to do an inner crossfade for action"));

		sample_pos_t crossfadeStartTime=(sample_pos_t)(gCrossfadeStartTime*actionSound.sound->getSampleRate()/1000.0);
		sample_pos_t crossfadeStopTime=(sample_pos_t)(gCrossfadeStopTime*actionSound.sound->getSampleRate()/1000.0);

		// don't go off the end of the sound
		crossfadeStartTime=min(crossfadeStartTime,actionSound.sound->getLength()-startPos);

		// don't read before the beginning of the sound
		crossfadeStopTime=min(crossfadeStopTime,stopPos);




		// backup the area to crossfade after the start position
		crossfadeStart=startPos;

		if(startPos>0)
			crossfadeStartLength=crossfadeStartTime;
		else
			// since the crossfade will be with the very first sample, just let it start and not do the crossfade
			crossfadeStartLength=0;

		tempCrossfadePoolKeyStart=actionSound.sound->copyDataToTemp(allChannels,startPos,crossfadeStartLength);






		// backup the area to crossfade before the stop position
		crossfadeStop=stopPos-crossfadeStopTime;

		if(stopPos<actionSound.sound->getLength()-1)
			crossfadeStopLength=crossfadeStopTime;
		else
			// since the crossfade will be with the very last sample, just let it end and not do the crossfade
			crossfadeStopLength=0;

		tempCrossfadePoolKeyStop=actionSound.sound->copyDataToTemp(allChannels,stopPos-crossfadeStopTime,crossfadeStopLength);

	}
}

void AAction::crossfadeEdgesInner(const CActionSound &actionSound)
{
	bool allChannels[MAX_CHANNELS];
	for(size_t t=0;t<MAX_CHANNELS;t++)
		allChannels[t]=true;

	crossfadeMoveMul=1;

	sample_pos_t crossfadeStartTime=crossfadeStartLength;
	sample_pos_t crossfadeStopTime=crossfadeStopLength;

	if(actionSound.selectionLength()<(crossfadeStartLength+crossfadeStopLength))
		// crossfades would overlap.. fall back to selectionLength/2 as the start and stop crossfade times
		crossfadeStartTime=crossfadeStopTime= actionSound.selectionLength()/2;

	crossfadeStartTime=min(crossfadeStartTime,crossfadeStartLength);
	crossfadeStopTime=min(crossfadeStopTime,crossfadeStopLength);


	// ??? I don't like this... it's would be better if were just correct from the normal logic
	// special case
	if(actionSound.start==actionSound.stop)
		// min of what's possible and how much prepareForInnerCrossfade backed up
		crossfadeStartTime=min(actionSound.sound->getLength()-actionSound.start,crossfadeStartLength);

	// crossfade at the start position
	{

		// save what we're about to modify because undoAction needs the data the way it was
		// just after doAction
		int tempPoolKey=actionSound.sound->copyDataToTemp(allChannels,actionSound.start,crossfadeStartTime);

		for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
		{	
			CRezPoolAccesser dest=actionSound.sound->getAudio(i);
			const CRezPoolAccesser src=actionSound.sound->getTempAudio(tempCrossfadePoolKeyStart,i);

/* 
??? when I do careful listening on a crossfade after a 
delete of a selection on a sine wave, there's always a 
little click and it looks like it left out one sample, 
but I can't see in the code why... perhaps a careful 
run-through on paper would help
*/

// ??? I might wanna use x^.5 instead of x^2  ... something makes me think it would be better... fade out 1-x^.5 and fade in x^.5

			sample_pos_t srcPos=0;

			// crossfade with what already there and what's in the temp audio pool
			if(crossfadeStartTime>1) // avoid divide by zero
			for(sample_pos_t t=actionSound.start;t<actionSound.start+crossfadeStartTime;t++,srcPos++)
			{
				float g1=(float)srcPos/(float)(crossfadeStartTime-1);
				float g2=1.0-g1;
				if(gCrossfadeFadeMethod==cfmParabolic) { g1*=g1; g2*=g2; }
				dest[t]=(sample_t)(dest[t]*g1 + src[srcPos]*g2);
			}
		}

		// setup so undoCrossfade will restore the part we just modified
		actionSound.sound->removeTempAudioPools(tempCrossfadePoolKeyStart);
		tempCrossfadePoolKeyStart=tempPoolKey;
		crossfadeStart=actionSound.start;
		crossfadeStartLength=crossfadeStartTime;
	}
		
	// crossfade at stop position
	{
		int tempPoolKey=actionSound.sound->copyDataToTemp(allChannels,actionSound.stop-crossfadeStopTime+1,crossfadeStopTime);

		for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
		{	
			CRezPoolAccesser dest=actionSound.sound->getAudio(i);
			const CRezPoolAccesser src=actionSound.sound->getTempAudio(tempCrossfadePoolKeyStop,i);

			sample_pos_t srcPos=0;
			sample_pos_t srcOffset=crossfadeStopLength-crossfadeStopTime; // amount that we're not using of the stop.. so the *end* of the data will match up

			// crossfade with what already there and what's in the temp audio pool
			if(crossfadeStopTime>1) // avoid divide by zero
			for(sample_pos_t t=actionSound.stop-crossfadeStopTime+1;t<=actionSound.stop;t++,srcPos++)
			{
				float g1=(float)srcPos/(float)(crossfadeStopTime-1);
				float g2=1.0-g1;
				if(gCrossfadeFadeMethod==cfmParabolic) { g1*=g1; g2*=g2; }
				dest[t]=(sample_t)(dest[t]*g2 + src[srcPos]*g1);
			}
		}

		// setup so undoCrossfade will restore the part we just modified
		actionSound.sound->removeTempAudioPools(tempCrossfadePoolKeyStop);
		tempCrossfadePoolKeyStop=tempPoolKey;
		crossfadeStop=actionSound.stop-crossfadeStopTime+1;
		crossfadeStopLength=crossfadeStopTime;

	}
	
	didCrossfadeEdges=cetInner;
}

void AAction::crossfadeEdgesOuter(const CActionSound &actionSound)
{
	bool allChannels[MAX_CHANNELS];
	for(size_t t=0;t<MAX_CHANNELS;t++)
		allChannels[t]=true;

	crossfadeMoveMul=2;

	// crossfade at the start position
	sample_pos_t crossfadeTime=(sample_pos_t)(gCrossfadeStartTime*actionSound.sound->getSampleRate()/1000.0);
		// make crossfadeTime smaller if there isn't room around the start position to fully do the crossfade
	crossfadeTime=min(crossfadeTime,min(actionSound.start,(actionSound.sound->getLength()-actionSound.start)));

	if(crossfadeTime>0)
	{
		// we don't look at actionSound.whichChannels so that when we 
		// make the sound slightly shorter, it keeps the channels in sync

		// The two areas to be crossfaded are consecutive in the at start-crossfadeTime and spans for
		// 2*crossfade-time.  The second half of this regions is going to 'overlap' the first half and
		// a crossfade will be from one overlapping area to the other.  So, we need to move any cues 
		// existing in the second half of this region leftward by the crossfade-time
	
		for(size_t t=0;t<actionSound.sound->getCueCount();t++)
		{
			const sample_pos_t cueTime=actionSound.sound->getCueTime(t);
			if(cueTime>=actionSound.start && cueTime<actionSound.start+crossfadeTime)
				actionSound.sound->setCueTime(t,cueTime-crossfadeTime);
		}

		// backup the area to crossfade around the start position
		tempCrossfadePoolKeyStart=actionSound.sound->moveDataToTempAndReplaceSpace(allChannels,actionSound.start-crossfadeTime,crossfadeTime*2,crossfadeTime);

		// info to uncrossfadeEdges()
		crossfadeStart=actionSound.start-crossfadeTime;
		crossfadeStartLength=crossfadeTime;

		for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
		{	
			CRezPoolAccesser dest=actionSound.sound->getAudio(i);
			const CRezPoolAccesser src=actionSound.sound->getTempAudio(tempCrossfadePoolKeyStart,i);

			sample_pos_t srcPos=0;

			// fade out what was before the start position
			if(crossfadeTime>1) // avoid divide by zero
			for(sample_pos_t t=actionSound.start-crossfadeTime;t<actionSound.start;t++,srcPos++)
			{
				float g=1.0-((float)srcPos/(float)(crossfadeTime-1));
				if(gCrossfadeFadeMethod==cfmParabolic) g*=g;
				dest[t]=(sample_t)(src[srcPos]*g);
			}

			// fade in what was after the start position 
			// (not necesary to ClipSample since it's always a constant 1.0 gain
			if(crossfadeTime>1) // avoid divide by zero
			for(sample_pos_t t=actionSound.start-crossfadeTime;t<actionSound.start;t++,srcPos++)
			{
				float g=(float)(srcPos-crossfadeTime)/(float)(crossfadeTime-1);
				if(gCrossfadeFadeMethod==cfmParabolic) g*=g;
				dest[t]+=(sample_t)(src[srcPos]*g);
			}

		}

		actionSound.start-=crossfadeTime;
		actionSound.stop-=crossfadeTime;

		didCrossfadeEdges=cetOuter;
	}


	if(actionSound.start!=actionSound.stop)
	{ // don't do stop crossfade if they're the same point
	
		// crossfade at the stop position
		sample_pos_t crossfadeTime=(sample_pos_t)(gCrossfadeStopTime*actionSound.sound->getSampleRate()/1000.0);
			// make crossfadeTime smaller if there isn't room around the stop position to fully do the crossfade
		crossfadeTime=min(crossfadeTime,min(actionSound.stop,actionSound.sound->getLength()-(actionSound.stop+1)));

		if(crossfadeTime>0)
		{
			// move cues just as we did in the start case
			for(size_t t=0;t<actionSound.sound->getCueCount();t++)
			{
				const sample_pos_t cueTime=actionSound.sound->getCueTime(t);
				if(cueTime>=actionSound.stop && cueTime<actionSound.stop+crossfadeTime)
					actionSound.sound->setCueTime(t,cueTime-crossfadeTime);
			}

			// backup the area to crossfade around the stop position
			tempCrossfadePoolKeyStop=actionSound.sound->moveDataToTempAndReplaceSpace(allChannels,actionSound.stop-crossfadeTime,crossfadeTime*2,crossfadeTime);

			// info to uncrossfadeEdges()
			crossfadeStop=actionSound.stop-crossfadeTime;
			crossfadeStopLength=crossfadeTime;

			for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
			{	
				CRezPoolAccesser dest=actionSound.sound->getAudio(i);
				const CRezPoolAccesser src=actionSound.sound->getTempAudio(tempCrossfadePoolKeyStop,i);

				sample_pos_t srcPos=0;

				// fade out what was before the stop position
				if(crossfadeTime>1) // avoid divide by zero
				for(sample_pos_t t=actionSound.stop-crossfadeTime;t<actionSound.stop;t++,srcPos++)
				{
					float g=1.0-((float)srcPos/(float)(crossfadeTime-1));
					if(gCrossfadeFadeMethod==cfmParabolic) g*=g;
					dest[t]=(sample_t)(src[srcPos]*g);
				}

				// fade in what was after the stop position 
				// (not necesary to ClipSample since it's always a constant 1.0 gain
				if(crossfadeTime>1) // avoid divide by zero
				for(sample_pos_t t=actionSound.stop-crossfadeTime;t<actionSound.stop;t++,srcPos++)
				{
					float g=(float)(srcPos-crossfadeTime)/(float)(crossfadeTime-1);
					if(gCrossfadeFadeMethod==cfmParabolic) g*=g;
					dest[t]+=(sample_t)(src[srcPos]*g);
				}

			}

			didCrossfadeEdges=cetOuter;
		}
	}
}

void AAction::uncrossfadeEdges()
{
	/*
		Normally, for an inner crossfade there would be no need to restore the edges
		except that not all actions restore the data using temp pools (i.e. rotate and reverse)
	*/
	if(didCrossfadeEdges==cetOuter || didCrossfadeEdges==cetInner)
	{
		bool allChannels[MAX_CHANNELS];
		for(size_t t=0;t<MAX_CHANNELS;t++)
			allChannels[t]=true;

		if(crossfadeStopLength>0)
			actionSound.sound->removeSpaceAndMoveDataFromTemp(allChannels,crossfadeStop,crossfadeStopLength,tempCrossfadePoolKeyStop,crossfadeStop,crossfadeStopLength*crossfadeMoveMul,false);
			
		if(crossfadeStartLength>0)
			actionSound.sound->removeSpaceAndMoveDataFromTemp(allChannels,crossfadeStart,crossfadeStartLength,tempCrossfadePoolKeyStart,crossfadeStart,crossfadeStartLength*crossfadeMoveMul,false);
			
		didCrossfadeEdges=cetNone;
	}
}


void AAction::freeAllTempPools()
{
	try
	{
		// free the temp pools possibly used by moveSelectionToTempPools
		if(tempAudioPoolKey!=-1)
		{
			actionSound.sound->removeTempAudioPools(tempAudioPoolKey);
			tempAudioPoolKey=-1;
		}
		if(tempAudioPoolKey2!=-1)
		{
			actionSound.sound->removeTempAudioPools(tempAudioPoolKey2);
			tempAudioPoolKey2=-1;
		}

		// free the temp pools possibly used by crossfadeEdges
		if(tempCrossfadePoolKeyStart!=-1)
		{
			actionSound.sound->removeTempAudioPools(tempCrossfadePoolKeyStart);
			tempCrossfadePoolKeyStart=-1;
		}
		if(tempCrossfadePoolKeyStop!=-1)
		{
			actionSound.sound->removeTempAudioPools(tempCrossfadePoolKeyStop);
			tempCrossfadePoolKeyStop=-1;
		}
		
	}
	catch(...)
	{
		// oh well
	}
}

// fudgeFactor is used to copy a few more samples of data past the end of the moved data.  Useful for effects like CChangeRateEffect which uses the undo data as a source buffer... but needs to always read ahead a little
// ??? if I thought it was going to be happening... I should lock the undo CPoolFile before changing its size in case 2 threads were using it at the same time
void AAction::moveSelectionToTempPools(const CActionSound &actionSound,const MoveModes moveMode,sample_pos_t replaceLength,sample_pos_t fudgeFactor)
{
	if(tempAudioPoolKey!=-1)
		throw(runtime_error(string(__func__)+" -- selection already moved"));

	restoreMoveMode=moveMode;

	const sample_pos_t length=actionSound.sound->getLength();
	sample_pos_t start=actionSound.start;
	sample_pos_t stop=actionSound.stop;
	sample_pos_t selectionLength=actionSound.selectionLength();

	restoreWhere=start;
	restoreLength=selectionLength;

	/* 
	 * - This value is used in restoreSelectionFromTempPools to not re-add silence to the end of channels 
	 *   if only some channels are selected to which to apply the action.
	 * - For instance: if I selected samples 0-29 of 100 samples and only applied it to channel 1 of 2, 
	 *   and I did a delete: then there would be 30 samples removed from the beginning of channel 1 and then 
	 *   30 samples of silence appended to channel 1 to keep the lengths of the channels equivalent.  Next, 
	 *   when I undo: I would insert the 30 deleted samples back to the beginning of channel 1 which would cause,
	 *   30 samples of silence to be appended to the end of channel 2 to keep the lengths of the channels 
	 *   equivalent again.  By passing the maxLength parameter to the space modifier methods of CSound in 
	 *   restoreSelectionFromTempPools it avoids this side-effect of 30 samples of silence being there after 
	 *   the undo.
	 */
	restoreTotalLength=actionSound.sound->getLength();

	switch(moveMode)
	{
	case mmAll:
		// the whole channel... we just change the effective start 
		// and stop positions and let it drop into the next case
		start=0;
		stop=actionSound.sound->getLength()-1;
		selectionLength=(stop-start)+1;

		restoreWhere=start;
		restoreLength=selectionLength;

	case mmSelection:
		// whats between start and stop
		tempAudioPoolKey=actionSound.sound->moveDataToTempAndReplaceSpace(actionSound.doChannel,start,selectionLength,replaceLength,fudgeFactor);
		break;

	case mmAllButSelection:
		if(fudgeFactor>0)
			throw(runtime_error(string(__func__)+" -- fudgeFactor cannot be used when moveMethod is mmAllButSelection"));
		if(replaceLength>0)
			throw(runtime_error(string(__func__)+" -- replaceLength cannot be used when moveMethod is mmAllButSelection"));

		restoreWhere=0;
		restoreLength=start;
		restoreLength2=(length-stop)-1;

		// on both sides outside of start and stop
		tempAudioPoolKey=actionSound.sound->moveDataToTemp(actionSound.doChannel,0,start);
		tempAudioPoolKey2=actionSound.sound->moveDataToTemp(actionSound.doChannel,selectionLength,(length-stop)-1);
		break;

	default:
		throw(runtime_error(string(__func__)+" -- unhandled moveMode: "+istring(moveMode)));
	}
}

void AAction::restoreSelectionFromTempPools(const CActionSound &actionSound,sample_pos_t removeWhere,sample_pos_t removeLength)
{
	if(tempAudioPoolKey==-1)
		return;

	// ??? I guess I'm passing false to tell it not to remove the temp audio pools so that I can redo if possible

	switch(restoreMoveMode)
	{
	case mmAll:
	case mmSelection:
		actionSound.sound->removeSpaceAndMoveDataFromTemp(actionSound.doChannel,removeWhere,removeLength,tempAudioPoolKey,restoreWhere,restoreLength,false,restoreTotalLength);
		break;

	case mmAllButSelection:
		if(removeWhere!=0 || removeLength!=0)
			throw(runtime_error(string(__func__)+" -- cannot pass removeWhere/removeLength when moveMode was mmAllButSelection"));

		// on both sides outside of start and stop
		actionSound.sound->moveDataFromTemp(actionSound.doChannel,tempAudioPoolKey,restoreWhere,restoreLength,false,restoreTotalLength);
		if(actionSound.countChannels()==actionSound.sound->getChannelCount())
			actionSound.sound->moveDataFromTemp(actionSound.doChannel,tempAudioPoolKey2,actionSound.sound->getLength(),restoreLength2,false,restoreTotalLength);
		else
			actionSound.sound->removeSpaceAndMoveDataFromTemp(actionSound.doChannel,restoreLength+actionSound.selectionLength(),restoreLength2,tempAudioPoolKey2,restoreLength+actionSound.selectionLength(),restoreLength2,false,restoreTotalLength);
		break;

	default:
		throw(runtime_error(string(__func__)+" -- unhandled moveMode: "+istring(restoreMoveMode)));
	}
}

