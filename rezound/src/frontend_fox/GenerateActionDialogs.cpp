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

// --- Generate Noise -------------------------

	// x is 0 to 1 and should return the range of the units of this function's intented parameter
static const double interpretValue_length(const double x,const int scalar) { return x*scalar; }
	// inverse of interpretValue_length
static const double uninterpretValue_length(const double x,const int scalar) { return x/scalar; }

static const double interpretValue_volume(const double x,const int scalar) { return scalar_to_dB(x); }
static const double uninterpretValue_volume(const double x,const int scalar) { return dB_to_scalar(x); }

CGenerateNoiseDialog::CGenerateNoiseDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,N_("Generate Noise"))
{
	vector<string> items;

	void *p0=newVertPanel(NULL);
		void *p1=newHorzPanel(p0);

		addSlider(p1,N_("Length"),"s",interpretValue_length,uninterpretValue_length,NULL,1.0,1,10000,1,false);

		addSlider(p1,N_("Volume"),"dBFS",interpretValue_volume,uninterpretValue_volume,dB_to_scalar,-6.0,0,0,0,false);

	// these need to follow the order in the enum in CGenerateNoiseAction.cpp
	items.clear();
	items.push_back("White (Equal Energy per Frequency)");
	items.push_back("Pink (Natural, Equal Energy per Octave)");
	//items.push_back("Brown");
	//items.push_back("Green");
	//items.push_back("Blue");
	//items.push_back("Violet");
	//items.push_back("Binary");
	//items.push_back("Black");
	addComboTextEntry(p0,"Noise Color",items);

	// these need to follow the order in the enum in CGenerateNoiseAction.cpp
	items.clear();
	items.push_back("Independent Channels");
	items.push_back("Mono");
	items.push_back("Inverse Mono");
	//items.push_back("Spatial stereo");
	addComboTextEntry(p0,"Stereo Image",items);
		
}

