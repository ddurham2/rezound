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

#ifndef OggSaveParametersDialog_H__
#define OggSaveParametersDialog_H__

#include <QGroupBox>

#include "../../config/common.h"

#include "qt_compat.h"

class OggSaveParametersDialog;

#include "ActionParam/ActionParamDialog.h"

#include "../backend/AFrontendHooks.h"

class OggSaveParametersDialog : public ActionParamDialog
{
	Q_OBJECT
public:
	OggSaveParametersDialog(QWidget *mainWindow);
	virtual ~OggSaveParametersDialog(){}

	bool show(AFrontendHooks::OggCompressionParameters &parameters);

private Q_SLOTS:
	void onGroupBoxToggled(bool on);

private:
	QGroupBox *qualityGroupBox;
		TextParamValue *qualityEdit;

	QGroupBox *CBRGroupBox;
		ComboTextParamValue *CBRCombo;

	QGroupBox *VBRGroupBox;
		ComboTextParamValue *minVBRCombo;
		ComboTextParamValue *normalVBRCombo;
		ComboTextParamValue *maxVBRCombo;
};

#endif
