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

#include <stdexcept>

#include <istring>

#include "CLoadedSound.h"
#include "AStatusComm.h"

#include "settings.h"


// ----------------------------------------------------------------------
// -- CActionSound ------------------------------------------------------
// ----------------------------------------------------------------------

CActionSound::CActionSound(CSoundPlayerChannel *_channel,bool _doCrossfadeEdges) :
	sound(_channel->sound),
	doCrossfadeEdges(_doCrossfadeEdges)
{
	if(_channel==NULL)
		throw(runtime_error(string(__func__)+" -- channel parameter is NULL"));

	start=_channel->getStartPosition();
	stop=_channel->getStopPosition();
	allChannels();
}

CActionSound::CActionSound(const CActionSound &src)
{
	operator=(src);
}

unsigned CActionSound::countChannels() const
{
	unsigned i=0;
	for(unsigned t=0;t<MAX_CHANNELS;t++)
	{
		if(doChannel[t])
			i++;
	}
	return(i);
}

void CActionSound::allChannels()
{
	for(unsigned t=0;t<sound->getChannelCount();t++)
		doChannel[t]=true;
	for(unsigned t=sound->getChannelCount();t<MAX_CHANNELS;t++)
		doChannel[t]=false;
}

void CActionSound::noChannels()
{
	for(unsigned t=0;t<MAX_CHANNELS;t++)
		doChannel[t]=false;
}

sample_pos_t CActionSound::selectionLength() const
{
	return((stop-start)+1);
}

void CActionSound::selectAll() const
{
	start=0;
	stop=sound->getLength()-1;
}

void CActionSound::selectNone() const
{
	start=1;
	stop=0;
}

CActionSound &CActionSound::operator=(const CActionSound &rhs)
{
	sound=rhs.sound;
	doCrossfadeEdges=rhs.doCrossfadeEdges;

	for(unsigned t=0;t<MAX_CHANNELS;t++)
		doChannel[t]=rhs.doChannel[t];

	start=rhs.start;
	stop=rhs.stop;

	return(*this);
}




// ----------------------------------------------------------------------
// -- CActionParameters -------------------------------------------------
// ----------------------------------------------------------------------
CActionParameters::CActionParameters()
{
}

CActionParameters::CActionParameters(const CActionParameters &src)
{
	for(unsigned t=0;t<src.parameters.size();t++)
	{
		parameterTypes.push_back(src.parameterTypes[t]);
		switch(parameterTypes[parameterTypes.size()-1])
		{
		case ptBool:
			parameters.push_back(new bool(*((bool *)src.parameters[t])));
			break;

		case ptString:
			parameters.push_back(new string(*((string *)src.parameters[t])));
			break;

		case ptUnsigned:
			parameters.push_back(new unsigned(*((unsigned *)src.parameters[t])));
			break;

		case ptSamplePos:
			parameters.push_back(new sample_pos_t(*((sample_pos_t *)src.parameters[t])));
			break;

		case ptDouble:
			parameters.push_back(new double(*((double *)src.parameters[t])));
			break;

		case ptGraph:
			parameters.push_back(new CGraphParamValueNodeList(*((CGraphParamValueNodeList *)src.parameters[t])));
			break;

		default:
			throw(string(__func__)+" -- internal error -- unhandled parameter type: "+istring(parameterTypes[parameterTypes.size()-1]));
		}
	}
}

CActionParameters::~CActionParameters()
{
	clear();
}

void CActionParameters::clear()
{
	for(unsigned t=0;t<parameters.size();t++)
	{
		switch(parameterTypes[t])
		{
		case ptBool:
			delete ((bool *)parameters[t]);
			break;

		case ptString:
			delete ((string *)parameters[t]);
			break;

		case ptUnsigned:
			delete ((unsigned *)parameters[t]);
			break;

		case ptSamplePos:
			delete ((sample_pos_t *)parameters[t]);
			break;

		case ptDouble:
			delete ((double *)parameters[t]);
			break;

		case ptGraph:
			delete ((CGraphParamValueNodeList *)parameters[t]);
			break;

		default:
			throw(string(__func__)+" -- internal error -- unhandled parameter type: "+istring(parameterTypes[parameterTypes.size()-1]));
		}
	}
	parameterTypes.clear();
	parameters.clear();
}

