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

#ifndef __ActionDialogs_H__
#define __ActionDialogs_H__

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




// --- save as multiple filenames --------

class CSaveAsMultipleFilesDialog : public CActionParamDialog
{
public:
	CSaveAsMultipleFilesDialog(FXWindow *mainWindow);
	virtual ~CSaveAsMultipleFilesDialog(){}

protected:
	const string getExplaination() const;
};






#endif
