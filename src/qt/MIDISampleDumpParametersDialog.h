/* 
 * Copyright (C) 2007 - David W. Durham
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

#ifndef MIDISampleDumpParametersDialog_H__
#define MIDISampleDumpParametersDialog_H__

#include "../../config/common.h"

#include "qt_compat.h"

class MIDISampleDumpParametersDialog;

#include "ActionParam/ActionParamDialog.h"

#include "../backend/AFrontendHooks.h"

class MIDISampleDumpParametersDialog : public ActionParamDialog
{
	Q_OBJECT
public:
	MIDISampleDumpParametersDialog(QWidget *mainWindow);
	virtual ~MIDISampleDumpParametersDialog(){}

	bool showForOpen(int &sysExChannel,int &waveformId);
	bool showForSave(int &sysExChannel,int &waveformId,int &loopType);

private Q_SLOTS:
	void onWaitForDumpCheckBox_changed();

private:
	CheckBoxParamValue *m_waitForDump;
    TextParamValue *m_sampleID;
    TextParamValue *m_sysexChannel;
    TextParamValue *m_loopType;

};

#endif
