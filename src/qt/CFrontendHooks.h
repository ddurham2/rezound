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

#ifndef __CFrontendHooks_H__
#define __CFrontendHooks_H__

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

class CMainWindow;
class CNewSoundDialog;
class CRecordDialog;
class CRecordMacroDialog;
class CMacroActionParamsDialog;
class CJACKPortChoiceDialog;
class CRezSaveParametersDialog;
class CRawParametersDialog;
class COggSaveParametersDialog;
class CMp3SaveParametersDialog;
class CVoxOpenParametersDialog;
class CMIDISampleDumpParametersDialog;
class ClibaudiofileSaveParametersDialog;

class CFrontendHooks : public AFrontendHooks
{
public:
	CFrontendHooks(CMainWindow *mainWindow);
	virtual ~CFrontendHooks();

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
	CMainWindow *mainWindow;

	QFileDialog *openDialog;
		//FXCheckButton *openAsRawCheckButton;
	QFileDialog *saveDialog;
		//FXCheckButton *saveAsRawCheckButton;
	QFileDialog *dirDialog;

	CNewSoundDialog *newSoundDialog;
	CRecordDialog *recordDialog;
	CMacroActionParamsDialog *macroActionParamsDialog;
	CRezSaveParametersDialog *rezSaveParametersDialog;
	//CJACKPortChoiceDialog *JACKPortChoiceDialog;
	CRawParametersDialog *rawParametersDialog;
	COggSaveParametersDialog *oggSaveParametersDialog;
	CMp3SaveParametersDialog *mp3Dialog;
	CVoxOpenParametersDialog *voxDialog;
	CMIDISampleDumpParametersDialog *MIDISampleDumpParametersDialog;
	ClibaudiofileSaveParametersDialog *libaudiofileSaveParametersDialog;
};

#endif
