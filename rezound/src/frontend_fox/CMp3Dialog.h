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

#ifndef __CMp3Dialog_H__
#define __CMp3Dialog_H__

#include "../../config/common.h"
#include "fox_compat.h"

class CMp3Dialog;

#include "FXModalDialogBox.h"

#include "../backend/AFrontendHooks.h"

class CMp3Dialog : public FXModalDialogBox
{
	FXDECLARE(CMp3Dialog);
public:
	CMp3Dialog(FXWindow *mainWindow);
	virtual ~CMp3Dialog();

	bool show(AFrontendHooks::Mp3CompressionParameters &parameters);

	long onRadioButton(FXObject *sender,FXSelector sel,void *ptr);

	enum 
	{
		ID_WHICH_BUTTON=FXModalDialogBox::ID_LAST,
		ID_LAST
	};

protected:
	CMp3Dialog() {}

private:

	FXRadioButton *CBRButton;
	FXComposite *CBRFrame;
		FXComboBox *bitRateComboBox;

	FXRadioButton *ABRButton;
	FXComposite *ABRFrame;
		FXComboBox *minRateComboBox;
		FXComboBox *normRateComboBox;
		FXComboBox *maxRateComboBox;

	FXRadioButton *qualityButton;
	FXComposite *qualityFrame;
		FXComboBox *qualityComboBox;

	FXComposite *flagsFrame;
		FXCheckButton *useOnlyFlagsButton;
		FXTextField *additionalFlagsTextBox;
};

#endif
