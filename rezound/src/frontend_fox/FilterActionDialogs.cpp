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



// --- convolution -----------------------

static const double interpretValue_wetdry_mix(const double x,const int s) { return(x*200.0-100.0); }
static const double uninterpretValue_wetdry_mix(const double x,const int s) { return((x+100.0)/200.0); }

static const double froo=log(80.0/(24.0+80.0))/log(0.5); // this is to make the middle come out 0.00dB
static const double interpretValue_dB_gain(const double x,const int s) { return(unitRange_to_otherRange_linear(pow(x,froo),-80,24)); }
static const double uninterpretValue_dB_gain(const double x,const int s) { return(pow(otherRange_to_unitRange_linear(x,-80,24),1.0/froo)); }

static const double interpretValue_delayTime(const double x,const int s) { return(x*s); }
static const double uninterpretValue_delayTime(const double x,const int s) { return(x/s); }

CConvolutionFilterDialog::CConvolutionFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Convolution Filter")
{
	// ??? still need predelay sliders
	void *p0=newVertPanel(NULL,true);
			void *p1=newHorzPanel(p0,false);
				addSlider(p1,"Wet/Dry Mix","%",interpretValue_wetdry_mix,uninterpretValue_wetdry_mix,NULL,50.0,0,0,0,true);

				addSlider(p1,"Input Lowpass","Hz",interpretValue_filter,uninterpretValue_filter,NULL,25000.0,5,100000,25000,false);
				addSlider(p1,"Input Gain","dB",interpretValue_dB_gain,uninterpretValue_dB_gain,dB_to_scalar,0.0,0,0,0,false);
				addSlider(p1,"Output Gain","dB",interpretValue_dB_gain,uninterpretValue_dB_gain,dB_to_scalar,0.0,0,0,0,false);
				addSlider(p1,"Predelay","ms",interpretValue_delayTime,uninterpretValue_delayTime,NULL,50.0,1,500,100,false);

				void *p2=newVertPanel(p1,false);
					addDiskEntityEntry(p2,"Filter Kernel","",FXDiskEntityParamValue::detAudioFilename,"The Audio File to Use as the Filter Kernel");

					void *p3=newHorzPanel(p2,false);
						addSlider(p3,"FK Gain","x",interpretValue_gain,uninterpretValue_gain,NULL,0.06667,1,50,30,true);
						addSlider(p3,"FK Lowpass","Hz",interpretValue_filter,uninterpretValue_filter,NULL,25000.0,5,100000,25000,false);
						void *p4=newVertPanel(p3,false);
							addSlider(p4,"FK Rate","x",interpretValue_gain,uninterpretValue_gain,NULL,1.0,2,10,2,true);
							addCheckBoxEntry(p4,"Reverse",false);
			p1=newHorzPanel(p0,false);
				addCheckBoxEntry(p1,"Wrap Decay back to Beginning",false);
}

// --- single pole lowpass ---------------

CSinglePoleLowpassFilterDialog::CSinglePoleLowpassFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Single Pole Lowpass Filter")
{
	void *p=newHorzPanel(NULL);
		addSlider(p,"Gain","x",interpretValue_gain,uninterpretValue_gain,NULL,1.0,2,50,2,true);
		addSlider(p,"Cutoff Frequency","Hz",interpretValue_filter,uninterpretValue_filter,NULL,500.0,5,100000,5000,false);
}

// --- single pole highpass --------------

CSinglePoleHighpassFilterDialog::CSinglePoleHighpassFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Single Pole Highpass Filter")
{
	void *p=newHorzPanel(NULL);
		addSlider(p,"Gain","x",interpretValue_gain,uninterpretValue_gain,NULL,1.0,2,50,2,true);
		addSlider(p,"Cutoff Frequency","Hz",interpretValue_filter,uninterpretValue_filter,NULL,1000.0,5,100000,10000,false);
}

// --- bandpass --------------------------

CBandpassFilterDialog::CBandpassFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Bandpass Filter")
{
	void *p=newHorzPanel(NULL);
		addSlider(p,"Gain","x",interpretValue_gain,uninterpretValue_gain,NULL,1.0,2,50,2,true);
		addSlider(p,"Center Frequency","Hz",interpretValue_filter,uninterpretValue_filter,NULL,1000.0,5,100000,10000,false);
		addSlider(p,"Band Width","Hz",interpretValue_filter,uninterpretValue_filter,NULL,500.0,5,100000,1000,false);
}

// --- notch -----------------------------

CNotchFilterDialog::CNotchFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Notch Filter")
{
	void *p=newHorzPanel(NULL);
		addSlider(p,"Gain","x",interpretValue_gain,uninterpretValue_gain,NULL,1.0,2,50,2,true);
		addSlider(p,"Center Frequency","Hz",interpretValue_filter,uninterpretValue_filter,NULL,1000.0,5,100000,10000,false);
		addSlider(p,"Band Width","Hz",interpretValue_filter,uninterpretValue_filter,NULL,500.0,5,100000,1000,false);
}



// --- biquad resonant lowpass -----------

CBiquadResLowpassFilterDialog::CBiquadResLowpassFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Biquad Resonant Lowpass Filter")
{
	void *p=newHorzPanel(NULL);
		addSlider(p,"Gain","x",interpretValue_gain,uninterpretValue_gain,NULL,1.0,2,50,2,true);
		addSlider(p,"Cutoff Frequency","Hz",interpretValue_filter,uninterpretValue_filter,NULL,500.0,5,100000,5000,false);
		addSlider(p,"Resonance","x",interpretValue_gain,uninterpretValue_gain,NULL,2.0,1,20,2,true);
}

// --- biquad resonant highpass ----------

CBiquadResHighpassFilterDialog::CBiquadResHighpassFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Biquad Resonant Highpass Filter")
{
	void *p=newHorzPanel(NULL);
		addSlider(p,"Gain","x",interpretValue_gain,uninterpretValue_gain,NULL,1.0,2,50,2,true);
		addSlider(p,"Cutoff Frequency","Hz",interpretValue_filter,uninterpretValue_filter,NULL,500.0,5,100000,5000,false);
		addSlider(p,"Resonance","x",interpretValue_gain,uninterpretValue_gain,NULL,2.0,1,20,2,true);
}

// --- biquad resonant bandpass ----------

CBiquadResBandpassFilterDialog::CBiquadResBandpassFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Biquad Resonant Bandpass Filter")
{
	void *p=newHorzPanel(NULL);
		addSlider(p,"Gain","x",interpretValue_gain,uninterpretValue_gain,NULL,1.0,2,50,2,true);
		addSlider(p,"Center Frequency","Hz",interpretValue_filter,uninterpretValue_filter,NULL,500.0,5,100000,5000,false);
		addSlider(p,"Resonance","x",interpretValue_gain,uninterpretValue_gain,NULL,2.0,1,20,2,true);
}

