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

#ifndef __CPasteChannelsDialog_H__
#define __CPasteChannelsDialog_H__

#include "../../config/common.h"
#include "fox_compat.h"

#include <vector>
#include <utility>

class CPasteChannelsDialog;

#include "FXModalDialogBox.h"

#include "../backend/AActionDialog.h"
#include "../backend/CSound_defs.h"


extern CPasteChannelsDialog *gPasteChannelsDialog;
class FXConstantParamValue;

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
class CPasteChannelsDialog : public FXModalDialogBox, public AActionDialog
{
	FXDECLARE(CPasteChannelsDialog);
public:
	CPasteChannelsDialog(FXWindow *mainWindow);
	virtual ~CPasteChannelsDialog();

	bool show(CActionSound *actionSound,CActionParameters *actionParameters);
	void hide();

	void setTitle(const string title) { }

	void *getUserData();

	enum
	{
		ID_DEFAULT_BUTTON=FXModalDialogBox::ID_LAST,
		ID_CLEAR_BUTTON,

		ID_REPEAT_TYPE_COMBOBOX
	};

	long onDefaultButton(FXObject *sender,FXSelector sel,void *ptr);
	long onClearButton(FXObject *sender,FXSelector sel,void *ptr);

	long onRepeatTypeChange(FXObject *sender,FXSelector sel,void *ptr);

protected:
	CPasteChannelsDialog() {}

private:
	CActionSound *actionSound;

	FXLabel *label;
	FXHorizontalSeparator *horzSeparator;
	FXComposite *topFrame;
		FXMatrix *routingContents;
			// dummy
			FXLabel *sourceLabel;
			FXLabel *destinationLabel;
			FXMatrix *checkBoxMatrix;
				FXCheckButton *checkBoxes[MAX_CHANNELS][MAX_CHANNELS];

		FXVerticalSeparator *vertSeparator;

		FXComposite *repeatFrame;
			FXSwitcher *repeatTypeSwitcher;
				FXConstantParamValue *repeatCountSlider;
				FXConstantParamValue *repeatTimeSlider;
			FXComboBox *repeatTypeComboBox;
		
	
	FXPacker *mixTypeFrame;
		FXComboBox *mixTypeComboBox;

	vector<vector<bool> > pasteChannels;
};

#endif
