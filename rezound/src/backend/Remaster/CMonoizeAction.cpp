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

#include "CMonoizeAction.h"

#include "../CActionSound.h"
#include "../CActionParameters.h"

CMonoizeAction::CMonoizeAction(const CActionSound &actionSound,const vector<float> _gains,const MonoizeMethods _method) :
	AAction(actionSound),

	gains(_gains),
	method(_method),

	origChannelCount(0),
	tempAudioPoolKey(-1),
	sound(NULL)
{
}

CMonoizeAction::~CMonoizeAction()
{
	if(tempAudioPoolKey!=-1)
		sound->removeTempAudioPools(tempAudioPoolKey);
}

bool CMonoizeAction::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	if(method==mmRemoveAllButOneChannel)
	{
		// backup channel 0 into a temp pool
		if(prepareForUndo)
		{
			CActionSound actionSound1(actionSound);
			actionSound1.noChannels();
			actionSound1.doChannel[0]=true;
			moveSelectionToTempPools(actionSound1,mmAll,actionSound.sound->getLength());
		}

		// create accessors for all channels (first is from channel 0 (either backed up or the real think) and rest are from the actionSound
		vector<const CRezPoolAccesser *> srces;
		try
		{
			vector<float> _gains;

			if(gains[0]!=0.0)
			{
				srces.push_back(new CRezPoolAccesser(prepareForUndo ? actionSound.sound->getTempAudio(AAction::tempAudioPoolKey,0) : actionSound.sound->getAudio(0)));
				_gains.push_back(gains[0]);
			}
			for(unsigned i=1;i<actionSound.sound->getChannelCount();i++)
			{
				if(gains[i]!=0.0)
				{
					srces.push_back(new CRezPoolAccesser(actionSound.sound->getAudio(i)));
					_gains.push_back(gains[i]);
				}
			}
		
			// mix all srces down to channel 0 according to their gains
			if(srces.size()<=0)
			{ // all gains were 0
				if(!prepareForUndo)
					actionSound.sound->silenceSound(0,0,actionSound.sound->getLength());
			}
			else
			{
				CStatusBar statusBar(_("Monoizing"),0,actionSound.sound->getLength(),true);
		
				CRezPoolAccesser dest=actionSound.sound->getAudio(0);
		
				const sample_pos_t length=actionSound.sound->getLength();
				const unsigned channelCount=srces.size();
				for(sample_pos_t t=0;t<length;t++)
				{
					mix_sample_t s=(mix_sample_t)((*(srces[0]))[t]*_gains[0]);
					for(unsigned i=1;i<channelCount;i++)
						s+=(mix_sample_t)((*(srces[i]))[t]*_gains[i]);
					dest[t]=ClipSample(s);

					if(statusBar.update(t))
					{ // cancelled
						if(prepareForUndo)
						{
							// restore channel 0's data
							CActionSound actionSound1(actionSound);
							actionSound1.noChannels();
							actionSound1.doChannel[0]=true;
							restoreSelectionFromTempPools(actionSound1,0,actionSound.sound->getLength());
						}
						else
							actionSound.sound->invalidatePeakData(0u,0,t);

						for(size_t t=0;t<srces.size();t++)
							delete srces[t];

						return false;
					}
				}
			}
		
			for(size_t t=0;t<srces.size();t++)
				delete srces[t];
		}
		catch(...)
		{
			for(size_t t=0;t<srces.size();t++)
				delete srces[t];
			throw;
		}

		// remove all but channel 0 (either backup or really remove)
		if(prepareForUndo)
		{
			CActionSound actionSound2(actionSound);
			actionSound2.allChannels();
			actionSound2.doChannel[0]=false;
			
			origChannelCount=actionSound.sound->getChannelCount();
			tempAudioPoolKey=actionSound.sound->moveChannelsToTemp(actionSound2.doChannel);
			sound=actionSound.sound;
		}
		else
		{
			actionSound.sound->invalidatePeakData(0u,0,actionSound.sound->getLength());
			actionSound.sound->removeChannels(1,actionSound.sound->getChannelCount()-1);
		}
	}
	else // if(method==mmMakeAllChannelsToSame)
	{ 
		/* 
		 * much of this is the same as above, but I didn't want to 
		 * make it look more complicated by combining these two cases 
		 */


		// backup all channels into a temp pool
		if(prepareForUndo)
		{
			CActionSound actionSound1(actionSound);
			actionSound1.allChannels();
			moveSelectionToTempPools(actionSound1,mmAll,actionSound.sound->getLength());
		}

		// create accessors for all channels (either to the temp pools or the real thing)
		vector<const CRezPoolAccesser *> srces;
		try
		{
			vector<float> _gains;

			for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
			{
				if(gains[i]!=0.0)
				{
					srces.push_back(new CRezPoolAccesser(prepareForUndo ? actionSound.sound->getTempAudio(AAction::tempAudioPoolKey,i) : actionSound.sound->getAudio(i)));
					_gains.push_back(gains[i]);
				}
			}
		
			// mix all srces down to channel 0 according to their gains
			if(srces.size()<=0)
			{ // all gains were 0
				if(prepareForUndo)
					actionSound.sound->silenceSound(0,0,actionSound.sound->getLength());
				else
				{
					for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
						actionSound.sound->silenceSound(i,0,actionSound.sound->getLength());
				}
			
			}
			else
			{
				CStatusBar statusBar(_("Monoizing"),0,actionSound.sound->getLength(),true);
		
				CRezPoolAccesser dest=actionSound.sound->getAudio(0);
		
				const sample_pos_t length=actionSound.sound->getLength();
				const unsigned srcChannelCount=srces.size();
				for(sample_pos_t t=0;t<length;t++)
				{
					// make s the mixing of all the srces
					mix_sample_t s=(mix_sample_t)((*(srces[0]))[t]*_gains[0]);
					for(unsigned i=1;i<srcChannelCount;i++)
						s+=(mix_sample_t)((*(srces[i]))[t]*_gains[i]);

					// copy to channel 0
					dest[t]=ClipSample(s);

					if(statusBar.update(t))
					{ // cancelled
						CActionSound actionSound1(actionSound);
						actionSound1.allChannels();

						if(prepareForUndo)
							restoreSelectionFromTempPools(actionSound1,0,actionSound.sound->getLength());
						else
							actionSound.sound->invalidatePeakData(actionSound1.doChannel,0,t);

						for(size_t t=0;t<srces.size();t++)
							delete srces[t];

						return false;
					}
				}

				// copy channel 0 to all the other channels
				for(unsigned i=1;i<actionSound.sound->getChannelCount();i++)
					actionSound.sound->mixSound(i,0,dest,0,actionSound.sound->getSampleRate(),actionSound.sound->getLength(),mmOverwrite,sftNone,true,true);
			}
			
			for(size_t t=0;t<srces.size();t++)
				delete srces[t];
		}
		catch(...)
		{
			for(size_t t=0;t<srces.size();t++)
				delete srces[t];
			throw;
		}


	}

	return true;
}

