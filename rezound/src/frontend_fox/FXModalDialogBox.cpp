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

#include "FXModalDialogBox.h"

#include "../images/images.h"

#include "rememberShow.h"

FXDEFMAP(FXModalDialogBox) FXModalDialogBoxMap[]=
{
//	Message_Type			ID					Message_Handler
	FXMAPFUNC(SEL_COMMAND,		FXModalDialogBox::ID_OKAY_BUTTON,	FXModalDialogBox::onOkayButton),
};
		

FXIMPLEMENT(FXModalDialogBox,FXDialogBox,FXModalDialogBoxMap,ARRAYNUMBER(FXModalDialogBoxMap))



// ----------------------------------------

FXModalDialogBox::FXModalDialogBox(FXWindow *owner,const FXString &title,int w,int h,FrameTypes frameType) :
	FXDialogBox(owner,title,DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE, 0,0,w,h, 0,0,0,0, 0,0),

	contents(new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 0,0)),
		upperFrame(
			frameType==ftHorizontal 
				? 
				(FXPacker *)new FXHorizontalFrame(contents,LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 0,0)
				:
				(FXPacker *)new FXVerticalFrame(contents,LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 0,0)
			),
		lowerFrame(new FXHorizontalFrame(contents,FRAME_RAISED|FRAME_THICK | LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT, 0,0,0,65)),
			buttonPacker(new FXHorizontalFrame(lowerFrame,LAYOUT_CENTER_X|LAYOUT_CENTER_Y, 0,0,0,0, 0,0,0,0, 12)),
				okayButton(new FXButton(buttonPacker,"&Okay",new FXGIFIcon(getApp(),GreenCheck1),this,ID_OKAY_BUTTON,FRAME_RAISED|FRAME_THICK | JUSTIFY_NORMAL | ICON_ABOVE_TEXT | LAYOUT_FIX_WIDTH, 0,0,60,0, 2,2,2,2)),
				cancelButton(new FXButton(buttonPacker,"&Cancel",new FXGIFIcon(getApp(),RedX1),this,ID_CANCEL,FRAME_RAISED|FRAME_THICK | JUSTIFY_NORMAL | ICON_ABOVE_TEXT | LAYOUT_FIX_WIDTH, 0,0,60,0, 2,2,2,2))
{
	getFrame()->setPadLeft(5); getFrame()->setPadRight(5); getFrame()->setPadTop(5); getFrame()->setPadBottom(5);
	getFrame()->setHSpacing(DEFAULT_SPACING); getFrame()->setVSpacing(DEFAULT_SPACING);
	getFrame()->setFrameStyle(getFrame()->getFrameStyle()|FRAME_RAISED|FRAME_THICK);

		// ??? this doesn't seem to be having any effect... ask mailing list
	okayButton->setDefault(TRUE);

	// keep the dialog from getting too narrow
	//ASSURE_WIDTH(contents,160);
}


void FXModalDialogBox::disableFrameDecor()
{
	getFrame()->setPadLeft(0); getFrame()->setPadRight(0); getFrame()->setPadTop(0); getFrame()->setPadBottom(0);
	getFrame()->setHSpacing(0); getFrame()->setVSpacing(0);
	getFrame()->setFrameStyle(getFrame()->getFrameStyle()& ~FRAME_RAISED & ~FRAME_THICK);
}

long FXModalDialogBox::onOkayButton(FXObject *sender,FXSelector sel,void *ptr)
{
	if(validateOnOkay())
		FXDialogBox::handle(this,MKUINT(ID_ACCEPT,SEL_COMMAND),ptr);

	return(1);
}

void FXModalDialogBox::show(FXuint placement)
{
	if(!rememberShow(this))
		FXDialogBox::show(placement);
	else
		FXDialogBox::show(placement/*wouldn't send placement except remember show can't reliable set the window position*/);
}

void FXModalDialogBox::hide()
{
	rememberHide(this);
	FXDialogBox::hide();
}

FXPacker *FXModalDialogBox::getFrame()
{
	return(upperFrame);
}

