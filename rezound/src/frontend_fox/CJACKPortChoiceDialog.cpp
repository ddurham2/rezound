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

#include "CJACKPortChoiceDialog.h"

#include <stdlib.h>

FXDEFMAP(CJACKPortChoiceDialog) CJACKPortChoiceDialogMap[]=
{
//	Message_Type			ID					Message_Handler
	//FXMAPFUNC(SEL_COMMAND,	CJACKPortChoiceDialog::ID_WHICH_BUTTON,		CJACKPortChoiceDialog::onRadioButton),
};
		

FXIMPLEMENT(CJACKPortChoiceDialog,FXModalDialogBox,CJACKPortChoiceDialogMap,ARRAYNUMBER(CJACKPortChoiceDialogMap))



// ----------------------------------------


CJACKPortChoiceDialog::CJACKPortChoiceDialog(FXWindow *mainWindow) :
	FXModalDialogBox(mainWindow,_("JACK Port Selection"),0,0,FXModalDialogBox::ftVertical)
{
	messageLabel=new FXLabel(getFrame(),"",NULL,LAYOUT_CENTER_X);
	portNamesComboBox=new FXComboBox(getFrame(),10,8,NULL,0,COMBOBOX_NORMAL|LAYOUT_CENTER_X|FRAME_SUNKEN|FRAME_THICK|COMBOBOX_STATIC);
}

CJACKPortChoiceDialog::~CJACKPortChoiceDialog()
{
}

bool CJACKPortChoiceDialog::show(const string message,const vector<string> portNames)
{
	messageLabel->setText(message.c_str());
	portNamesComboBox->clearItems();
	for(size_t t=0;t<portNames.size();t++)
		portNamesComboBox->appendItem(portNames[t].c_str());

	if(execute(PLACEMENT_SCREEN))
		return true;
	return false;
}

const string CJACKPortChoiceDialog::getPortName() const
{
	return portNamesComboBox->getText().text();
}

