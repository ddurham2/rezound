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

#include "CUserNotesDialog.h"

#include "../backend/CLoadedSound.h"
#include "../backend/CSound.h"

CUserNotesDialog *gUserNotesDialog=NULL;


FXDEFMAP(CUserNotesDialog) CUserNotesDialogMap[]=
{
//	Message_Type			ID					Message_Handler
	//FXMAPFUNC(SEL_COMMAND,	CUserNotesDialog::ID_OKAY_BUTTON,	CUserNotesDialog::onOkayButton),
};
		

FXIMPLEMENT(CUserNotesDialog,FXModalDialogBox,CUserNotesDialogMap,ARRAYNUMBER(CUserNotesDialogMap))



// ----------------------------------------

CUserNotesDialog::CUserNotesDialog(FXWindow *mainWindow) :
	FXModalDialogBox(mainWindow,N_("User Notes"),450,400,FXModalDialogBox::ftVertical),

	notesFrame(new FXPacker(getFrame(),FRAME_SUNKEN|FRAME_THICK | LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 0,0)),
		notesTextBox(new FXText(notesFrame,NULL,0,TEXT_WORDWRAP | LAYOUT_FILL_X|LAYOUT_FILL_Y))
{
}

CUserNotesDialog::~CUserNotesDialog()
{
}

void CUserNotesDialog::show(CLoadedSound *loadedSound,FXint placement)
{
	notesTextBox->setText(loadedSound->sound->getUserNotes().c_str());
	if(execute(placement))
	{
		loadedSound->sound->setIsModified(true);
		loadedSound->sound->setUserNotes(notesTextBox->getText().text());
	}
}

