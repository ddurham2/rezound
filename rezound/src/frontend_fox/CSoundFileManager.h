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

#include "../backend/ASoundFileManager.h"

#include <vector>
class CSoundWindow;
class CNestedDataFile;
class FXWindow;

class CSoundFileManager : public ASoundFileManager
{
public:
	CSoundFileManager(FXWindow *mainWindow,ASoundPlayer *soundPlayer,CNestedDataFile *loadedRegistryFile);

	void untoggleActiveForAllSoundWindows(CSoundWindow *exceptThisOne);
	
	CLoadedSound *getActive();
	void updateAfterEdit();

protected:
	bool promptForOpen(string &filename,bool &readOnly);
	bool promptForSave(string &filename,const string extension);
	bool promptForNewSoundParameters(string &filename,unsigned &channelCount,unsigned &sampleRate,sample_pos_t &length);
	bool promptForNewSoundParameters(string &filename,unsigned &channelCount,unsigned &sampleRate);

	bool promptForRecord(ASoundRecorder *recorder);

	void createWindow(CLoadedSound *loaded);
	void destroyWindow(CLoadedSound *loaded);

private:
	FXWindow *mainWindow;

	vector<CSoundWindow *> soundWindows; // all the existing windows created by createWindow()

	CSoundWindow *getActiveWindow();

};

extern CSoundFileManager *gSoundFileManager;

#endif