AAction::CanUndoResults CMonoizeAction::canUndo(const CActionSound &actionSound) const
{
	return curYes;
}

void CMonoizeAction::undoActionSizeSafe(const CActionSound &actionSound)
{
	if(method==mmRemoveAllButOneChannel)
	{
		// restore removed channels back into place
		CActionSound actionSound2(actionSound);
			// make actionSound2 exactly how it was when we called moveChannelsToTemp
		actionSound2.noChannels();
		for(unsigned t=1;t<origChannelCount;t++)
			actionSound2.doChannel[t]=true;
		actionSound.sound->moveChannelsFromTemp(tempAudioPoolKey,actionSound2.doChannel);
		tempAudioPoolKey=-1;
	
		// restore channel 0's data
		CActionSound actionSound1(actionSound);
		actionSound1.noChannels();
		actionSound1.doChannel[0]=true;
		restoreSelectionFromTempPools(actionSound1,0,actionSound.sound->getLength());
	}
	else // if(method==mmMakeAllChannelsTheSame)
	{
		CActionSound actionSound1(actionSound);
		actionSound1.allChannels();

		restoreSelectionFromTempPools(actionSound1,0,actionSound.sound->getLength());
	}
}


#include "../CLoadedSound.h"
bool CMonoizeAction::doPreactionSetup(CLoadedSound *loadedSound) const
{
	return loadedSound->sound->getChannelCount()>1;
}


// ---------------------------------------------

CMonoizeActionFactory::CMonoizeActionFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog) :
	AActionFactory(N_("Monoize"),"",channelSelectDialog,dialog,true,false)
{
}

CMonoizeActionFactory::~CMonoizeActionFactory()
{
}

CMonoizeAction *CMonoizeActionFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	vector<float> gains;
	for(unsigned t=0;t<actionSound.sound->getChannelCount();t++)
		gains.push_back(actionParameters->getDoubleParameter("Channel "+istring(t)));

	return new CMonoizeAction(
		actionSound,
		gains,
		(CMonoizeAction::MonoizeMethods)actionParameters->getUnsignedParameter("Method")
	);
}

