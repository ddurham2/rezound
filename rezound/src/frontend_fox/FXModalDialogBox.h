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

#ifndef __FXModalDialogBox_H__
#define __FXModalDialogBox_H__

#include "../../config/common.h"
#include "fox_compat.h"

#include <fox/fx.h>

// used to place an invisible FXFrame in another widget to make sure it has a minimum height or width
#define ASSURE_HEIGHT(parent,height) new FXFrame(parent,FRAME_NONE|LAYOUT_SIDE_LEFT | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT,0,0,1,height);
#define ASSURE_WIDTH(parent,width) new FXFrame(parent,FRAME_NONE|LAYOUT_SIDE_BOTTOM | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT,0,0,width,1);

/*
 * This is meant to be a base class for a modal dialog
 *
 * It goes ahead and creates two panels with an okay and
 * cancel button in the lower panel... The derived class
 * should put whatever is needed in the upper panel
 */
class FXModalDialogBox : public FXDialogBox
{
	FXDECLARE(FXModalDialogBox);

public:
	enum FrameTypes { ftHorizontal,ftVertical };

	FXModalDialogBox(FXWindow *owner,const FXString &title,int w,int h,FrameTypes frameType=ftHorizontal);

	virtual void show(FXuint placement);
	virtual FXuint execute(FXuint placement=PLACEMENT_CURSOR);
	virtual void hide();

	void disableFrameDecor();

	long onOkayButton(FXObject *sender,FXSelector sel,void *ptr);

	enum 
	{
		ID_OKAY_BUTTON=FXDialogBox::ID_LAST,
		ID_LAST
	};

protected:
	FXModalDialogBox() {}

	FXPacker *getFrame();

	// a derived class can override this to validate stuff before the okay button closes the window
	virtual bool validateOnOkay() { return(true); }

private:
	FXVerticalFrame *contents;
		FXPacker *upperFrame;
		FXHorizontalFrame *lowerFrame;
			FXHorizontalFrame *buttonPacker;
				FXButton *okayButton;
				FXButton *cancelButton;


};

#endif
