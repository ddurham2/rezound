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

#ifndef __SoundFileManager_H__
#define __SoundFileManager_H__

#include "../../config/common.h"
#include "qt_compat.h"

#include "../backend/ASoundFileManager.h"

#include <vector>

class SoundWindow;
class MainWindow;
class CNestedDataFile;

/*
#ifdef FOX_NO_NAMESPACE
	class FXWindow;
#else
	namespace FX { class FXWindow; }
	using namespace FX;
#endif
*/

class SoundFileManager : public ASoundFileManager
{
public:
    SoundFileManager(MainWindow *mainWindow,ASoundPlayer *soundPlayer,CNestedDataFile *loadedRegistryFile);
    virtual ~SoundFileManager();

    void untoggleActiveForAllSoundWindows(SoundWindow *exceptThisOne);
	
	const size_t getOpenedCount() const;
    CLoadedSound *getSound(size_t index);
    SoundWindow *getSoundWindow(size_t index);
	void setActiveSound(size_t index);
    void setActiveSoundWindow(SoundWindow *win);

    SoundWindow *getSoundWindow(CLoadedSound *loadedSound);

    CLoadedSound *getActive();
    SoundWindow *getActiveWindow();
    void updateAfterEdit(CLoadedSound *soundToUpdate=NULL,bool undoing=false); // if NULL, then use the active one

    const map<string,string> getPositionalInfo(CLoadedSound *sound=NULL);
    void setPositionalInfo(const map<string,string> positionalInfo,CLoadedSound *sound=NULL);

    bool isValidLoadedSound(const CLoadedSound *sound) const;

	MainWindow *getMainWindow() const { return mainWindow; }

	void updateModifiedStatusIndicators();



    SoundWindow *previousActiveWindow;

protected:
    void createWindow(CLoadedSound *loaded);
    void destroyWindow(CLoadedSound *loaded);

private:
	MainWindow *mainWindow;

    vector<SoundWindow *> soundWindows; // all the existing windows created by createWindow()

};

extern SoundFileManager *gSoundFileManager;

#endif
