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

#ifndef __CActionParamDialog_H__
#define __CActionParamDialog_H__

#include "../../config/common.h"
#include "fox_compat.h"


class CActionParamDialog;

#include <vector>
#include <utility>

#include "FXModalDialogBox.h"

#include "FXConstantParamValue.h"
#include "FXTextParamValue.h"
#include "FXFilenameParamValue.h"
#include "FXComboTextParamValue.h"
#include "FXCheckBoxParamValue.h"
#include "FXGraphParamValue.h"
#include "FXLFOParamValue.h"

#include "../backend/AActionDialog.h"
#include "../backend/CGraphParamValueNode.h"


class CNestedDataFile;

class CActionParamDialog : public FXModalDialogBox, public AActionDialog
{
	FXDECLARE(CActionParamDialog);
public:
	typedef const double (*f_at_x)(const double x);

	CActionParamDialog(FXWindow *mainWindow,const FXString title,bool showPresetPanel=true);
	virtual ~CActionParamDialog();

	// these are used to create new parents for the controls
	// 	pass NULL the first time
	void *newHorzPanel(void *parent,bool createBorder=true);
	void *newVertPanel(void *parent,bool createBorder=true);

	void addSlider(void *parent,const string name,const string units,FXConstantParamValue::f_at_xs interpretValue,FXConstantParamValue::f_at_xs uninterpretValue,f_at_x optRetValueConv,const double initialValue,const int minScalar,const int maxScalar,const int initScalar,bool showInverseButton);
	void addTextEntry(void *parent,const string name,const string units,const double initialValue,const double minValue,const double maxValue,const string unitsTipText="");
	void addFilenameEntry(void *parent,const string name,const string intialFilename,const string tipText="");
		/* is isEditable then the value is an integer of the actual value, if isEditable is false, then the integer value is the index of the items */
	void addComboTextEntry(void *parent,const string name,const vector<string> &items,const string tipText="",bool isEditable=false);
		FXComboTextParamValue *getComboText(const string name); // so a derived class can set the values
	void addCheckBoxEntry(void *parent,const string name,const bool checked,const string tipText="");
	void addGraph(void *parent,const string name,const string horzAxisLabel,const string horzUnits,FXGraphParamValue::f_at_xs horzInterpretValue,FXGraphParamValue::f_at_xs horzUninterpretValue,const string vertAxisLabel,const string vertUnits,FXGraphParamValue::f_at_xs vertInterpretValue,FXGraphParamValue::f_at_xs vertUninterpretValue,f_at_x optRetValueConv,const int minScalar,const int maxScalar,const int initialScalar);
	void addGraphWithWaveform(void *parent,const string name,const string vertAxisLabel,const string vertUnits,FXGraphParamValue::f_at_xs vertInterpretValue,FXGraphParamValue::f_at_xs vertUninterpretValue,f_at_x optRetValueConv,const int minScalar,const int maxScalar,const int initialScalar);
	void addLFO(void *parent,const string name,const string ampUnits,const string ampTitle,const double maxAmp,const string freqUnits,const double maxFreq,const bool hideBipolarLFOs);

	/* 
	 * index corrisponds to the order that the add...() methods were called 
	 * and this can only be called for sliders, text entries, check boxes, and
	 * to set the index of a combobox
	 */
	void setValue(size_t index,const double value);

	//void setControlHeight(size_t index,const size_t height);
	//const size_t getControlHeight(size_t index) const;

	void setTipText(size_t index,const string tipText);

	// don't like this, but it will do for now... someday I've got to come up with just how to specify placement of the added wigets
	void setMargin(FXint margin); // will add a margin the left and right of all the controls

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
	CActionParamDialog() {}

private:
	const CActionSound *actionSound;

	enum ParamTypes
	{
		ptConstant,
		ptText,
		ptFilename,
		ptComboText,
		ptCheckBox,
		ptGraph,
		ptGraphWithWaveform,
		ptLFO
	};

	// the void * points to either an FXConstantParamValue, FXTextParamValue, FXComboTextParamValue, FXCheckBoxParamValue or an FXGraphParamValue
	vector<pair<ParamTypes,void *> > parameters;
	vector<f_at_x> retValueConvs;

	FXSplitter *splitter;
		FXPacker *topPanel;
			FXFrame *leftMargin;
			FXPacker *controlsFrame;
			FXFrame *rightMargin;
		FXPacker *presetsFrame;
			FXList *nativePresetList;
			FXList *userPresetList;

	bool shrinkPresetsFrame;

	void buildPresetLists();
	void buildPresetList(CNestedDataFile *f,FXList *list);

};

#endif
