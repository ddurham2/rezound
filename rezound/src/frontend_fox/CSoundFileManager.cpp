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

#include "CSoundFileManager.h"

#include <stdexcept>
#include <string>
#include <vector>

#include <cc++/path.h>

#include "settings.h"

#include "CMainWindow.h"
#include "CSoundWindow.h"
#include "CSoundListWindow.h"

#include "CNewSoundDialog.h"

#include <fox/fx.h>

#define MAX_REOPEN_HISTORY 16 // needs to be a preference ???

CSoundFileManager *gSoundFileManager=NULL;


CSoundFileManager::CSoundFileManager(CSoundManager &_soundManager,ASoundPlayer &_soundPlayer,TPoolFile<unsigned,unsigned> &_loadedRegistryFile) :
	ASoundFileManager(_soundManager,_soundPlayer,_loadedRegistryFile)
{
}

bool CSoundFileManager::promptForOpen(string &filename,bool &readOnly)
{
	// I can do getOpenFilenames to be able to open multiple files ??? then the parameter would need to be a vector<string>
	FXString _filename=FXFileDialog::getOpenFilename(gMainWindow,"Open file",gPromptDialogDirectory.c_str(),"Supported Files (*.wav,*.WAV,*.rez,*.au,*.AU,*.raw)\nAll Files(*)",0);
	if(_filename!="")
	{
		// save directory to open the opendialog to next time
		gPromptDialogDirectory=ost::Path(_filename.text()).DirName();
		gPromptDialogDirectory.append(&ost::Path::dirDelim,1);

		filename=_filename.text();
		readOnly=false; // can do it.. but I need to invoke the open dialog differently
		return(true);
	}
	return(false);
}

bool CSoundFileManager::promptForSave(string &filename,const string _extension)
{
	istring extension=_extension;
	extension.lower();

	string _filename=FXFileDialog::getSaveFilename(gMainWindow,"Save file",filename=="" ? gPromptDialogDirectory.c_str() : filename.c_str(),("Save Type (*."+istring(extension).lower()+",*."+istring(extension).upper()+")\nAll Files(*)").c_str(),0).text();
	if(_filename!="")
	{
		// add the extension if the user didn't type it
		if(ost::Path(_filename).Extension()=="")
			_filename.append("."+extension);

		// save directory to open the opendialog to next time
		gPromptDialogDirectory=ost::Path(_filename).DirName();
		gPromptDialogDirectory.append(&ost::Path::dirDelim,1);


		filename=_filename;
		return(true);
	}
	return(false);
}

bool CSoundFileManager::promptForNewSoundParameters(string &filename,unsigned &channelCount,unsigned &sampleRate,sample_pos_t &length)
{
	if(gNewSoundDialog->execute(PLACEMENT_CURSOR))	
	{
		filename=gNewSoundDialog->getFilename();
		channelCount=gNewSoundDialog->getChannelCount();
		sampleRate=gNewSoundDialog->getSampleRate();
		length=gNewSoundDialog->getLength();
		return(true);
	}
	return(false);
}

void CSoundFileManager::createWindow(CLoadedSound *loaded)
{
	CSoundWindow *win=new CSoundWindow(gMainWindow,loaded);
	win->create();

	soundWindows.push_back(win);

	if(gFocusMethod==fmSoundWindowList)
		gSoundListWindow->addSoundWindow(win);

	win->setActiveState(true);
}

void CSoundFileManager::destroyWindow(CLoadedSound *loaded)
{
	for(size_t t=0;t<soundWindows.size();t++)
	{
		if(soundWindows[t]->loadedSound==loaded)
		{
			CSoundWindow *win=soundWindows[t];

			if(gFocusMethod==fmSoundWindowList)
				gSoundListWindow->removeSoundWindow(win);

			soundWindows.erase(soundWindows.begin()+t);

			// make new active window
			if(!soundWindows.empty())
				soundWindows[0]->setActiveState(true);

			delete win;

			return;
		}
	}
}

void CSoundFileManager::untoggleActiveForAllSoundWindows(CSoundWindow *exceptThisOne)
{
	for(size_t t=0;t<soundWindows.size();t++)
	{
		if(soundWindows[t]!=exceptThisOne)
			soundWindows[t]->setActiveState(false);
	}
}

CLoadedSound *CSoundFileManager::getActive()
{
	CSoundWindow *activeSoundWindow=getActiveWindow();
	if(activeSoundWindow)
		return(activeSoundWindow->loadedSound);
	else
		return(NULL);
}

CSoundWindow *CSoundFileManager::getActiveWindow()
{
	// search all the open windows for which one has its focus toggle button depressed
	for(size_t t=0;t<soundWindows.size();t++)
	{
		if(soundWindows[t]->getActiveState())
			return(soundWindows[t]);
	}
	return(NULL);
}

/*
// recursively call update for all descendants of a given window
void updateAll(FXWindow *w)
{
	w->update();
	for(int t=0;t<w->numChildren();t++)
		updateAll(w->childAtIndex(t));
}
*/

// ??? rename this to updateFromEdit
void CSoundFileManager::redrawActive()
{
	CSoundWindow *activeSoundWindow=getActiveWindow();
	if(activeSoundWindow)
	{
		activeSoundWindow->setTitle(activeSoundWindow->loadedSound->getSound()->getFilename().c_str());
		activeSoundWindow->updateFromEdit();
		//updateAll(activeSoundWindow);
		//activeSoundWindow->layout(); // ??? I should be able to do something less expensive than re-layout-ing the whole CSoundWindow, but just the waveView
		//activeSoundWindow->flush();
	}
}

void CSoundFileManager::updateReopenHistory(const string &filename)
{
	// rewrite the reopen history to the gSettingsRegistry
	vector<string> reopenFilenames;
	
	for(size_t t=0;gSettingsRegistry->contains("ReopenHistory"+istring(t));t++)
	{
		const string h=gSettingsRegistry->getValue("ReopenHistory"+istring(t));
		if(h!=filename)
			reopenFilenames.push_back(h);
	}

	if(reopenFilenames.size()>=MAX_REOPEN_HISTORY)
		reopenFilenames.erase(reopenFilenames.begin()+reopenFilenames.size()-1);
		
	reopenFilenames.insert(reopenFilenames.begin(),filename);

	for(size_t t=0;t<reopenFilenames.size();t++)
		gSettingsRegistry->setValue("ReopenHistory"+istring(t),reopenFilenames[t]);
}

