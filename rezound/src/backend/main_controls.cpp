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

#include "main_controls.h"

#include <exception>

#include "settings.h"
#include "CSound.h"
#include "AStatusComm.h"
#include "ASoundFileManager.h"
#include "CSoundPlayerChannel.h"
#include "CLoadedSound.h"
#include "AAction.h"
#include "AFrontendHooks.h"




// -----------------------------------------------------------------------------------
// --- File Operations ---------------------------------------------------------------
// -----------------------------------------------------------------------------------

bool openSound(ASoundFileManager *soundFileManager,const string filename)
{
	try
	{
		return soundFileManager->open(filename);
	}
	catch(exception &e)
	{
		Error(e.what());
		return false;
	}
}

void newSound(ASoundFileManager *soundFileManager)
{
	try
	{
		soundFileManager->createNew();
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}

void closeSound(ASoundFileManager *soundFileManager)
{
	try
	{
		soundFileManager->close(ASoundFileManager::ctSaveYesNoCancel);
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}

void saveSound(ASoundFileManager *soundFileManager)
{
	try
	{
		soundFileManager->save();
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}

void saveAsSound(ASoundFileManager *soundFileManager)
{
	try
	{
		soundFileManager->saveAs();
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}

void revertSound(ASoundFileManager *soundFileManager)
{
	try
	{
		soundFileManager->revert();
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}

void recordSound(ASoundFileManager *soundFileManager)
{
	try
	{
		soundFileManager->recordToNew();
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}

void recordMacro()
{
	try
	{
		if(AActionFactory::macroRecorder.isRecording())
		{
			AActionFactory::macroRecorder.stopRecording();
		}
		else
		{
			string macroName; // initialize to something that doesn't exist "untitled1".. 
			if(gFrontendHooks->showRecordMacroDialog(macroName))
				AActionFactory::macroRecorder.startRecording(gUserMacroStore,macroName);
		}
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}

//---------------------------------------------------------------------------

const bool exitReZound(ASoundFileManager *soundFileManager)
{
	try
	{
		// make sure the user wants to quit
			// >1 instead of >0 because it's easy to open the 1 (unsaved) file again, rather than several (because if the one is unsaved, you get a chance to cancel)
		if(soundFileManager->getOpenedCount()>1) 
		{
			if(Question(_("Are you sure you want to quit?"),yesnoQues)!=yesAns)
				return false;
		}

		// stop recording a macro if there is one
		// we do this prior to closing files since they could cancel the quit while files are closing
		AActionFactory::macroRecorder.stopRecording();

		// close all opened files
		while(soundFileManager->getActive()!=NULL)
		{
			if(!soundFileManager->close(ASoundFileManager::ctSaveYesNoStop))
				throw EStopClosing();
		}
		return true;
	}
	catch(EStopClosing &e)
	{
		return false;
	}
	catch(exception &e)
	{
		Error(e.what());
		return false;
	}
}




// -----------------------------------------------------------------------------------
// --- Play Control Operations -------------------------------------------------------
// -----------------------------------------------------------------------------------

void play(ASoundFileManager *soundFileManager,CSoundPlayerChannel::LoopTypes loopType,bool selectionOnly)
{
	try
	{
		CLoadedSound *loaded=soundFileManager->getActive();
		if(loaded!=NULL)
			loaded->channel->play(0,loopType,selectionOnly);
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}

void play(ASoundFileManager *soundFileManager,sample_pos_t position)
{
	try
	{
		CLoadedSound *loaded=soundFileManager->getActive();
		if(loaded!=NULL)
			loaded->channel->play(position);
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
			CSound *sound=loaded->sound;
		
			sample_pos_t smallestDistance=MAX_LENGTH;
			size_t previousCueIndex=0; // zero means not found yet, and we always store the index+1 when we do find one

			// For each cue whose time is in front of the current play position, find the nearest one.
			// But since the play position is always advancing, I also ignore cues that aren't more than
			// 0.4 of a second from the play position so that a user can click faster than 0.4 of a second
			// to go back, back, back on the cues.  (EXCEPT, if it's paused, forget about the 0.4 minimum 
			// requirement)
			const size_t cueCount=sound->getCueCount();
			for(size_t t=0;t<cueCount;t++)
			{
				const sample_pos_t cueTime=sound->getCueTime(t);
				if(cueTime<playPosition)
				{
					const sample_pos_t distance=playPosition-cueTime;
					const sample_fpos_t distanceInTime=((sample_fpos_t)distance)/(sample_fpos_t)sound->getSampleRate();
					if(previousCueIndex==0 || (distance<smallestDistance && (distanceInTime>0.4 || channel->isPaused())))
					{
						smallestDistance=(distanceInTime>0.4 || channel->isPaused()) ? distance : smallestDistance;
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
			CSound *sound=loaded->sound;
		
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


 



// -----------------------------------------------------------------------------------
// --- Undo Operations ---------------------------------------------------------------
// -----------------------------------------------------------------------------------

void undo(ASoundFileManager *soundFileManager)
{
	try
	{
		CLoadedSound *s=soundFileManager->getActive();
		if(s!=NULL)
		{
			if(!s->actions.empty())
			{
				AAction *a=s->actions.top();
				s->actions.pop();

				// if we're recording a macro, then un-record this action being undone
				try {
					if(AActionFactory::macroRecorder.isRecording())
						AActionFactory::macroRecorder.popAction(a->getFactory()->getName());
				} catch(...) { }

				a->undoAction(s->channel);

				// restore the frontend position information as part of undoing the action
				soundFileManager->setPositionalInfo(a->positionalInfo);

				delete a; // ??? what ever final logic is implemented for undo, it should probably push it onto a redo stack

				soundFileManager->updateAfterEdit(NULL,true);
			}
			else
				gStatusComm->beep();
		}
		else
			gStatusComm->beep();
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}

void clearUndoHistory(ASoundFileManager *soundFileManager)
{
	try
	{
		CLoadedSound *s=soundFileManager->getActive();
		if(s!=NULL)
		{
			s->clearUndoHistory();
			soundFileManager->updateAfterEdit();
		}
		else
			gStatusComm->beep();
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}



