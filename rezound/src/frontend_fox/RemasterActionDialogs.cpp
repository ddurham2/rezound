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

#include <istring>

#include "CFOXIcons.h"

#include "../backend/CActionSound.h"
#include "../backend/CActionParameters.h"
#include "../backend/CSound.h"

#include "ActionParamMappers.h"



// --- balance ------------------------

#include "../backend/Remaster/CBalanceAction.h"
static const double ret_balance(const double x) { return x/100.0; }

CSimpleBalanceActionDialog::CSimpleBalanceActionDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p0=newVertPanel(NULL,true);
		void *p1=newHorzPanel(p0,false);
			void *p2=newVertPanel(p1,false);
				vector<string> items;
				addComboTextEntry(p2,
					N_("Channel A"),
					items,
					""
				);

				addComboTextEntry(p2,
					N_("Channel B"),
					items,
					""
				);

			
			CActionParamMapper_linear_bipolar *m=new CActionParamMapper_linear_bipolar(0.0,100,100);
			m->setMultiplier(-1.0);
			addSlider(p1,
				N_("Balance"),
				"%",
				m,
				ret_balance,
				false
			);

			vector<string> balanceTypes;
			balanceTypes.push_back(N_("Strict Balance"));
			balanceTypes.push_back(N_("1x Pan"));
			balanceTypes.push_back(N_("2x Pan"));
		addComboTextEntry(p0,
			N_("Balance Type"),
			balanceTypes,
			CBalanceAction::getBalanceTypeExplanation()
		);
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
	CActionParamDialog(mainWindow)
{
	void *p0=newVertPanel(NULL,true);
		void *p1=newHorzPanel(p0,false);
			void *p2=newVertPanel(p1,false);
				vector<string> items;
				addComboTextEntry(p2,
					N_("Channel A"),
					items,
					""
				);

				addComboTextEntry(p2,
					N_("Channel B"),
					items,
					""
				);

			CActionParamMapper_linear_bipolar *m=new CActionParamMapper_linear_bipolar(0.0,100,100);
			m->setMultiplier(-1.0);
			addGraphWithWaveform(p1,
				N_("Balance Curve"),
				N_("Balance"),
				"%",
				m,
				ret_balance
			);

		vector<string> balanceTypes;
		balanceTypes.push_back(N_("Strict Balance"));
		balanceTypes.push_back(N_("1x Pan"));
		balanceTypes.push_back(N_("2x Pan"));
		addComboTextEntry(p0,
			N_("Balance Type"),
			balanceTypes,
			CBalanceAction::getBalanceTypeExplanation()
		);
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

static const double retconv_monoize(const double x) { return x/100.0 ; }

CMonoizeActionDialog::CMonoizeActionDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,true,"",FXModalDialogBox::stShrinkWrap)
{
		void *p0=newVertPanel(NULL);
			void *p1=newHorzPanel(p0,false);
			for(unsigned t=0;t<MAX_CHANNELS;t++)
			{
				addSlider(p1,
					N_("Channel ")+istring(t),
					"%",
					new CActionParamMapper_linear(100.0,100),
					retconv_monoize,
					false
				);
			}
			
			vector<string> options;
			options.push_back(N_("Remove All But One Channel"));
			options.push_back(N_("Make All Channels The Same"));
			addComboTextEntry(p0,
				N_("Method"),
				options
			);
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

CNoiseGateDialog::CNoiseGateDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addSlider(p,
			N_("Window Time"),
			"ms",
			new CActionParamMapper_linear(35.0,30,5,1000),
			NULL,
			false
		);

		addSlider(p,
			N_("Threshold"),
			"dBFS",
			new CActionParamMapper_dBFS(-30.0),
			NULL,
			false
		);

		addSlider(p,
			N_("Gain Attack Time"),
			"ms",
			new CActionParamMapper_linear(10.0,30,5,1000),
			NULL,
			false
		);

		addSlider(p,
			N_("Gain Release Time"),
			"ms",
			new CActionParamMapper_linear(10.0,30,5,1000),
			NULL,
			false
		);
}

#include "../backend/Remaster/CNoiseGateAction.h"
const string CNoiseGateDialog::getExplanation() const
{
	return CNoiseGateAction::getExplanation();
}



// --- compressor --------------------------

