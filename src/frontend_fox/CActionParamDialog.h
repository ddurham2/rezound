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
#include "FXDiskEntityParamValue.h"
#include "FXComboTextParamValue.h"
#include "FXCheckBoxParamValue.h"
#include "FXGraphParamValue.h"
#include "FXLFOParamValue.h"
#include "FXPluginRoutingParamValue.h"

#include "../backend/AActionDialog.h"
#include "../backend/CGraphParamValueNode.h"

#include "../backend/AActionParamMapper.h"

class CNestedDataFile;

class CActionParamDialog : public FXModalDialogBox, public AActionDialog
{
	FXDECLARE(CActionParamDialog);
public:
	typedef const double (*f_at_x)(const double x);

	// the presetPrefix value will get prefixed to all the read/writes on the presets file
	CActionParamDialog(FXWindow *mainWindow,bool showPresetPanel=true,const string presetPrefix="",FXModalDialogBox::ShowTypes showType=FXModalDialogBox::stRememberSizeAndPosition);
	virtual ~CActionParamDialog();

	// these are used to create new parents for the controls
	// or something to lay other FOX widgets on (but not controls since they won't be saved in presets)
	// 	pass NULL the first time
	FXPacker *newHorzPanel(void *parent,bool createMargin=true,bool createFrame=false);
	FXPacker *newVertPanel(void *parent,bool createMargin=true,bool createFrame=false);

	FXConstantParamValue *addSlider(void *parent,const string name,const string units,AActionParamMapper *valueMapper,f_at_x optRetValueConv,bool showInverseButton);
		FXConstantParamValue *getSliderParam(const string name);
	FXTextParamValue *addNumericTextEntry(void *parent,const string name,const string units,const double initialValue,const double minValue,const double maxValue,const string unitsTipText="");
	FXTextParamValue *addStringTextEntry(void *parent,const string name,const string initialValue,const string tipText="");
		FXTextParamValue *getTextParam(const string name);
	FXDiskEntityParamValue *addDiskEntityEntry(void *parent,const string name,const string intialEntityName,FXDiskEntityParamValue::DiskEntityTypes entityType,const string tipText="");
		FXDiskEntityParamValue *getDiskEntityParam(const string name);
		enum ComboParamValueTypes { cpvtAsInteger/* if editable, then atoi of text, else index*/ ,cpvtAsString };
		/* is isEditable then the value is an integer of the actual value, if isEditable is false, then the integer value is the index of the items */
	FXComboTextParamValue *addComboTextEntry(void *parent,const string name,const vector<string> &items,ComboParamValueTypes type,const string tipText="",bool isEditable=false);
		FXComboTextParamValue *getComboText(const string name); // so a derived class can set the values
	FXCheckBoxParamValue *addCheckBoxEntry(void *parent,const string name,const bool checked,const string tipText="");
		FXCheckBoxParamValue *getCheckBoxParam(const string name);
	FXGraphParamValue *addGraph(void *parent,const string name,const string horzAxisLabel,const string horzUnits,AActionParamMapper *horzValueMapper,const string vertAxisLabel,const string vertUnits,AActionParamMapper *vertValueMapper,f_at_x optRetValueConv);
		FXGraphParamValue *getGraphParam(const string name); // so a derived class can set some ranges
	FXGraphParamValue *addGraphWithWaveform(void *parent,const string name,const string vertAxisLabel,const string vertUnits,AActionParamMapper *vertValueMapper,f_at_x optRetValueConv);
	FXLFOParamValue *addLFO(void *parent,const string name,const string ampUnits,const string ampTitle,const double maxAmp,const string freqUnits,const double maxFreq,const bool hideBipolarLFOs);
		FXLFOParamValue *getLFOParam(const string name); 
	FXPluginRoutingParamValue *addPluginRoutingParam(void *parent,const string name,const LADSPA_Descriptor *desc);
		FXPluginRoutingParamValue *getPluginRoutingParam(const string name); 

	// show or hide a control
	void showControl(const string name,bool show);

	/* 
	 * index corrisponds to the order that the add...() methods were called 
	 * and this can only be called for sliders, text entries, check boxes, and
	 * to set the index of a combobox
	 */
	void setValue(size_t index,const double value);

	//void setControlHeight(size_t index,const size_t height);
	//const size_t getControlHeight(size_t index) const;

	void setTipText(const string name,const string tipText);

	// don't like this, but it will do for now... someday I've got to come up with just how to specify placement of the added wigets
	void setMargin(FXint margin); // will add a margin the left and right of all the controls

	bool show(CActionSound *actionSound,CActionParameters *actionParameters);
	void hide();

	enum 
	{
		ID_NATIVE_PRESET_BUTTON=FXModalDialogBox::ID_LAST,
		ID_NATIVE_PRESET_LIST,

		ID_USER_PRESET_LIST,
		ID_USER_PRESET_USE_BUTTON,
		ID_USER_PRESET_SAVE_BUTTON,
		ID_USER_PRESET_REMOVE_BUTTON,

		ID_EXPLAIN_BUTTON,

		ID_LAST
	};

	
	long onPresetUseButton(FXObject *sender,FXSelector sel,void *ptr);
	long onPresetSaveButton(FXObject *sender,FXSelector sel,void *ptr);
	long onPresetRemoveButton(FXObject *sender,FXSelector sel,void *ptr);

	long onExplainButton(FXObject *sender,FXSelector sel,void *ptr);

	void create();

	void setTitle(const string title) { FXModalDialogBox::setTitle(title); }

protected:
	CActionParamDialog() {}

	// can be overridden to return an explanation of the action which will cause the appearance of an 'explain' button on the dialog
	virtual const string getExplanation() const { return ""; }

private:
	const CActionSound *actionSound;

	bool showPresetPanel;

	bool explanationButtonCreated;

	enum ParamTypes
	{
		ptConstant,
		ptNumericText,
		ptStringText,
		ptDiskEntity,
		ptComboText,
		ptCheckBox,
		ptGraph,
		ptGraphWithWaveform,
		ptLFO,
		ptPluginRouting
	};

	// the FXWindow * points to either an FXConstantParamValue, FXTextParamValue, FXComboTextParamValue, FXCheckBoxParamValue or an FXGraphParamValue
	vector<pair<ParamTypes,FXWindow *> > parameters;
	vector<f_at_x> retValueConvs;

	FXSplitter *splitter;
		FXPacker *topPanel;
			FXFrame *leftMargin;
			FXPacker *controlsFrame;
			FXFrame *rightMargin;
		FXPacker *presetsFrame;
			FXList *nativePresetList;
			FXList *userPresetList;

	const string presetPrefix;

	void buildPresetLists();
	void buildPresetList(CNestedDataFile *f,FXList *list);

	unsigned findParamByName(const string name) const;

	bool firstShowing;

};

#endif
