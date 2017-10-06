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

#include "../backend/ActionParamMappers.h"


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
					CActionParamDialog::cpvtAsInteger,
					""
				);

				addComboTextEntry(p2,
					N_("Channel B"),
					items,
					CActionParamDialog::cpvtAsInteger,
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
			CActionParamDialog::cpvtAsInteger,
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
				items.push_back(_("Channel ")+istring(t));

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
					CActionParamDialog::cpvtAsInteger,
					""
				);

				addComboTextEntry(p2,
					N_("Channel B"),
					items,
					CActionParamDialog::cpvtAsInteger,
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
			CActionParamDialog::cpvtAsInteger,
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
				items.push_back(_("Channel ")+istring(t));

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
				options,
				CActionParamDialog::cpvtAsInteger
			);
}

bool CMonoizeActionDialog::show(CActionSound *actionSound,CActionParameters *actionParameters)
{
	if(actionSound->sound->getChannelCount()<1)
		return false;
	else
	{
		for(unsigned t=0;t<MAX_CHANNELS;t++)
			showControl(N_("Channel ")+istring(t), t<actionSound->sound->getChannelCount() );

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

			addCheckBoxEntry(p1,
				N_("Look Ahead For Level"),
				true,
				_("This Toggles That the Compressor Should Look Ahead (by Half the Window Time Amount) in the Input Signal When Calculating the Level of the Input Signal")
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


// --- adaptive normalize ------------------

CAdaptiveNormalizeDialog::CAdaptiveNormalizeDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p1=newVertPanel(NULL);
		void *p2=newHorzPanel(p1,false);
			addSlider(p2,
				N_("Normalization Level"),
				"dBFS",
				new CActionParamMapper_dBFS(-20.0),
				NULL,
				false
			);

			addSlider(p2,
				N_("Window Time"),
				"ms",
				new CActionParamMapper_linear_range_scaled(2000.0,10,1000,1,1,100),
				NULL,
				false
			);

			addSlider(p2,
				N_("Maximum Gain"),
				"dB",
				new CActionParamMapper_linear(60.0,100,1,200),
				NULL,
				true
			);

		p2=newVertPanel(p1,false);
			addCheckBoxEntry(p2,
				N_("Lock Channels"),
				true,
				_("Calculate Signal Level Across All Channels Simultaneously")
			);
}

#include "../backend/Remaster/CAdaptiveNormalizeAction.h"
const string CAdaptiveNormalizeDialog::getExplanation() const
{
	return CAdaptiveNormalizeAction::getExplanation();
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


// --- shorten quiet areas --------------------

CShortenQuietAreasDialog::CShortenQuietAreasDialog(FXWindow *mainWindow) :
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

			t=addSlider(p2,
				N_("Shorten Found Area To"),
				"x",
				new CActionParamMapper_linear(0.5,1,1,1),
				NULL,
				false
			);
			t->setTipText(_("When a quiet area is found it will be shorted to this much of its original length."));
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
			CActionParamDialog::cpvtAsInteger,
			"",
			true
		);
			getComboText("New Sample Rate")->setIntegerValue(44100);
}


// --- change pitch ------------------------

CChangePitchDialog::CChangePitchDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,false)
{
	FXPacker *p0=newVertPanel(NULL);
	FXPacker *p1=new FXTabBook(p0,NULL,0,TABBOOK_NORMAL|LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_NONE, 0,0,0,0, 0,0,0,0);

		new FXTabItem((FXTabBook *)p1,_("Pitch"),NULL,TAB_TOP_NORMAL);
		FXPacker *p2=newHorzPanel(p1,true,true);
			addSlider(
				p2,
				N_("Semitones"),
				_("semitones"),
				new CActionParamMapper_linear_bipolar(1,7,60),
				NULL,
				true);

		FXConstantParamValue *s;

		new FXTabItem((FXTabBook *)p1,_("Tweaking"),NULL,TAB_TOP_NORMAL);
		p2=newVertPanel(p1,true,true);
			new FXLabel(p2,_("CAUTION: Some of combinations of these parameters cause the SoundTouch library to generate assertion errors \n that kill the current process.  It is not difficult to cause ReZound to exit by adjusting these parameters.  \n Play with these parameters with some testing data, and then save presets that do not cause problems.  \n Hopefully this issue will be worked out in the future.\nSee the SoundTouch header file for more information about these parameters."));
			FXPacker *p3=newHorzPanel(p2,false,false);
				addCheckBoxEntry(p3,
					N_("Use Anti-alias Filter"),
					true,
					_("Enable/disable anti-alias filter in pitch transposer")
				);

				CActionParamMapper_linear_range *aafl_m=new CActionParamMapper_linear_range(32,8,128);
				aafl_m->floorTheValue(true);
				addSlider(p3,
					N_("Anti-alias Filter Length"),
					"taps",
					aafl_m,
					NULL,
					false
				);

				addCheckBoxEntry(p3,
					N_("Use Quick Seek"),
					false,
					_("Enable/disable quick seeking algorithm in tempo changer routine (enabling quick seeking lowers CPU utilization but causes a minor sound quality compromising)")
				);
				
				CActionParamMapper_linear_range *sl_m=new CActionParamMapper_linear_range(82,1,500);
				sl_m->floorTheValue(true);
				s=addSlider(p3,
					N_("Sequence Length"),
					"ms",
					sl_m,
					NULL,
					false
				);
				s->setTipText(_("This is the default length of a single processing sequence, in milliseconds. Determines to how long sequences the original sound is chopped in time-stretch algorithm. The larger this value is, the lesser sequences are used in processing. In principle a bigger value sounds better when slowing down tempo, but worse when increasing tempo and vice versa."));

				CActionParamMapper_linear_range *swl_m=new CActionParamMapper_linear_range(28,1,500);
				swl_m->floorTheValue(true);
				s=addSlider(p3,
					N_("Seek Window Length"),
					"ms",
					swl_m,
					NULL,
					false
				);
				s->setTipText(_("Seeking window default length in milliseconds for algorithm that seeks for the best possible overlapping location. This determines from how wide sample 'window' the algorithm can look for an optimal mixing location when the sound sequences are to be linked back together.  The bigger this window setting is, the higher the possibility to find a better mixing position becomes, but at the same time large values may cause a 'drifting' sound artifact because neighbouring sequences can be chosen at more uneven intervals. If there's a disturbing artifact that sounds as if a constant frequency was drifting around, try reducing this setting."));

				CActionParamMapper_linear_range *ol_m=new CActionParamMapper_linear_range(28,1,500);
				ol_m->floorTheValue(true);
				s=addSlider(p3,
					N_("Overlap Length"),
					"ms",
					ol_m,
					NULL,
					false
				);
				s->setTipText(_("Overlap length in milliseconds. When the chopped sound sequences are mixed back together to form again a continuous sound stream, this parameter defines over how long period the two consecutive sequences are allowed to overlap each other.  This shouldn't be that critical of a parameter. If you reduce the 'Sequence Length' setting by a large amount, you might wish to try a smaller value on this."));
}


