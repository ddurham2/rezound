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

#ifndef __CProgressDialog_H__
#define __CProgressDialog_H__

#include "../../config/common.h"
#include "fox_compat.h"

#include <fox/fx.h>

class CProgressDialog : public FXDialogBox
{
	FXDECLARE(CProgressDialog);

public:
	CProgressDialog(FXWindow *owner,const FXString &title,bool showCancelButton);
	virtual ~CProgressDialog();

	virtual void show(FXuint placement=PLACEMENT_OWNER);
	virtual void hide();

	void setProgress(int progress); // 0 to 100

	long onCancelButton(FXObject *sender,FXSelector sel,void *ptr);
	long onCloseWindow(FXObject *sender,FXSelector sel,void *ptr);

	enum 
	{
		ID_CANCEL_BUTTON=FXDialogBox::ID_LAST,
		ID_LAST
	};

	bool isCancelled;

protected:
	CProgressDialog() {}

private:
	FXHorizontalFrame *contents;
		FXProgressBar *progressBar;
		FXButton *cancelButton;
};

#endif
