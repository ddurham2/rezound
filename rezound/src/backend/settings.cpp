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


string gClipboardDir="/tmp"; // ??? would be something else on non-unix platforms
string gClipboardFilenamePrefix="rezclip";
size_t gWhichClipboard=0;


size_t gMaxReopenHistory=16;


float gSkipMiddleMarginSeconds=2.0;
float gLoopGapLengthSeconds=0.75;


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
	
	GET_SETTING("promptDialogDirectory",gPromptDialogDirectory,string)


	GET_SETTING("DesiredOutputSampleRate",gDesiredOutputSampleRate,unsigned)
	GET_SETTING("DesiredOutputChannelCount",gDesiredOutputChannelCount,unsigned)
	GET_SETTING("DesiredOutputBufferCount",gDesiredOutputBufferCount,int)
		gDesiredOutputBufferCount=max(2,gDesiredOutputBufferCount);
	GET_SETTING("DesiredOutputBufferSize",gDesiredOutputBufferSize,unsigned)
		if(gDesiredOutputBufferSize<256 || log((double)gDesiredOutputBufferSize)/log(2.0)!=floor(log((double)gDesiredOutputBufferSize)/log(2.0)))
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
	GET_SETTING("fallbackWorkDir",gFallbackWorkDir,string)

	GET_SETTING("clipboardDir",gClipboardDir,string)

	GET_SETTING("clipboardFilenamePrefix",gClipboardFilenamePrefix,string)

	GET_SETTING("whichClipboard",gWhichClipboard,size_t)

	GET_SETTING("ReopenHistory" DOT "maxReopenHistory",gMaxReopenHistory,size_t)

	GET_SETTING("skipMiddleMarginSeconds",gSkipMiddleMarginSeconds,float)

	GET_SETTING("loopGapLengthSeconds",gLoopGapLengthSeconds,float)


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

	gSettingsRegistry->createValue<string>("LADSPA_PATH",gLADSPAPath);

	gSettingsRegistry->createValue<string>("fallbackWorkDir",gFallbackWorkDir);

	gSettingsRegistry->createValue<string>("clipboardDir",gClipboardDir);
	gSettingsRegistry->createValue<string>("clipboardFilenamePrefix",gClipboardFilenamePrefix);
	gSettingsRegistry->createValue<size_t>("whichClipboard",gWhichClipboard);

	gSettingsRegistry->createValue<size_t>("ReopenHistory" DOT "maxReopenHistory",gMaxReopenHistory);

	gSettingsRegistry->createValue<float>("skipMiddleMarginSeconds",gSkipMiddleMarginSeconds);
	gSettingsRegistry->createValue<float>("loopGapLengthSeconds",gLoopGapLengthSeconds);

	gSettingsRegistry->createValue<unsigned>("Meters" DOT "meterUpdateTime",gMeterUpdateTime);

	gSettingsRegistry->createValue<bool>("Meters" DOT "Level" DOT "enabled",gLevelMetersEnabled);
	gSettingsRegistry->createValue<unsigned>("Meters" DOT "Level" DOT "RMSWindowTime",gMeterRMSWindowTime);
	gSettingsRegistry->createValue<unsigned>("Meters" DOT "Level" DOT "maxPeakFallDelayTime",gMaxPeakFallDelayTime);
	gSettingsRegistry->createValue<double>("Meters" DOT "Level" DOT "maxPeakFallRate",gMaxPeakFallRate);

	gSettingsRegistry->createValue<bool>("Meters" DOT "StereoPhase" DOT "enabled",gStereoPhaseMetersEnabled);
	gSettingsRegistry->createValue<unsigned>("Meters" DOT "StereoPhase" DOT "pointCount",gStereoPhaseMeterPointCount);
	gSettingsRegistry->createValue<bool>("Meters" DOT "StereoPhase" DOT "unrotate",gStereoPhaseMeterUnrotate);

	gSettingsRegistry->createValue<bool>("Meters" DOT "Analyzer" DOT "enabled",gFrequencyAnalyzerEnabled);
	gSettingsRegistry->createValue<unsigned>("Meters" DOT "Analyzer" DOT "peakFallDelayTime",gAnalyzerPeakFallDelayTime);
	gSettingsRegistry->createValue<double>("Meters" DOT "Analyzer" DOT "peakFallRate",gAnalyzerPeakFallRate);

	gSettingsRegistry->createValue<int>("crossfadeEdges",(int)gCrossfadeEdges);
	gSettingsRegistry->createValue<float>("crossfadeStartTime",gCrossfadeStartTime);
	gSettingsRegistry->createValue<float>("crossfadeStopTime",gCrossfadeStopTime);
	gSettingsRegistry->createValue<int>("crossfadeFadeMethod",(int)gCrossfadeFadeMethod);
}
