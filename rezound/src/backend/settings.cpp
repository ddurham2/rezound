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

#include <string.h>
unsigned gDesiredOutputSampleRate=44100;
unsigned gDesiredOutputChannelCount=2;


#ifdef HAVE_LIBPORTAUDIO
int gPortAudioOutputDevice=0;
int gPortAudioInputDevice=0;
#else
string gOSSOutputDevice="/dev/dsp";
string gOSSInputDevice="/dev/dsp";
#endif


string gFallbackWorkDir="/tmp"; // ??? would be something else on non-unix platforms


string gClipboardDir="/tmp"; // ??? would be something else on non-unix platforms
string gClipboardFilenamePrefix="rezclip";
size_t gWhichClipboard=0;


bool gSnapToCues=true;
unsigned gSnapToCueDistance=5;


bool gFollowPlayPosition=true;


double gInitialLengthToShow=120.0; // default 120 seconds


unsigned gMeterUpdateTime=50;
unsigned gMeterRMSWindowTime=200;
unsigned gMaxPeakFallDelayTime=500;
double gMaxPeakFallRate=0.02;
unsigned gAnalyzerPeakFallDelayTime=400;
double gAnalyzerPeakFallRate=0.025;


CrossfadeEdgesTypes gCrossfadeEdges=cetInner;
float gCrossfadeStartTime=10.0;	 // default 20ms crossfade time
float gCrossfadeStopTime=10.0;
CrossfadeFadeMethods gCrossfadeFadeMethod=cfmLinear;


