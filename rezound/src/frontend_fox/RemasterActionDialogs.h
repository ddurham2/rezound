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

#ifndef __RemasterActionDialogs_H__
#define __RemasterActionDialogs_H__

#include "../../config/common.h"
#include "fox_compat.h"

#include "CActionParamDialog.h"


// --- balance ------------------------

class CSimpleBalanceActionDialog : public CActionParamDialog
{
public:
	CSimpleBalanceActionDialog(FXWindow *mainWindow);
	virtual ~CSimpleBalanceActionDialog(){}

	bool show(CActionSound *actionSound,CActionParameters *actionParameters);
};

class CCurvedBalanceActionDialog : public CActionParamDialog
{
public:
	CCurvedBalanceActionDialog(FXWindow *mainWindow);
	virtual ~CCurvedBalanceActionDialog(){}

	bool show(CActionSound *actionSound,CActionParameters *actionParameters);
};



// --- monoize ------------------------

class CMonoizeActionDialog : public CActionParamDialog
{
public:
	CMonoizeActionDialog(FXWindow *mainWindow);
	virtual ~CMonoizeActionDialog(){}

	bool show(CActionSound *actionSound,CActionParameters *actionParameters);
};



// --- noise gate ------------------------

class CNoiseGateDialog : public CActionParamDialog
{
public:
	CNoiseGateDialog(FXWindow *mainWindow);
	virtual ~CNoiseGateDialog(){}
};



// --- compressor ------------------------

class CCompressorDialog : public CActionParamDialog
{
public:
	CCompressorDialog(FXWindow *mainWindow);
	virtual ~CCompressorDialog(){}

protected:
	const string getExplaination() const;
};



// --- normalize -------------------------

class CNormalizeDialog : public CActionParamDialog
{
public:
	CNormalizeDialog(FXWindow *mainWindow);
	virtual ~CNormalizeDialog(){}
};



// --- remaster --------------------------

class CResampleDialog : public CActionParamDialog
{
public:
	CResampleDialog(FXWindow *mainWindow);
	virtual ~CResampleDialog(){}
};



#endif
