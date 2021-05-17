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

#include "settings.h"

#include <stddef.h>
#include <stdlib.h> // for getenv

#include <istring>

CNestedDataFile *gSettingsRegistry=NULL;

CNestedDataFile *gUserMacroStore=NULL;

CNestedDataFile *gKeyBindingsStore=NULL;
const CNestedDataFile *gDefaultKeyBindingsStore=NULL;


string gPromptDialogDirectory="";


string gUserDataDirectory="";
string gSysDataDirectory="";

const string encodeFilenamePresetParameter(const string _filename)
{
	string filename=_filename;
	if(gSysDataDirectory!="" && filename.find(gSysDataDirectory+"/")==0)
		filename.replace(0,gSysDataDirectory.size(),"$share");
	return filename;
}

const string decodeFilenamePresetParameter(const string _filename)
{
	string filename=_filename;
	if(gSysDataDirectory!="" && filename.find("$share/")==0)
		filename.replace(0,6,gSysDataDirectory);
	return filename;
}


string gUserPresetsFilename="";
CNestedDataFile *gUserPresetsFile=NULL;

string gSysPresetsFilename="";
CNestedDataFile *gSysPresetsFile=NULL;


string gDefaultAudioMethod="";

unsigned gDesiredOutputSampleRate=44100;
unsigned gDesiredOutputChannelCount=2;
int gDesiredOutputBufferCount=2;
unsigned gDesiredOutputBufferSize=2048; // in frames (must be a power of 2)


#ifdef ENABLE_OSS
string gOSSOutputDevice="/dev/dsp";
string gOSSInputDevice="/dev/dsp";
#endif
#ifdef ENABLE_ALSA
string gALSAOutputDevice="hw:0";
string gALSAInputDevice="hw:0";
#endif
#ifdef ENABLE_PORTAUDIO
int gPortAudioOutputDevice=0;
int gPortAudioInputDevice=0;
#endif
#ifdef ENABLE_JACK
string gJACKOutputPortNames[64];
string gJACKInputPortNames[64];
#endif


string gLADSPAPath="";


string gFallbackWorkDir="/tmp"; // ??? would be something else on non-unix platforms
string gPrimaryWorkDir="";


string gClipboardDir="/tmp"; // ??? would be something else on non-unix platforms
string gClipboardFilenamePrefix="rezclip";
size_t gWhichClipboard=0;


size_t gMaxReopenHistory=16;


float gSkipMiddleMarginSeconds=2.0;
float gLoopGapLengthSeconds=0.75;

float gPlayPositionShift=-0.08; // a guess at latency between hearing and where it would place the cue

string gAddCueWhilePlaying_CueName="";
bool gAddCueWhilePlaying_Anchored=false;

string gAddCueAtClick_CueName="";
bool gAddCueAtClick_Anchored=false;


unsigned gMeterUpdateTime=50;

bool gLevelMetersEnabled=true;
unsigned gMeterRMSWindowTime=200;
unsigned gMaxPeakFallDelayTime=500;
double gMaxPeakFallRate=0.02;

bool gStereoPhaseMetersEnabled=true;
unsigned gStereoPhaseMeterPointCount=100;
bool gStereoPhaseMeterUnrotate=true;

bool gFrequencyAnalyzerEnabled=true;
unsigned gAnalyzerPeakFallDelayTime=400;
double gAnalyzerPeakFallRate=0.025;


CrossfadeEdgesTypes gCrossfadeEdges=cetInner;
float gCrossfadeStartTime=10.0;	 // default 20ms crossfade time
float gCrossfadeStopTime=10.0;
CrossfadeFadeMethods gCrossfadeFadeMethod=cfmLinear;



map<string,AActionFactory *> gRegisteredActionFactories;

// ----------------------------------------------------------------------------

#include "AStatusComm.h"
#include "AFrontendHooks.h"
#include "CSound_defs.h"