// --- change tempo ------------------------

CChangeTempoDialog::CChangeTempoDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,false)
{
	FXPacker *p0=newVertPanel(NULL);
	FXPacker *p1=new FXTabBook(p0,NULL,0,TABBOOK_NORMAL|LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_NONE, 0,0,0,0, 0,0,0,0);

		new FXTabItem((FXTabBook *)p1,_("Pitch"),NULL,TAB_TOP_NORMAL);
		FXPacker *p2=newHorzPanel(p1,true,true);

			addSlider(
				p2,
				N_("Tempo Change"),
				"x",
				new CActionParamMapper_recipsym(0.9,2,1,10),
				NULL,
				true);

		FXConstantParamValue *s;

		new FXTabItem((FXTabBook *)p1,_("Tweaking"),NULL,TAB_TOP_NORMAL);
		p2=newVertPanel(p1,true,true);
			new FXLabel(p2,_("CAUTION: Some of combinations of these parameters cause the SoundTouch library to generate assertion errors \n that kill the current process.  It is not difficult to cause ReZound to exit by adjusting these parameters.  \n Play with these parameters with some testing data, and then save presets that do not cause problems.  \n Hopefully this issue will be worked out in the future.\nSee the SoundTouch header file for more information about these parameters."));
			FXPacker *p3=newHorzPanel(p2,false,false);
				addCheckBoxEntry(p3,
					N_("Use Anti-alias Filter"),
					true,
					_("Enable/disable anti-alias filter in pitch transposer")
				);

				CActionParamMapper_linear_range *aafl_m=new CActionParamMapper_linear_range(32,8,128);
				aafl_m->floorTheValue(true);
				addSlider(p3,
					N_("Anti-alias Filter Length"),
					"taps",
					aafl_m,
					NULL,
					false
				);

				addCheckBoxEntry(p3,
					N_("Use Quick Seek"),
					false,
					_("Enable/disable quick seeking algorithm in tempo changer routine (enabling quick seeking lowers CPU utilization but causes a minor sound quality compromising)")
				);
				
				CActionParamMapper_linear_range *sl_m=new CActionParamMapper_linear_range(82,1,500);
				sl_m->floorTheValue(true);
				s=addSlider(p3,
					N_("Sequence Length"),
					"ms",
					sl_m,
					NULL,
					false
				);
				s->setTipText(_("This is the default length of a single processing sequence, in milliseconds. Determines to how long sequences the original sound is chopped in time-stretch algorithm. The larger this value is, the lesser sequences are used in processing. In principle a bigger value sounds better when slowing down tempo, but worse when increasing tempo and vice versa."));

				CActionParamMapper_linear_range *swl_m=new CActionParamMapper_linear_range(28,1,500);
				swl_m->floorTheValue(true);
				s=addSlider(p3,
					N_("Seek Window Length"),
					"ms",
					swl_m,
					NULL,
					false
				);
				s->setTipText(_("Seeking window default length in milliseconds for algorithm that seeks for the best possible overlapping location. This determines from how wide sample 'window' the algorithm can look for an optimal mixing location when the sound sequences are to be linked back together.  The bigger this window setting is, the higher the possibility to find a better mixing position becomes, but at the same time large values may cause a 'drifting' sound artifact because neighbouring sequences can be chosen at more uneven intervals. If there's a disturbing artifact that sounds as if a constant frequency was drifting around, try reducing this setting."));

				CActionParamMapper_linear_range *ol_m=new CActionParamMapper_linear_range(28,1,500);
				ol_m->floorTheValue(true);
				s=addSlider(p3,
					N_("Overlap Length"),
					"ms",
					ol_m,
					NULL,
					false
				);
				s->setTipText(_("Overlap length in milliseconds. When the chopped sound sequences are mixed back together to form again a continuous sound stream, this parameter defines over how long period the two consecutive sequences are allowed to overlap each other.  This shouldn't be that critical of a parameter. If you reduce the 'Sequence Length' setting by a large amount, you might wish to try a smaller value on this."));
}

