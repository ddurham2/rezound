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
	CActionParamDialog(mainWindow,"Noise Gate",550,400)
{
	addSlider("Window Time","ms",interpretValue_noiseGate,uninterpretValue_noiseGate,NULL,35.0,5,1000,30,false);
	addSlider("Threshold","%",interpretValue_noiseGate,uninterpretValue_noiseGate,NULL,3.0,5,100,20,false);
	addSlider("Gain Attack Time","ms",interpretValue_noiseGate,uninterpretValue_noiseGate,NULL,10.0,5,1000,30,false);
	addSlider("Gain Release Time","ms",interpretValue_noiseGate,uninterpretValue_noiseGate,NULL,10.0,5,1000,30,false);
}

