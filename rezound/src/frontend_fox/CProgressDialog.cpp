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

#include "CProgressDialog.h"

//#include "images.h"

FXDEFMAP(CProgressDialog) CProgressDialogMap[]=
{
//	Message_Type			ID					Message_Handler
	//FXMAPFUNC(SEL_COMMAND,		CProgressDialog::ID_CANCEL_BUTTON,	CProgressDialog::onCancelButton),
};
		

FXIMPLEMENT(CProgressDialog,FXDialogBox,CProgressDialogMap,ARRAYNUMBER(CProgressDialogMap))



// ----------------------------------------

CProgressDialog::CProgressDialog(FXWindow *owner,const FXString &title/*,bool showCancelButton*/) :
	FXDialogBox(owner,title,DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE, 0,0,250+6*2,25+6*2, 0,0,0,0, 0,0),

	contents(new FXHorizontalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y | FRAME_RAISED|FRAME_THICK, 0,0,0,0, 6,6,6,6, 2,2)),
		progressBar(new FXProgressBar(contents,NULL,0,PROGRESSBAR_NORMAL | LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,250,25))//,
		//cancelButton(!showCancelButton ? NULL : new FXButton(buttonPacker,"&Cancel",new FXGIFIcon(getApp(),RedX1_gif),this,ID_CANCEL,FRAME_RAISED|FRAME_THICK | JUSTIFY_NORMAL | ICON_ABOVE_TEXT | LAYOUT_FIX_WIDTH, 0,0,60,0, 2,2,2,2))
{
	progressBar->setTotal(100);
	progressBar->showNumber();
}


/*
long CProgressDialog::onCancelButton(FXObject *sender,FXSelector sel,void *ptr)
{
	return(1);
}
*/

void CProgressDialog::setProgress(int progress)
{
	progressBar->setProgress(progress);
}

void CProgressDialog::show(FXuint placement)
{
	FXDialogBox::show(placement);
}

void CProgressDialog::hide()
{
	FXDialogBox::hide();
}