CCompressorDialog::CCompressorDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p0=newVertPanel(NULL,true);
		void *p1=newHorzPanel(p0,false);
			addSlider(p1,
				N_("Window Time"),
				"ms",
				new CActionParamMapper_linear_range_scaled(35.0,.1,100,1,1,10),
				NULL,
				false
			);

			addSlider(p1,
				N_("Threshold"),
				"dBFS",
				new CActionParamMapper_dBFS(-12.0),
				NULL,
				false
			);

			addSlider(p1,
				N_("Ratio"),
				":1",
				new CActionParamMapper_linear_min_to_scalar(2.0,1,6,2,20),
				NULL,
				false
			);

			addSlider(p1,
				N_("Attack Time"),
				"ms",
				new CActionParamMapper_parabola_range(10.0,0.1,100.0),
				NULL,
				false
			);

			addSlider(p1,
				N_("Release Time"),
				"ms",
				new CActionParamMapper_parabola_range(50.0,1.0,1000.0),
				NULL,
				false
			);

			addSlider(p1,
				N_("Input Gain"),
				"x",
				new CActionParamMapper_recipsym(1.0,10,0,0),
				NULL,
				true
			);

			addSlider(p1,
				N_("Output Gain"),
				"x",
				new CActionParamMapper_recipsym(1.0,10,0,0),
				NULL,
				true
			);

		p1=newVertPanel(p0,false);
			addCheckBoxEntry(p1,
				N_("Lock Channels"),
				true,
				_("This Toggles That Compression Should be Applied to Both Channels when Either Channel Needs Compression")
			);
}

// ??? remove this before 1.0 and make sure it is up-to-standard as far as a compressor goes before 1.0
#include "../backend/Remaster/CCompressorAction.h"
const string CCompressorDialog::getExplanation() const
{
	return CCompressorAction::getExplanation();
}


// --- normalize ---------------------------

CNormalizeDialog::CNormalizeDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p1=newVertPanel(NULL);
		void *p2=newHorzPanel(p1,false);
			addSlider(p2,
				N_("Normalization Level"),
				"dBFS",
				new CActionParamMapper_dBFS(-0.5),
				NULL,
				false
			);

			CActionParamMapper_linear_range *m=new CActionParamMapper_linear_range(1.0,1.0,100.0);
			m->floorTheValue(true);
			addSlider(p2,
				N_("Region Count"),
				"",
				m,
				NULL,
				false
			);

		p2=newVertPanel(p1,false);
			addCheckBoxEntry(p2,
				N_("Lock Channels"),
				true,
				_("Calculate Maximum Sample Value in a Region Across All Channels")
			);
}


// --- mark quiet areas --------------------

CMarkQuietAreasDialog::CMarkQuietAreasDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	FXConstantParamValue *t;
	void *p1=newVertPanel(NULL);
		void *p2=newHorzPanel(p1,false);
			t=addSlider(p2,
				N_("Threshold for Quiet"),
				"dBFS",
				new CActionParamMapper_dBFS(-48.0),
				NULL,
				false
			);
			t->setTipText(_("An audio level below this threshold is considered to be quiet."));

			t=addSlider(p2,
				N_("Must Remain Quiet for"),
				"ms",
				new CActionParamMapper_linear(500,1000,10,10000),
				NULL,
				false
			);
			t->setTipText(_("The audio level must remain below the threshold for this long before a beginning cue will be added."));

			t=addSlider(p2,
				N_("Must Remain Unquiet for"),
				"ms",
				new CActionParamMapper_linear(0,1000,10,10000),
				NULL,
				false
			);
			t->setTipText(_("After the beginning of a quiet area has been detected the audio level must rise above the threshold for this long for an ending cue to be added."));

			t=addSlider(p2,
				N_("Level Detector Window Time"),
				"ms",
				new CActionParamMapper_linear_range_scaled(35.0,.1,100,1,1,10),
				NULL,
				false
			);
			t->setTipText(_("This is the length of the window of audio to analyze for detecting the audio level."));

		p2=newVertPanel(p1,false);
			addStringTextEntry(p2,
				N_("Quiet Begin Cue Name"),
				"[",
				_("What to Name a Cue That Marks Beginning of a Quiet Region")
			);

			addStringTextEntry(p2,
				N_("Quiet End Cue Name"),
				"]",
				_("What to Name a Cue That Marks End of a Quiet Region")
			);
}


// --- resample ----------------------------

CResampleDialog::CResampleDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,false)
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
		addComboTextEntry(p0,
			N_("New Sample Rate"),
			items,
			"",
			true
		);
			getComboText("New Sample Rate")->setValue(44100);
}

