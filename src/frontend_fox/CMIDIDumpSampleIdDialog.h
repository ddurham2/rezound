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

#ifndef __CMIDIDumpSampleIdDialog_H__
#define __CMIDIDumpSampleIdDialog_H__

#include "../../config/common.h"
#include "fox_compat.h"

class CMIDIDumpSampleIdDialog;

#include "FXModalDialogBox.h"

class CMIDIDumpSampleIdDialog : public FXModalDialogBox
{
	FXDECLARE(CMIDIDumpSampleIdDialog);
public:
	CMIDIDumpSampleIdDialog(FXWindow *mainWindow);
	virtual ~CMIDIDumpSampleIdDialog();

	bool showForOpen(int &sysExChannel,int &waveformId);
	bool showForSave(int &sysExChannel,int &waveformId,int &loopType);

	long onWaitCheckButton(FXObject *sender,FXuint sel,void *ptr);

	enum
	{
		ID_WAIT_CHECKBUTTON=FXModalDialogBox::ID_LAST,

		ID_LAST
	};

protected:
	CMIDIDumpSampleIdDialog() {}

private:
	FXCheckButton *waitCheckButton;

	FXTextField *waveformIdEdit;

	FXSpinner *sysExChannelSpinner;

	FXComposite *loopTypeFrame;
		FXTextField *loopTypeEdit;
};

#endif
