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

CCrossfadeEdgesDialog *gCrossfadeEdgesDialog=NULL;


CCrossfadeEdgesDialog::CCrossfadeEdgesDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Crossfade Edges Settings",310,150)
{

	/*
	getFrame()->setVSpacing(1);
	getFrame()->setHSpacing(1);
	FXPacker *p;

	p=new FXHorizontalFrame(getFrame(),LAYOUT_CENTER_X|LAYOUT_CENTER_Y);
		new FXLabel(p,"Crossfade Start Edge for ",NULL);
		//new FXTextField(p,5);
		startSpinner=new FXSpinner(new FXHorizontalFrame(p,FRAME_THICK|FRAME_SUNKEN,0,0,0,0, 0,0,0,0, 0,0),5,this,ID_START_SPINNER,SPIN_NORMAL);
			startSpinner->setRange(0,1000);
		new FXLabel(p,"ms\tmilliseconds",NULL);

	p=new FXHorizontalFrame(getFrame(),LAYOUT_CENTER_X|LAYOUT_CENTER_Y);
		new FXLabel(p,"Crossfade  Stop Edge for ",NULL);
		//new FXTextField(p,5);
		stopSpinner=new FXSpinner(new FXHorizontalFrame(p,FRAME_THICK|FRAME_SUNKEN,0,0,0,0, 0,0,0,0, 0,0),5,this,ID_START_SPINNER,SPIN_NORMAL);
			stopSpinner->setRange(0,1000);
		new FXLabel(p,"ms\tmilliseconds",NULL);
	*/

	/*
	startParam=new FXConstantParamValue(getFrame(),LAYOUT_CENTER_X|LAYOUT_CENTER_Y,"Crossfade Start Edge");
	startParam->setUnits("ms","milliseconds");

	stopParam=new FXConstantParamValue(getFrame(),LAYOUT_CENTER_X|LAYOUT_CENTER_Y,"Crossfade  Stop Edge");
	stopParam->setUnits("ms","milliseconds");
	*/
	
	addTextEntry("Crossfade Start Edge","ms",gCrossfadeStartTime,0,10000,"milliseconds");
	addTextEntry("Crossfade Stop Edge","ms",gCrossfadeStopTime,0,10000,"milliseconds");

}

void CCrossfadeEdgesDialog::showIt()
{
	/*
	startSpinner->setValue((FXint)gCrossfadeStartTime);
	stopSpinner->setValue((FXint)gCrossfadeStopTime);
	*/

	CActionParameters actionParameters;
	setValue(0,gCrossfadeStartTime);
	setValue(1,gCrossfadeStopTime);
	if(CActionParamDialog::show(NULL,&actionParameters))
	{
		gCrossfadeStartTime=actionParameters.getDoubleParameter(0);
		gCrossfadeStopTime=actionParameters.getDoubleParameter(1);
	}
}

/*
bool CCrossfadeEdgesDialog::validateOnOkay()
{

	return(true);
}
*/

