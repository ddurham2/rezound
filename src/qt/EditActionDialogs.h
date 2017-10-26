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

#ifndef EditActionDialogs_H__
#define EditActionDialogs_H__

#include "../../config/common.h"
#include "qt_compat.h"

#include "ActionParam/ActionParamDialog.h"


// --- insert silence --------------------

class InsertSilenceDialog : public ActionParamDialog
{
public:
    InsertSilenceDialog(QWidget *mainWindow);
    virtual ~InsertSilenceDialog(){}
};



// --- rotate ----------------------------

class RotateDialog : public ActionParamDialog
{
public:
    RotateDialog(QWidget *mainWindow);
    virtual ~RotateDialog(){}
};



// --- swap channels ---------------------

class SwapChannelsDialog : public ActionParamDialog
{
public:
    SwapChannelsDialog(QWidget *mainWindow);
    virtual ~SwapChannelsDialog(){}

	bool show(CActionSound *actionSound,CActionParameters *actionParameters);
};



// --- add channels ----------------------

class AddChannelsDialog : public ActionParamDialog
{
public:
    AddChannelsDialog(QWidget *mainWindow);
    virtual ~AddChannelsDialog(){}
};



// --- duplicate channel -----------------

class DuplicateChannelDialog : public ActionParamDialog
{
public:
    DuplicateChannelDialog(QWidget *mainWindow);
    virtual ~DuplicateChannelDialog(){}

	bool show(CActionSound *actionSound,CActionParameters *actionParameters);
};



// --- grow or slide selection dialog -----

class GrowOrSlideSelectionDialog : public ActionParamDialog
{
public:
    GrowOrSlideSelectionDialog(QWidget *mainWindow);
    virtual ~GrowOrSlideSelectionDialog(){}
};






#endif
