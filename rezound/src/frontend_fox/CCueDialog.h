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

#ifndef __CCueDialog_H__
#define __CCueDialog_H__

#include "../../config/common.h"


class CCueDialog;

#include "FXModalDialogBox.h"

#include "../backend/AAction.h"

extern CCueDialog *gCueDialog;

class CCueDialog : public FXModalDialogBox, public AActionDialog
{
	FXDECLARE(CCueDialog);
public:
	CCueDialog(FXWindow *mainWindow);

	bool show(CActionSound *actionSound,CActionParameters *actionParameters);

protected:
	CCueDialog() {}

	bool validateOnOkay();

private:
	FXPacker *cueNamePacker;
		FXLabel *cueNameLabel;
		FXTextField *cueNameTextBox;

	FXPacker *cueTimePacker;
		FXLabel *cueTimeLabel;
		FXTextField *cueTimeTextBox;

	FXPacker *isAnchoredPacker;
		FXCheckButton *isAnchoredCheckButton;

	string cueName;
	sample_pos_t cueTime;
	bool isAnchored;

	CActionSound *actionSound;
};

#endif
