/* 
 * Copyright (C) 2005 - David W. Durham
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

#ifndef __FileActionDialogs_H__
#define __FileActionDialogs_H__

#include "../../config/common.h"
#include "fox_compat.h"

#include "CActionParamDialog.h"
 
// --- new audio file --------------------

class CNewAudioFileActionDialog : public CActionParamDialog
{
public:
	CNewAudioFileActionDialog(FXWindow *mainWindow);
	virtual ~CNewAudioFileActionDialog(){}

	bool show(CActionSound *actionSound,CActionParameters *actionParameters);
};

// --- open audio file --------------------

class COpenAudioFileActionDialog : public CActionParamDialog
{
public:
	COpenAudioFileActionDialog(FXWindow *mainWindow);
	virtual ~COpenAudioFileActionDialog(){}

	bool show(CActionSound *actionSound,CActionParameters *actionParameters);
};

// --- save as -----------------

class CSaveAsAudioFileActionDialog : public CActionParamDialog
{
public:
	CSaveAsAudioFileActionDialog(FXWindow *mainWindow);
	virtual ~CSaveAsAudioFileActionDialog(){}

	bool show(CActionSound *actionSound,CActionParameters *actionParameters);
};

// --- save as multiple filenames --------

class CSaveAsMultipleFilesDialog : public CActionParamDialog
{
public:
	CSaveAsMultipleFilesDialog(FXWindow *mainWindow);
	virtual ~CSaveAsMultipleFilesDialog(){}

protected:
	const string getExplanation() const;
};

// --- burn to CD ------------------------

class CBurnToCDDialog : public CActionParamDialog
{
	FXDECLARE(CBurnToCDDialog);
public:
	CBurnToCDDialog(FXWindow *mainWindow);
	virtual ~CBurnToCDDialog(){}

	enum {
		ID_DETECT_DEVICE_BUTTON=CActionParamDialog::ID_LAST,
	};

	long onDetectDeviceButton(FXObject *object,FXSelector sel,void *ptr);

protected:
	CBurnToCDDialog::CBurnToCDDialog() {}

	const string getExplanation() const;
};

// --- run macro dialog -------------------

class CRunMacroDialog : public CActionParamDialog
{
	FXDECLARE(CRunMacroDialog);
public:
public:
	CRunMacroDialog(FXWindow *mainWindow);
	virtual ~CRunMacroDialog(){}

	bool show(CActionSound *actionSound,CActionParameters *actionParameters);

	enum {
		ID_REMOVE_BUTTON=CActionParamDialog::ID_LAST,
	};

	long onRemoveButton(FXObject *object,FXSelector sel,void *ptr);

protected:
	CRunMacroDialog::CRunMacroDialog() {}

};


#endif
