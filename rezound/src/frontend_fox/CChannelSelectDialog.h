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

#ifndef __CChannelSelectDialog_H__
# define __CChannelSelectDialog_H__

#include "../../config/common.h"
#include "fox_compat.h"


class CChannelSelectDialog;

#include "FXModalDialogBox.h"
#include "../backend/AActionDialog.h"
#include "../backend/CActionSound.h"

extern CChannelSelectDialog *gChannelSelectDialog;

/*
 * This is the implementation of AActionDialog that the backend
 * asks to show whenever there is a question of what channels to 
 * apply to action to...   This dialog's show method returns
 * true if the user press okay.. or false if they hit cancel.  
 *
 * The show method upon returning true should have also set 
 * actionSound's doChannel values according to the checkboxes
 * on the dialog
 */
class CChannelSelectDialog : public FXModalDialogBox, public AActionDialog
{
	FXDECLARE(CChannelSelectDialog);
public:
	CChannelSelectDialog(FXWindow *mainWindow);
	virtual ~CChannelSelectDialog();

	bool show(CActionSound *actionSound,CActionParameters *actionParameters);
	void hide();

	void setTitle(const string title) { };

	enum
	{
		ID_DEFAULT_BUTTON=FXModalDialogBox::ID_LAST,
		ID_CLEAR_BUTTON,
	};

	long onDefaultButton(FXObject *sender,FXSelector sel,void *ptr);
	long onClearButton(FXObject *sender,FXSelector sel,void *ptr);

protected:
	CChannelSelectDialog() {}

private:
	const CActionSound *actionSound;

	FXLabel *label;
	FXCheckButton *checkBoxes[MAX_CHANNELS];

};

#endif
