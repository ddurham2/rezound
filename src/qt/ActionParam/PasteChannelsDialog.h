/* 
 * Copyright (C) 2007 - David W. Durham
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

#ifndef __PasteChannelsDialog_H__
#define __PasteChannelsDialog_H__

#include "../../../config/common.h"
#include "../qt_compat.h"

#include <QCheckBox>

#include <vector>
#include <utility>

class PasteChannelsDialog;

#include "../ModalDialog.h"
#include "ui_PasteChannelsDialogContent.h"

#include "../../backend/AActionDialog.h"
#include "../../backend/CSound_defs.h"


extern PasteChannelsDialog *gPasteChannelsDialog;
class ConstantParamValue;

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
class PasteChannelsDialog : public ModalDialog, public AActionDialog
{
	Q_OBJECT
public:
    PasteChannelsDialog(QWidget *mainWindow);
    virtual ~PasteChannelsDialog();

	bool show(CActionSound *actionSound,CActionParameters *actionParameters);
	void hide();

	void setTitle(const string title) { ModalDialog::setTitle(title.c_str()); }

	void *getUserData();

private Q_SLOTS:
	void onDefaultButton();
	void onClearButton();

private:
	Ui::PasteChannelsDialogContent pcdc;
	CActionSound *actionSound;

	ConstantParamValue *repeatCountSlider;
	ConstantParamValue *repeatTimeSlider;

	QCheckBox *checkBoxes[MAX_CHANNELS][MAX_CHANNELS];
	QLabel *srcLabels[MAX_CHANNELS];
	QLabel *destLabels[MAX_CHANNELS];

	vector<vector<bool> > pasteChannels;
};

#endif
