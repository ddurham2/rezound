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

//#include "CFOXIcons.h"

FXDEFMAP(CProgressDialog) CProgressDialogMap[]=
{
//	Message_Type			ID					Message_Handler
	FXMAPFUNC(SEL_CLOSE,		0,					CProgressDialog::onCloseWindow),
	FXMAPFUNC(SEL_COMMAND,		CProgressDialog::ID_CANCEL_BUTTON,	CProgressDialog::onCancelButton),
};
		

FXIMPLEMENT(CProgressDialog,FXDialogBox,CProgressDialogMap,ARRAYNUMBER(CProgressDialogMap))



// ----------------------------------------

#define ASSURE_WIDTH(parent,width) new FXFrame(parent,FRAME_NONE|LAYOUT_SIDE_BOTTOM | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT,0,0,width,1);

CProgressDialog::CProgressDialog(FXWindow *owner,const FXString &title,bool showCancelButton) :
	FXDialogBox(owner,title,DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE, 0,0,0,0, 0,0,0,0, 0,0),
	isCancelled(false),
	vContents(new FXVerticalFrame(this,FRAME_RAISED|FRAME_THICK | LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 6,6,6,6, 0,2)),
		hContents(new FXHorizontalFrame(vContents,LAYOUT_FILL_X | FRAME_NONE, 0,0,0,0, 0,0,0,0, 6,0)),
			progressBar(new FXProgressBar(hContents,NULL,0,PROGRESSBAR_NORMAL | LAYOUT_FILL_X,0,0,250,20)),
			cancelButton(!showCancelButton ? NULL : new FXButton(hContents,_("&Cancel"),/*FOXIcons->RedX1*/NULL,this,ID_CANCEL_BUTTON,LAYOUT_CENTER_Y | FRAME_RAISED|FRAME_THICK | JUSTIFY_NORMAL | ICON_ABOVE_TEXT, 0,0,0,0, 10,10,2,2)),
		timeElapsedLabel(new FXLabel(vContents,"Elapsed Time xxx:xx:xx",NULL,LAYOUT_FILL_X|JUSTIFY_RIGHT)),
		timeRemainingLabel(new FXLabel(vContents,"Estimated Time Remaining xxx:xx:xx",NULL,LAYOUT_FILL_X|JUSTIFY_RIGHT))
{
	ASSURE_WIDTH(this,260)
	progressBar->setTotal(100);
	progressBar->showNumber();
}

CProgressDialog::~CProgressDialog()
{
}

long CProgressDialog::onCancelButton(FXObject *sender,FXSelector sel,void *ptr)
{
	isCancelled=true;
	return 1;
}

long CProgressDialog::onCloseWindow(FXObject *sender,FXSelector sel,void *ptr)
{
	// don't close
	return 1;
}

void CProgressDialog::setProgress(int progress,const string timeElapsed,const string timeRemaining)
{
	progressBar->setProgress(progress);
	if(timeElapsed=="")
	{
		if(timeElapsedLabel->shown())
			timeElapsedLabel->hide();
	}
	else
	{
		if(!timeElapsedLabel->shown())
			timeElapsedLabel->show();
		timeElapsedLabel->setText((_("Time Elapsed ")+timeElapsed).c_str());
	}

	if(timeRemaining=="")
	{
		if(timeRemainingLabel->shown())
			timeRemainingLabel->hide();
	}
	else
	{
		if(!timeRemainingLabel->shown())
			timeRemainingLabel->show();
		timeRemainingLabel->setText((_("Estimated Time Remaining ")+timeRemaining).c_str());
	}

	if(cancelButton)
		getApp()->runModalWhileEvents(cancelButton); // give cancel button an oppertunity to be clicked
}

void CProgressDialog::show(FXuint placement)
{
	FXDialogBox::show(placement);
	resize(vContents->getDefaultWidth(),vContents->getDefaultHeight());
}

void CProgressDialog::hide()
{
	FXDialogBox::hide();
}

