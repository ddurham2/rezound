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

#ifndef __CCueListDialog_H__
#define __CCueListDialog_H__

#include "../../config/common.h"
#include "fox_compat.h"

#include <fox/fx.h>

class CLoadedSound;

class CAddCueActionFactory;
class CRemoveCueActionFactory;
class CReplaceCueActionFactory;


/*
 * This is meant to be a base class for a modal dialog
 *
 * It goes ahead and creates two panels with an okay and
 * cancel button in the lower panel... The derived class
 * should put whatever is needed in the upper panel
 */
class CCueListDialog : public FXDialogBox
{
	FXDECLARE(CCueListDialog);

public:
	CCueListDialog(FXWindow *owner);
	virtual ~CCueListDialog();

	long onCloseButton(FXObject *sender,FXSelector sel,void *ptr);

	long onAddCueButton(FXObject *sender,FXSelector sel,void *ptr);
	long onRemoveCueButton(FXObject *sender,FXSelector sel,void *ptr);
	long onEditCueButton(FXObject *sender,FXSelector sel,void *ptr);

	enum 
	{
		ID_CLOSE_BUTTON=FXDialogBox::ID_LAST,

		ID_ADD_CUE_BUTTON,
		ID_REMOVE_CUE_BUTTON,
		ID_CHANGE_CUE_BUTTON,

		ID_CUE_LIST,

		ID_LAST
	};

	void setLoadedSound(CLoadedSound *_loadedSound) { loadedSound=_loadedSound; }

	virtual void show(FXuint placement);
	virtual void hide();

protected:
	CCueListDialog() {}

private:
	FXVerticalFrame *contents;
		FXPacker *upperFrame;
			FXComposite *cueListFrame;
				FXList *cueList;
		FXHorizontalFrame *lowerFrame;
			FXHorizontalFrame *buttonPacker;
				FXButton *closeButton;

				FXButton *addCueButton;
				FXButton *removeCueButton;
				FXButton *editCueButton;

	CLoadedSound *loadedSound;

	CAddCueActionFactory * addCueActionFactory;
	CRemoveCueActionFactory * removeCueActionFactory;
	CReplaceCueActionFactory * replaceCueActionFactory;



	void rebuildCueList();
};

extern CCueListDialog *gCueListDialog;

#endif
