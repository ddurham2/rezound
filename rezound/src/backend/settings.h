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

#ifndef __backend_setting_H__
#define __backend_setting_H__

#include "../../config/common.h"


/*
 * This is a repository to which settings can be saved
 */
class CNestedDataFile;
extern CNestedDataFile *gSettingsRegistry;


/*
 * This is the directory that a open/save dialog should open to
 */
#include <string>
extern string gPromptDialogDirectory;


/*
 * These are the paths to the /usr/share/rezound or ~/.rezound directories
 */
extern string gUserDataDirectory;		// "~/.rezound"
extern string gSysDataDirectory;		// "/usr/share/rezound" (or from whereever the --prefix was set)

// if the value of gSysDataDirectory+"/" is a prefix of filename, then replace with "$share/"
const string encodeFilenamePresetParameter(const string filename);
// if the "$share/" is a prefix of filename, then replace with the value of gSysDataDirectory
const string decodeFilenamePresetParameter(const string filename);


extern string gUserPresetsFilename;		// gUserDataDirectory+"/presets.dat"
extern CNestedDataFile *gUserPresetsFile;	// opened to gUserPresetsFilename

extern string gSysPresetsFilename;		// gSysDataDirectory+"/presets.dat"
extern CNestedDataFile *gSysPresetsFile;	// opened to gSysPresetsFilename


// the desired output device parameters to try to open with
extern unsigned gDesiredOutputSampleRate;	// defaulted to 44100
extern unsigned gDesiredOutputChannelCount;	// defaulted to 2
extern int gDesiredOutputBufferCount;		// defaulted to 2
extern unsigned gDesiredOutputBufferSize;	// defaulted to 2048 (in frames)


#ifdef ENABLE_OSS
// the OSS devices to use (when not using libportaudio)
extern string gOSSOutputDevice;			// defaulted to "/dev/dsp"
extern string gOSSInputDevice;			// defaulted to "/dev/dsp"
#endif
#ifdef ENABLE_PORTAUDIO
// the PortAudio devices to use
extern int gPortAudioOutputDevice;		// defaulted to 0
extern int gPortAudioInputDevice;		// defaulted to 0
#endif
#ifdef ENABLE_JACK
// the JACK port names to use
extern string gJACKOutputPortNames[64];
extern string gJACKInputPortNames[64];
#endif

#if !defined(ENABLE_OSS) && !defined(ENABLE_PORTAUDIO) && !defined(ENABLE_JACK)
	#error enable one of the audio I/O systems with the --enable-XXX flags to the configure script
#endif 


// This specifies the directory to fall back to if we cannot write
// to the directory where we're loading a file from, or if it's 
// a nearly full file system
extern string gFallbackWorkDir;			// defaulted to /tmp


// This specifies where to open the clipboard poolfiles
extern string gClipboardDir;			// defaulted to /tmp

// This specifies the prefix on the filename used for the clipboard pool files
extern string gClipboardFilenamePrefix;

// specifies an index into AAction::clipboards which specifies which clipboard 
// should be used for the subsequent edit operations which use a clipboard
extern size_t gWhichClipboard;


// this is the max number of reopen items to keep in the reopen history
extern size_t gMaxReopenHistory; 		// defaulted to 16


/*
 * These are the parameters of whether or not to snap the selection positions
 * to the cue position, and if so, how far from a cue to does the position have
 * to be before it snaps to it.
 */
extern bool gSnapToCues;
extern unsigned gSnapToCueDistance;


/*
 * True if the sound windows should follow the play position
 */
extern bool gFollowPlayPosition;


/*
 * True if the wave view rendering should color max-valued samples brightly
 */
extern bool gRenderClippingWarning;


/*
 * dealing when loopType is ltLoopSkipMost
 * Specifies how much time should be played before skipping past the middle to (this much also before) the loop point
 */
extern float gSkipMiddleMarginSeconds;
/*
 * dealing when loopType is ltLoopSkipMost or ltLoopGapBeforeRepeat
 * Specifies how long the skip is when skipping the middle when looping or how long the gap is when inserting a gap before repeating
 */
extern float gLoopGapLengthSeconds;


/*
 * The initial about of audio to show on screen in a newly created sound window
 */
extern double gInitialLengthToShow;


/*
 * settings pertaining to audio level meters and frequency analyzer
 */
extern unsigned gMeterUpdateTime;	// the number of milliseconds before update the audio level meters again

extern bool gLevelMetersEnabled;	// flag whether the level meters are enabled
extern unsigned gMeterRMSWindowTime;	// the number of milliseconds of audio that the RMS should do a moving average on (normally 10-1000 milliseconds)
extern unsigned gMaxPeakFallDelayTime;	// the number of milliseconds before the max peak indicators should hold before falling
extern double gMaxPeakFallRate;		// the fraction of the maximum sample value that should be subtracted from the max peak level durring falling (normally 2% or 0.02)

extern bool gFrequencyAnalyzerEnabled;	// flag whether the frequency analyzer is enabled
extern unsigned gAnalyzerPeakFallDelayTime;	// the number of milliseconds before the analyzer peaks should hold before falling
extern double gAnalyzerPeakFallRate;	// the the fraction of 1 that should be subtracted from tne analyzer peaks durring falling 


/*
 * Setting for how to cross fade the edges
 */
enum CrossfadeEdgesTypes
{
	cetNone=0,	// do no crossfading of the edges


	// - blend with the data existing near the inner sides of the edges	...|<- ... ->|...
	// - this causes no change to the data outside of the selection except
	//   when the new selection after the action is made shorter than the 
	//   crossfade start time (i.e. a delete). 
	//		...|...|... --paste_action--> ...|X...X|... <- crossfade after action
	//		...|...|... --delete_action--> ...|X... <- crossfade after action
	cetInner=1,


	// - blend with the data existing just outside the selection		... ->| ... |<- ...
	// - this lessens the amount of space outside the selection because it was
	//   blended with the data inside the selection after the action has occured
	//		...|...|... --paste_action--> ...X|...|X... <- crossfade after action
	cetOuter=2
};
extern CrossfadeEdgesTypes gCrossfadeEdges;
extern float gCrossfadeStartTime;
extern float gCrossfadeStopTime;
enum CrossfadeFadeMethods { cfmLinear=0, cfmParabolic=1 };
extern CrossfadeFadeMethods gCrossfadeFadeMethod;

#endif