#include <stdio.h>

#include <stdexcept>

#include <CPath.h>
#include <CNestedDataFile/CNestedDataFile.h>

/* read backend setting variables with the exception of gUserDataDir */
void readBackendSettings()
{
	// make sure that ~/.rezound exists
	gUserDataDirectory=string(getenv("HOME"))+istring(CPath::dirDelim)+".rezound";
	const int mkdirResult=mkdir(gUserDataDirectory.c_str(),0700);
	const int mkdirErrno=errno;
	if(mkdirResult!=0 && mkdirErrno!=EEXIST)
		throw(runtime_error(string(__func__)+" -- error creating "+gUserDataDirectory+" -- "+strerror(mkdirErrno)));


		// if there is an error opening the registry file then
		// the system probably crashed... delete the registry file and
		// warn user that the previous run history is lost.. but that
		// they may beable to recover the last edits if they go load
		// the files that were being edited (since the pool files will
		// still exist for all previously open files)
	const string registryFilename=gUserDataDirectory+istring(CPath::dirDelim)+"registry.dat";
		// ??? this can be more easily handled with a createIfMissing flag 
	try
	{
		delete gSettingsRegistry;
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
		// TODO this needs to be something we can alter at install time.. at least the whole prefix may need to be re-written or simply computed from the location of the binary.. and let all values which are based on prefix use the variable rather than hardcoding it.. rather, hardcode a single instance of prefix within the binary or none.. and always compute based on binary location?.. unless it was specified at configure time.. actually need it to not be configurable if we're going to assume relative path from binary
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
	
	GET_SETTING("promptDialogDirectory",gPromptDialogDirectory,string)


	GET_SETTING("DesiredOutputSampleRate",gDesiredOutputSampleRate,unsigned)
	GET_SETTING("DesiredOutputChannelCount",gDesiredOutputChannelCount,unsigned)
	GET_SETTING("DesiredOutputBufferCount",gDesiredOutputBufferCount,int)
		gDesiredOutputBufferCount=max(2,gDesiredOutputBufferCount);
	GET_SETTING("DesiredOutputBufferSize",gDesiredOutputBufferSize,unsigned)
		if(gDesiredOutputBufferSize<256 || (gDesiredOutputBufferSize & (gDesiredOutputBufferSize-1)))
			throw runtime_error(string(__func__)+" -- DesiredOutputBufferSize in "+gSettingsRegistry->getFilename()+" must be a power of 2 and >= than 256");


#ifdef ENABLE_OSS
	GET_SETTING("OSSOutputDevice",gOSSOutputDevice,string)
	GET_SETTING("OSSInputDevice",gOSSInputDevice,string)
#endif

#ifdef ENABLE_ALSA
	GET_SETTING("ALSAOutputDevice",gALSAOutputDevice,string)
	GET_SETTING("ALSAInputDevice",gALSAInputDevice,string)
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

	GET_SETTING("LADSPA_PATH",gLADSPAPath,string)
	if(gLADSPAPath=="")
	{
		if(getenv("LADSPA_PATH")==NULL)
		{
    			fprintf(stderr,"Notice: You do not have a LADSPA_PATH environment variable set.  Defaulting to \"/usr/local/lib/ladspa:/usr/lib/ladspa\"\n");
			gLADSPAPath="/usr/local/lib/ladspa:/usr/lib/ladspa";
		}
		else
			gLADSPAPath=mnn(getenv("LADSPA_PATH"));
	}

	// where ReZound should fallback to put working files if it can't write to where it loaded a file from
		// ??? This could be a vector where it would try multiple locations finding one that isn't full or close to full relative to the loaded file size
	GET_SETTING("primaryWorkDir",gPrimaryWorkDir,string)
	GET_SETTING("fallbackWorkDir",gFallbackWorkDir,string)

	GET_SETTING("clipboardDir",gClipboardDir,string)

	GET_SETTING("clipboardFilenamePrefix",gClipboardFilenamePrefix,string)

	GET_SETTING("whichClipboard",gWhichClipboard,size_t)

	GET_SETTING("ReopenHistory" DOT "maxReopenHistory",gMaxReopenHistory,size_t)

	GET_SETTING("skipMiddleMarginSeconds",gSkipMiddleMarginSeconds,float)

	GET_SETTING("loopGapLengthSeconds",gLoopGapLengthSeconds,float)

	GET_SETTING("playPositionShift",gPlayPositionShift,float)

	GET_SETTING("addCueWhilePlaying_CueName",gAddCueWhilePlaying_CueName,string)
	GET_SETTING("addCueWhilePlaying_Anchored",gAddCueWhilePlaying_Anchored,bool)

	GET_SETTING("addCueAtClick_CueName",gAddCueAtClick_CueName,string)
	GET_SETTING("addCueAtClick_Anchored",gAddCueAtClick_Anchored,bool)

	GET_SETTING("Meters" DOT "meterUpdateTime",gMeterUpdateTime,unsigned)

	GET_SETTING("Meters" DOT "Level" DOT "enabled",gLevelMetersEnabled,bool)
	GET_SETTING("Meters" DOT "Level" DOT "RMSWindowTime",gMeterRMSWindowTime,unsigned)
	GET_SETTING("Meters" DOT "Level" DOT "maxPeakFallDelayTime",gMaxPeakFallDelayTime,unsigned)
	GET_SETTING("Meters" DOT "Level" DOT "maxPeakFallRate",gMaxPeakFallRate,double)

	GET_SETTING("Meters" DOT "StereoPhase" DOT "enabled",gStereoPhaseMetersEnabled,bool)
	GET_SETTING("Meters" DOT "StereoPhase" DOT "pointCount",gStereoPhaseMeterPointCount,unsigned)
	GET_SETTING("Meters" DOT "StereoPhase" DOT "unrotate",gStereoPhaseMeterUnrotate,bool)

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
}

void writeBackendSettings()
{
	gSettingsRegistry->setValue<string>("shareDirectory",gSysDataDirectory);
	gSettingsRegistry->setValue<string>("promptDialogDirectory",gPromptDialogDirectory);


	gSettingsRegistry->setValue<unsigned>("DesiredOutputSampleRate",gDesiredOutputSampleRate);
	gSettingsRegistry->setValue<unsigned>("DesiredOutputChannelCount",gDesiredOutputChannelCount);
	gSettingsRegistry->setValue<int>("DesiredOutputBufferCount",gDesiredOutputBufferCount);
	gSettingsRegistry->setValue<unsigned>("DesiredOutputBufferSize",gDesiredOutputBufferSize);


#ifdef ENABLE_OSS
	gSettingsRegistry->setValue<string>("OSSOutputDevice",gOSSOutputDevice);
	gSettingsRegistry->setValue<string>("OSSInputDevice",gOSSInputDevice);
#endif

#ifdef ENABLE_PORTAUDIO
	gSettingsRegistry->setValue<int>("PortAudioOutputDevice",gPortAudioOutputDevice);
	gSettingsRegistry->setValue<int>("PortAudioInputDevice",gPortAudioInputDevice);
#endif

#ifdef ENABLE_JACK
	// ??? could do these with vector<string> values I suppose
	for(unsigned t=0;t<MAX_CHANNELS;t++)
	{
		if(gJACKOutputPortNames[t]!="")
			gSettingsRegistry->setValue<string>("JACKOutputPortName"+istring(t+1),gJACKOutputPortNames[t]);
		else
		{
			gSettingsRegistry->removeKey("JACKOutputPortName"+istring(t+2));
			break;
		}
	}
	for(unsigned t=0;t<MAX_CHANNELS;t++)
	{
		if(gJACKInputPortNames[t]!="")
			gSettingsRegistry->setValue<string>("JACKInputPortName"+istring(t+1),gJACKInputPortNames[t]);
		else
		{
			gSettingsRegistry->removeKey("JACKInputPortName"+istring(t+2));
			break;
		}
	}
#endif

	gSettingsRegistry->setValue<string>("LADSPA_PATH",gLADSPAPath);

	gSettingsRegistry->setValue<string>("primaryWorkDir",gPrimaryWorkDir);
	gSettingsRegistry->setValue<string>("fallbackWorkDir",gFallbackWorkDir);

	gSettingsRegistry->setValue<string>("clipboardDir",gClipboardDir);
	gSettingsRegistry->setValue<string>("clipboardFilenamePrefix",gClipboardFilenamePrefix);
	gSettingsRegistry->setValue<size_t>("whichClipboard",gWhichClipboard);

	gSettingsRegistry->setValue<size_t>("ReopenHistory" DOT "maxReopenHistory",gMaxReopenHistory);

	gSettingsRegistry->setValue<float>("skipMiddleMarginSeconds",gSkipMiddleMarginSeconds);
	gSettingsRegistry->setValue<float>("loopGapLengthSeconds",gLoopGapLengthSeconds);

	gSettingsRegistry->setValue<float>("playPositionShift",gPlayPositionShift);

	gSettingsRegistry->setValue<string>("addCueWhilePlaying_CueName",gAddCueWhilePlaying_CueName);
	gSettingsRegistry->setValue<bool>("addCueWhilePlaying_Anchored",gAddCueWhilePlaying_Anchored);

	gSettingsRegistry->setValue<string>("addCueAtClick_CueName",gAddCueAtClick_CueName);
	gSettingsRegistry->setValue<bool>("addCueAtClick_Anchored",gAddCueAtClick_Anchored);

	gSettingsRegistry->setValue<unsigned>("Meters" DOT "meterUpdateTime",gMeterUpdateTime);

	gSettingsRegistry->setValue<bool>("Meters" DOT "Level" DOT "enabled",gLevelMetersEnabled);
	gSettingsRegistry->setValue<unsigned>("Meters" DOT "Level" DOT "RMSWindowTime",gMeterRMSWindowTime);
	gSettingsRegistry->setValue<unsigned>("Meters" DOT "Level" DOT "maxPeakFallDelayTime",gMaxPeakFallDelayTime);
	gSettingsRegistry->setValue<double>("Meters" DOT "Level" DOT "maxPeakFallRate",gMaxPeakFallRate);

	gSettingsRegistry->setValue<bool>("Meters" DOT "StereoPhase" DOT "enabled",gStereoPhaseMetersEnabled);
	gSettingsRegistry->setValue<unsigned>("Meters" DOT "StereoPhase" DOT "pointCount",gStereoPhaseMeterPointCount);
	gSettingsRegistry->setValue<bool>("Meters" DOT "StereoPhase" DOT "unrotate",gStereoPhaseMeterUnrotate);

	gSettingsRegistry->setValue<bool>("Meters" DOT "Analyzer" DOT "enabled",gFrequencyAnalyzerEnabled);
	gSettingsRegistry->setValue<unsigned>("Meters" DOT "Analyzer" DOT "peakFallDelayTime",gAnalyzerPeakFallDelayTime);
	gSettingsRegistry->setValue<double>("Meters" DOT "Analyzer" DOT "peakFallRate",gAnalyzerPeakFallRate);

	gSettingsRegistry->setValue<int>("crossfadeEdges",(int)gCrossfadeEdges);
	gSettingsRegistry->setValue<float>("crossfadeStartTime",gCrossfadeStartTime);
	gSettingsRegistry->setValue<float>("crossfadeStopTime",gCrossfadeStopTime);
	gSettingsRegistry->setValue<int>("crossfadeFadeMethod",(int)gCrossfadeFadeMethod);
}

