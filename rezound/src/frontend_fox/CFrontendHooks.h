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

#ifndef __CFrontendHooks_H__
#define __CFrontendHooks_H__

#include "../../config/common.h"
#include "fox_compat.h"

#include "../backend/AFrontendHooks.h"

#ifdef FOX_NO_NAMESPACE
	class FXWindow;
	class FXFileDialog;
	class FXDirDialog;
	class FXCheckButton;
#else
	namespace FX { class FXWindow; class FXFileDialog; class FXDirDialog; class FXCheckButton; }
	using namespace FX;
#endif

class CNewSoundDialog;
class CRecordDialog;
class CRawDialog;
class COggDialog;
class CMp3Dialog;
class CVoxDialog;

class CFrontendHooks : public AFrontendHooks
{
public:
	CFrontendHooks(FXWindow *mainWindow);
	virtual ~CFrontendHooks();

	void doSetupAfterBackendIsSetup();

	const string getFOXFileTypes() const; // returns a string to pass as the file types drop-down in FOX file dialogs

	bool promptForOpenSoundFilename(string &filename,bool &readOnly,bool &openAsRaw);
	bool promptForOpenSoundFilenames(vector<string> &filenames,bool &readOnly,bool &openAsRaw);
	bool promptForSaveSoundFilename(string &filename,bool &saveAsRaw);

	bool promptForNewSoundParameters(string &filename,bool &rawFormat,unsigned &channelCount,unsigned &sampleRate,sample_pos_t &length);
	bool promptForNewSoundParameters(string &filename,bool &rawFormat,unsigned &channelCount,unsigned &sampleRate);
	bool promptForNewSoundParameters(unsigned &channelCount,unsigned &sampleRate);

	bool promptForDirectory(string &dirname,const string title);

	bool promptForRecord(ASoundRecorder *recorder);

	bool promptForRawParameters(RawParameters &parameters,bool showOffsetAndLengthParameters);
	bool promptForOggCompressionParameters(OggCompressionParameters &parameters);
	bool promptForMp3CompressionParameters(Mp3CompressionParameters &parameters);
	bool promptForVoxParameters(VoxParameters &parameters);

private:
	FXWindow *mainWindow;

	FXFileDialog *openDialog;
		FXCheckButton *openAsRawCheckButton;
	FXFileDialog *saveDialog;
		FXCheckButton *saveAsRawCheckButton;
	FXDirDialog *dirDialog;

	CNewSoundDialog *newSoundDialog;
	CRecordDialog *recordDialog;
	CRawDialog *rawDialog;
	COggDialog *oggDialog;
	CMp3Dialog *mp3Dialog;
	CVoxDialog *voxDialog;
};

#endif
