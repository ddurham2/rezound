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

	soundWindows.push_back(win);

	win->setActiveState(true);

	mainWindow->rebuildSoundWindowList();
}

void CSoundFileManager::destroyWindow(CLoadedSound *loaded)
{
	for(size_t t=0;t<soundWindows.size();t++)
	{
		if(soundWindows[t]->loadedSound==loaded)
		{
			CSoundWindow *win=soundWindows[t];

			soundWindows.erase(soundWindows.begin()+t);

			// make new active window (either in the same position or the last one)
			if(!soundWindows.empty())
			{
				if(t<soundWindows.size())
					soundWindows[t]->setActiveState(true);
				else
					soundWindows[soundWindows.size()-1]->setActiveState(true);
			}

			delete win;

			break;
		}
	}
	mainWindow->rebuildSoundWindowList();
}

const size_t CSoundFileManager::getOpenedCount() const
{
	return soundWindows.size();
}

CSoundWindow *CSoundFileManager::getSoundWindow(size_t index)
{
	return soundWindows[index];
}

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
		return activeSoundWindow->loadedSound;
	else
		return NULL;
}

CSoundWindow *CSoundFileManager::getActiveWindow()
{
	// search all the open windows for which one has its focus toggle button depressed
	for(size_t t=0;t<soundWindows.size();t++)
	{
		if(soundWindows[t]->getActiveState())
			return soundWindows[t];
	}
	return NULL;
}

CSoundWindow *CSoundFileManager::getSoundWindow(CLoadedSound *loadedSound)
{
	for(size_t t=0;t<soundWindows.size();t++)
	{
		if(soundWindows[t]->loadedSound==loadedSound)
			return soundWindows[t];
	}
	throw runtime_error(string(__func__)+" -- given soundToUpdate was not found in the list of loaded sounds");
}

void CSoundFileManager::updateAfterEdit(CLoadedSound *soundToUpdate)
{
	CSoundWindow *activeSoundWindow=getActiveWindow();

	// however, if soundToUpdate was passed, then find that sound window
	if(soundToUpdate!=NULL)
		activeSoundWindow=getSoundWindow(soundToUpdate);

	if(activeSoundWindow)
	{
		activeSoundWindow->updateFromEdit();
		mainWindow->rebuildSoundWindowList();
	}
}

bool CSoundFileManager::isValidLoadedSound(const CLoadedSound *sound) const
{
	for(size_t t=0;t<soundWindows.size();t++)
	{
		if(soundWindows[t]->loadedSound==sound)
			return true;
	}
	return false;
}

