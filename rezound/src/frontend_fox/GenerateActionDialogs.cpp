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

#include "GenerateActionDialogs.h"
#include "../backend/unit_conv.h"

#include "ActionParamMappers.h"

// --- Generate Noise -------------------------

FXDEFMAP(CGenerateNoiseDialog) CGenerateNoiseDialogMap[]=
{
        //Message_Type                  ID                                                 Message_Handler
	FXMAPFUNC(SEL_CHANGED,          CGenerateNoiseDialog::ID_NOISE_COLOR_COMBOBOX,     CGenerateNoiseDialog::onNoiseColorChange),
};
		
FXIMPLEMENT(CGenerateNoiseDialog,CActionParamDialog,CGenerateNoiseDialogMap,ARRAYNUMBER(CGenerateNoiseDialogMap)) 

CGenerateNoiseDialog::CGenerateNoiseDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	vector<string> items;

	void *p0=newVertPanel(NULL);
		void *p1=newHorzPanel(p0);

		addSlider(p1,
			N_("Length"),
			"s",
			new CActionParamMapper_linear(1.0,1,1,10000),
			NULL,
			false
		);

		addSlider(p1,
			N_("Volume"),
			"dBFS",
			new CActionParamMapper_dBFS(-6.0),
			dB_to_scalar,
			false
		);

		addSlider(p1,
			N_("Max Particle Velocity"),
			"%",
			new CActionParamMapper_linear(50.0,100),
			NULL,
			false
		);

	// these need to follow the order in the enum in CGenerateNoiseAction.cpp
	items.clear();
	items.push_back(_("White (Equal Energy per Frequency)"));
	items.push_back(_("Pink (Natural, Equal Energy per Octave; response: f^(-1))"));
	items.push_back(_("Brown (as in Brownian Motion; response: f^(-2))"));
	items.push_back(_("Black :)"));
	//items.push_back("Green");
	//items.push_back("Blue");
	//items.push_back("Violet");
	//items.push_back("Binary");
	FXComboTextParamValue *noiseColorComboBox=addComboTextEntry(p0,
		N_("Noise Color"),
		items,
		CActionParamDialog::cpvtAsInteger
	);
		noiseColorComboBox->setTarget(this);
		noiseColorComboBox->setSelector(ID_NOISE_COLOR_COMBOBOX);

	onNoiseColorChange(noiseColorComboBox,0,NULL);
	

	// these need to follow the order in the enum in CGenerateNoiseAction.cpp
	items.clear();
	items.push_back(_("Independent Channels"));
	items.push_back(_("Mono"));
	items.push_back(_("Inverse Mono"));
	//items.push_back("Spatial stereo");
	addComboTextEntry(p0,
		N_("Stereo Image"),
		items,
		CActionParamDialog::cpvtAsInteger
	);
		
}

long CGenerateNoiseDialog::onNoiseColorChange(FXObject *sender,FXSelector sel,void *ptr)
{
	if(((FXComboTextParamValue *)sender)->getIntegerValue()==2)
		getSliderParam("Max Particle Velocity")->enable();
	else
		getSliderParam("Max Particle Velocity")->disable();
	return 1;
}


// --- Generate Tone -------------------------

FXDEFMAP(CGenerateToneDialog) CGenerateToneDialogMap[]=
{
        //Message_Type                  ID                                                 Message_Handler
	//FXMAPFUNC(SEL_CHANGED,          CGenerateToneDialog::ID_NOISE_COLOR_COMBOBOX,     CGenerateToneDialog::onToneColorChange),
};
		
FXIMPLEMENT(CGenerateToneDialog,CActionParamDialog,CGenerateToneDialogMap,ARRAYNUMBER(CGenerateToneDialogMap)) 

CGenerateToneDialog::CGenerateToneDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
#warning make some presets
	void *p0=newVertPanel(NULL);
		void *p1=newHorzPanel(p0);
			addSlider(p1,
				N_("Frequency"),
				"Hz",
				new CActionParamMapper_linear(60.0,440,0,48000),
				NULL,
				false
			);

			addSlider(p1,
				N_("Length"),
				"s",
				new CActionParamMapper_linear(1.0,1,1,10000),
				NULL,
				false
			);

			addSlider(p1,
				N_("Volume"),
				"dBFS",
				new CActionParamMapper_dBFS(-6.0),
				dB_to_scalar,
				false
			);

		vector<string> toneTypes;
			// these must match the order that they're defined in CGenerateToneAction::ToneTypes
		toneTypes.push_back(_("Sine Wave"));
		toneTypes.push_back(_("Square Wave"));
		toneTypes.push_back(_("Rising Sawtooth Wave"));
		toneTypes.push_back(_("Falling Sawtooth Wave"));
		toneTypes.push_back(_("Triangle Wave"));
		addComboTextEntry(p0,
			N_("Tone Type"),
			toneTypes,
			CActionParamDialog::cpvtAsInteger
		); 
}

