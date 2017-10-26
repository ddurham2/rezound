/* 
 * Copyright (C) 2006 - David W. Durham
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

#ifndef FrontendHooks_H__
#define FrontendHooks_H__

#include "../../config/common.h"
#include "qt_compat.h"

#include <QStringList>
#include <QInputDialog>

#include "../backend/AFrontendHooks.h"

/*
#ifdef FOX_NO_NAMESPACE
	class FXWindow;
	class FXFileDialog;
	class FXDirDialog;
	class FXCheckButton;
#else
	namespace FX { class FXWindow; class FXFileDialog; class FXDirDialog; class FXCheckButton; }
	using namespace FX;
#endif
*/

class QFileDialog;

class MainWindow;
class NewSoundDialog;
class RecordDialog;
class RecordMacroDialog;
class MacroActionParamsDialog;
class JACKPortChoiceDialog;
class RezSaveParametersDialog;
class RawParametersDialog;
class OggSaveParametersDialog;
class Mp3SaveParametersDialog;
class VoxOpenParametersDialog;
class MIDISampleDumpParametersDialog;
class LibaudiofileSaveParametersDialog;

class FrontendHooks : public AFrontendHooks
{
public:
    FrontendHooks(MainWindow *mainWindow);
    virtual ~FrontendHooks();

	void doSetupAfterBackendIsSetup();

	void setWhichClipboard(size_t whichClipboard);

	const QStringList getQtFileTypes() const; // returns a string to pass as the file types drop-down in FOX file dialogs

	bool promptForOpenSoundFilename(string &filename,bool &readOnly,bool &openAsRaw);
	bool promptForOpenSoundFilenames(vector<string> &filenames,bool &readOnly,bool &openAsRaw);
	bool promptForSaveSoundFilename(string &filename,bool &saveAsRaw);

	bool promptForNewSoundParameters(string &filename,bool &rawFormat,bool hideFilename,unsigned &channelCount,bool hideChannelCount,unsigned &sampleRate,bool hideSampleRate,sample_pos_t &length,bool hideLength);

	bool promptForDirectory(string &dirname,const string title);

	bool promptForRecord(ASoundRecorder *recorder);

	bool showRecordMacroDialog(string &macroName);

    bool showMacroActionParamsDialog(const AActionFactory *actionFactory,MacroActionParameters &macroActionParameters,CLoadedSound *loadedSound);

	const string promptForJACKPort(const string message,const vector<string> portNames);

	bool promptForRezSaveParameters(RezSaveParameters &parameters);
	bool promptForRawParameters(RawParameters &parameters,bool showOffsetAndLengthParameters);
	bool promptForOggCompressionParameters(OggCompressionParameters &parameters);
	bool promptForMp3CompressionParameters(Mp3CompressionParameters &parameters);
	bool promptForVoxParameters(VoxParameters &parameters);

#if defined(ENABLE_LADSPA)
	AActionDialog *getChannelSelectDialog();
	AActionDialog *getLADSPAActionDialog(const LADSPA_Descriptor *desc);
#endif

	bool promptForOpenMIDISampleDump(int &sysExChannel,int &waveformId);
	bool promptForSaveMIDISampleDump(int &sysExChannel,int &waveformId,int &loopType);

	bool promptForlibaudiofileSaveParameters(libaudiofileSaveParameters &parameters,const string formatName);

private:
	MainWindow *mainWindow;

	QFileDialog *openDialog;
		//FXCheckButton *openAsRawCheckButton;
	QFileDialog *saveDialog;
		//FXCheckButton *saveAsRawCheckButton;
	QFileDialog *dirDialog;

    NewSoundDialog *newSoundDialog;
    RecordDialog *recordDialog;
    MacroActionParamsDialog *macroActionParamsDialog;
    RezSaveParametersDialog *rezSaveParametersDialog;
	//CJACKPortChoiceDialog *JACKPortChoiceDialog;
    RawParametersDialog *rawParametersDialog;
    OggSaveParametersDialog *oggSaveParametersDialog;
    Mp3SaveParametersDialog *mp3Dialog;
    VoxOpenParametersDialog *voxDialog;
    MIDISampleDumpParametersDialog *midiSampleDumpParametersDialog;
    LibaudiofileSaveParametersDialog *libaudiofileSaveParametersDialog;
};

#endif
