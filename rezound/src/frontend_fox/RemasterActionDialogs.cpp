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

#include <istring>

#include "CFOXIcons.h"

#include "../backend/CActionSound.h"
#include "../backend/CActionParameters.h"
#include "../backend/CSound.h"



// --- balance ------------------------

#include "../backend/Remaster/CBalanceAction.h"
static const double interpretValue_balance(const double x,const int s) { return unitRange_to_otherRange_linear(x,100,-100); }
static const double uninterpretValue_balance(const double x,const int s) { return otherRange_to_unitRange_linear(x,100,-100); }
static const double ret_balance(const double x) { return x/100.0; }

CSimpleBalanceActionDialog::CSimpleBalanceActionDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,N_("Simple Balance Change"))
{
	void *p0=newVertPanel(NULL,true);
		void *p1=newHorzPanel(p0,false);
			void *p2=newVertPanel(p1,false);
				vector<string> items;
				addComboTextEntry(p2,N_("Channel A"),items,"");
				addComboTextEntry(p2,N_("Channel B"),items,"");

			
			addSlider(p1,N_("Balance"),"%",interpretValue_balance,uninterpretValue_balance,ret_balance,0.0,0,0,0,false);

			vector<string> balanceTypes;
			balanceTypes.push_back(N_("Strict Balance"));
			balanceTypes.push_back(N_("1x Pan"));
			balanceTypes.push_back(N_("2x Pan"));
		addComboTextEntry(p0,N_("Balance Type"),balanceTypes,CBalanceAction::getBalanceTypeExplaination());
		/* not possible
			vector<FXIcon *> balanceTypeIcons;
			balanceTypeIcons.push_back(FOXIcons->Falling_Sawtooth_Wave___0_1_);
			balanceTypeIcons.push_back(FOXIcons->Falling_Sawtooth_Wave___0_1_);
			balanceTypeIcons.push_back(FOXIcons->Falling_Sawtooth_Wave___0_1_);
		getComboText("Balance Type")->setIcons(balanceTypeIcons);
		*/
}

bool CSimpleBalanceActionDialog::show(CActionSound *actionSound,CActionParameters *actionParameters)
{
	// exact same implementation as CCurvedBalanceActionDialog::show()

	if(actionSound->sound->getChannelCount()<2)
		return false;
	else
	{
		if(getComboText("Channel A")->getItems().size()!=actionSound->sound->getChannelCount()) // don't alter selected channels if they didn't change in number
		{
			vector<string> items;
			for(size_t t=0;t<actionSound->sound->getChannelCount();t++)
				items.push_back(N_("Channel ")+istring(t));

			// set the combo boxes according to actionSound
			getComboText("Channel A")->setItems(items);
			getComboText("Channel B")->setItems(items);
			getComboText("Channel A")->setCurrentItem(0);
			getComboText("Channel B")->setCurrentItem(1);
		}

		return CActionParamDialog::show(actionSound,actionParameters);
	}
}



CCurvedBalanceActionDialog::CCurvedBalanceActionDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,N_("Curved Balance Change"))
{
	void *p0=newVertPanel(NULL,true);
		void *p1=newHorzPanel(p0,false);
			void *p2=newVertPanel(p1,false);
				vector<string> items;
				addComboTextEntry(p2,N_("Channel A"),items,"");
				addComboTextEntry(p2,N_("Channel B"),items,"");

			addGraphWithWaveform(p1,N_("Balance Curve"),N_("Balance"),"%",interpretValue_balance,uninterpretValue_balance,ret_balance,0,0,0);

		vector<string> balanceTypes;
		balanceTypes.push_back(N_("Strict Balance"));
		balanceTypes.push_back(N_("1x Pan"));
		balanceTypes.push_back(N_("2x Pan"));
		addComboTextEntry(p0,N_("Balance Type"),balanceTypes,CBalanceAction::getBalanceTypeExplaination());
}

bool CCurvedBalanceActionDialog::show(CActionSound *actionSound,CActionParameters *actionParameters)
{
	// exact same implementation as CSimpleBalanceActionDialog::show()

	if(actionSound->sound->getChannelCount()<2)
		return false;
	else
	{
		if(getComboText("Channel A")->getItems().size()!=actionSound->sound->getChannelCount()) // don't alter selected channels if they didn't change in number
		{
			vector<string> items;
			for(size_t t=0;t<actionSound->sound->getChannelCount();t++)
				items.push_back(N_("Channel ")+istring(t));

			// set the combo boxes according to actionSound
			getComboText("Channel A")->setItems(items);
			getComboText("Channel B")->setItems(items);
			getComboText("Channel A")->setCurrentItem(0);
			getComboText("Channel B")->setCurrentItem(1);
		}

		return CActionParamDialog::show(actionSound,actionParameters);
	}
}





// --- monoize -----------------------------

static const double interpretValue_monoize(const double x,const int s) { return x*100.0; }
static const double uninterpretValue_monoize(const double x,const int s) { return x/100.0; }
static const double retconv_monoize(const double x) { return x/100.0 ; }

