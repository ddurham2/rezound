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

#include "EffectActionDialogs.h"
#include "interpretValue.h"
#include "../backend/unit_conv.h"

// --- volume ---------------------------------

CNormalVolumeChangeDialog::CNormalVolumeChangeDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addSlider(p,N_("Volume Change"),"dB",interpretValue_bipolar_scalar,uninterpretValue_bipolar_scalar,dB_to_scalar,3.0,1,100,3,true);
}



CNormalGainDialog::CNormalGainDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addSlider(p,N_("Gain"),"x",interpretValue_recipsym_scalar,uninterpretValue_recipsym_scalar,NULL,2.0,2,100,2,true);
		// ??? add a check box for negation of gain
}

static const double interpretValue_gain_curve(const double x,const int s) { return x*s; }
static const double uninterpretValue_gain_curve(const double x,const int s) { return x/s; }

CAdvancedGainDialog::CAdvancedGainDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addGraphWithWaveform(p,N_("Gain Curve"),N_("Gain"),"x",interpretValue_gain_curve,uninterpretValue_gain_curve,NULL,-10,10,2);
}



// --- rate -----------------------------------

CNormalRateChangeDialog::CNormalRateChangeDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addSlider(p,N_("Rate Change"),"x",interpretValue_recipsym_scalar,uninterpretValue_recipsym_scalar,NULL,2.0,1,100,2,true);
}

CAdvancedRateChangeDialog::CAdvancedRateChangeDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addGraphWithWaveform(p,N_("Rate Curve"),N_("Rate"),"x",interpretValue_scalar,uninterpretValue_scalar,NULL,1,100,2);
}





// --- flange ------------------------------

static const double interpretValue_flange_feedback(const double x,const int s) { return unitRange_to_otherRange_linear(x,-0.95,0.95); }
static const double uninterpretValue_flange_feedback(const double x,const int s) { return otherRange_to_unitRange_linear(x,-0.95,0.95); }

CFlangeDialog::CFlangeDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addSlider(p,N_("Delay"),"ms",interpretValue_scalar,uninterpretValue_scalar,NULL,2.0,2,10,2,false);
		addSlider(p,N_("Wet Gain"),"x",interpretValue_scalar,uninterpretValue_scalar,NULL,1.0,-20,20,2,false);
		addSlider(p,N_("Dry Gain"),"x",interpretValue_scalar,uninterpretValue_scalar,NULL,1.0,-20,20,2,false);
		addLFO(p,N_("Flange LFO"),"ms",N_("LFO Depth"),20,"Hz",20,true);
		addSlider(p,N_("Feedback"),"x",interpretValue_flange_feedback,uninterpretValue_flange_feedback,NULL,0.0,0,0,0,true);
}





// --- delay -------------------------------

CSimpleDelayDialog::CSimpleDelayDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addSlider(p,N_("Delay"),"ms",interpretValue_scalar,uninterpretValue_scalar,NULL,250.0,1,10000,1000,false);
		addSlider(p,N_("Gain"),"x",interpretValue_scalar,uninterpretValue_scalar,NULL,0.75,-20,20,2,false);
		addSlider(p,N_("Feedback"),"x",interpretValue_scalar,uninterpretValue_scalar,NULL,0.35,-5,5,1,false);
}





// --- quantize ----------------------------

static const double interpretValue_quantize(const double x,const int s) { return floor(max(1.0,x*s)); }
static const double uninterpretValue_quantize(const double x,const int s) { return x/s; }

CQuantizeDialog::CQuantizeDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addSlider(p,N_("Input Gain"),"x",interpretValue_recipsym_scalar,uninterpretValue_recipsym_scalar,NULL,1.0,2,100,2,true);
		addSlider(p,N_("Quantum Count"),"",interpretValue_quantize,uninterpretValue_quantize,NULL,8,4,1000000,32,false);
			setTipText("Quantum Count",_("The Number of Possible Sample Values Above Zero"));
		addSlider(p,N_("Output Gain"),"x",interpretValue_recipsym_scalar,uninterpretValue_recipsym_scalar,NULL,1.0,2,100,2,true);

}





// --- distortion --------------------------

CDistortionDialog::CDistortionDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addGraph(p,N_("Curve"),N_("Input"),"dBFS",interpretValue_dBFS,uninterpretValue_dBFS,N_("Output"),"dBFS",interpretValue_dBFS,uninterpretValue_dBFS,NULL,0,0,0);

}





// --- varied repeat -----------------------

CVariedRepeatDialog::CVariedRepeatDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addLFO(p,N_("LFO"),"","",0,"Hz",200,true);
		addSlider(p,N_("Time"),"s",interpretValue_scalar,uninterpretValue_scalar,NULL,4.0,1,100,10,false);
			// ??? need a checkbox that enables a gain parameter to make it mix ontop of the current audio instead of inserting into it
}


