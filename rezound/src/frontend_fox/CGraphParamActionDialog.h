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

#ifndef __CGraphParamActionDialog_H__
#define __CGraphParamActionDialog_H__

#include "../../config/common.h"


class CGraphParamActionDialog;

#include "FXModalDialogBox.h"
#include "FXGraphParamValue.h"

#include "../backend/AAction.h"
#include "../backend/CGraphParamValueNode.h"

class CMainWindow;

extern CGraphParamActionDialog *gGraphParamActionDialog;

class CGraphParamActionDialog : public FXModalDialogBox, public AActionDialog
{
	FXDECLARE(CGraphParamActionDialog);
public:
	typedef const double (*f_at_x)(const double x);

	CGraphParamActionDialog(FXWindow *mainWindow,const FXString title,const FXString units,FXGraphParamValue::f_at_xs interpretValue,FXGraphParamValue::f_at_xs uninterpretValue,f_at_x optRetValueConv,const int minScalar,const int maxScalar,const int initialScalar);

	bool show(CActionSound *actionSound,CActionParameters *actionParameters);

protected:
	CGraphParamActionDialog() {}

private:
	const CActionSound *actionSound;
	FXGraphParamValue *graph;
	f_at_x retValueConv;

};

#endif
