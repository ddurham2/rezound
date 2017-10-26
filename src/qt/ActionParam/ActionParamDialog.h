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

#ifndef ActionParamDialog_H__
#define ActionParamDialog_H__

#include "../../../config/common.h"
#include "../qt_compat.h"


class ActionParamDialog;
class QMainWindow;

#include "ui_ActionParamDialogContent.h"
#include "../ModalDialog.h"

#include <vector>
#include <utility>

#include "ConstantParamValue.h"
#include "TextParamValue.h"
#include "DiskEntityParamValue.h"
#include "ComboTextParamValue.h"
#include "CheckBoxParamValue.h"
#include "GraphParamValue.h"
#include "LFOParamValue.h"
#include "PluginRoutingParamValue.h"

#include "../../backend/AActionDialog.h"
#include "../../backend/CGraphParamValueNode.h"

#include "../../backend/AActionParamMapper.h"

class CNestedDataFile;

class ActionParamDialog : public ModalDialog, public AActionDialog
{
	Q_OBJECT
public:
	typedef const double (*f_at_x)(const double x);

	// the presetPrefix value will get prefixed to all the read/writes on the presets file
	ActionParamDialog(QWidget *mainWindow,bool showPresetPanel=true,const string presetPrefix="",ModalDialog::ShowTypes showType=ModalDialog::stRememberSizeAndPosition);
	virtual ~ActionParamDialog();

	// these are used to create new parents for the controls
	// or something to lay other FOX widgets on (but not controls since they won't be saved in presets)
	// 	pass NULL the first time
	QWidget *newHorzPanel(QWidget *parent,bool createMargin=true,bool createFrame=false);
	QWidget *newVertPanel(QWidget *parent,bool createMargin=true,bool createFrame=false);

	ConstantParamValue *addSlider(QWidget *parent,const string name,const string units,AActionParamMapper *valueMapper,f_at_x optRetValueConv,bool showInverseButton);
		ConstantParamValue *getSliderParam(const string name);
	TextParamValue *addNumericTextEntry(QWidget *parent,const string name,const string units,const double initialValue,const double minValue,const double maxValue,const string unitsTipText="");
	TextParamValue *addStringTextEntry(QWidget *parent,const string name,const string initialValue,const string tipText="");
		TextParamValue *getTextParam(const string name);
	DiskEntityParamValue *addDiskEntityEntry(QWidget *parent,const string name,const string intialEntityName,DiskEntityParamValue::DiskEntityTypes entityType,const string tipText="");
		DiskEntityParamValue *getDiskEntityParam(const string name);
		enum ComboParamValueTypes { cpvtAsInteger/* if editable, then atoi of text, else index*/ ,cpvtAsString };
		/* is isEditable then the value is an integer of the actual value, if isEditable is false, then the integer value is the index of the items */
	ComboTextParamValue *addComboTextEntry(QWidget *parent,const string name,const vector<string> &items,ComboParamValueTypes type,const string tipText="",bool isEditable=false);
		ComboTextParamValue *getComboText(const string name); // so a derived class can set the values
	CheckBoxParamValue *addCheckBoxEntry(QWidget *parent,const string name,const bool checked,const string tipText="");
		CheckBoxParamValue *getCheckBoxParam(const string name);
	GraphParamValue *addGraph(QWidget *parent,const string name,const string horzAxisLabel,const string horzUnits,AActionParamMapper *horzValueMapper,const string vertAxisLabel,const string vertUnits,AActionParamMapper *vertValueMapper,f_at_x optRetValueConv);
		GraphParamValue *getGraphParam(const string name); // so a derived class can set some ranges
	GraphParamValue *addGraphWithWaveform(QWidget *parent,const string name,const string vertAxisLabel,const string vertUnits,AActionParamMapper *vertValueMapper,f_at_x optRetValueConv);
    LFOParamValue *addLFO(QWidget *parent,const string name,const string ampUnits,const string ampTitle,const double maxAmp,const string freqUnits,const double maxFreq,const bool hideBipolarLFOs);
        LFOParamValue *getLFOParam(const string name);
    PluginRoutingParamValue *addPluginRoutingParam(QWidget *parent,const string name,const LADSPA_Descriptor *desc);
        PluginRoutingParamValue *getPluginRoutingParam(const string name);

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
//	void setMargin(int margin); // will add a margin the left and right of all the controls

	bool show(CActionSound *actionSound,CActionParameters *actionParameters);
	void hide();
#if 0

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

#endif
	
	//void create();

	void setTitle(const string title) { ModalDialog::setTitle(title.c_str()); }

	void setMargin(int margin) { apdc.leftMargin->setFixedWidth(margin); apdc.rightMargin->setFixedWidth(margin); }

#if 0

protected:
	ActionParamDialog() {}
#endif

	// can be overridden to return an explanation of the action which will cause the appearance of an 'explain' button on the dialog
	virtual const string getExplanation() const { return ""; }

private Q_SLOTS:
	void onPresetUseButton();
	void onPresetSaveButton();
	void onPresetRemoveButton();

	void onExplainButton();

private:
	Ui::ActionParamDialogContent apdc;
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

	// the QWidget * points to either an ConstantParamValue, TextParamValue, ComboTextParamValue, CheckBoxParamValue or an GraphParamValue
	vector<pair<ParamTypes,QWidget *> > parameters;
	vector<f_at_x> retValueConvs;

	const string presetPrefix;

	bool firstShowing;

	QWidget *newPanel(bool horz,QWidget *parent,bool createMargin=true,bool createFrame=false);

	void buildPresetLists();
	void buildPresetList(CNestedDataFile *f,QListWidget *list);

	unsigned findParamByName(const string name) const;
};

#endif
