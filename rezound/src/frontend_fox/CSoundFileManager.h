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

#ifndef __CSoundFileManager_H__
#define __CSoundFileManager_H__

#include "../../config/common.h"
#include "fox_compat.h"

#include "../backend/ASoundFileManager.h"

#include <vector>

class CSoundWindow;
class CMainWindow;
class CNestedDataFile;

#ifdef FOX_NO_NAMESPACE
	class FXWindow;
#else
	namespace FX { class FXWindow; }
	using namespace FX;
#endif

class CSoundFileManager : public ASoundFileManager
{
public:
	CSoundFileManager(CMainWindow *mainWindow,ASoundPlayer *soundPlayer,CNestedDataFile *loadedRegistryFile);
	virtual ~CSoundFileManager();

	void untoggleActiveForAllSoundWindows(CSoundWindow *exceptThisOne);
	
	const size_t getOpenedCount() const;
	CSoundWindow *getSoundWindow(size_t index);

	CSoundWindow *getSoundWindow(CLoadedSound *loadedSound);

	CLoadedSound *getActive();
	CSoundWindow *getActiveWindow();
	void updateAfterEdit(CLoadedSound *soundToUpdate=NULL,bool undoing=false); // if NULL, then use the active one

	const map<string,string> getPositionalInfo(CLoadedSound *sound=NULL);
	void setPositionalInfo(const map<string,string> positionalInfo,CLoadedSound *sound=NULL);

	bool isValidLoadedSound(const CLoadedSound *sound) const;

	CMainWindow *getMainWindow() const { return mainWindow; }

protected:
	void createWindow(CLoadedSound *loaded);
	void destroyWindow(CLoadedSound *loaded);

private:
	CMainWindow *mainWindow;

	vector<CSoundWindow *> soundWindows; // all the existing windows created by createWindow()

};

extern CSoundFileManager *gSoundFileManager;

#endif
