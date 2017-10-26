/* 
 * Copyright (C) 2006 - David W. Durham
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

#include "SoundFileManager.h"

#include <stdexcept>
#include <string>
#include <vector>

#include "settings.h"

#include "../backend/CLoadedSound.h"
#include "SoundWindow.h"
#include "MainWindow.h"
#include "SoundListWindow.h"


SoundFileManager *gSoundFileManager=NULL;

SoundFileManager::SoundFileManager(MainWindow *_mainWindow,ASoundPlayer *_soundPlayer,CNestedDataFile *_loadedRegistryFile) :
	ASoundFileManager(_soundPlayer,_loadedRegistryFile),
	previousActiveWindow(NULL),
	mainWindow(_mainWindow)
{
}

SoundFileManager::~SoundFileManager()
{
}

void SoundFileManager::createWindow(CLoadedSound *loaded)
{
	SoundWindow *win=new SoundWindow(mainWindow->getSoundWindowLayout(),loaded);

	soundWindows.push_back(win);

	// to avoid flicker, SoundWindow is hidden initially, then layout is calculated, then setActiveState() shows it
	QApplication::processEvents();
	win->setActiveState(true);

	mainWindow->soundListWindow()->rebuildSoundWindowList();
}

void SoundFileManager::destroyWindow(CLoadedSound *loaded)
{
	for(size_t t=0;t<soundWindows.size();t++)
	{
		if(soundWindows[t]->loadedSound==loaded)
		{
			SoundWindow *win=soundWindows[t];

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
	mainWindow->soundListWindow()->rebuildSoundWindowList();
}

const size_t SoundFileManager::getOpenedCount() const
{
	return soundWindows.size();
}

CLoadedSound *SoundFileManager::getSound(size_t index)
{
	if(index<soundWindows.size())
		return soundWindows[index]->loadedSound;
	else
		throw runtime_error(string(__func__)+" -- index out of range");
}

SoundWindow *SoundFileManager::getSoundWindow(size_t index)
{
	if(index<soundWindows.size())
		return soundWindows[index];
	else
		throw runtime_error(string(__func__)+" -- index out of range");
}

void SoundFileManager::untoggleActiveForAllSoundWindows(SoundWindow *exceptThisOne)
{
	for(size_t t=0;t<soundWindows.size();t++)
	{
		if(soundWindows[t]!=exceptThisOne)
			soundWindows[t]->setActiveState(false);
	}
}

CLoadedSound *SoundFileManager::getActive()
{
	SoundWindow *activeSoundWindow=getActiveWindow();
	if(activeSoundWindow)
		return activeSoundWindow->loadedSound;
	else
		return NULL;
}

SoundWindow *SoundFileManager::getActiveWindow()
{
	// search all the open windows for which one has its focus toggle button depressed
	for(size_t t=0;t<soundWindows.size();t++)
	{
		if(soundWindows[t]->getActiveState())
			return soundWindows[t];
	}
	return NULL;
}

void SoundFileManager::setActiveSound(size_t index)
{
	if(index<soundWindows.size())
	{
		previousActiveWindow=getActiveWindow();
		soundWindows[index]->setActiveState(true);
	}
}

void SoundFileManager::setActiveSoundWindow(SoundWindow *win)
{
	for(size_t t=0;t<soundWindows.size();t++)
	{
		if(soundWindows[t]==win)
		{
			setActiveSound(t);
			break;
		}
	}
}

SoundWindow *SoundFileManager::getSoundWindow(CLoadedSound *loadedSound)
{
	for(size_t t=0;t<soundWindows.size();t++)
	{
		if(soundWindows[t]->loadedSound==loadedSound)
			return soundWindows[t];
	}
	throw runtime_error(string(__func__)+" -- given soundToUpdate was not found in the list of loaded sounds");
}

void SoundFileManager::updateAfterEdit(CLoadedSound *soundToUpdate,bool undoing)
{
	SoundWindow *activeSoundWindow=getActiveWindow();

	// however, if soundToUpdate was passed, then find that sound window
	if(soundToUpdate!=NULL)
		activeSoundWindow=getSoundWindow(soundToUpdate);

	if(activeSoundWindow)
	{
		activeSoundWindow->updateFromEdit(undoing);
		mainWindow->soundListWindow()->rebuildSoundWindowList();
	}
}

const map<string,string> SoundFileManager::getPositionalInfo(CLoadedSound *sound)
{
	map<string,string> info;
	SoundWindow *activeSoundWindow=getActiveWindow();

	// however, if soundToUpdate was passed, then find that sound window
	if(sound!=NULL)
		activeSoundWindow=getSoundWindow(sound);

	if(activeSoundWindow)
		info=activeSoundWindow->getPositionalInfo();

	return info;

}

void SoundFileManager::setPositionalInfo(const map<string,string> positionalInfo,CLoadedSound *sound)
{
	if(!positionalInfo.empty())
	{
		SoundWindow *activeSoundWindow=getActiveWindow();
	
		// however, if soundToUpdate was passed, then find that sound window
		if(sound!=NULL)
			activeSoundWindow=getSoundWindow(sound);

		if(activeSoundWindow)
			activeSoundWindow->setPositionalInfo(positionalInfo);
	}
}

bool SoundFileManager::isValidLoadedSound(const CLoadedSound *sound) const
{
	for(size_t t=0;t<soundWindows.size();t++)
	{
		if(soundWindows[t]->loadedSound==sound)
			return true;
	}
	return false;
}

void SoundFileManager::updateModifiedStatusIndicators()
{
	mainWindow->soundListWindow()->rebuildSoundWindowList();
}
