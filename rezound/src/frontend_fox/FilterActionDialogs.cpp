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

#include "FilterActionDialogs.h"
#include "../backend/unit_conv.h"




static const double interpretValue_filter(const double x,const int s) { return(x*s); }
static const double uninterpretValue_filter(const double x,const int s) { return(x/s); }

static const double interpretValue_gain(const double x,const int s) { return(unitRange_to_bipolarRange_exp(x,s)); }
static const double uninterpretValue_gain(const double x,const int s) { return(bipolarRange_to_unitRange_exp(x,s)); }

// --- single pole lowpass ---------------

CSinglePoleLowpassFilterDialog::CSinglePoleLowpassFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Single Pole Lowpass Filter")
{
	addSlider("Gain","x",interpretValue_gain,uninterpretValue_gain,NULL,1.0,2,50,2,true);
	addSlider("Cutoff Frequency","Hz",interpretValue_filter,uninterpretValue_filter,NULL,500.0,5,100000,5000,false);
}

// --- single pole highpass --------------

CSinglePoleHighpassFilterDialog::CSinglePoleHighpassFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Single Pole Highpass Filter")
{
	addSlider("Gain","x",interpretValue_gain,uninterpretValue_gain,NULL,1.0,2,50,2,true);
	addSlider("Cutoff Frequency","Hz",interpretValue_filter,uninterpretValue_filter,NULL,1000.0,5,100000,10000,false);
}

// --- bandpass --------------------------

CBandpassFilterDialog::CBandpassFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Bandpass Filter")
{
	addSlider("Gain","x",interpretValue_gain,uninterpretValue_gain,NULL,1.0,2,50,2,true);
	addSlider("Center Frequency","Hz",interpretValue_filter,uninterpretValue_filter,NULL,1000.0,5,100000,10000,false);
	addSlider("Band Width","Hz",interpretValue_filter,uninterpretValue_filter,NULL,500.0,5,100000,1000,false);
}

// --- notch -----------------------------

CNotchFilterDialog::CNotchFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Notch Filter")
{
	addSlider("Gain","x",interpretValue_gain,uninterpretValue_gain,NULL,1.0,2,50,2,true);
	addSlider("Center Frequency","Hz",interpretValue_filter,uninterpretValue_filter,NULL,1000.0,5,100000,10000,false);
	addSlider("Band Width","Hz",interpretValue_filter,uninterpretValue_filter,NULL,500.0,5,100000,1000,false);
}



// --- biquad resonant lowpass -----------

CBiquadResLowpassFilterDialog::CBiquadResLowpassFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Biquad Resonant Lowpass Filter")
{
	addSlider("Gain","x",interpretValue_gain,uninterpretValue_gain,NULL,1.0,2,50,2,true);
	addSlider("Cutoff Frequency","Hz",interpretValue_filter,uninterpretValue_filter,NULL,500.0,5,100000,5000,false);
	addSlider("Resonance","x",interpretValue_gain,uninterpretValue_gain,NULL,2.0,1,20,2,true);
}

// --- biquad resonant highpass ----------

CBiquadResHighpassFilterDialog::CBiquadResHighpassFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Biquad Resonant Highpass Filter")
{
	addSlider("Gain","x",interpretValue_gain,uninterpretValue_gain,NULL,1.0,2,50,2,true);
	addSlider("Cutoff Frequency","Hz",interpretValue_filter,uninterpretValue_filter,NULL,500.0,5,100000,5000,false);
	addSlider("Resonance","x",interpretValue_gain,uninterpretValue_gain,NULL,2.0,1,20,2,true);
}

// --- biquad resonant bandpass ----------

CBiquadResBandpassFilterDialog::CBiquadResBandpassFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Biquad Resonant Bandpass Filter")
{
	addSlider("Gain","x",interpretValue_gain,uninterpretValue_gain,NULL,1.0,2,50,2,true);
	addSlider("Center Frequency","Hz",interpretValue_filter,uninterpretValue_filter,NULL,500.0,5,100000,5000,false);
	addSlider("Resonance","x",interpretValue_gain,uninterpretValue_gain,NULL,2.0,1,20,2,true);
}

