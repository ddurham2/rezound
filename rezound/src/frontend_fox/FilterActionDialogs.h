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


// --- arbitrary FIR filter --------------

class CArbitraryFIRFilterDialog : public CActionParamDialog
{
	FXDECLARE(CArbitraryFIRFilterDialog);
public:
	CArbitraryFIRFilterDialog(FXWindow *mainWindow);
	virtual ~CArbitraryFIRFilterDialog(){}

	long onFrequencyRangeChange(FXObject *sender,FXSelector sel,void *ptr);

	enum
	{
		ID_BASE_FREQUENCY=CActionParamDialog::ID_LAST,
		ID_NUMBER_OF_OCTAVES,
		ID_LAST
	};

protected:
	CArbitraryFIRFilterDialog() {}
};


// --- morphing arbitrary FIR filter --------------

class CMorphingArbitraryFIRFilterDialog : public CActionParamDialog
{
	FXDECLARE(CMorphingArbitraryFIRFilterDialog);
public:
	CMorphingArbitraryFIRFilterDialog(FXWindow *mainWindow);
	virtual ~CMorphingArbitraryFIRFilterDialog(){}

	long onFrequencyRangeChange(FXObject *sender,FXSelector sel,void *ptr);
	long onUseLFOCheckBox(FXObject *sender,FXSelector sel,void *ptr);
	long on1To2Button(FXObject *sender,FXSelector sel,void *ptr); // handles several operations between response1 and response2

	enum
	{
		ID_BASE_FREQUENCY=CActionParamDialog::ID_LAST,
		ID_NUMBER_OF_OCTAVES,
		ID_USE_LFO_CHECKBOX,
		ID_COPY_1_TO_2,
		ID_COPY_2_TO_1,
		ID_SWAP_1_AND_2,
		ID_LAST
	};

protected:
	CMorphingArbitraryFIRFilterDialog() {}

	// gets called by FXModalDialogBox before the okay button is accepted
	bool validateOnOkay();

	const string getExplanation() const;
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
