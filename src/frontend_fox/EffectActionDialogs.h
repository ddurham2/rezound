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
#include "fox_compat.h"

#include "CActionParamDialog.h"


// --- volume ----------------------------

class CNormalVolumeChangeDialog : public CActionParamDialog
{
public:
	CNormalVolumeChangeDialog(FXWindow *mainWindow);
	virtual ~CNormalVolumeChangeDialog(){}
};

class CNormalGainDialog : public CActionParamDialog
{
public:
	CNormalGainDialog(FXWindow *mainWindow);
	virtual ~CNormalGainDialog(){}
};

class CAdvancedGainDialog : public CActionParamDialog
{
public:
	CAdvancedGainDialog(FXWindow *mainWindow);
	virtual ~CAdvancedGainDialog(){}
};



// --- rate ------------------------------

class CNormalRateChangeDialog : public CActionParamDialog
{
public:
	CNormalRateChangeDialog(FXWindow *mainWindow);
	virtual ~CNormalRateChangeDialog(){}
};

class CAdvancedRateChangeDialog : public CActionParamDialog
{
public:
	CAdvancedRateChangeDialog(FXWindow *mainWindow);
	virtual ~CAdvancedRateChangeDialog(){}
};





// --- flange ----------------------------

class CFlangeDialog : public CActionParamDialog
{
public:
	CFlangeDialog(FXWindow *mainWindow);
	virtual ~CFlangeDialog(){}
};





// --- delay -----------------------------

class CSimpleDelayDialog : public CActionParamDialog
{
public:
	CSimpleDelayDialog(FXWindow *mainWindow);
	virtual ~CSimpleDelayDialog(){}
};





// --- quantize --------------------------

class CQuantizeDialog : public CActionParamDialog
{
public:
	CQuantizeDialog(FXWindow *mainWindow);
	virtual ~CQuantizeDialog(){}
};





// --- distortion ------------------------

class CDistortionDialog : public CActionParamDialog
{
public:
	CDistortionDialog(FXWindow *mainWindow);
	virtual ~CDistortionDialog(){}
};





// --- varied repeat ---------------------

class CVariedRepeatDialog : public CActionParamDialog
{
public:
	CVariedRepeatDialog(FXWindow *mainWindow);
	virtual ~CVariedRepeatDialog(){}
};



#endif
