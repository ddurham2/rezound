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

#include "RemasterActionDialogs.h"
#include "../backend/unit_conv.h"





// --- noise gate --------------------------

static const double interpretValue_noiseGate(const double x,const int s) { return(x*s); }
static const double uninterpretValue_noiseGate(const double x,const int s) { return(x/s); }

CNoiseGateDialog::CNoiseGateDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Noise Gate")
{
	void *p=newHorzPanel(NULL);
		addSlider(p,"Window Time","ms",interpretValue_noiseGate,uninterpretValue_noiseGate,NULL,35.0,5,1000,30,false);
		addSlider(p,"Threshold","%",interpretValue_noiseGate,uninterpretValue_noiseGate,NULL,3.0,5,100,20,false);
		addSlider(p,"Gain Attack Time","ms",interpretValue_noiseGate,uninterpretValue_noiseGate,NULL,10.0,5,1000,30,false);
		addSlider(p,"Gain Release Time","ms",interpretValue_noiseGate,uninterpretValue_noiseGate,NULL,10.0,5,1000,30,false);
}



// --- compressor --------------------------

static const double interpretValue_compressorWindowTime(const double x,const int s) { return(unitRange_to_otherRange_linear(x,.1,100)*s); }
static const double uninterpretValue_compressorWindowTime(const double x,const int s) { return(otherRange_to_unitRange_linear(x/s,.1,100)); }

static const double interpretValue_dBFS(const double x,const int s) { return(amp_to_dBFS(x,1.0)); }
static const double uninterpretValue_dBFS(const double x,const int s) { return(dBFS_to_amp(x,1.0)); }

static const double interpretValue_compressionRatio(const double x,const int s) { return(unitRange_to_otherRange_linear(x,1,s)); }
static const double uninterpretValue_compressionRatio(const double x,const int s) { return(otherRange_to_unitRange_linear(x,1,s)); }

static const double interpretValue_compressAttack(const double x,const int s) { return(unitRange_to_otherRange_linear(unitRange_to_unitRange_squared(x),.1,100)); }
static const double uninterpretValue_compressAttack(const double x,const int s) { return(unitRange_to_unitRange_unsquared(otherRange_to_unitRange_linear(x,.1,100))); }

static const double interpretValue_compressRelease(const double x,const int s) { return(unitRange_to_otherRange_linear(unitRange_to_unitRange_squared(x),1,1000)); }
static const double uninterpretValue_compressRelease(const double x,const int s) { return(unitRange_to_unitRange_unsquared(otherRange_to_unitRange_linear(x,1,1000))); }
											// ??? I really need to rename this "bipolar" it's really a misleading name...
static const double interpretValue_compressGain(const double x,const int s) { return(unitRange_to_bipolarRange_exp(x,10.0)); }
static const double uninterpretValue_compressGain(const double x,const int s) { return(bipolarRange_to_unitRange_exp(x,10.0)); }

CCompressorDialog::CCompressorDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Compressor")
{
	void *p0=newVertPanel(NULL,true);
		void *p1=newHorzPanel(p0,false);
			addSlider(p1,"Window Time","ms",interpretValue_compressorWindowTime,uninterpretValue_compressorWindowTime,NULL,35.0,1,10,1,false);
			addSlider(p1,"Threshold","dBFS",interpretValue_dBFS,uninterpretValue_dBFS,NULL,-12.0,0,0,1,false);
			addSlider(p1,"Ratio",":1",interpretValue_compressionRatio,uninterpretValue_compressionRatio,NULL,2.0,2,20,6,false);
			addSlider(p1,"Attack Time","ms",interpretValue_compressAttack,uninterpretValue_compressAttack,NULL,10.0,0,0,0,false);
			addSlider(p1,"Release Time","ms",interpretValue_compressRelease,uninterpretValue_compressRelease,NULL,50.0,0,0,0,false);
			addSlider(p1,"Input Gain","x",interpretValue_compressGain,uninterpretValue_compressGain,NULL,1.0,0,0,0,true);
			addSlider(p1,"Output Gain","x",interpretValue_compressGain,uninterpretValue_compressGain,NULL,1.0,0,0,0,true);
		p1=newVertPanel(p0,false);
			addCheckBoxEntry(p1,"Lock Channels",true);
}



// --- normalize ---------------------------

static const double interpretValue_regionCount(const double x,const int s) { return(floor(x*99.0)+1.0); }
static const double uninterpretValue_regionCount(const double x,const int s) { return((x-1.0)/99.0); }

CNormalizeDialog::CNormalizeDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Normalize")
{
	void *p1=newVertPanel(NULL);
		void *p2=newHorzPanel(p1,false);
			addSlider(p2,"Normalization Level","dBFS",interpretValue_dBFS,uninterpretValue_dBFS,NULL,-0.5,0,0,1,false);
			addSlider(p2,"Region Count","",interpretValue_regionCount,uninterpretValue_regionCount,NULL,1,0,0,0,false);
		p2=newVertPanel(p1,false);
			addCheckBoxEntry(p2,"Lock Channels",true,"Calculate Maximum Sample Value in a Region Across All Channels");
}


// --- resample ----------------------------

CResampleDialog::CResampleDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Resample",false)
{
	vector<string> items;
	items.push_back("4000");
	items.push_back("8000");
	items.push_back("11025");
	items.push_back("22050");
	items.push_back("44100");
	items.push_back("48000");
	items.push_back("88200");
	items.push_back("96000");

	void *p0=newVertPanel(NULL);
		addComboTextEntry(p0,"New Sample Rate",items,"",true);
			getComboText("New Sample Rate")->setValue(44100);
}

