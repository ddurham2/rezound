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

#include <istring>

#include "settings.h"

#include "../backend/CLoadedSound.h"
#include "CSoundWindow.h"
#include "CMainWindow.h"

#include <fox/fx.h>

CSoundFileManager *gSoundFileManager=NULL;

CSoundFileManager::CSoundFileManager(CMainWindow *_mainWindow,ASoundPlayer *_soundPlayer,CNestedDataFile *_loadedRegistryFile) :
	ASoundFileManager(_soundPlayer,_loadedRegistryFile),
	mainWindow(_mainWindow)
{
}

CSoundFileManager::~CSoundFileManager()
{
}

void CSoundFileManager::createWindow(CLoadedSound *loaded)
{
	CSoundWindow *win=new CSoundWindow(mainWindow->getParentOfSoundWindows(),loaded);
	win->create();
	win->show();

	mainWindow->addSoundWindow(win);

	soundWindows.push_back(win);

	win->setActiveState(true);
}

void CSoundFileManager::destroyWindow(CLoadedSound *loaded)
{
	for(size_t t=0;t<soundWindows.size();t++)
	{
		if(soundWindows[t]->loadedSound==loaded)
		{
			CSoundWindow *win=soundWindows[t];

			mainWindow->removeSoundWindow(win);

			soundWindows.erase(soundWindows.begin()+t);

			// make new active window
			if(!soundWindows.empty())
				soundWindows[0]->setActiveState(true);

			delete win;

			return;
		}
	}
}

const size_t CSoundFileManager::getOpenedCount() const
{
	return(soundWindows.size());
}

#include "../backend/CSound.h"
CSoundWindow *previousActiveWindow=NULL; // used for alt-` meaning switch back to the previously active window
void CSoundFileManager::untoggleActiveForAllSoundWindows(CSoundWindow *exceptThisOne)
{
	previousActiveWindow=getActiveWindow();
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

void CSoundFileManager::updateAfterEdit()
{
	CSoundWindow *activeSoundWindow=getActiveWindow();
	if(activeSoundWindow)
	{
		activeSoundWindow->updateFromEdit();
		mainWindow->updateSoundWindowName(activeSoundWindow);
	}
}

