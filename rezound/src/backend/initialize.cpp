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

#include "ASoundPlayer.h"
static ASoundPlayer *gSoundPlayer=NULL; // saved value for deinitialize


#include "AAction.h"
#include "CNativeSoundClipboard.h"
#include "CRecordSoundClipboard.h"


// for mkdir  --- possibly wouldn't port???
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>	// for getenv

#include <CPath.h>

static bool checkForHelpFlag(int argc,char *argv[]);
static bool checkForVersionFlag(int argc,char *argv[]);
static void printUsage(const string app);

bool initializeBackend(ASoundPlayer *&soundPlayer,int argc,char *argv[])
{
	try
	{
		if(checkForHelpFlag(argc,argv))
			return false;
		if(checkForVersionFlag(argc,argv))
			return false;


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
			else if(gSettingsRegistry->keyExists("shareDirectory") && CPath(gSettingsRegistry->getValue<string>("shareDirectory")).exists())
				gSysDataDirectory=gSettingsRegistry->getValue<string>("shareDirectory");
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
		
		#define GET_SETTING(key,variable,type)					\
			if(gSettingsRegistry->keyExists((key)))				\
				variable= gSettingsRegistry->getValue<type>((key));

		GET_SETTING("promptDialogDirectory",gPromptDialogDirectory,string)

	
		GET_SETTING("DesiredOutputSampleRate",gDesiredOutputSampleRate,unsigned)
		GET_SETTING("DesiredOutputChannelCount",gDesiredOutputChannelCount,unsigned)
		GET_SETTING("DesiredOutputBufferCount",gDesiredOutputBufferCount,int)
			gDesiredOutputBufferCount=max(2,gDesiredOutputBufferCount);
		GET_SETTING("DesiredOutputBufferSize",gDesiredOutputBufferSize,unsigned)
			if(gDesiredOutputBufferSize<256 || log((double)gDesiredOutputBufferSize)/log(2.0)!=floor(log((double)gDesiredOutputBufferSize)/log(2.0)))
				throw runtime_error(string(__func__)+" -- DesiredOutputBufferSize in "+registryFilename+" must be a power of 2 and >= than 256");


#ifdef ENABLE_OSS
		GET_SETTING("OSSOutputDevice",gOSSOutputDevice,string)
		GET_SETTING("OSSInputDevice",gOSSInputDevice,string)
#endif

#ifdef ENABLE_PORTAUDIO
		GET_SETTING("PortAudioOutputDevice",gPortAudioOutputDevice,int)
		GET_SETTING("PortAudioInputDevice",gPortAudioInputDevice,int)
#endif

#ifdef ENABLE_JACK
		// ??? could do these with vector values I suppose
		for(unsigned t=0;t<MAX_CHANNELS;t++)
		{
			GET_SETTING("JACKOutputPortName"+istring(t+1),gJACKOutputPortNames[t],string)
			else
				break;
		}
	
		for(unsigned t=0;t<MAX_CHANNELS;t++)
		{
			GET_SETTING("JACKInputPortName"+istring(t+1),gJACKInputPortNames[t],string)
			else
				break;
		}
#endif


		// where ReZound should fallback to put working files if it can't write to where it loaded a file from
			// ??? This could be a vector where it would try multiple locations finding one that isn't full or close to full relative to the loaded file size
		GET_SETTING("fallbackWorkDir",gFallbackWorkDir,string)

		GET_SETTING("clipboardDir",gClipboardDir,string)

		GET_SETTING("clipboardFilenamePrefix",gClipboardFilenamePrefix,string)

		GET_SETTING("whichClipboard",gWhichClipboard,size_t)

		GET_SETTING("ReopenHistory" DOT "maxReopenHistory",gMaxReopenHistory,size_t)

		// ??? where are the SnapToCues handlers?

		GET_SETTING("followPlayPosition",gFollowPlayPosition,bool)

		GET_SETTING("renderClippingWarning",gRenderClippingWarning,bool)

		GET_SETTING("skipMiddleMarginSeconds",gSkipMiddleMarginSeconds,float)

		GET_SETTING("loopGapLengthSeconds",gLoopGapLengthSeconds,float)

		GET_SETTING("initialLengthToShow",gInitialLengthToShow,double)


		GET_SETTING("Meters" DOT "meterUpdateTime",gMeterUpdateTime,unsigned)

		GET_SETTING("Meters" DOT "Level" DOT "enabled",gLevelMetersEnabled,bool)
		GET_SETTING("Meters" DOT "Level" DOT "RMSWindowTime",gMeterRMSWindowTime,unsigned)
		GET_SETTING("Meters" DOT "Level" DOT "maxPeakFallDelayTime",gMaxPeakFallDelayTime,unsigned)
		GET_SETTING("Meters" DOT "Level" DOT "maxPeakFallRate",gMaxPeakFallRate,double)

		GET_SETTING("Meters" DOT "StereoPhase" DOT "enabled",gStereoPhaseMetersEnabled,bool)
		GET_SETTING("Meters" DOT "StereoPhase" DOT "pointCount",gStereoPhaseMeterPointCount,unsigned)

		GET_SETTING("Meters" DOT "Analyzer" DOT "enabled",gFrequencyAnalyzerEnabled,bool)
		GET_SETTING("Meters" DOT "Analyzer" DOT "peakFallDelayTime",gAnalyzerPeakFallDelayTime,unsigned)
		GET_SETTING("Meters" DOT "Analyzer" DOT "peakFallRate",gAnalyzerPeakFallRate,double)


		if(gSettingsRegistry->keyExists("crossfadeEdges"))
		{
			gCrossfadeEdges= (CrossfadeEdgesTypes)gSettingsRegistry->getValue<int>("crossfadeEdges");
			gCrossfadeStartTime= gSettingsRegistry->getValue<float>("crossfadeStartTime");
			gCrossfadeStopTime= gSettingsRegistry->getValue<float>("crossfadeStopTime");
			gCrossfadeFadeMethod= (CrossfadeFadeMethods)gSettingsRegistry->getValue<int>("crossfadeFadeMethod");
		}


		// -- 2
		soundPlayer=gSoundPlayer=ASoundPlayer::createInitializedSoundPlayer();


		// -- 3
		for(unsigned t=1;t<=3;t++)
		{
			const string filename=gClipboardDir+CPath::dirDelim+gClipboardFilenamePrefix+".clipboard"+istring(t)+"."+istring(getuid());
			try
			{
				AAction::clipboards.push_back(new CNativeSoundClipboard("Native Clipboard "+istring(t),filename));
			}
			catch(exception &e)
			{
				Warning(e.what());
				remove(filename.c_str());
			}
		}
		for(unsigned t=1;t<=3;t++)
		{
			const string filename=gClipboardDir+CPath::dirDelim+gClipboardFilenamePrefix+".record"+istring(t)+"."+istring(getuid());
			try
			{
				AAction::clipboards.push_back(new CRecordSoundClipboard("Record Clipboard "+istring(t),filename,soundPlayer));
			}
			catch(exception &e)
			{
				Warning(e.what());
				remove(filename.c_str());
			}
		}


			// make sure the global clipboard selector index is in range
		if(gWhichClipboard>=AAction::clipboards.size())
			gWhichClipboard=0; 
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


	// -- 3
	while(!AAction::clipboards.empty())
	{
		delete AAction::clipboards[0];
		AAction::clipboards.erase(AAction::clipboards.begin());
	}


	// -- 2
	if(gSoundPlayer!=NULL)
	{
		gSoundPlayer->deinitialize();
		delete gSoundPlayer;
	}


	// -- 1
	gSettingsRegistry->createValue<string>("shareDirectory",gSysDataDirectory);
	gSettingsRegistry->createValue<string>("promptDialogDirectory",gPromptDialogDirectory);


	gSettingsRegistry->createValue<unsigned>("DesiredOutputSampleRate",gDesiredOutputSampleRate);
	gSettingsRegistry->createValue<unsigned>("DesiredOutputChannelCount",gDesiredOutputChannelCount);
	gSettingsRegistry->createValue<int>("DesiredOutputBufferCount",gDesiredOutputBufferCount);
	gSettingsRegistry->createValue<unsigned>("DesiredOutputBufferSize",gDesiredOutputBufferSize);


#ifdef ENABLE_OSS
	gSettingsRegistry->createValue<string>("OSSOutputDevice",gOSSOutputDevice);
	gSettingsRegistry->createValue<string>("OSSInputDevice",gOSSInputDevice);
#endif

#ifdef ENABLE_PORTAUDIO
	gSettingsRegistry->createValue<int>("PortAudioOutputDevice",gPortAudioOutputDevice);
	gSettingsRegistry->createValue<int>("PortAudioInputDevice",gPortAudioInputDevice);
#endif

#ifdef ENABLE_JACK
	// ??? could do these with vector<string> values I suppose
	for(unsigned t=0;t<MAX_CHANNELS;t++)
	{
		if(gJACKOutputPortNames[t]!="")
			gSettingsRegistry->createValue<string>("JACKOutputPortName"+istring(t+1),gJACKOutputPortNames[t]);
		else
		{
			gSettingsRegistry->removeKey("JACKOutputPortName"+istring(t+2));
			break;
		}
	}
	for(unsigned t=0;t<MAX_CHANNELS;t++)
	{
		if(gJACKInputPortNames[t]!="")
			gSettingsRegistry->createValue<string>("JACKInputPortName"+istring(t+1),gJACKInputPortNames[t]);
		else
		{
			gSettingsRegistry->removeKey("JACKInputPortName"+istring(t+2));
			break;
		}
	}
#endif

	gSettingsRegistry->createValue<string>("fallbackWorkDir",gFallbackWorkDir);

	gSettingsRegistry->createValue<string>("clipboardDir",gClipboardDir);
	gSettingsRegistry->createValue<string>("clipboardFilenamePrefix",gClipboardFilenamePrefix);
	gSettingsRegistry->createValue<size_t>("whichClipboard",gWhichClipboard);

	gSettingsRegistry->createValue<size_t>("ReopenHistory" DOT "maxReopenHistory",gMaxReopenHistory);

	gSettingsRegistry->createValue<bool>("followPlayPosition",gFollowPlayPosition);

	gSettingsRegistry->createValue<bool>("renderClippingWarning",gRenderClippingWarning);

	gSettingsRegistry->createValue<float>("skipMiddleMarginSeconds",gSkipMiddleMarginSeconds);
	gSettingsRegistry->createValue<float>("loopGapLengthSeconds",gLoopGapLengthSeconds);

	gSettingsRegistry->createValue<double>("initialLengthToShow",gInitialLengthToShow);

	gSettingsRegistry->createValue<unsigned>("Meters" DOT "meterUpdateTime",gMeterUpdateTime);

	gSettingsRegistry->createValue<bool>("Meters" DOT "Level" DOT "enabled",gLevelMetersEnabled);
	gSettingsRegistry->createValue<unsigned>("Meters" DOT "Level" DOT "RMSWindowTime",gMeterRMSWindowTime);
	gSettingsRegistry->createValue<unsigned>("Meters" DOT "Level" DOT "maxPeakFallDelayTime",gMaxPeakFallDelayTime);
	gSettingsRegistry->createValue<double>("Meters" DOT "Level" DOT "maxPeakFallRate",gMaxPeakFallRate);

	gSettingsRegistry->createValue<bool>("Meters" DOT "StereoPhase" DOT "enabled",gStereoPhaseMetersEnabled);
	gSettingsRegistry->createValue<unsigned>("Meters" DOT "StereoPhase" DOT "pointCount",gStereoPhaseMeterPointCount);

	gSettingsRegistry->createValue<bool>("Meters" DOT "Analyzer" DOT "enabled",gFrequencyAnalyzerEnabled);
	gSettingsRegistry->createValue<unsigned>("Meters" DOT "Analyzer" DOT "peakFallDelayTime",gAnalyzerPeakFallDelayTime);
	gSettingsRegistry->createValue<double>("Meters" DOT "Analyzer" DOT "peakFallRate",gAnalyzerPeakFallRate);

	gSettingsRegistry->createValue<int>("crossfadeEdges",(int)gCrossfadeEdges);
	gSettingsRegistry->createValue<float>("crossfadeStartTime",gCrossfadeStartTime);
	gSettingsRegistry->createValue<float>("crossfadeStopTime",gCrossfadeStopTime);
	gSettingsRegistry->createValue<int>("crossfadeFadeMethod",(int)gCrossfadeFadeMethod);


	gSettingsRegistry->save();
	delete gSettingsRegistry;

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
	printf("Project homepage:\n\thttp://rezound.sourceforge.net\n");
	printf("Please report bugs to:\n\thttp://sourceforge.net/tracker/?atid=105056&group_id=5056\n");

	printf("Please consider joining the ReZound-users mailing list:\n\thttp://lists.sourceforge.net/lists/listinfo/rezound-users\n");

	
	printf("\n");
}

