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

#include "ActionParamMappers.h"


// --- volume ---------------------------------

CNormalVolumeChangeDialog::CNormalVolumeChangeDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addSlider(p,
			N_("Volume Change"),
			"dB",
			new CActionParamMapper_linear_bipolar(3.0,3,100),
			dB_to_scalar,
			true
		);
}



CNormalGainDialog::CNormalGainDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addSlider(p,
			N_("Gain"),
			"x",
			new CActionParamMapper_recipsym(2.0,2,2,100),
			NULL,
			true
		);

		// ??? add a check box for negation of gain
}

CAdvancedGainDialog::CAdvancedGainDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addGraphWithWaveform(p,
			N_("Gain Curve"),
			N_("Gain"),
			"x",
			new CActionParamMapper_linear(1.0,2,-10,10),
			NULL
		);
}



// --- rate -----------------------------------

CNormalRateChangeDialog::CNormalRateChangeDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addSlider(p,
			N_("Rate Change"),
			"x",
			new CActionParamMapper_recipsym(2.0,2,1,100),
			NULL,
			true
		);
}

CAdvancedRateChangeDialog::CAdvancedRateChangeDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addGraphWithWaveform(p,
			N_("Rate Curve"),
			N_("Rate"),
			"x",
			new CActionParamMapper_linear(1.0,2,1,100),
			NULL
		);
}





// --- flange ------------------------------

CFlangeDialog::CFlangeDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addSlider(p,
			N_("Delay"),
			"ms",
			new CActionParamMapper_linear(2.0,2,2,100),
			NULL,
			false
		);

		addSlider(p,
			N_("Wet Gain"),
			"x",
			new CActionParamMapper_linear(1.0,2,-20,20),
			NULL,
			false
		);

		addSlider(p,
			N_("Dry Gain"),
			"x",
			new CActionParamMapper_linear(1.0,2,-20,10),
			NULL,
			false
		);

		addLFO(p,
			N_("Flange LFO"),
			"ms",
			N_("LFO Depth"),
			20,
			"Hz",
			20,
			true
		);

		addSlider(p,
			N_("Feedback"),
			"x",
			new CActionParamMapper_linear_range(0.0,-0.95,0.95),
			NULL,
			true
		);
}





// --- delay -------------------------------

CSimpleDelayDialog::CSimpleDelayDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addSlider(p,
			N_("Delay"),
			"ms",
			new CActionParamMapper_linear(250.0,1000,1,10000),
			NULL,
			false
		);

		addSlider(p,
			N_("Gain"),
			"x",
			new CActionParamMapper_linear(0.75,2,-20,20),
			NULL,
			false
		);

		addSlider(p,
			N_("Feedback"),
			"x",
			new CActionParamMapper_linear(0.35,1,-5,5),
			NULL,
			false
		);
}





// --- quantize ----------------------------

CQuantizeDialog::CQuantizeDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addSlider(p,
			N_("Input Gain"),
			"x",
			new CActionParamMapper_recipsym(1.0,2,2,100),
			NULL,
			true
		);

		CActionParamMapper_linear_min_to_scalar *m=new CActionParamMapper_linear_min_to_scalar(8.0,1.0,32,4,1000000);
		m->floorTheValue(true);
		addSlider(p,
			N_("Quantum Count"),
			"",
			m,
			NULL,
			false
		);
			setTipText("Quantum Count",_("The Number of Possible Sample Values Above Zero"));

		addSlider(p,
			N_("Output Gain"),
			"x",
			new CActionParamMapper_recipsym(1.0,2,2,100),
			NULL,
			true
		);

}





// --- distortion --------------------------

CDistortionDialog::CDistortionDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addGraph(p,
			N_("Curve"),
			N_("Input"),
			"dBFS",
			new CActionParamMapper_dBFS(-6.0),
			N_("Output"),
			"dBFS",
			new CActionParamMapper_dBFS(-6.0),
			NULL
		);
}





// --- varied repeat -----------------------

CVariedRepeatDialog::CVariedRepeatDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addLFO(p,
			N_("LFO"),
			"",
			"", // no amp units (so it won't show
			0,
			"Hz",
			200,
			true
		);

		addSlider(p,
			N_("Time"),
			"s",
			new CActionParamMapper_linear(4.0,10,1,100),
			NULL,
			false
		);

		// ??? need a checkbox that enables a gain parameter to make it mix ontop of the current audio instead of inserting into it
}

