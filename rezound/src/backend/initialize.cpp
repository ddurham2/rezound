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

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <stdexcept>
#include <string>

#include <CNestedDataFile/CNestedDataFile.h>

#define DOT string(CNestedDataFile::delimChar)

// one or the other of these two will ifdef itself in or out based on HAVE_LIBPORTAUDIO
#include "CPortAudioSoundPlayer.h"
#include "COSSSoundPlayer.h"
static ASoundPlayer *soundPlayer=NULL;


#include "AAction.h"
#include "CNativeSoundClipboard.h"
#include "CRecordSoundClipboard.h"


// for mkdir  --- possibly wouldn't port???
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>	// for getenv

#include <CPath.h>

static void setupSoundTranslators();

static bool checkForHelpFlag(int argc,char *argv[]);
static bool checkForVersionFlag(int argc,char *argv[]);
static void printUsage(const string app);

bool initializeBackend(ASoundPlayer *&_soundPlayer,int argc,char *argv[])
{
	try
	{
		if(checkForHelpFlag(argc,argv))
			return false;
		if(checkForVersionFlag(argc,argv))
			return false;


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
		const string registryFilename=gUserDataDirectory+istring(CPath::dirDelim)+"registry.dat";
		try
		{
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

	
		if(gSettingsRegistry->keyExists("DesiredOutputSampleRate"))
			gDesiredOutputSampleRate= atoi(gSettingsRegistry->getValue("DesiredOutputSampleRate").c_str());

		if(gSettingsRegistry->keyExists("DesiredOutputChannelCount"))
			gDesiredOutputChannelCount= atoi(gSettingsRegistry->getValue("DesiredOutputChannelCount").c_str());

		if(gSettingsRegistry->keyExists("DesiredOutputBufferCount"))
			gDesiredOutputBufferCount= atoi(gSettingsRegistry->getValue("DesiredOutputBufferCount").c_str());
		gDesiredOutputBufferCount=max(2,gDesiredOutputBufferCount);

		if(gSettingsRegistry->keyExists("DesiredOutputBufferSize"))
			gDesiredOutputBufferSize= atoi(gSettingsRegistry->getValue("DesiredOutputBufferSize").c_str());
		if(gDesiredOutputBufferSize<256 || log((double)gDesiredOutputBufferSize)/log(2.0)!=floor(log((double)gDesiredOutputBufferSize)/log(2.0)))
			throw runtime_error(string(__func__)+" -- DesiredOutputBufferSize in "+registryFilename+" must be a power of 2 and >= than 256");


#ifdef HAVE_LIBPORTAUDIO
		if(gSettingsRegistry->keyExists("PortAudioOutputDevice"))
			gPortAudioOutputDevice= atoi(gSettingsRegistry->getValue("PortAudioOutputDevice").c_str());

		if(gSettingsRegistry->keyExists("PortAudioInputDevice"))
			gPortAudioInputDevice= atoi(gSettingsRegistry->getValue("PortAudioInputDevice").c_str());
#else
		if(gSettingsRegistry->keyExists("OSSOutputDevice"))
			gOSSOutputDevice= gSettingsRegistry->getValue("OSSOutputDevice");

		if(gSettingsRegistry->keyExists("OSSInputDevice"))
			gOSSInputDevice= gSettingsRegistry->getValue("OSSInputDevice");
#endif


		// where ReZound should fallback to put working files if it can't write to where it loaded a file from
			// ??? This could be an array where it would try multiple locations finding one that isn't full or close to full relative to the loaded file size
		if(gSettingsRegistry->keyExists("fallbackWorkDir"))
			gFallbackWorkDir= gSettingsRegistry->getValue("fallbackWorkDir");


		if(gSettingsRegistry->keyExists("clipboardDir"))
			gClipboardDir= gSettingsRegistry->getValue("clipboardDir");

		if(gSettingsRegistry->keyExists("clipboardFilenamePrefix"))
			gClipboardFilenamePrefix= gSettingsRegistry->getValue("clipboardFilenamePrefix");

		if(gSettingsRegistry->keyExists("whichClipboard"))
			gWhichClipboard= atoi(gSettingsRegistry->getValue("whichClipboard").c_str());

		if(gSettingsRegistry->keyExists(("ReopenHistory"+DOT+"maxReopenHistory").c_str()))
			gMaxReopenHistory= atoi(gSettingsRegistry->getValue(("ReopenHistory"+DOT+"maxReopenHistory").c_str()).c_str());

		if(gSettingsRegistry->keyExists("followPlayPosition"))
			gFollowPlayPosition= gSettingsRegistry->getValue("followPlayPosition")=="true";

		if(gSettingsRegistry->keyExists("levelMetersEnabled"))
			gLevelMetersEnabled= gSettingsRegistry->getValue("levelMetersEnabled")=="true";

		if(gSettingsRegistry->keyExists("frequencyAnalyzerEnabled"))
			gFrequencyAnalyzerEnabled= gSettingsRegistry->getValue("frequencyAnalyzerEnabled")=="true";

		if(gSettingsRegistry->keyExists("initialLengthToShow"))
			gInitialLengthToShow= atof(gSettingsRegistry->getValue("initialLengthToShow").c_str());

		if(gSettingsRegistry->keyExists(("Meters"+DOT+"meterUpdateTime").c_str()))
			gMeterUpdateTime= atoi(gSettingsRegistry->getValue(("Meters"+DOT+"meterUpdateTime").c_str()).c_str());
		if(gSettingsRegistry->keyExists(("Meters"+DOT+"Level"+DOT+"RMSWindowTime").c_str()))
			gMeterRMSWindowTime= atoi(gSettingsRegistry->getValue(("Meters"+DOT+"Level"+DOT+"RMSWindowTime").c_str()).c_str());
		if(gSettingsRegistry->keyExists(("Meters"+DOT+"Level"+DOT+"maxPeakFallDelayTime").c_str()))
			gMaxPeakFallDelayTime= atoi(gSettingsRegistry->getValue(("Meters"+DOT+"Level"+DOT+"maxPeakFallDelayTime").c_str()).c_str());
		if(gSettingsRegistry->keyExists(("Meters"+DOT+"Level"+DOT+"maxPeakFallRate").c_str()))
			gMaxPeakFallRate= atof(gSettingsRegistry->getValue(("Meters"+DOT+"Level"+DOT+"maxPeakFallRate").c_str()).c_str());
		if(gSettingsRegistry->keyExists(("Meters"+DOT+"Analyzer"+DOT+"peakFallDelayTime").c_str()))
			gAnalyzerPeakFallDelayTime= atoi(gSettingsRegistry->getValue(("Meters"+DOT+"Analyzer"+DOT+"peakFallDelayTime").c_str()).c_str());
		if(gSettingsRegistry->keyExists(("Meters"+DOT+"Analyzer"+DOT+"peakFallRate").c_str()))
			gAnalyzerPeakFallRate= atof(gSettingsRegistry->getValue(("Meters"+DOT+"Analyzer"+DOT+"peakFallRate").c_str()).c_str());


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
#ifdef HAVE_LIBPORTAUDIO
		_soundPlayer=soundPlayer=new CPortAudioSoundPlayer();
#else
		_soundPlayer=soundPlayer=new COSSSoundPlayer();
#endif


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

	return true;
}

#include "ASoundFileManager.h"
bool handleMoreBackendArgs(ASoundFileManager *fileManager,int argc,char *argv[])
{
	bool forceFilenames=false;
	for(int t=1;t<argc;t++)
	{
		if(strcmp(argv[t],"--")==0)
		{ // anything after a '--' flag is assumed as a filename to load
			forceFilenames=true;
			continue;
		}

#ifdef HAVE_LIBAUDIOFILE
		// also handle a --raw flag to force the loading of the following argument as a file
		if(!forceFilenames && strcmp(argv[t],"--raw")==0)
		{
			if(t>=argc-1)
			{
				printUsage(argv[0]);
				return(false);
			}

			t++; // next arg is filename
			const string filename=argv[t];

			try
			{
				fileManager->open(filename,true);
			}
			catch(exception &e)
			{
				Error(e.what());
			}
		}
		else
#endif
		// load as a filename if the first char of the arg is not a '-'
		if(forceFilenames || argv[t][0]!='-')
		{ // not a flag
			const string filename=argv[t];
			try
			{
				fileManager->open(filename);
			}
			catch(exception &e)
			{
				Error(e.what());
			}
		}
	}

	return true;
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


	gSettingsRegistry->createKey("DesiredOutputSampleRate",gDesiredOutputSampleRate);
	gSettingsRegistry->createKey("DesiredOutputChannelCount",gDesiredOutputChannelCount);
	gSettingsRegistry->createKey("DesiredOutputBufferCount",gDesiredOutputBufferCount);
	gSettingsRegistry->createKey("DesiredOutputBufferSize",gDesiredOutputBufferSize);


#ifdef HAVE_LIBPORTAUDIO
	gSettingsRegistry->createKey("PortAudioOutputDevice",gPortAudioOutputDevice);
	gSettingsRegistry->createKey("PortAudioInputDevice",gPortAudioInputDevice);
#else
	gSettingsRegistry->createKey("OSSOutputDevice",gOSSOutputDevice);
	gSettingsRegistry->createKey("OSSInputDevice",gOSSInputDevice);
#endif

	gSettingsRegistry->createKey("fallbackWorkDir",gFallbackWorkDir);

	gSettingsRegistry->createKey("clipboardDir",gClipboardDir);
	gSettingsRegistry->createKey("clipboardFilenamePrefix",gClipboardFilenamePrefix);
	gSettingsRegistry->createKey("whichClipboard",gWhichClipboard);

	gSettingsRegistry->createKey(("ReopenHistory"+DOT+"maxReopenHistory").c_str(),gMaxReopenHistory);

	gSettingsRegistry->createKey("followPlayPosition",gFollowPlayPosition ? "true" : "false");

	gSettingsRegistry->createKey("levelMetersEnabled",gLevelMetersEnabled ? "true" : "false");
	gSettingsRegistry->createKey("frequencyAnalyzerEnabled",gFrequencyAnalyzerEnabled ? "true" : "false");

	gSettingsRegistry->createKey("initialLengthToShow",gInitialLengthToShow);

	gSettingsRegistry->createKey(("Meters"+DOT+"meterUpdateTime").c_str(),gMeterUpdateTime);
	gSettingsRegistry->createKey(("Meters"+DOT+"Level"+DOT+"RMSWindowTime").c_str(),gMeterRMSWindowTime);
	gSettingsRegistry->createKey(("Meters"+DOT+"Level"+DOT+"maxPeakFallDelayTime").c_str(),gMaxPeakFallDelayTime);
	gSettingsRegistry->createKey(("Meters"+DOT+"Level"+DOT+"maxPeakFallRate").c_str(),gMaxPeakFallRate);
	gSettingsRegistry->createKey(("Meters"+DOT+"Analyzer"+DOT+"peakFallDelayTime").c_str(),gAnalyzerPeakFallDelayTime);
	gSettingsRegistry->createKey(("Meters"+DOT+"Analyzer"+DOT+"peakFallRate").c_str(),gAnalyzerPeakFallRate);

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
#include "CvoxSoundTranslator.h"
#include "CrawSoundTranslator.h"
#include "Cold_rezSoundTranslator.h"
static void setupSoundTranslators()
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

	if(ClameSoundTranslator::checkForApp())
	{
		static const ClameSoundTranslator lameSoundTranslator;
		ASoundTranslator::registeredTranslators.push_back(&lameSoundTranslator);
	}

	if(CvoxSoundTranslator::checkForApp())
	{
		static const CvoxSoundTranslator voxSoundTranslator;
		ASoundTranslator::registeredTranslators.push_back(&voxSoundTranslator);
	}

	static const Cold_rezSoundTranslator old_rezSoundTranslator;
	ASoundTranslator::registeredTranslators.push_back(&old_rezSoundTranslator);
	
}


static bool checkForHelpFlag(int argc,char *argv[])
{
	for(int t=1;t<argc;t++)
	{
		if(strcmp(argv[t],"--")==0)
			break; // stop at '--'

		if(strcmp(argv[t],"--help")==0)
		{
			printUsage(argv[0]);
			return true;
		}
	}
	return false;
}

static bool checkForVersionFlag(int argc,char *argv[])
{
	for(int t=1;t<argc;t++)
	{
		if(strcmp(argv[t],"--")==0)
			break; // stop at '--'

		if(strcmp(argv[t],"--version")==0)
		{
			printf("%s %s\n",REZOUND_PACKAGE,REZOUND_VERSION);
			printf("Written primarily by David W. Durham\nSee the AUTHORS document for more details\n\n");
			printf("This is free software; see the source for copying conditions.  There is NO\nwarranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
			printf("\n");
			printf("Project homepage\n\thttp://rezound.sourceforge.net\n");
			printf("Please report bugs to\n\thttp://sourceforge.net/tracker/?atid=105056&group_id=5056\n");
			printf("\n");
			return true;
		}
	}
	return false;
}

static void printUsage(const string app)
{
	printf("Usage: %s [option | filename]... [-- [filename]...]\n",app.c_str());
	printf("Options:\n");
#ifdef HAVE_LIBAUDIOFILE
	printf("\t--raw filename   load filename interpreted as raw data\n");
#endif                             
	printf("\n");              
	printf("\t--help           show this help message and exit\n");
	printf("\t--version        show version information and exit\n");

	printf("\n");
	printf("Notes:\n");
	printf("\t- Anything after a '--' flag will be assumed as a filename to load\n");
	printf("\t- The file ~/.rezound/registry.dat does contain some settings that\n\t  can only be changed by editing the file (right now)\n");

	printf("\n");
	printf("Project homepage\n\thttp://rezound.sourceforge.net\n");
	printf("Please report bugs to\n\thttp://sourceforge.net/tracker/?atid=105056&group_id=5056\n");
	
	printf("\n");
}

