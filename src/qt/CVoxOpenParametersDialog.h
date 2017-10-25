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

#ifndef __CVoxOpenParametersDialog_H__
#define __CVoxOpenParametersDialog_H__

#include "../../config/common.h"

#include "qt_compat.h"

class CVoxOpenParametersDialog;

#include "ActionParam/CActionParamDialog.h"

#include "../backend/AFrontendHooks.h"

class CVoxOpenParametersDialog : public CActionParamDialog
{
	Q_OBJECT
public:
	CVoxOpenParametersDialog(QWidget *mainWindow);
	virtual ~CVoxOpenParametersDialog(){}

	bool show(AFrontendHooks::VoxParameters &parameters);

private:
	CComboTextParamValue *m_channels;
	CComboTextParamValue *m_sampleRate;
};

#endif
