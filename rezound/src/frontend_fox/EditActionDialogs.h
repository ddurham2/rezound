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

#ifndef __EditActionDialogs_H__
#define __EditActionDialogs_H__

#include "../../config/common.h"
#include "fox_compat.h"

#include "CActionParamDialog.h"


// --- insert silence --------------------

class CInsertSilenceDialog : public CActionParamDialog
{
public:
	CInsertSilenceDialog(FXWindow *mainWindow);
	virtual ~CInsertSilenceDialog(){}
};



// --- rotate ----------------------------

class CRotateDialog : public CActionParamDialog
{
public:
	CRotateDialog(FXWindow *mainWindow);
	virtual ~CRotateDialog(){}
};



// --- swap channels ---------------------

class CSwapChannelsDialog : public CActionParamDialog
{
public:
	CSwapChannelsDialog(FXWindow *mainWindow);
	virtual ~CSwapChannelsDialog(){}

	bool show(CActionSound *actionSound,CActionParameters *actionParameters);
};



// --- add channels ----------------------

class CAddChannelsDialog : public CActionParamDialog
{
public:
	CAddChannelsDialog(FXWindow *mainWindow);
	virtual ~CAddChannelsDialog(){}
};



// --- duplicate channel -----------------

class CDuplicateChannelDialog : public CActionParamDialog
{
public:
	CDuplicateChannelDialog(FXWindow *mainWindow);
	virtual ~CDuplicateChannelDialog(){}

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


// --- grow or slide selection dialog -----

class CGrowOrSlideSelectionDialog : public CActionParamDialog
{
public:
	CGrowOrSlideSelectionDialog(FXWindow *mainWindow);
	virtual ~CGrowOrSlideSelectionDialog(){}
};






#endif
