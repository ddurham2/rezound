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

#ifndef FileActionDialogs_H__
#define FileActionDialogs_H__

#include "../../config/common.h"

#include "qt_compat.h"

#include "ActionParam/ActionParamDialog.h"

// --- new audio file --------------------

class NewAudioFileActionDialog : public ActionParamDialog
{
public:
    NewAudioFileActionDialog(QWidget *mainWindow);
    virtual ~NewAudioFileActionDialog(){}

	bool show(CActionSound *actionSound,CActionParameters *actionParameters);
};


// --- open audio file --------------------

class OpenAudioFileActionDialog : public ActionParamDialog
{
public:
    OpenAudioFileActionDialog(QWidget *mainWindow);
    virtual ~OpenAudioFileActionDialog(){}

	bool show(CActionSound *actionSound,CActionParameters *actionParameters);
};

// --- save as -----------------

class SaveAsAudioFileActionDialog : public ActionParamDialog
{
public:
    SaveAsAudioFileActionDialog(QWidget *mainWindow);
    virtual ~SaveAsAudioFileActionDialog(){}

	bool show(CActionSound *actionSound,CActionParameters *actionParameters);
};

// --- save as multiple filenames --------

class SaveAsMultipleFilesDialog : public ActionParamDialog
{
public:
    SaveAsMultipleFilesDialog(QWidget *mainWindow);
    virtual ~SaveAsMultipleFilesDialog(){}

protected:
	const string getExplanation() const;
};

// --- burn to CD ------------------------

class BurnToCDDialog : public ActionParamDialog
{
	Q_OBJECT
public:
    BurnToCDDialog(QWidget *mainWindow);
    virtual ~BurnToCDDialog(){}

protected:
	const string getExplanation() const;

private Q_SLOTS:
	void onDetectDeviceClicked();
};

// --- run macro dialog -------------------

class RunMacroDialog : public ActionParamDialog
{
	Q_OBJECT
public:
    RunMacroDialog(QWidget *mainWindow);
    virtual ~RunMacroDialog(){}

	bool show(CActionSound *actionSound,CActionParameters *actionParameters);

private Q_SLOTS:
	void onRemoveClicked();
};


#endif
