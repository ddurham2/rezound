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


#include "CConstantParamActionDialog.h"
#include "CGraphParamActionDialog.h"

#include "CActionParamDialog.h"


// --- volume ----------------------------

class CNormalAmplitudeChangeDialog : public CConstantParamActionDialog
{
public:
	CNormalAmplitudeChangeDialog(FXWindow *mainWindow);
};

class CAdvancedAmplitudeChangeDialog : public CGraphParamActionDialog
{
public:
	CAdvancedAmplitudeChangeDialog(FXWindow *mainWindow);
};



// --- rate ------------------------------

class CNormalRateChangeDialog : public CConstantParamActionDialog
{
public:
	CNormalRateChangeDialog(FXWindow *mainWindow);
};

class CAdvancedRateChangeDialog : public CGraphParamActionDialog
{
public:
	CAdvancedRateChangeDialog(FXWindow *mainWindow);
};





// --- flange ----------------------------

class CFlangeDialog : public CActionParamDialog
{
public:
	CFlangeDialog(FXWindow *mainWindow);
};





// --- delay -----------------------------

class CSimpleDelayDialog : public CConstantParamActionDialog
{
public:
	CSimpleDelayDialog(FXWindow *mainWindow);
};





// --- varied repeat ---------------------

class CVariedRepeatDialog : public CConstantParamActionDialog
{
public:
	CVariedRepeatDialog(FXWindow *mainWindow);
};



#endif
