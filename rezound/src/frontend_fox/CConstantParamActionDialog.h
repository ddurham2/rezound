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

#ifndef __CConstantParamActionDialog_H__
#define __CConstantParamActionDialog_H__

#include "../../config/common.h"


class CConstantParamActionDialog;

#include <vector>

#include "FXModalDialogBox.h"
#include "FXConstantParamValue.h"

#include "../backend/AAction.h"
#include "../backend/CGraphParamValueNode.h"


class CNestedDataFile;

class CConstantParamActionDialog : public FXModalDialogBox, public AActionDialog
{
	FXDECLARE(CConstantParamActionDialog);
public:
	typedef const double (*f_at_x)(const double x);

	CConstantParamActionDialog(FXWindow *mainWindow,const FXString title,int w,int h);

	void addSlider(const FXString name,const FXString units,FXConstantParamValue::f_at_xs interpretValue,FXConstantParamValue::f_at_xs uninterpretValue,f_at_x optRetValueConv,const double initialValue,const int minScalar,const int maxScalar,const int initScalar,bool showInverseButton);
	void addValueEntry(const FXString name,const FXString units,const double initialValue);

	bool show(CActionSound *actionSound,CActionParameters *actionParameters);

	enum 
	{
		ID_NATIVE_PRESET_BUTTON=FXModalDialogBox::ID_LAST,
		ID_NATIVE_PRESET_LIST,

		ID_USER_PRESET_LIST,
		ID_USER_PRESET_USE_BUTTON,
		ID_USER_PRESET_SAVE_BUTTON,
		ID_USER_PRESET_REMOVE_BUTTON,

		ID_LAST
	};

	
	long onPresetUseButton(FXObject *sender,FXSelector sel,void *ptr);
	long onPresetSaveButton(FXObject *sender,FXSelector sel,void *ptr);
	long onPresetRemoveButton(FXObject *sender,FXSelector sel,void *ptr);

protected:
	CConstantParamActionDialog() {}

private:
	const CActionSound *actionSound;

	vector<FXConstantParamValue *> parameters;
	vector<f_at_x> retValueConvs;

	FXSplitter *splitter;
		FXPacker *controlsFrame;
		FXPacker *presetsFrame;
			FXList *nativePresetList;
			FXList *userPresetList;

	void buildPresetLists();
	void buildPresetList(CNestedDataFile &f,FXList *list);

};

#endif
