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

#include "../../config/common.h"

#include "CStatusComm.h"
#include "initialize.h"
#include "settings.h"

#include <stdint.h>
#include <errno.h>
#include <string.h>

#include <stdexcept>
#include <string>

#include <CStringDiskTable.h>


// ... except for a few things.. this could probably be moved into the backend.. it'd have to be done for all frontends if I didn't move it

#include "../backend/file.h"

#include "../backend/CSoundManager.h"
static CSoundManager *soundManager=NULL;

//#include <CWinSoundPlayer.h>
//static CWinSoundPlayer *soundPlayer=NULL;
#include "../backend/COSSSoundPlayer.h"
static COSSSoundPlayer *soundPlayer=NULL;

#include <TPoolFile.h>
static TPoolFile<unsigned,unsigned> *loadedRegistryPoolFile=NULL;


#include "CSoundFileManager.h"

#include "../backend/AAction.h"


// for mkdir  --- possibly wouldn't port???
#include <sys/stat.h>
#include <sys/types.h>

#include <cc++/path.h>

void initializeReZound()
{
	try
	{

		// make sure that ~/.rezound exists
		gUserDataDirectory=string(getenv("HOME"))+"/.rezound";
		const int mkdirResult=mkdir(gUserDataDirectory.c_str(),0700);
		const int mkdirErrno=errno;
		if(mkdirResult!=0 && mkdirErrno!=EEXIST)
			throw(runtime_error(string(__func__)+" -- error creating "+gUserDataDirectory+" -- "+strerror(mkdirErrno)));

		// determine where /usr/share/rezound has been placed (try the install-from directory first)
		gSysDataDirectory=SOURCE_DIR"/share/rezound";
		if(!ost::Path(gSysDataDirectory).Exists()) 
			gSysDataDirectory=INSTALL_PREFIX"/share/rezound";

		// -- 1
		// ??? I probably want to use an XML library or CNestedDataFile class for the registry instead of a pool file so it can be edited more easily
		loadedRegistryPoolFile=new TPoolFile<unsigned,unsigned>(4096,"ReZoundR");
		loadedRegistryPoolFile->openFile(gUserDataDirectory+"/registry.dat",true);


		// -- 2
			// if there is an error opening the registry file then
			// the system probably crashed... delete the registry file and
			// warn user that the previous run history is lost.. but that
			// they may beable to recover the last edits if they go load
			// the files that were being edited (since the pool files will
			// still exist for all previously open files)
		gSettingsRegistry=new CStringDiskTable(*loadedRegistryPoolFile,"ReZound Settings");
		gPromptDialogDirectory=gSettingsRegistry->getValue("promptDialogDirectory");


		// -- 3
						// ??? this filename needs to be an application setting just as in ASound.cpp
		const string clipboardPoolFilename="/tmp/rezound.clipboard";
		remove(clipboardPoolFilename.c_str());
		AAction::clipboardPoolFile=new ASound::PoolFile_t();
		AAction::clipboardPoolFile->openFile(clipboardPoolFilename,true);


		// -- 4
		soundPlayer=new COSSSoundPlayer();


		// -- 5
		soundManager=new CSoundManager();


		// -- 6
		gSoundFileManager=new CSoundFileManager(*soundManager,*soundPlayer,*loadedRegistryPoolFile);


		// -- 7
		soundPlayer->initialize();

	}
	catch(exception &e)
	{
		Error(e.what());
		try { deinitializeReZound(); } catch(...) { /* oh well */ }
		exit(0);
	}
}

void deinitializeReZound()
{
	// reverse order of creation


	// -- 7
	if(soundPlayer!=NULL)
		soundPlayer->deinitialize();


	// -- 6
	delete gSoundFileManager;


	// -- 5
	delete soundManager;


	// -- 4
	delete soundPlayer;


	// -- 3
	if(AAction::clipboardPoolFile!=NULL)
	{
		if(AAction::clipboardPoolFile->isOpen())
			AAction::clipboardPoolFile->closeFile(false,true);
		delete AAction::clipboardPoolFile;
		AAction::clipboardPoolFile=NULL;
	}


	// -- 2
	gPromptDialogDirectory=istring(gPromptDialogDirectory).truncate(_CSDT_STRING_SIZE-1); // define from CStringDiskTable.h
	gSettingsRegistry->setValue("promptDialogDirectory",gPromptDialogDirectory);
	delete gSettingsRegistry;


	// -- 1
	if(loadedRegistryPoolFile!=NULL)
		loadedRegistryPoolFile->closeFile(true,false);
	delete loadedRegistryPoolFile;
}


