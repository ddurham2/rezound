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
#include "../backend/unit_conv.h"

// --- volume ---------------------------------

static const double interpretValue_amp(const double x,const int scalar)
{ // [0,1] -> [-scalar,scalar] -- slider pos to dB
	//return(unitRange_to_otherRange_linear(unitRange_to_unitRange_cubed(x),-scalar,scalar));
	return(unitRange_to_otherRange_linear(x,-scalar,scalar));
}

static const double uninterpretValue_amp(const double x,const int scalar)
{ // [-scalar,scalar] -> [0,1] -- dB to slider pos
	//return(unitRange_to_unitRange_uncubed(otherRange_to_unitRange_linear(x,-scalar,scalar)));
	return(otherRange_to_unitRange_linear(x,-scalar,scalar));
}

CNormalVolumeChangeDialog::CNormalVolumeChangeDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Change Volume")
{
	addSlider("Volume Change","dB",interpretValue_amp,uninterpretValue_amp,dB_to_scalar,3.0,1,50,3,true);
}



static const double interpretValue_gain(const double x,const int s) { return(unitRange_to_bipolarRange_exp(x,s)); }
static const double uninterpretValue_gain(const double x,const int s) { return(bipolarRange_to_unitRange_exp(x,s)); }

static const double interpretValue_gain_curve(const double x,const int s) { return(x*s); }
static const double uninterpretValue_gain_curve(const double x,const int s) { return(x/s); }

CNormalGainDialog::CNormalGainDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Change Gain")
{
	addSlider("Gain","x",interpretValue_gain,uninterpretValue_gain,NULL,2.0,2,50,2,true);
}

CAdvancedGainDialog::CAdvancedGainDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Change Gain According to a Curve")
{
	addGraph("Gain Curve","x",interpretValue_gain_curve,uninterpretValue_gain_curve,NULL,-10,10,2);
}



// --- rate -----------------------------------

static const double interpretValue_rate(const double x,const int s)
{ // [0,1] -> [1/s,s] -- slider pos to rate scalar
	return(unitRange_to_bipolarRange_exp(x,s));
}

static const double uninterpretValue_rate(const double x,const int s)
{ // [1/s,s] -> [0,1] -- rate scalar to slider pos
	return(bipolarRange_to_unitRange_exp(x,s));
}

CNormalRateChangeDialog::CNormalRateChangeDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Change Rate")
{
	addSlider("Rate Change","x",interpretValue_rate,uninterpretValue_rate,NULL,2.0,1,100,2,true);
}

static const double interpretValue_rate2(const double x,const int s)
{ // [0.0,1.0] -> [0.1,2.0] -- slider pos to rate scalar
        return(unitRange_to_otherRange_linear(x,0.0,s));
}

static const double uninterpretValue_rate2(const double x,const int s)
{ // [0.1,1.0] -> [0.0,1.0] -- rate scalar to slider pos
        return(otherRange_to_unitRange_linear(x,0.0,s));
}


CAdvancedRateChangeDialog::CAdvancedRateChangeDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Change Rate According to a Curve")
{
	addGraph("Rate Curve","x",interpretValue_rate2,uninterpretValue_rate2,NULL,1,100,2);
}





// --- flange ------------------------------

static const double interpretValue_flange(const double x,const int s) { return(x*s); }
static const double uninterpretValue_flange(const double x,const int s) { return(x/s); }

/*
static const double interpretValue_flange_LFO_phase(const double x,const int s) { return(unitRange_to_otherRange_linear(x,0,360)); }
static const double uninterpretValue_flange_LFO_phase(const double x,const int s) { return(otherRange_to_unitRange_linear(x,0,360)); }
*/

static const double interpretValue_flange_feedback(const double x,const int s) { return(unitRange_to_otherRange_linear(x,-0.95,0.95)); }
static const double uninterpretValue_flange_feedback(const double x,const int s) { return(otherRange_to_unitRange_linear(x,-0.95,0.95)); }

CFlangeDialog::CFlangeDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Flange")
{
	addSlider("Delay","ms",interpretValue_flange,uninterpretValue_flange,NULL,2.0,2,10,2,false);
	addSlider("Wet Gain","x",interpretValue_flange,uninterpretValue_flange,NULL,1.0,-5,5,1,false);
	addSlider("Dry Gain","x",interpretValue_flange,uninterpretValue_flange,NULL,1.0,-5,5,1,false);
	addLFO("Flange LFO","ms","LFO Depth",20,"Hz",20,true);
	addSlider("Feedback","x",interpretValue_flange_feedback,uninterpretValue_flange_feedback,NULL,0.0,0,0,0,true);
}





// --- delay -------------------------------

static const double interpretValue_delay(const double x,const int s) { return(x*s); }
static const double uninterpretValue_delay(const double x,const int s) { return(x/s); }

CSimpleDelayDialog::CSimpleDelayDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Simple Delay")
{
	addSlider("Delay","ms",interpretValue_delay,uninterpretValue_delay,NULL,250.0,1,10000,1000,false);
	addSlider("Gain","x",interpretValue_delay,uninterpretValue_delay,NULL,0.75,-5,5,1,false);
	addSlider("Feedback","x",interpretValue_delay,uninterpretValue_delay,NULL,0.35,-5,5,1,false);
}





// --- quantize ----------------------------

static const double interpretValue_quantize(const double x,const int s) { return(floor(max(1.0,x*s))); }
static const double uninterpretValue_quantize(const double x,const int s) { return(x/s); }

static const double interpretValue_quantizeGain(const double x,const int s) { return(unitRange_to_bipolarRange_exp(x,s)); }
static const double uninterpretValue_quantizeGain(const double x,const int s) { return(bipolarRange_to_unitRange_exp(x,s)); }

CQuantizeDialog::CQuantizeDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Quantize the Number of Possible Sample Values")
{
	addSlider("Input Gain","x",interpretValue_quantizeGain,uninterpretValue_quantizeGain,NULL,1.0,2,100,2,true);
	addSlider("Quantum Count","",interpretValue_quantize,uninterpretValue_quantize,NULL,8,4,1000000,32,false);
		setTipText(1,"The Number of Possible Sample Values Above Zero");
	addSlider("Output Gain","x",interpretValue_quantizeGain,uninterpretValue_quantizeGain,NULL,1.0,2,100,2,true);

}





// --- varied repeat -----------------------

static const double interpretValue_varied_repeat(const double x,const int s) { return(x*s); }
static const double uninterpretValue_varied_repeat(const double x,const int s) { return(x/s); }
static const double interpretValue_varied_repeat_LFO_phase(const double x,const int s) { return(unitRange_to_otherRange_linear(x,0,360)); }
static const double uninterpretValue_varied_repeat_LFO_phase(const double x,const int s) { return(otherRange_to_unitRange_linear(x,0,360)); }

static const double interpretValue_flange_LFO_phase(const double x,const int s) { return(unitRange_to_otherRange_linear(x,0,360)); }
static const double uninterpretValue_flange_LFO_phase(const double x,const int s) { return(otherRange_to_unitRange_linear(x,0,360)); }

CVariedRepeatDialog::CVariedRepeatDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Varied Repeat")
{
	addLFO("LFO","","",0,"Hz",200,true);
	addSlider("Time","s",interpretValue_varied_repeat,uninterpretValue_varied_repeat,NULL,4.0,1,100,10,false);
		// ??? need a checkbox that enables a gain parameter to make it mix ontop of the current audio instead of inserting into it
}


