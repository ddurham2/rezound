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
#include "AFrontendHooks.h"
#include "initialize.h"
#include "settings.h"

#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <stdexcept>
#include <string>

#include <CNestedDataFile/CNestedDataFile.h>



//#include "CWinSoundPlayer.h"
//static CWinSoundPlayer *soundPlayer=NULL;
#include "COSSSoundPlayer.h"
static COSSSoundPlayer *soundPlayer=NULL;


#include "AAction.h"
#include "CNativeSoundClipboard.h"
#include "CRecordSoundClipboard.h"


// for mkdir  --- possibly wouldn't port???
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>	// for getenv
#include <stdio.h>	// for possible printf of REZ_SHARE_DIR

#include <CPath.h>

void setupSoundTranslators();

void initializeBackend(ASoundPlayer *&_soundPlayer)
{
	try
	{
		setupSoundTranslators();

		// make sure that ~/.rezound exists
		gUserDataDirectory=string(getenv("HOME"))+istring(CPath::dirDelim)+".rezound";
		const int mkdirResult=mkdir(gUserDataDirectory.c_str(),0700);
		const int mkdirErrno=errno;
		if(mkdirResult!=0 && mkdirErrno!=EEXIST)
			throw(runtime_error(string(__func__)+" -- error creating "+gUserDataDirectory+" -- "+strerror(mkdirErrno)));



		// -- 1
			// if there is an error opening the registry file then
			// the system probably crashed... delete the registry file and
			// warn user that the previous run history is lost.. but that
			// they may beable to recover the last edits if they go load
			// the files that were being edited (since the pool files will
			// still exist for all previously open files)
		try
		{
			const string registryFilename=gUserDataDirectory+istring(CPath::dirDelim)+"registry.dat";
			CPath(registryFilename).touch();
			gSettingsRegistry=new CNestedDataFile(registryFilename,true);
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


		// determine where the share data is located
		{
			// first try env var
			const char *rezShareEnvVar=getenv("REZ_SHARE_DIR");
			if(rezShareEnvVar!=NULL && CPath(rezShareEnvVar).exists())
			{
				printf("using environment variable $REZ_SHARE_DIR='%s' to override normal setting for share data direcory\n",rezShareEnvVar);
				gSysDataDirectory=rezShareEnvVar;
			}
			// next try the source directory where the code was built
			else if(CPath(SOURCE_DIR"/share").exists())
				gSysDataDirectory=SOURCE_DIR"/share";
			// next try the registry setting
			else if(gSettingsRegistry->keyExists("shareDirectory") && CPath(gSettingsRegistry->getValue("shareDirectory")).exists())
				gSysDataDirectory=gSettingsRegistry->getValue("shareDirectory");
			// finally fall back on the #define set by configure saying where ReZound will be installed
			else
				gSysDataDirectory=DATA_DIR"/rezound";

			recheckShareDataDir:

			string checkFile=gSysDataDirectory+istring(CPath::dirDelim)+"presets.dat";

			// now, if the presets.dat file doesn't exist in the share data dir, ask for a setting
			if(!CPath(checkFile).exists())
			{
				if(Question("presets.dat not found in share data dir, '"+gSysDataDirectory+"'.\n  Would you like to manually select the share data directory directory?",yesnoQues)==yesAns)
				{
					gFrontendHooks->promptForDirectory(gSysDataDirectory,"Select Share Data Directory");
					goto recheckShareDataDir;
				}
			}

			// fully qualify the share data directory
			gSysDataDirectory=CPath(gSysDataDirectory).realPath();

			printf("using path '%s' for share data directory\n",gSysDataDirectory.c_str());
		}


		// parse the system presets
		gSysPresetsFilename=gSysDataDirectory+istring(CPath::dirDelim)+"presets.dat";
		gSysPresetsFile=new CNestedDataFile("",false);
		try
		{
			gSysPresetsFile->parseFile(gSysPresetsFilename);
		}
		catch(exception &e)
		{
			Error(e.what());
		}

		
		// parse the user presets
		gUserPresetsFilename=gUserDataDirectory+istring(CPath::dirDelim)+"presets.dat";
		CPath(gUserPresetsFilename).touch();
		gUserPresetsFile=new CNestedDataFile("",false);
		try
		{
			gUserPresetsFile->parseFile(gUserPresetsFilename);
		}
		catch(exception &e)
		{
			Error(e.what());
		}
		

		gPromptDialogDirectory=gSettingsRegistry->getValue("promptDialogDirectory");


		// where ReZound should fallback to put working files if it can't write to where it loaded a file from
			// ??? This could be an array where it would try multiple locations finding one that isn't full or close to full relative to the loaded file size
		if(gSettingsRegistry->keyExists("fallbackWorkDir"))
			gFallbackWorkDir= gSettingsRegistry->getValue("fallbackWorkDir");

		if(gSettingsRegistry->keyExists("whichClipboard"))
			gWhichClipboard= atoi(gSettingsRegistry->getValue("whichClipboard").c_str());

		if(gSettingsRegistry->keyExists("clipboardDir"))
			gClipboardDir= gSettingsRegistry->getValue("clipboardDir");

		if(gSettingsRegistry->keyExists("clipboardFilenamePrefix"))
			gClipboardFilenamePrefix= gSettingsRegistry->getValue("clipboardFilenamePrefix");

		if(gSettingsRegistry->keyExists("followPlayPosition"))
			gFollowPlayPosition= gSettingsRegistry->getValue("followPlayPosition")=="true";

		if(gSettingsRegistry->keyExists("initialLengthToShow"))
			gInitialLengthToShow= atof(gSettingsRegistry->getValue("initialLengthToShow").c_str());

		if(gSettingsRegistry->keyExists("crossfadeEdges"))
		{
			gCrossfadeEdges= (CrossfadeEdgesTypes)atof(gSettingsRegistry->getValue("crossfadeEdges").c_str());
			gCrossfadeStartTime= atof(gSettingsRegistry->getValue("crossfadeStartTime").c_str());
			gCrossfadeStopTime= atof(gSettingsRegistry->getValue("crossfadeStopTime").c_str());
			gCrossfadeFadeMethod= (CrossfadeFadeMethods)atof(gSettingsRegistry->getValue("crossfadeFadeMethod").c_str());
		}


		// -- 2
		AAction::clipboards.push_back(new CNativeSoundClipboard("Native Clipboard 1",gClipboardDir+CPath::dirDelim+gClipboardFilenamePrefix+".clipboard1"));
		AAction::clipboards.push_back(new CNativeSoundClipboard("Native Clipboard 2",gClipboardDir+CPath::dirDelim+gClipboardFilenamePrefix+".clipboard2"));
		AAction::clipboards.push_back(new CNativeSoundClipboard("Native Clipboard 3",gClipboardDir+CPath::dirDelim+gClipboardFilenamePrefix+".clipboard3"));
		AAction::clipboards.push_back(new CRecordSoundClipboard("Record Clipboard 1",gClipboardDir+CPath::dirDelim+gClipboardFilenamePrefix+".record1"));
		AAction::clipboards.push_back(new CRecordSoundClipboard("Record Clipboard 2",gClipboardDir+CPath::dirDelim+gClipboardFilenamePrefix+".record2"));
		AAction::clipboards.push_back(new CRecordSoundClipboard("Record Clipboard 3",gClipboardDir+CPath::dirDelim+gClipboardFilenamePrefix+".record3"));

			// make sure the global clipboard selector index is in range
		if(gWhichClipboard>=AAction::clipboards.size())
			gWhichClipboard=0; 


		// -- 3
		_soundPlayer=soundPlayer=new COSSSoundPlayer();


		// -- 4
		try
		{
			soundPlayer->initialize();
		}
		catch(exception &e)
		{
			Error(string(e.what())+"\nPlaying will be disabled.");
		}

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
	while(!AAction::clipboards.empty())
	{
		delete AAction::clipboards[0];
		AAction::clipboards.erase(AAction::clipboards.begin());
	}


	// -- 1
	gSettingsRegistry->createKey("shareDirectory",gSysDataDirectory);
	gSettingsRegistry->createKey("promptDialogDirectory",gPromptDialogDirectory);
	gSettingsRegistry->createKey("fallbackWorkDir",gFallbackWorkDir);
	gSettingsRegistry->createKey("clipboardDir",gClipboardDir);
	gSettingsRegistry->createKey("clipboardFilenamePrefix",gClipboardFilenamePrefix);
	gSettingsRegistry->createKey("whichClipboard",gWhichClipboard);
	gSettingsRegistry->createKey("followPlayPosition",gFollowPlayPosition ? "true" : "false");
	gSettingsRegistry->createKey("initialLengthToShow",gInitialLengthToShow);
	gSettingsRegistry->createKey("crossfadeEdges",(float)gCrossfadeEdges);
		gSettingsRegistry->createKey("crossfadeStartTime",gCrossfadeStartTime);
		gSettingsRegistry->createKey("crossfadeStopTime",gCrossfadeStopTime);
		gSettingsRegistry->createKey("crossfadeFadeMethod",(float)gCrossfadeFadeMethod);


	gSettingsRegistry->save();
	delete gSettingsRegistry;

}

#include "CrezSoundTranslator.h"
#include "ClibvorbisSoundTranslator.h"
#include "ClibaudiofileSoundTranslator.h"
#include "ClameSoundTranslator.h"
#include "CrawSoundTranslator.h"
#include "Cold_rezSoundTranslator.h"
void setupSoundTranslators()
{
	ASoundTranslator::registeredTranslators.clear();

	static const CrezSoundTranslator rezSoundTranslator;
	ASoundTranslator::registeredTranslators.push_back(&rezSoundTranslator);

#ifdef HAVE_LIBVORBIS
	static const ClibvorbisSoundTranslator libvorbisSoundTranslator;
	ASoundTranslator::registeredTranslators.push_back(&libvorbisSoundTranslator);
#endif

#ifdef HAVE_LIBAUDIOFILE
	static const ClibaudiofileSoundTranslator libaudiofileSoundTranslator;
	ASoundTranslator::registeredTranslators.push_back(&libaudiofileSoundTranslator);

	static const CrawSoundTranslator rawSoundTranslator;
	ASoundTranslator::registeredTranslators.push_back(&rawSoundTranslator);
#endif

	if(ClameSoundTranslator::checkForLame())
	{
		static const ClameSoundTranslator lameSoundTranslator;
		ASoundTranslator::registeredTranslators.push_back(&lameSoundTranslator);
	}

	static const Cold_rezSoundTranslator old_rezSoundTranslator;
	ASoundTranslator::registeredTranslators.push_back(&old_rezSoundTranslator);
	
}


