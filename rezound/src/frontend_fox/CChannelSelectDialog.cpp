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

#include "CChannelSelectDialog.h"

#include <istring>

#include "../backend/CSound.h"


CChannelSelectDialog *gChannelSelectDialog=NULL;


FXDEFMAP(CChannelSelectDialog) CChannelSelectDialogMap[]=
{
//	Message_Type			ID					Message_Handler
	//FXMAPFUNC(SEL_COMMAND,		CChannelSelectDialog::ID_OKAY_BUTTON,	CChannelSelectDialog::onOkayButton),
};
		

FXIMPLEMENT(CChannelSelectDialog,FXModalDialogBox,CChannelSelectDialogMap,ARRAYNUMBER(CChannelSelectDialogMap))



// ----------------------------------------

CChannelSelectDialog::CChannelSelectDialog(FXWindow *mainWindow) :
	FXModalDialogBox(mainWindow,"Channel Select",310,270,FXModalDialogBox::ftVertical),

	label(new FXLabel(getFrame(),"Channels to Which This Action Should Apply:",NULL,LAYOUT_CENTER_X))
{
	getFrame()->setVSpacing(1);
	getFrame()->setHSpacing(1);

	for(unsigned t=0;t<MAX_CHANNELS;t++)			    // ??? could map it to some name like "Left, Right, Center, Bass... etc"
		checkBoxes[t]=new FXCheckButton(getFrame(),("Channel "+istring(t+1)).c_str(),NULL,0,CHECKBUTTON_NORMAL | LAYOUT_CENTER_X);
}

bool CChannelSelectDialog::show(CActionSound *actionSound,CActionParameters *actionParameters)
{
	// don't show the dialog if there is only one channel
	if(actionSound->sound->getChannelCount()<=1)
	{
		actionSound->doChannel[0]=true;
		return(true);
	}
	
	// uncheck all check boxes
	// only enable the check boxes that there are channels for
	for(unsigned t=0;t<MAX_CHANNELS;t++)
	{
		checkBoxes[t]->setCheck(FALSE);
		if(t<actionSound->sound->getChannelCount())
			checkBoxes[t]->enable();
		else
			checkBoxes[t]->disable();
	}

	for(unsigned t=0;t<actionSound->sound->getChannelCount();t++)
		checkBoxes[t]->setCheck(actionSound->doChannel[t] ? TRUE : FALSE);

	if(execute(PLACEMENT_CURSOR))
	{
		bool ret=false; // or all the checks together... if it's false, then it's like hitting cancel
		for(unsigned t=0;t<MAX_CHANNELS;t++)
		{
			actionSound->doChannel[t]=checkBoxes[t]->getCheck()==TRUE ? true : false;
			ret|=actionSound->doChannel[t];
		}

		return(ret);
	}
	return(false);

}

