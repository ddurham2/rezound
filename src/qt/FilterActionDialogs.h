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

#ifndef __FilterActionDialogs_H__
#define __FilterActionDialogs_H__

#include "../../config/common.h"
#include "qt_compat.h"

#include "ActionParam/CActionParamDialog.h"


// --- convolution -----------------------

class CConvolutionFilterDialog : public CActionParamDialog
{
public:
	CConvolutionFilterDialog(QWidget *mainWindow);
	virtual ~CConvolutionFilterDialog(){}
};


// --- arbitrary FIR filter --------------

class CActionParamMapper_arbitraryFIRFilter_freq;
class CArbitraryFIRFilterDialog : public CActionParamDialog
{
	Q_OBJECT
public:
	CArbitraryFIRFilterDialog(QWidget *mainWindow);
	virtual ~CArbitraryFIRFilterDialog(){}

	//long onFrequencyRangeChange(FXObject *sender,FXSelector sel,void *ptr);
private Q_SLOTS:
	void on_frequencyRange_changed();

/*
	enum
	{
		ID_BASE_FREQUENCY=CActionParamDialog::ID_LAST,
		ID_NUMBER_OF_OCTAVES,
		ID_LAST
	};
*/

private:
	CActionParamMapper_arbitraryFIRFilter_freq *freqMapper;
};


// --- morphing arbitrary FIR filter --------------

class CMorphingArbitraryFIRFilterDialog : public CActionParamDialog
{
	Q_OBJECT
public:
	CMorphingArbitraryFIRFilterDialog(QWidget *mainWindow);
	virtual ~CMorphingArbitraryFIRFilterDialog(){}

private Q_SLOTS:
	void on_frequencyRange_changed();
	void on_useLFOCheckBox_changed();
	void on_copy1To2();
	void on_swap1And2();
	void on_copy2To1();

protected:
	// gets called by CModalDialog before the okay button is accepted
	bool validateOnOkay();

	const string getExplanation() const;

	CActionParamMapper_arbitraryFIRFilter_freq *freqMapper;
};


// --- single pole lowpass ---------------

class CSinglePoleLowpassFilterDialog : public CActionParamDialog
{
public:
	CSinglePoleLowpassFilterDialog(QWidget *mainWindow);
	virtual ~CSinglePoleLowpassFilterDialog(){}
};


// --- single pole highpass --------------

class CSinglePoleHighpassFilterDialog : public CActionParamDialog
{
public:
	CSinglePoleHighpassFilterDialog(QWidget *mainWindow);
	virtual ~CSinglePoleHighpassFilterDialog(){}
};


// --- bandpass --------------------------

class CBandpassFilterDialog : public CActionParamDialog
{
public:
	CBandpassFilterDialog(QWidget *mainWindow);
	virtual ~CBandpassFilterDialog(){}
};


// --- notch -----------------------------

class CNotchFilterDialog : public CActionParamDialog
{
public:
	CNotchFilterDialog(QWidget *mainWindow);
	virtual ~CNotchFilterDialog(){}
};


// --- biquad resonant lowpass -----------

class CBiquadResLowpassFilterDialog : public CActionParamDialog
{
public:
	CBiquadResLowpassFilterDialog(QWidget *mainWindow);
	virtual ~CBiquadResLowpassFilterDialog(){}
};

// --- biquad resonant highpass ----------

class CBiquadResHighpassFilterDialog : public CActionParamDialog
{
public:
	CBiquadResHighpassFilterDialog(QWidget *mainWindow);
	virtual ~CBiquadResHighpassFilterDialog(){}
};

// --- biquad resonant bandpass ----------

class CBiquadResBandpassFilterDialog : public CActionParamDialog
{
public:
	CBiquadResBandpassFilterDialog(QWidget *mainWindow);
	virtual ~CBiquadResBandpassFilterDialog(){}
};


#endif
