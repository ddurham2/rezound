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

#include "CCrossfadeEdgesDialog.h"

#include "settings.h"

#include "../backend/CActionParameters.h"

CCrossfadeEdgesDialog *gCrossfadeEdgesDialog=NULL;


CCrossfadeEdgesDialog::CCrossfadeEdgesDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,N_("Crossfade Edges Settings"))
{
	setMargin(130);

	void *p=newVertPanel(NULL);
		addNumericTextEntry(p,N_("Crossfade Start Edge"),"ms",gCrossfadeStartTime,0,10000,_("milliseconds"));
		addNumericTextEntry(p,N_("Crossfade Stop Edge"),"ms",gCrossfadeStopTime,0,10000,_("milliseconds"));
		
		vector<string> fadeMethods;
			fadeMethods.push_back(N_("Linear Fade"));
			fadeMethods.push_back(N_("Parabolic Fade")); // a more industry standard term for this??? it means a gain x^2 where 0<=x<=1
		addComboTextEntry(p,N_("Crossfade Fade Method"),fadeMethods,_("Linear will maintain a constant gain of 1.0\nParabolic sounds a bit more natural\nFor very quick fades (less than 250ms) linear is recommended"));

}

CCrossfadeEdgesDialog::~CCrossfadeEdgesDialog()
{
}

void CCrossfadeEdgesDialog::showIt()
{
	CActionParameters actionParameters(NULL);
	setValue(0,gCrossfadeStartTime);
	setValue(1,gCrossfadeStopTime);
	setValue(2,gCrossfadeFadeMethod);
	if(CActionParamDialog::show(NULL,&actionParameters))
	{
		gCrossfadeStartTime=actionParameters.getDoubleParameter("Crossfade Start Edge");
		gCrossfadeStopTime=actionParameters.getDoubleParameter("Crossfade Stop Edge");
		gCrossfadeFadeMethod=(CrossfadeFadeMethods)actionParameters.getUnsignedParameter("Crossfade Fade Method");
	}
}

