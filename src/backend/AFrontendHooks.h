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

#ifndef __AFrontendHooks_H__
#define __AFrontendHooks_H__

#include "../../config/common.h"

class AFrontendHooks;

#include <string>
#include <vector>

#include "CSound_defs.h"

#ifdef ENABLE_LADSPA
	#include "AActionDialog.h"
	#include "LADSPA/ladspa.h"
#endif

class AActionFactory;
class ASoundRecorder;
class CLoadedSound;

extern AFrontendHooks *gFrontendHooks;

class AFrontendHooks
{
public:

	AFrontendHooks() { }
	virtual ~AFrontendHooks() { }

	virtual void setWhichClipboard(size_t whichClipboard)=0;

	// prompt with an open file dialog (return false if the prompt was cancelled)
	virtual bool promptForOpenSoundFilename(string &filename,bool &readOnly,bool &openAsRaw)=0;
	virtual bool promptForOpenSoundFilenames(vector<string> &filenames,bool &readOnly,bool &openAsRaw)=0;

	// prompt with a save file dialog (return false if the prompt was cancelled)
	virtual bool promptForSaveSoundFilename(string &filename,bool &saveAsRaw)=0;

	// prompt for a new sound to be created asking for the given parameters (return false if the prompt was cancelled)
	virtual bool promptForNewSoundParameters(string &filename,bool &rawFormat,bool hideFilename,unsigned &channelCount,bool hideChannelCount,unsigned &sampleRate,bool hideSampleRate,sample_pos_t &length,bool hideLength)=0;

	// should prompt for the user to choose a directory
	virtual bool promptForDirectory(string &dirname,const string title)=0;

	// prompt for recording, this function will have to be more than just an interface and do work 
	// since it should probably show level meters and be able to insert cues while recording etc.
	virtual bool promptForRecord(ASoundRecorder *recorder)=0;

	// prompt for parameters for recording a macro
	virtual bool showRecordMacroDialog(string &macroName)=0;

	// prompt for the information needed to store with the macro after each action of the recording macro is recorded
	struct MacroActionParameters
	{
		// true if at playback time the user should be prompted for the action parameters rather than it using the ones defined at record time
	 	bool askToPromptForActionParametersAtPlayback;

		enum SelectionPositioning
		{
			spLeaveAlone=1,

			spAbsoluteTimeFromBeginning=2,
			spAbsoluteTimeFromEnd=3,
			spProportionateTimeFromBeginning=4,

			spAbsoluteTimeFromStopPosition=5, 	// only allowed for the startPosPositioning
			spProportionateTimeFromStopPosition=6,	// only allowed for the startPosPositioning

			spAbsoluteTimeFromStartPosition=7, 	// only allowed for the stopPosPositioning
			spProportionateTimeFromStartPosition=8,	// only allowed for the stopPosPositioning

			// NOTE: 5 or 6 for a startPosPositioning cannot be used at the same time that 7 or 8 is used for the stopPosPositioning
			// the positionsAreRelativeToEachOther() method can conveniently be called to test for this condition

			spSameCueName=9,
		};

		SelectionPositioning startPosPositioning;
		SelectionPositioning stopPosPositioning;

		bool positionsAreRelativeToEachOther() const
		{
			return (startPosPositioning==5 || startPosPositioning==6) && (stopPosPositioning==7 || stopPosPositioning==8);
		}

		string startPosCueName;
		string stopPosCueName;
	};
	virtual bool showMacroActionParamsDialog(const AActionFactory *actionFactory,MacroActionParameters &macroActionParams,CLoadedSound *loadedSound)=0; // loadedSound may be NULL


#ifdef ENABLE_JACK
	// prompt for JACK port names (only used whenever JACK is the audio I/O subsystem
	// 	??? this could be a generalized promptWithCombo and given it a message, but I've never needed it until now
	virtual const string promptForJACKPort(const string message,const vector<string> portNames)=0;
#endif

