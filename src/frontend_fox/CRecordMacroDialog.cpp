/* 
 * Copyright (C) 2005 - David W. Durham
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

#include "CRecordMacroDialog.h"

#include "CFOXIcons.h"
#include <istring>

FXDEFMAP(CRecordMacroDialog) CRecordMacroDialogMap[]=
{
//	Message_Type			ID						Message_Handler
	//FXMAPFUNC(SEL_COMMAND,		CRecordMacroDialog::ID_START_BUTTON,			CRecordMacroDialog::onStartButton),
};
		

FXIMPLEMENT(CRecordMacroDialog,FXModalDialogBox,CRecordMacroDialogMap,ARRAYNUMBER(CRecordMacroDialogMap))



// ----------------------------------------

CRecordMacroDialog::CRecordMacroDialog(FXWindow *mainWindow) :
	FXModalDialogBox(mainWindow,N_("Record Macro"),350,100,FXModalDialogBox::ftVertical)
{
	//setIcon(FOXIcons->small_record_macro);

	getFrame()->setHSpacing(1);
	getFrame()->setVSpacing(1);

	FXPacker *frame1;

	frame1=new FXHorizontalFrame(getFrame(),LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 1,1);
		new FXLabel(frame1,_("Macro Name:"));
		macroNameEdit=new FXTextField(frame1,40,NULL,0,LAYOUT_FILL_X|FRAME_SUNKEN|FRAME_THICK);
}

CRecordMacroDialog::~CRecordMacroDialog()
{
}

bool CRecordMacroDialog::showIt()
{
	macroNameEdit->setText(macroName.c_str());

	macroNameEdit->setFocus();

	if(execute(PLACEMENT_CURSOR))
	{
		macroName=istring(macroNameEdit->getText().text()).trim();
		if(macroName!="")
			return true;
		else
			return false;
	}
	return false;
}