unsigned CActionParameters::getParameterCount() const
{
	return(parameters.size());
}

void CActionParameters::addBoolParameter(const bool v)
{
	parameterTypes.push_back(ptBool);
	parameters.push_back(new bool(v));
}

void CActionParameters::addStringParameter(const string v)
{
	parameterTypes.push_back(ptString);
	parameters.push_back(new string(v));
}

void CActionParameters::addUnsignedParameter(const unsigned v)
{
	parameterTypes.push_back(ptUnsigned);
	parameters.push_back(new unsigned(v));
}

void CActionParameters::addSamplePosParameter(const sample_pos_t v)
{
	parameterTypes.push_back(ptSamplePos);
	parameters.push_back(new sample_pos_t(v));
}

void CActionParameters::addDoubleParameter(const double v)
{
	parameterTypes.push_back(ptDouble);
	parameters.push_back(new double(v));
}

void CActionParameters::addGraphParameter(const CGraphParamValueNodeList &v)
{
	parameterTypes.push_back(ptGraph);
	parameters.push_back(new CGraphParamValueNodeList(v));
}

const bool CActionParameters::getBoolParameter(const unsigned i) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameterTypes[i]!=ptBool)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptBool"));
	else
		return(*((bool *)parameters[i]));
}

const string CActionParameters::getStringParameter(const unsigned i) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameterTypes[i]!=ptString)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptString"));
	else
		return(*((string *)parameters[i]));
}

const unsigned CActionParameters::getUnsignedParameter(const unsigned i) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameterTypes[i]!=ptUnsigned)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptUnsigned"));
	else
		return(*((unsigned *)parameters[i]));
}

const sample_pos_t CActionParameters::getSamplePosParameter(const unsigned i) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameterTypes[i]!=ptSamplePos)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptSamplePos"));
	else
		return(*((sample_pos_t *)parameters[i]));
}

const double CActionParameters::getDoubleParameter(const unsigned i) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameterTypes[i]!=ptDouble)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptDouble"));
	else
		return(*((double *)parameters[i]));
}

const CGraphParamValueNodeList CActionParameters::getGraphParameter(const unsigned i) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameterTypes[i]!=ptGraph)
	{
		if(parameterTypes[i]==ptDouble)
			return(singleValueToGraph(*((double *)parameters[i])));
		else
			throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptDouble or ptGraph"));
	}
	else
		return(*((CGraphParamValueNodeList *)parameters[i]));
}



void CActionParameters::setBoolParameter(const unsigned i,const bool v) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameterTypes[i]!=ptBool)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptBool"));
	else
		(*((bool *)parameters[i]))=v;
}

void CActionParameters::setStringParameter(const unsigned i,const string v) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameterTypes[i]!=ptString)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptString"));
	else
		(*((string *)parameters[i]))=v;
}

void CActionParameters::setUnsignedParameter(const unsigned i,const unsigned v) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameterTypes[i]!=ptUnsigned)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptUnsigned"));
	else
		(*((unsigned *)parameters[i]))=v;
}

void CActionParameters::setSamplePosParameter(const unsigned i,const sample_pos_t v) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameterTypes[i]!=ptSamplePos)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptSamplePos"));
	else
		(*((sample_pos_t *)parameters[i]))=v;
}

void CActionParameters::setDoubleParameter(const unsigned i,const double v) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameterTypes[i]!=ptDouble)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptDouble"));
	else
		(*((double *)parameters[i]))=v;
}

void CActionParameters::setGraphParameter(const unsigned i,const CGraphParamValueNodeList &v) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameterTypes[i]!=ptGraph)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptGraph"));
	else
		(*((CGraphParamValueNodeList *)parameters[i]))=v;
}



// ----------------------------------------------------------------------
// -- AActionFactory ----------------------------------------------------
// ----------------------------------------------------------------------

