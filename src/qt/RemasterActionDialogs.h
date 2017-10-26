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

#ifndef __RemasterActionDialogs_H__
#define __RemasterActionDialogs_H__

#include "../../config/common.h"
#include "qt_compat.h"

#include "ActionParam/ActionParamDialog.h"


// --- balance ------------------------

class SimpleBalanceActionDialog : public ActionParamDialog
{
public:
    SimpleBalanceActionDialog(QWidget *mainWindow);
    virtual ~SimpleBalanceActionDialog(){}

	bool show(CActionSound *actionSound,CActionParameters *actionParameters);
};

class CurvedBalanceActionDialog : public ActionParamDialog
{
public:
    CurvedBalanceActionDialog(QWidget *mainWindow);
    virtual ~CurvedBalanceActionDialog(){}

	bool show(CActionSound *actionSound,CActionParameters *actionParameters);
};



// --- monoize ------------------------

class MonoizeActionDialog : public ActionParamDialog
{
public:
    MonoizeActionDialog(QWidget *mainWindow);
    virtual ~MonoizeActionDialog(){}

	bool show(CActionSound *actionSound,CActionParameters *actionParameters);
};



// --- noise gate ------------------------

class NoiseGateDialog : public ActionParamDialog
{
public:
    NoiseGateDialog(QWidget *mainWindow);
    virtual ~NoiseGateDialog(){}

	const string getExplanation() const;
};



// --- compressor ------------------------

class CompressorDialog : public ActionParamDialog
{
public:
    CompressorDialog(QWidget *mainWindow);
    virtual ~CompressorDialog(){}

protected:
	const string getExplanation() const;
};



// --- normalize -------------------------

class NormalizeDialog : public ActionParamDialog
{
public:
    NormalizeDialog(QWidget *mainWindow);
    virtual ~NormalizeDialog(){}
};



// --- adaptive normalize ----------------

class AdaptiveNormalizeDialog : public ActionParamDialog
{
public:
    AdaptiveNormalizeDialog(QWidget *mainWindow);
    virtual ~AdaptiveNormalizeDialog(){}

protected:
	const string getExplanation() const;
};



// --- mark quiet areas ------------------

class MarkQuietAreasDialog : public ActionParamDialog
{
public:
    MarkQuietAreasDialog(QWidget *mainWindow);
    virtual ~MarkQuietAreasDialog(){}
};



// --- shorten quiet areas ------------------

class ShortenQuietAreasDialog : public ActionParamDialog
{
public:
    ShortenQuietAreasDialog(QWidget *mainWindow);
    virtual ~ShortenQuietAreasDialog(){}
};



// --- resample --------------------------

class ResampleDialog : public ActionParamDialog
{
public:
    ResampleDialog(QWidget *mainWindow);
    virtual ~ResampleDialog(){}
};



// --- change pitch ----------------------

class ChangePitchDialog : public ActionParamDialog
{
public:
    ChangePitchDialog(QWidget *mainWindow);
    virtual ~ChangePitchDialog(){}
};



// --- change tempo ----------------------

class ChangeTempoDialog : public ActionParamDialog
{
public:
    ChangeTempoDialog(QWidget *mainWindow);
    virtual ~ChangeTempoDialog(){}
};



#endif