CMonoizeActionDialog::CMonoizeActionDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,N_("Monoize"),true,FXModalDialogBox::stShrinkWrap)
{
		void *p0=newVertPanel(NULL);
			void *p1=newHorzPanel(p0,false);
			for(unsigned t=0;t<MAX_CHANNELS;t++)
				addSlider(p1,N_("Channel ")+istring(t),"%",interpretValue_monoize,uninterpretValue_monoize,retconv_monoize,100.0,0,0,0,false);
			
			vector<string> options;
			options.push_back(N_("Remove All But One Channel"));
			options.push_back(N_("Make All Channels The Same"));
			addComboTextEntry(p0,N_("Method"),options);
}

bool CMonoizeActionDialog::show(CActionSound *actionSound,CActionParameters *actionParameters)
{
	if(actionSound->sound->getChannelCount()<1)
		return false;
	else
	{
		for(unsigned t=0;t<MAX_CHANNELS;t++)
			showControl("Channel "+istring(t), t<actionSound->sound->getChannelCount() );

		return CActionParamDialog::show(actionSound,actionParameters);
	}
}


// --- noise gate --------------------------

static const double interpretValue_noiseGate(const double x,const int s) { return(x*s); }
static const double uninterpretValue_noiseGate(const double x,const int s) { return(x/s); }

CNoiseGateDialog::CNoiseGateDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,N_("Noise Gate"))
{
	void *p=newHorzPanel(NULL);
		addSlider(p,N_("Window Time"),"ms",interpretValue_noiseGate,uninterpretValue_noiseGate,NULL,35.0,5,1000,30,false);
		addSlider(p,N_("Threshold"),"%",interpretValue_noiseGate,uninterpretValue_noiseGate,NULL,3.0,5,100,20,false);
		addSlider(p,N_("Gain Attack Time"),"ms",interpretValue_noiseGate,uninterpretValue_noiseGate,NULL,10.0,5,1000,30,false);
		addSlider(p,N_("Gain Release Time"),"ms",interpretValue_noiseGate,uninterpretValue_noiseGate,NULL,10.0,5,1000,30,false);
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
	CActionParamDialog(mainWindow,N_("Compressor"))
{
	void *p0=newVertPanel(NULL,true);
		void *p1=newHorzPanel(p0,false);
			addSlider(p1,N_("Window Time"),"ms",interpretValue_compressorWindowTime,uninterpretValue_compressorWindowTime,NULL,35.0,1,10,1,false);
			addSlider(p1,N_("Threshold"),"dBFS",interpretValue_dBFS,uninterpretValue_dBFS,NULL,-12.0,0,0,1,false);
			addSlider(p1,N_("Ratio"),":1",interpretValue_compressionRatio,uninterpretValue_compressionRatio,NULL,2.0,2,20,6,false);
			addSlider(p1,N_("Attack Time"),"ms",interpretValue_compressAttack,uninterpretValue_compressAttack,NULL,10.0,0,0,0,false);
			addSlider(p1,N_("Release Time"),"ms",interpretValue_compressRelease,uninterpretValue_compressRelease,NULL,50.0,0,0,0,false);
			addSlider(p1,N_("Input Gain"),"x",interpretValue_compressGain,uninterpretValue_compressGain,NULL,1.0,0,0,0,true);
			addSlider(p1,N_("Output Gain"),"x",interpretValue_compressGain,uninterpretValue_compressGain,NULL,1.0,0,0,0,true);
		p1=newVertPanel(p0,false);
			addCheckBoxEntry(p1,N_("Lock Channels"),true,_("This Toggles That Compression Should be Applied to Both Channels when Either Channel Needs Compression"));
}

// ??? remove this before 1.0 and make sure it is up-to-standard as far as a compressor goes before 1.0
const string CCompressorDialog::getExplaination() const
{
	return "This is my first attempt at creating a compressor algorithm.  I'm not sure how it fairs with other 'professional' tools.  I really have little experience with hardware compressors myself.  If you are an experienced compressor user and can make intelligent suggestions about how to make this one better, then please contact me.  Contact information is in the about dialog under the file menu";
}


// --- normalize ---------------------------

static const double interpretValue_regionCount(const double x,const int s) { return(floor(x*99.0)+1.0); }
static const double uninterpretValue_regionCount(const double x,const int s) { return((x-1.0)/99.0); }

CNormalizeDialog::CNormalizeDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,N_("Normalize"))
{
	void *p1=newVertPanel(NULL);
		void *p2=newHorzPanel(p1,false);
			addSlider(p2,N_("Normalization Level"),"dBFS",interpretValue_dBFS,uninterpretValue_dBFS,NULL,-0.5,0,0,1,false);
			addSlider(p2,N_("Region Count"),"",interpretValue_regionCount,uninterpretValue_regionCount,NULL,1,0,0,0,false);
		p2=newVertPanel(p1,false);
			addCheckBoxEntry(p2,N_("Lock Channels"),true,"Calculate Maximum Sample Value in a Region Across All Channels");
}


// --- resample ----------------------------

CResampleDialog::CResampleDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,N_("Resample"),false)
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
		addComboTextEntry(p0,N_("New Sample Rate"),items,"",true);
			getComboText("New Sample Rate")->setValue(44100);
}

