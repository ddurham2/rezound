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

#ifndef __FilterActionDialogs_H__
#define __FilterActionDialogs_H__

#include "../../config/common.h"
#include "fox_compat.h"

#include "CActionParamDialog.h"


// --- convolution -----------------------

class CConvolutionFilterDialog : public CActionParamDialog
{
public:
	CConvolutionFilterDialog(FXWindow *mainWindow);
	virtual ~CConvolutionFilterDialog(){}
};


// --- single pole lowpass ---------------

class CSinglePoleLowpassFilterDialog : public CActionParamDialog
{
public:
	CSinglePoleLowpassFilterDialog(FXWindow *mainWindow);
	virtual ~CSinglePoleLowpassFilterDialog(){}
};


// --- single pole highpass --------------

class CSinglePoleHighpassFilterDialog : public CActionParamDialog
{
public:
	CSinglePoleHighpassFilterDialog(FXWindow *mainWindow);
	virtual ~CSinglePoleHighpassFilterDialog(){}
};


// --- bandpass --------------------------

class CBandpassFilterDialog : public CActionParamDialog
{
public:
	CBandpassFilterDialog(FXWindow *mainWindow);
	virtual ~CBandpassFilterDialog(){}
};


// --- notch -----------------------------

class CNotchFilterDialog : public CActionParamDialog
{
public:
	CNotchFilterDialog(FXWindow *mainWindow);
	virtual ~CNotchFilterDialog(){}
};


// --- biquad resonant lowpass -----------

class CBiquadResLowpassFilterDialog : public CActionParamDialog
{
public:
	CBiquadResLowpassFilterDialog(FXWindow *mainWindow);
	virtual ~CBiquadResLowpassFilterDialog(){}
};

// --- biquad resonant highpass ----------

class CBiquadResHighpassFilterDialog : public CActionParamDialog
{
public:
	CBiquadResHighpassFilterDialog(FXWindow *mainWindow);
	virtual ~CBiquadResHighpassFilterDialog(){}
};

// --- biquad resonant bandpass ----------

class CBiquadResBandpassFilterDialog : public CActionParamDialog
{
public:
	CBiquadResBandpassFilterDialog(FXWindow *mainWindow);
	virtual ~CBiquadResBandpassFilterDialog(){}
};



#endif
