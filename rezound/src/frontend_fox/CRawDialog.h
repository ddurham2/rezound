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

#ifndef __CRawDialog_H__
#define __CRawDialog_H__

#include "../../config/common.h"
#include "fox_compat.h"

class CRawDialog;

#include "FXModalDialogBox.h"

#include "../backend/AFrontendHooks.h"

class CRawDialog : public FXModalDialogBox
{
	FXDECLARE(CRawDialog);
public:
	CRawDialog(FXWindow *mainWindow);
	virtual ~CRawDialog();

	bool show(AFrontendHooks::RawParameters &parameters,bool showOffsetAndLengthParameters);

	//long onRadioButton(FXObject *sender,FXSelector sel,void *ptr);

	/*
	enum 
	{
		ID_WHICH_BUTTON=FXModalDialogBox::ID_LAST,
		ID_LAST
	};
	*/

protected:
	CRawDialog() {}

private:

	FXLabel *channelsCountLabel;
	FXComboBox *channelsCountComboBox;
	FXLabel *sampleRateLabel;
	FXComboBox *sampleRateComboBox;
	FXComboBox *sampleFormatComboBox;
	FXToggleButton *byteOrderToggleButton;
	FXLabel *offsetLabel;
	FXHorizontalFrame *offsetFrame;
		FXTextField *dataOffsetTextBox;
	FXLabel *lengthLabel;
	FXHorizontalFrame *lengthFrame;
		FXTextField *dataLengthTextBox;
};

#endif
