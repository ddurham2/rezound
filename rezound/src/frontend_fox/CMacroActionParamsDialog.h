/* 
 * Copyright (C) 2005 - David W. Durham
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

#ifndef __CMacroActionParamsDialog_H__
#define __CMacroActionParamsDialog_H__

#include "../../config/common.h"
#include "fox_compat.h"

#include "../backend/AFrontendHooks.h"

class CMacroActionParamsDialog;
class AActionFactory;

#include "FXModalDialogBox.h"

class CMacroActionParamsDialog : public FXModalDialogBox
{
	FXDECLARE(CMacroActionParamsDialog);
public:
	CMacroActionParamsDialog(FXWindow *mainWindow);
	virtual ~CMacroActionParamsDialog();

	bool showIt(const AActionFactory *actionFactory,AFrontendHooks::MacroActionParameters &macroActionParameters,CLoadedSound *loadedSound);

	enum
	{
		ID_RADIO_BUTTON=FXModalDialogBox::ID_LAST,

		ID_LAST
	};

	long onRadioButton(FXObject *sender,FXSelector sel,void *ptr);

protected:
	CMacroActionParamsDialog() {}

private:

	FXLabel *actionNameLabel;

	FXCheckButton *askToPromptForActionParametersAtPlaybackCheckButton;

	FXGroupBox *startPosPositioningGroupBox;
		FXRadioButton *startPosRadioButton1;
		FXRadioButton *startPosRadioButton2;
		FXRadioButton *startPosRadioButton3;
		FXRadioButton *startPosRadioButton4;
		FXRadioButton *startPosRadioButton7;

	FXGroupBox *stopPosPositioningGroupBox;
		FXRadioButton *stopPosRadioButton1;
		FXRadioButton *stopPosRadioButton2;
		FXRadioButton *stopPosRadioButton3;
		FXRadioButton *stopPosRadioButton4;
		FXRadioButton *stopPosRadioButton5;
		FXRadioButton *stopPosRadioButton6;
		FXRadioButton *stopPosRadioButton7;

};

#endif
