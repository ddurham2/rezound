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

#include "AStatusComm.h"
#include "initialize.h"
#include "settings.h"

#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <stdexcept>
#include <string>

#include <CNestedDataFile/CNestedDataFile.h>


// ... except for a few things.. this could probably be moved into the backend.. it'd have to be done for all frontends if I didn't move it

#include "file.h"

//#include "CWinSoundPlayer.h"
//static CWinSoundPlayer *soundPlayer=NULL;
#include "COSSSoundPlayer.h"
static COSSSoundPlayer *soundPlayer=NULL;

#include "AAction.h"


// for mkdir  --- possibly wouldn't port???
#include <sys/stat.h>
#include <sys/types.h>

#include <cc++/path.h>

void initializeBackend(ASoundPlayer *&_soundPlayer)
{
	try
	{

		// make sure that ~/.rezound exists
		gUserDataDirectory=string(getenv("HOME"))+istring(ost::Path::dirDelim)+".rezound";
		const int mkdirResult=mkdir(gUserDataDirectory.c_str(),0700);
		const int mkdirErrno=errno;
		if(mkdirResult!=0 && mkdirErrno!=EEXIST)
			throw(runtime_error(string(__func__)+" -- error creating "+gUserDataDirectory+" -- "+strerror(mkdirErrno)));


		// determine where /usr/share/rezound has been placed (try the install-from directory first)
		gSysDataDirectory=SOURCE_DIR"/share/rezound";
		if(!ost::Path(gSysDataDirectory).Exists()) 
			gSysDataDirectory=DATA_DIR"/rezound";


		gUserPresetsFile=gUserDataDirectory+istring(ost::Path::dirDelim)+"presets.dat";
		gSysPresetsFile=gSysDataDirectory+istring(ost::Path::dirDelim)+"presets.dat";

		ost::Path(gUserPresetsFile).Touch();



		// -- 1
			// if there is an error opening the registry file then
			// the system probably crashed... delete the registry file and
			// warn user that the previous run history is lost.. but that
			// they may beable to recover the last edits if they go load
			// the files that were being edited (since the pool files will
			// still exist for all previously open files)
		try
		{
			gSettingsRegistry=new CNestedDataFile(gUserDataDirectory+istring(ost::Path::dirDelim)+"registry.dat",true);
		}
		catch(exception &e)
		{
			// well, then start with an empty one
					// ??? call a function to setup the initial registry, we could either insert values manually, or copy a file from the share dir and maybe have to change a couple of things specific to this user
					// 	because later I expect there to be many necesary default settings
			gSettingsRegistry=new CNestedDataFile("",true);
			gSettingsRegistry->setFilename(gUserDataDirectory+"/registry.dat");

			Error(string("Error reading registry -- ")+e.what());
		}
		gPromptDialogDirectory=gSettingsRegistry->getValue("promptDialogDirectory");

		if(gSettingsRegistry->keyExists("followPlayPosition"))
			gFollowPlayPosition= gSettingsRegistry->getValue("followPlayPosition")=="true";

		if(gSettingsRegistry->keyExists("initialLengthToShow"))
			gInitialLengthToShow= atof(gSettingsRegistry->getValue("initialLengthToShow").c_str());

		if(gSettingsRegistry->keyExists("crossfadeEdges"))
		{
			gCrossfadeEdges= (CrossfadeEdgesTypes)atof(gSettingsRegistry->getValue("crossfadeEdges").c_str());
			gCrossfadeStartTime= atof(gSettingsRegistry->getValue("crossfadeStartTime").c_str());
			gCrossfadeStopTime= atof(gSettingsRegistry->getValue("crossfadeStopTime").c_str());
		}


		// -- 2
						// ??? this filename needs to be an application setting just as in CSound.cpp
		const string clipboardPoolFilename="/tmp/rezound.clipboard";
		remove(clipboardPoolFilename.c_str());
		AAction::clipboardPoolFile=new CSound::PoolFile_t();
		AAction::clipboardPoolFile->openFile(clipboardPoolFilename,true);


		// -- 3
		_soundPlayer=soundPlayer=new COSSSoundPlayer();


		// -- 4
		soundPlayer->initialize();

	}
	catch(exception &e)
	{
		Error(e.what());
		try { deinitializeBackend(); } catch(...) { /* oh well */ }
		exit(0);
	}
}

void deinitializeBackend()
{
	// reverse order of creation


	// -- 4
	if(soundPlayer!=NULL)
		soundPlayer->deinitialize();


	// -- 3
	delete soundPlayer;


	// -- 2
	if(AAction::clipboardPoolFile!=NULL)
	{
		if(AAction::clipboardPoolFile->isOpen())
			AAction::clipboardPoolFile->closeFile(false,true);
		delete AAction::clipboardPoolFile;
		AAction::clipboardPoolFile=NULL;
	}


	// -- 1
	gSettingsRegistry->createKey("promptDialogDirectory",gPromptDialogDirectory);
	gSettingsRegistry->createKey("followPlayPosition",gFollowPlayPosition ? "true" : "false");
	gSettingsRegistry->createKey("initialLengthToShow",gInitialLengthToShow);
	gSettingsRegistry->createKey("crossfadeEdges",(float)gCrossfadeEdges);
		gSettingsRegistry->createKey("crossfadeStartTime",gCrossfadeStartTime);
		gSettingsRegistry->createKey("crossfadeStopTime",gCrossfadeStopTime);


	gSettingsRegistry->save();
	delete gSettingsRegistry;

}


