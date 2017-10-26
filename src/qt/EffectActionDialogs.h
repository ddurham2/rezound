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

#ifndef __EffectActionDialogs_H__
#define __EffectActionDialogs_H__

#include "../../config/common.h"
#include "qt_compat.h"

#include "ActionParam/ActionParamDialog.h"


// --- volume ----------------------------

class NormalVolumeChangeDialog : public ActionParamDialog
{
public:
    NormalVolumeChangeDialog(QWidget *mainWindow);
    virtual ~NormalVolumeChangeDialog(){}
};

class NormalGainDialog : public ActionParamDialog
{
public:
    NormalGainDialog(QWidget *mainWindow);
    virtual ~NormalGainDialog(){}
};

class AdvancedGainDialog : public ActionParamDialog
{
public:
    AdvancedGainDialog(QWidget *mainWindow);
    virtual ~AdvancedGainDialog(){}
};



// --- rate ------------------------------

class NormalRateChangeDialog : public ActionParamDialog
{
public:
    NormalRateChangeDialog(QWidget *mainWindow);
    virtual ~NormalRateChangeDialog(){}
};

class AdvancedRateChangeDialog : public ActionParamDialog
{
public:
    AdvancedRateChangeDialog(QWidget *mainWindow);
    virtual ~AdvancedRateChangeDialog(){}
};





// --- flange ----------------------------

class FlangeDialog : public ActionParamDialog
{
public:
    FlangeDialog(QWidget *mainWindow);
    virtual ~FlangeDialog(){}
};





// --- delay -----------------------------

class SimpleDelayDialog : public ActionParamDialog
{
public:
    SimpleDelayDialog(QWidget *mainWindow);
    virtual ~SimpleDelayDialog(){}
};





// --- quantize --------------------------

class QuantizeDialog : public ActionParamDialog
{
public:
    QuantizeDialog(QWidget *mainWindow);
    virtual ~QuantizeDialog(){}
};





// --- distortion ------------------------

class DistortionDialog : public ActionParamDialog
{
public:
    DistortionDialog(QWidget *mainWindow);
    virtual ~DistortionDialog(){}
};





// --- varied repeat ---------------------

class VariedRepeatDialog : public ActionParamDialog
{
public:
    VariedRepeatDialog(QWidget *mainWindow);
    virtual ~VariedRepeatDialog(){}
};



#endif