AActionFactory::AActionFactory(const string _actionName,const string _actionDescription,const bool _hasAdvancedMode,AActionDialog *_channelSelectDialog,AActionDialog *_normalDialog,AActionDialog *_advancedDialog,bool _willResize,bool _crossfadeEdgesIsApplicable) :
	actionName(_actionName),
	actionDescription(_actionDescription),

	channelSelectDialog(_channelSelectDialog),
	normalDialog(_normalDialog),
	advancedDialog(_advancedDialog),

	hasAdvancedMode(_hasAdvancedMode),
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
		if(advancedMode && !hasAdvancedMode)
		{
			Message("This action does not have an advanced mode");
			return(false);
		}

		if(!doPreActionSetup())
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







// ----------------------------------------------------------------------
// -- AAction -----------------------------------------------------------
// ----------------------------------------------------------------------

//CPoolFile *AAction::undoPoolFile=NULL;
CSound::PoolFile_t *AAction::clipboardPoolFile=NULL;
//int AAction::poolNameCounter=0;

// ??? perhaps we could hold on to a pointer to our factory to easily be able to rePEAT the action

AAction::AAction(const CActionSound &_actionSound) :
	tempAudioPoolKey(-1),
	tempAudioPoolKey2(-1),

	actionSound(_actionSound),
	done(false),
	oldSelectStart(NIL_SAMPLE_POS),
	oldSelectStop(NIL_SAMPLE_POS),
	//undoBackupSaved(false)//,
	//backedUpActionSound(_actionSound)

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

bool AAction::doAction(CSoundPlayerChannel *channel,bool prepareForUndo,bool willResize,bool crossfadeEdgesIsApplicable)
{
	if(done)
		throw(runtime_error(string(__func__)+" -- action has already been done"));

	done=true;

	origIsModified=actionSound.sound->isModified();
	actionSound.sound->setIsModified(actionSound.sound->isModified() || doesWarrantSaving());

	// even though the AAction derived class might not resize the sound, we will if crossfading is to be done
	willResize|=actionSound.doCrossfadeEdges;

	if(willResize)
		actionSound.sound->lockForResize();
	else
		actionSound.sound->lockSize();

	try
	{
		// save the cues so that if they are modified by the action, then they can be restored at undo
		restoreCues.clear();
		for(size_t t=0;t<actionSound.sound->getCueCount();t++)
			restoreCues.push_back(CSound::RCue(actionSound.sound->getCueName(t).c_str(),actionSound.sound->getCueTime(t),actionSound.sound->isCueAnchored(t)));

		if(channel!=NULL)
		{
			oldSelectStart=channel->getStartPosition();
			oldSelectStop=channel->getStopPosition();
		}

		CActionSound _actionSound(actionSound);
	
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
			setSelection(_actionSound.start,_actionSound.stop,channel);

		if(willResize)
			actionSound.sound->unlockForResize();
		else
			actionSound.sound->unlockSize();

		actionSound.sound->flush();

		return(ret);
	}
	catch(EUserMessage &e)
	{
		endAllProgressBars();

		if(willResize)
			actionSound.sound->unlockForResize();
		else
			actionSound.sound->unlockSize();

		Message(e.what());
		return(false);
	}
	catch(...)
	{
		endAllProgressBars();

		if(willResize)
			actionSound.sound->unlockForResize();
		else
			actionSound.sound->unlockSize();

		throw;
	}
}

// ??? probably need to do what we can to be able to redo the action