	struct RezSaveParameters
	{
		AudioEncodingTypes audioEncodingType; // enum defined in CSound_defs.h
	};
	virtual bool promptForRezSaveParameters(RezSaveParameters &parameters)=0;


	// called when the user is loading a raw file and format parameters are needed
	struct RawParameters
	{
		unsigned channelCount;
		unsigned sampleRate;
		
		enum SampleFormats
		{
			f8BitSignedPCM=0,
			f8BitUnsignedPCM=4,
			f16BitSignedPCM=1,
			f16BitUnsignedPCM=5,
			f24BitSignedPCM=2,
			f24BitUnsignedPCM=6,
			f32BitSignedPCM=3,
			f32BitUnsignedPCM=7,
			f32BitFloatPCM=8,
			f64BitFloatPCM=9
		} sampleFormat;

		enum
		{
			eBigEndian=0,
			eLittleEndian=1
		} endian;

		unsigned dataOffset; // in bytes
		unsigned dataLength; // in frames; can be 0 for no user limit

	};
	virtual bool promptForRawParameters(RawParameters &parameters,bool showLoadRawParameters)=0;


	// called when the user is saving an ogg file and compression parameters are needed
	struct OggCompressionParameters
	{
		enum 
		{
			brVBR=0, 
			brQuality=1
		} method;

		// method==brVBR
		int minBitRate;
		int normBitRate;
		int maxBitRate;

		// method==brQuality
		float quality;
	};
	virtual bool promptForOggCompressionParameters(OggCompressionParameters &parameters)=0;


	// called when the user is saving an mp3 file and compression parameters are needed
	struct Mp3CompressionParameters
	{
		enum 
		{
			brCBR=0,
			brABR=1, 
			brQuality=2
		} method;

		// method==brCBR
		int constantBitRate;

		// method==brABR
		int minBitRate;
		int normBitRate;
		int maxBitRate;

		// method==brQuality
		int quality; // 0(highest quality) -> 9(lowest quality)

		bool useFlagsOnly;
		string additionalFlags;
	};
	virtual bool promptForMp3CompressionParameters(Mp3CompressionParameters &parameters)=0;


	// called when the user is loading a vox file and format parameters are needed
	struct VoxParameters
	{
		unsigned channelCount;
		unsigned sampleRate;

	};
	virtual bool promptForVoxParameters(VoxParameters &parameters)=0;


	// stuff to handle frontend interfaces to LADSPA plugins
#ifdef ENABLE_LADSPA
	virtual AActionDialog *getChannelSelectDialog()=0;
	// should return a frontend dialog which can handle the given LADSPA plugin descriptor
		// the backend will be responsible for deleteing the returned object
	virtual AActionDialog *getLADSPAActionDialog(const LADSPA_Descriptor *desc)=0;
#endif

	// -1 can be passed for the parameters not be be changed from the previous values on the dialog
	// if Id comes back -1, then just wait for one
	// if false is returned, then cancel was pressed
	virtual bool promptForOpenMIDISampleDump(int &sysExChannel,int &waveformId)=0;

	// -1 can be passed for the parameters not be be changed from the previous values on the dialog
	// if false is returned, then cancel was pressed
	virtual bool promptForSaveMIDISampleDump(int &sysExChannel,int &waveformId,int &loopType)=0;


	// called when the user is saving an format with libaudiofile
	struct libaudiofileSaveParameters
	{
		int sampleFormat;	// AF_SAMPFMT_xxx
		int defaultSampleFormatIndex; // of signed, unsigned, float, double
		int sampleWidth;	// bit rate
		int defaultSampleWidthIndex; // of 8, 16, 24, 32

		// this is an input value that indicates which compression types are supported
		// pair: name of compression type, audiofiles enum value for it
		vector<pair<string,int> > supportedCompressionTypes; 

		int defaultCompressionTypeIndex; // of the items in the vector
		int compressionType;	// AF_COMPRESSION_xxx

		bool saveCues;
		bool saveUserNotes;
	};
	virtual bool promptForlibaudiofileSaveParameters(libaudiofileSaveParameters &parameters,const string formatName)=0;
};

#endif
