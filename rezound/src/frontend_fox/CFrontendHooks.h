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
#else
	namespace FX { class FXWindow; class FXFileDialog; class FXDirDialog; }
	using namespace FX;
#endif

class CNewSoundDialog;
class CRecordDialog;
class COggDialog;

class CFrontendHooks : public AFrontendHooks
{
public:
	CFrontendHooks(FXWindow *mainWindow);
	virtual ~CFrontendHooks();

	void doSetupAfterBackendIsSetup();

	const string getFOXFileTypes() const; // returns a string to pass as the file types drop-down in FOX file dialogs

	bool promptForOpenSoundFilename(string &filename,bool &readOnly);
	bool promptForSaveSoundFilename(string &filename);

	bool promptForNewSoundParameters(string &filename,unsigned &channelCount,unsigned &sampleRate,sample_pos_t &length);
	bool promptForNewSoundParameters(string &filename,unsigned &channelCount,unsigned &sampleRate);
	bool promptForNewSoundParameters(unsigned &channelCount,unsigned &sampleRate);

	bool promptForDirectory(string &dirname,const string title);

	bool promptForRecord(ASoundRecorder *recorder);

	bool promptForOggCompressionParameters(OggCompressionParameters &parameters);

private:
	FXWindow *mainWindow;

	FXFileDialog *openDialog;
	FXFileDialog *saveDialog;
	FXDirDialog *dirDialog;

	CNewSoundDialog *newSoundDialog;
	CRecordDialog *recordDialog;
	COggDialog *oggDialog;
};

#endif
