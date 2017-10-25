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

#include "ActionParam/CActionParamDialog.h"


// --- volume ----------------------------

class CNormalVolumeChangeDialog : public CActionParamDialog
{
public:
	CNormalVolumeChangeDialog(QWidget *mainWindow);
	virtual ~CNormalVolumeChangeDialog(){}
};

class CNormalGainDialog : public CActionParamDialog
{
public:
	CNormalGainDialog(QWidget *mainWindow);
	virtual ~CNormalGainDialog(){}
};

class CAdvancedGainDialog : public CActionParamDialog
{
public:
	CAdvancedGainDialog(QWidget *mainWindow);
	virtual ~CAdvancedGainDialog(){}
};



// --- rate ------------------------------

class CNormalRateChangeDialog : public CActionParamDialog
{
public:
	CNormalRateChangeDialog(QWidget *mainWindow);
	virtual ~CNormalRateChangeDialog(){}
};

class CAdvancedRateChangeDialog : public CActionParamDialog
{
public:
	CAdvancedRateChangeDialog(QWidget *mainWindow);
	virtual ~CAdvancedRateChangeDialog(){}
};





// --- flange ----------------------------

class CFlangeDialog : public CActionParamDialog
{
public:
	CFlangeDialog(QWidget *mainWindow);
	virtual ~CFlangeDialog(){}
};





// --- delay -----------------------------

class CSimpleDelayDialog : public CActionParamDialog
{
public:
	CSimpleDelayDialog(QWidget *mainWindow);
	virtual ~CSimpleDelayDialog(){}
};





// --- quantize --------------------------

class CQuantizeDialog : public CActionParamDialog
{
public:
	CQuantizeDialog(QWidget *mainWindow);
	virtual ~CQuantizeDialog(){}
};





// --- distortion ------------------------

class CDistortionDialog : public CActionParamDialog
{
public:
	CDistortionDialog(QWidget *mainWindow);
	virtual ~CDistortionDialog(){}
};





// --- varied repeat ---------------------

class CVariedRepeatDialog : public CActionParamDialog
{
public:
	CVariedRepeatDialog(QWidget *mainWindow);
	virtual ~CVariedRepeatDialog(){}
};



#endif
