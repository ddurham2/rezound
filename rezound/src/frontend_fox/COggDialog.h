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

#ifndef __COggDialog_H__
#define __COggDialog_H__

#include "../../config/common.h"
#include "fox_compat.h"

class COggDialog;

#include "FXModalDialogBox.h"

#include "../backend/AFrontendHooks.h"

class COggDialog : public FXModalDialogBox
{
	FXDECLARE(COggDialog);
public:
	COggDialog(FXWindow *mainWindow);
	virtual ~COggDialog();

	bool show(AFrontendHooks::OggCompressionParameters &parameters);

	long onRadioButton(FXObject *sender,FXSelector sel,void *ptr);

	enum 
	{
		ID_WHICH_BUTTON=FXModalDialogBox::ID_LAST,
		ID_LAST
	};

protected:
	COggDialog() {}

private:

	FXRadioButton *qualityButton;
	FXComposite *qualityFrame;
		FXTextField *qualityTextBox;

	FXRadioButton *CBRButton;
	FXComposite *CBRFrame;
		FXComboBox *bitRateComboBox;

	FXRadioButton *VBRButton;
	FXComposite *VBRFrame;
		FXComboBox *minRateComboBox;
		FXComboBox *normRateComboBox;
		FXComboBox *maxRateComboBox;
};

#endif
