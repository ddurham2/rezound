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

#ifndef __ASoundFileManager_H__
#define __ASoundFileManager_H__

#include "../../config/common.h"

// ??? perhaps rename this to "CLoadedSoundManager"

class ASoundFileManager;

#include <string>

#include "CLoadedSound.h"

class CNestedDataFile;
class ASoundRecorder;

struct EStopClosing { };

class ASoundFileManager
{
public:

	ASoundFileManager(CSoundManager *soundManager,ASoundPlayer *soundPlayer,CNestedDataFile *loadedRegistryFile);
	// should a destructor be responsible for closing all files???
	virtual ~ASoundFileManager() { }

	void createNew();
	void open(const string filename="");
	// ??? should rename these to, saveActive... 
	void save();
	void saveAs();
	enum CloseTypes { ctSaveYesNoStop,ctSaveYesNoCancel,ctSaveNone };
	void close(CloseTypes closeType,CLoadedSound *closeWhichSound=NULL); // if nothing is passed for closeWhichSound, then the active sound is closed
	void revert();
	void recordToNew();

	static const string getUntitledFilename(const string directory,const string extension);

	const size_t getReopenHistorySize() const;
	const string getReopenHistoryItem(const size_t index) const;

	// return the CLoadedSound object associated with the sound window which is currently 'focused'
	// return NULL if there is no focused window
	virtual CLoadedSound *getActive()=0;

	// is called after an action is performed to update the screen or when the title
	// bar and other status information of a loaded sound window needs to be modified
	virtual void updateAfterEdit()=0;

	// returns a list of error messages
	const vector<string> loadFilesInRegistry();

protected:

	// prompt with an open file dialog (return false if the prompt was cancelled)
	virtual bool promptForOpen(string &filename,bool &readOnly)=0;

	// prompt with a save file dialog (return false if the prompt was cancelled)
	virtual bool promptForSave(string &filename,const string extension)=0;

	// prompt for a new sound to be created asking for the given parameters (return false if the prompt was cancelled)
	virtual bool promptForNewSoundParameters(string &filename,unsigned &channelCount,unsigned &sampleRate,sample_pos_t &length)=0;
	virtual bool promptForNewSoundParameters(string &filename,unsigned &channelCount,unsigned &sampleRate)=0;

	// prompt for recording, this function will have to be more than just an interface and do work 
	// since it will probably show level meters and be able to insert cues while recording etc
	virtual bool promptForRecord(ASoundRecorder *recorder)=0;



	// should create a new sound window with the given CLoadedSound object
	virtual void createWindow(CLoadedSound *loaded)=0;

	// should destroy the window which was created with the given CLoadedSound object
	// Note: it is possible that this called with a CLoadedSound object which doesn't have a window.  If so, just ignore
	virtual void destroyWindow(CLoadedSound *loaded)=0;



	// invoked whenever a file is successfully opened, new file created, file recorded, saveAs-ed, etc
	void updateReopenHistory(const string &filename);

private:

	CSoundManager *soundManager;
	ASoundPlayer *soundPlayer;

	CNestedDataFile *loadedRegistryFile;

	void prvOpen(const string &filename,bool readOnly,bool registerFilename);
	void registerFilename(const string filename);
	void unregisterFilename(const string filename);
	bool isFilenameRegistered(const string filename);
	CLoadedSound *prvCreateNew(bool askForLength);

};

#endif
