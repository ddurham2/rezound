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

#ifndef FilterActionDialogs_H__
#define FilterActionDialogs_H__

#include "../../config/common.h"
#include "qt_compat.h"

#include "ActionParam/ActionParamDialog.h"


// --- convolution -----------------------

class ConvolutionFilterDialog : public ActionParamDialog
{
public:
    ConvolutionFilterDialog(QWidget *mainWindow);
    virtual ~ConvolutionFilterDialog(){}
};


// --- arbitrary FIR filter --------------

class ActionParamMapper_arbitraryFIRFilter_freq;
class ArbitraryFIRFilterDialog : public ActionParamDialog
{
	Q_OBJECT
public:
    ArbitraryFIRFilterDialog(QWidget *mainWindow);
    virtual ~ArbitraryFIRFilterDialog(){}

	//long onFrequencyRangeChange(FXObject *sender,FXSelector sel,void *ptr);
private Q_SLOTS:
	void on_frequencyRange_changed();

/*
	enum
	{
		ID_BASE_FREQUENCY=ActionParamDialog::ID_LAST,
		ID_NUMBER_OF_OCTAVES,
		ID_LAST
	};
*/

private:
    ActionParamMapper_arbitraryFIRFilter_freq *freqMapper;
};


// --- morphing arbitrary FIR filter --------------

class MorphingArbitraryFIRFilterDialog : public ActionParamDialog
{
	Q_OBJECT
public:
    MorphingArbitraryFIRFilterDialog(QWidget *mainWindow);
    virtual ~MorphingArbitraryFIRFilterDialog(){}

private Q_SLOTS:
	void on_frequencyRange_changed();
	void on_useLFOCheckBox_changed();
	void on_copy1To2();
	void on_swap1And2();
	void on_copy2To1();

protected:
	// gets called by ModalDialog before the okay button is accepted
	bool validateOnOkay();

	const string getExplanation() const;

    ActionParamMapper_arbitraryFIRFilter_freq *freqMapper;
};


// --- single pole lowpass ---------------

class SinglePoleLowpassFilterDialog : public ActionParamDialog
{
public:
    SinglePoleLowpassFilterDialog(QWidget *mainWindow);
    virtual ~SinglePoleLowpassFilterDialog(){}
};


// --- single pole highpass --------------

class SinglePoleHighpassFilterDialog : public ActionParamDialog
{
public:
    SinglePoleHighpassFilterDialog(QWidget *mainWindow);
    virtual ~SinglePoleHighpassFilterDialog(){}
};


// --- bandpass --------------------------

class BandpassFilterDialog : public ActionParamDialog
{
public:
    BandpassFilterDialog(QWidget *mainWindow);
    virtual ~BandpassFilterDialog(){}
};


// --- notch -----------------------------

class NotchFilterDialog : public ActionParamDialog
{
public:
    NotchFilterDialog(QWidget *mainWindow);
    virtual ~NotchFilterDialog(){}
};


// --- biquad resonant lowpass -----------

class BiquadResLowpassFilterDialog : public ActionParamDialog
{
public:
    BiquadResLowpassFilterDialog(QWidget *mainWindow);
    virtual ~BiquadResLowpassFilterDialog(){}
};

// --- biquad resonant highpass ----------

class BiquadResHighpassFilterDialog : public ActionParamDialog
{
public:
    BiquadResHighpassFilterDialog(QWidget *mainWindow);
    virtual ~BiquadResHighpassFilterDialog(){}
};

// --- biquad resonant bandpass ----------

class BiquadResBandpassFilterDialog : public ActionParamDialog
{
public:
    BiquadResBandpassFilterDialog(QWidget *mainWindow);
    virtual ~BiquadResBandpassFilterDialog(){}
};


#endif