void AAction::undoAction(CSoundPlayerChannel *channel,bool willResize)
{
	if(!done)
		throw(runtime_error(string(__func__)+" -- action has not yet been done"));

	// even though the AAction derived class might not resize the sound, we will if crossfading is to be done
	willResize|=actionSound.doCrossfadeEdges;

	if(willResize)
		actionSound.sound->lockForResize();
	else
		actionSound.sound->lockSize();

	try
	{
		if(canUndo()!=curYes)
			throw(runtime_error(string(__func__)+" -- cannot undo action"));

		uncrossfadeEdges();

		//const CActionSound _actionSound(backedUpActionSound);
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
	


		if(channel!=NULL && oldSelectStart!=NIL_SAMPLE_POS && oldSelectStop!=NIL_SAMPLE_POS)
			setSelection(oldSelectStart,oldSelectStop,channel);

		// restore the cues
		actionSound.sound->clearCues();
		for(size_t t=0;t<restoreCues.size();t++)
			actionSound.sound->addCue(restoreCues[t].name,restoreCues[t].time,restoreCues[t].isAnchored);
		restoreCues.clear();


		if(willResize)
			actionSound.sound->unlockForResize();
		else
			actionSound.sound->unlockSize();

		actionSound.sound->flush();

		done=false;
	}
	catch(EUserMessage &e)
	{
		endAllProgressBars();

		if(willResize)
			actionSound.sound->unlockForResize();
		else
			actionSound.sound->unlockSize();

		Message(e.what());
	}
	catch(...)
	{
		endAllProgressBars();

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
void AAction::crossfadeEdges(CActionSound &actionSound)
{
	didCrossfadeEdges=false;

	// might have the user specify some other reasons not to do the crossfade if say, the selection length is below the crossfade times or something
	if(!actionSound.doCrossfadeEdges)
		return;

	bool allChannels[MAX_CHANNELS];
	for(size_t t=0;t<MAX_CHANNELS;t++)
		allChannels[t]=true;

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
		crossfadeStartLength=crossfadeTime*2;

		for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
		{	
			CRezPoolAccesser dest=actionSound.sound->getAudio(i);
			const CRezPoolAccesser src=actionSound.sound->getTempAudio(tempCrossfadePoolKeyStart,i);

			sample_pos_t srcPos=0;

			// fade out what was before the start position
			for(sample_pos_t t=actionSound.start-crossfadeTime;t<actionSound.start;t++,srcPos++)
				dest[t]=(sample_t)(src[srcPos]*(1.0-((float)srcPos/(float)crossfadeTime)));

			// fade in what was after the start position 
			// (not necesary to ClipSample since it's always a constant 1.0 gain
			for(sample_pos_t t=actionSound.start-crossfadeTime;t<actionSound.start;t++,srcPos++)
				dest[t]+=(sample_t)(src[srcPos]*(((float)(srcPos-crossfadeTime)/(float)crossfadeTime)));

		}

		actionSound.start-=crossfadeTime;
		actionSound.stop-=crossfadeTime;

		didCrossfadeEdges=true;
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
			crossfadeStopLength=crossfadeTime*2;

			for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
			{	
				CRezPoolAccesser dest=actionSound.sound->getAudio(i);
				const CRezPoolAccesser src=actionSound.sound->getTempAudio(tempCrossfadePoolKeyStop,i);

				sample_pos_t srcPos=0;

				// fade out what was before the stop position
				for(sample_pos_t t=actionSound.stop-crossfadeTime;t<actionSound.stop;t++,srcPos++)
					dest[t]=(sample_t)(src[srcPos]*(1.0-((float)srcPos/(float)crossfadeTime)));

				// fade in what was after the stop position 
				// (not necesary to ClipSample since it's always a constant 1.0 gain
				for(sample_pos_t t=actionSound.stop-crossfadeTime;t<actionSound.stop;t++,srcPos++)
					dest[t]+=(sample_t)(src[srcPos]*(((float)(srcPos-crossfadeTime)/(float)crossfadeTime)));

			}

			didCrossfadeEdges=true;
		}
	}
}

void AAction::uncrossfadeEdges()
{
	if(!didCrossfadeEdges)
		return;

	bool allChannels[MAX_CHANNELS];
	for(size_t t=0;t<MAX_CHANNELS;t++)
		allChannels[t]=true;

	if(crossfadeStopLength>0)
		actionSound.sound->removeSpaceAndMoveDataFromTemp(allChannels,crossfadeStop,crossfadeStopLength/2,tempCrossfadePoolKeyStop,crossfadeStop,crossfadeStopLength,false);
		
	if(crossfadeStartLength>0)
		actionSound.sound->removeSpaceAndMoveDataFromTemp(allChannels,crossfadeStart,crossfadeStartLength/2,tempCrossfadePoolKeyStart,crossfadeStart,crossfadeStartLength,false);
		
	didCrossfadeEdges=false;
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

// ---------------------------------------------------------

void AAction::clearClipboard()
{
	clipboardPoolFile->clear();
}

unsigned AAction::getClipboardChannelCount()
{
	return(clipboardPoolFile->getPoolCount());
}

sample_pos_t AAction::getClipboardLength()
{
	CRezPoolAccesser a=clipboardPoolFile->getPoolAccesser<sample_t>(clipboardPoolFile->getPoolIdByIndex(0));
	return(a.getSize());
}

