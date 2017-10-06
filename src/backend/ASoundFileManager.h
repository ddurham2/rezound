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

class ASoundFileManager;

#include <string>
#include <vector>
#include <map>

class CLoadedSound;
class CNestedDataFile;
class ASoundPlayer;
class ASoundTranslator;

#include "CSound_defs.h"

struct EStopClosing { };



class ASoundFileManager
{
public:

	ASoundFileManager(ASoundPlayer *soundPlayer,CNestedDataFile *loadedRegistryFile);
	// should a destructor be responsible for closing all files???
	virtual ~ASoundFileManager() { }

	void createNew();
	CLoadedSound *createNew(const string filename,unsigned channelCount,unsigned sampleRate,sample_pos_t length=1,bool rawFormat=false);
		// returns false if a prompt for filename was cancelled, or if there was an error loading
	bool open(const string filename="",bool openAsRaw=false);
	bool open(const vector<string> &filenames,bool openAsRaw=false);

//#warning "do this.. I can call soundFileManager->getActive() from actions when necessary"
	// ??? should rename these to, saveActive...  or pass them a CSound * (I prefer that), perhaps optionally pass saveAs a filename which can be ""
		// returns false if something was cancelled
	bool save();
		// returns false if something was cancelled
	bool saveAs(const string filename="",bool saveAsRaw=false);
		// returns false if something was cancelled
	bool savePartial(const CSound *sound,const string filename,const sample_pos_t saveStart,const sample_pos_t saveLength,bool useLastUserPrefs);

	enum CloseTypes { ctSaveYesNoStop,ctSaveYesNoCancel,ctSaveNone };
		// returns false if something was cancelled
	bool close(CloseTypes closeType,CLoadedSound *closeWhichSound=NULL); // if nothing is passed for closeWhichSound, then the active sound is closed

	void revert();
	void recordToNew();

	const string getUntitledFilename(const string directory,const string extension);

	const size_t getReopenHistorySize() const;
	const string getReopenHistoryItem(const size_t index) const;

	// return the CLoadedSound object associated with the sound window which is currently 'focused'
	// return NULL if there is no focused window
	virtual CLoadedSound *getActive()=0;

	// should be implemented to return the number of currently opened sound files
	virtual const size_t getOpenedCount() const=0;

	// given an index from 0 to getOpenedCount()-1 should be implemented to 
	// run the CLoadedSound pointer
	virtual CLoadedSound *getSound(size_t index)=0;

	// should be implemented to change the active sound to the one specified at the given index
	virtual void setActiveSound(size_t index)=0;

	// is called after an action is performed to update the screen or when the title
	// bar and other status information of a loaded sound window needs to be modified
	virtual void updateAfterEdit(CLoadedSound *sound=NULL,bool undoing=false)=0; // if NULL, then use the active one

	// these two methods should be implemented to get and set the positional information (i.e. zoom
	// factors and scroll positions) of the window with the given sound (or the active one if sound
	// is not passed in).
	virtual const map<string,string> getPositionalInfo(CLoadedSound *sound=NULL)=0; // would be 'const' but have to call getActive()
	virtual void setPositionalInfo(const map<string,string> positionalInfo,CLoadedSound *sound=NULL)=0;

	// returns a list of error messages
	const vector<string> loadFilesInRegistry();

protected:

	// should create a new sound window with the given CLoadedSound object
	virtual void createWindow(CLoadedSound *loaded)=0;

	// should destroy the window which was created with the given CLoadedSound object
	// Note: it is possible that this called with a CLoadedSound object which doesn't have a window.  If so, just ignore
	virtual void destroyWindow(CLoadedSound *loaded)=0;


	// invoked whenever a file is successfully opened, new file created, file recorded, saveAs-ed, etc
	void updateReopenHistory(const string filename);

private:

	ASoundPlayer *soundPlayer;

	CNestedDataFile *loadedRegistryFile;

	// returns false if cancelled
	bool prvOpen(const string filename,bool readOnly,bool registerFilename,bool asRaw=false,const ASoundTranslator *translatorToUse=NULL);

	void registerFilename(const string filename);
	void unregisterFilename(const string filename);
	bool isFilenameRegistered(const string filename);
	CLoadedSound *prvCreateNew(sample_pos_t length,bool askForLength,unsigned sampleRate,bool askForSampleRate);

};

#endif
