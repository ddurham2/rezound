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

#include "playcontrols.h"

#include <exception>

#include "AStatusComm.h"
#include "ASoundFileManager.h"

void play(ASoundFileManager *soundFileManager,bool looped,bool selectionOnly)
{
	try
	{
		CLoadedSound *loaded=soundFileManager->getActive();
		if(loaded!=NULL)
			loaded->channel->play(looped,selectionOnly);
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}

void pause(ASoundFileManager *soundFileManager)
{
	try
	{
		CLoadedSound *loaded=soundFileManager->getActive();
		if(loaded!=NULL)
			loaded->channel->pause();
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}

void stop(ASoundFileManager *soundFileManager)
{
	try
	{
		CLoadedSound *loaded=soundFileManager->getActive();
		if(loaded!=NULL)
			loaded->channel->stop();
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}

/*
// this below may ALL need to load the sound's size befor proceeding

void seekBack(ASoundFileManager *soundFileManager,int distance)
{
	try
	{
		CLoadedSound *loaded=soundFileManager->getActive();
		if(loaded!=NULL && loaded->channel->isPlaying())
		{
			CSoundPlayerChannel *channel=loaded->channel;
			if((channel->getPosition()-distance)>=0)
				channel->setPosition(channel->getPosition()-distance);
			else
				channel->setPosition(0);
		}
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}

void seekForward(ASoundFileManager *soundFileManager,int distance)
{
	try
	{
		CLoadedSound *loaded=soundFileManager->getActive();
		if(loaded!=NULL && loaded->channel->isPlaying())
		{
			CSoundPlayerChannel *channel=loaded->channel;
			if(channel->getPosition()+distance<channel->sound->getLength())
				channel->setPosition(channel->getPosition()+distance);
			else
				channel->setPosition(channel->sound->getLength()-1);
		}
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}

void selectSeekBack(ASoundFileManager *soundFileManager,int distance)
{
	try
	{
		CLoadedSound *loaded=soundFileManager->getActive();
		if(loaded!=NULL && loaded->channel->isPlaying())
		{
			CSoundPlayerChannel *channel=loaded->channel;
			if((channel->getPosition()-distance)>=channel->getStartPosition())
				channel->setPosition(channel->getPosition()-distance);
			else
				channel->setPosition(channel->getStartPosition());
		}
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}

void selectSeekForward(ASoundFileManager *soundFileManager,int distance)
{
	try
	{
		CLoadedSound *loaded=soundFileManager->getActive();
		if(loaded!=NULL && loaded->channel->isPlaying())
		{
			CSoundPlayerChannel *channel=loaded->channel;
			if(channel->getPosition()+distance<channel->getStopPosition())
				channel->setPosition(channel->getPosition()+distance);
			else
				channel->setPosition(channel->getStopPosition()-1);
		}
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}
*/

void jumpToBeginning(ASoundFileManager *soundFileManager)
{
	try
	{
		CLoadedSound *loaded=soundFileManager->getActive();
		if(loaded!=NULL && loaded->channel->isPlaying())
		{
			CSoundPlayerChannel *channel=loaded->channel;
			// this doesn't however, clear the data that's already been queued for playing
			channel->setPosition(0);
		}
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}

void jumpToStartPosition(ASoundFileManager *soundFileManager)
{
	try
	{
		CLoadedSound *loaded=soundFileManager->getActive();
		if(loaded!=NULL && loaded->channel->isPlaying())
		{
			CSoundPlayerChannel *channel=loaded->channel;
			channel->setPosition(channel->getStartPosition());
		}
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}

void jumpToPreviousCue(ASoundFileManager *soundFileManager)
{
	try
	{
		CLoadedSound *loaded=soundFileManager->getActive();
		if(loaded!=NULL && loaded->channel->isPlaying())
		{
			CSoundPlayerChannel *channel=loaded->channel;

			// find the previous cue relative to the current play position
			const sample_pos_t playPosition=channel->getPosition();
			ASound *sound=loaded->getSound();
		
			sample_pos_t smallestDistance=MAX_LENGTH;
			size_t previousCueIndex=0; // zero means not found yet, and we always store the index+1 when we do find one

			// For each cue whose time is in front of the current play position, find the nearest one.
			// But since the play position is always advancing, I also ignore cues that aren't more than
			// 0.4 of a second from the play position so that a user can click faster than 0.4 of a second
			// to go back, back, back on the cues.
			const size_t cueCount=sound->getCueCount();
			for(size_t t=0;t<cueCount;t++)
			{
				const sample_pos_t cueTime=sound->getCueTime(t);
				if(cueTime<playPosition)
				{
					const sample_pos_t distance=playPosition-cueTime;
					const sample_fpos_t distanceInTime=((sample_fpos_t)distance)/(sample_fpos_t)sound->getSampleRate();
					if(previousCueIndex==0 || (distance<smallestDistance && distanceInTime>0.4))
					{
						smallestDistance=distanceInTime>0.4 ? distance : smallestDistance;
						previousCueIndex=t+1;
					}
				}
			}

			if(previousCueIndex!=0)
			{
				previousCueIndex--;
				channel->setPosition(sound->getCueTime(previousCueIndex));
			}
			else
				gStatusComm->beep();
		}
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}

void jumpToNextCue(ASoundFileManager *soundFileManager)
{
	try
	{
		CLoadedSound *loaded=soundFileManager->getActive();
		if(loaded!=NULL && loaded->channel->isPlaying())
		{
			CSoundPlayerChannel *channel=loaded->channel;

			// find the previous cue relative to the current play position
			const sample_pos_t playPosition=channel->getPosition();
			ASound *sound=loaded->getSound();
		
			sample_pos_t smallestDistance=MAX_LENGTH;
			size_t nextCueIndex=0; // zero means not found yet, and we always store the index+1 when we do find one

			// For each cue whose time is beyond of the current play position, find the nearest one.
			const size_t cueCount=sound->getCueCount();
			for(size_t t=0;t<cueCount;t++)
			{
				const sample_pos_t cueTime=sound->getCueTime(t);
				if(cueTime>playPosition)
				{
					const sample_pos_t distance=cueTime-playPosition;
					if(nextCueIndex==0 || distance<smallestDistance)
					{
						smallestDistance=distance;
						nextCueIndex=t+1;
					}
				}
			}

			if(nextCueIndex!=0)
			{
				nextCueIndex--;
				channel->setPosition(sound->getCueTime(nextCueIndex));
			}
			else
				gStatusComm->beep();
		}
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}

