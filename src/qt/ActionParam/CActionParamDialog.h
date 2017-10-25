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

#ifndef __CActionParamDialog_H__
#define __CActionParamDialog_H__

#include "../../../config/common.h"
#include "../qt_compat.h"


class CActionParamDialog;
class QMainWindow;

#include "ui_CActionParamDialogContent.h"
#include "../CModalDialog.h"

#include <vector>
#include <utility>

#include "CConstantParamValue.h"
#include "CTextParamValue.h"
#include "CDiskEntityParamValue.h"
#include "CComboTextParamValue.h"
#include "CCheckBoxParamValue.h"
#include "CGraphParamValue.h"
#include "CLFOParamValue.h"
#include "CPluginRoutingParamValue.h"

#include "../../backend/AActionDialog.h"
#include "../../backend/CGraphParamValueNode.h"

#include "../../backend/AActionParamMapper.h"

class CNestedDataFile;

class CActionParamDialog : public CModalDialog, public AActionDialog
{
	Q_OBJECT
public:
	typedef const double (*f_at_x)(const double x);

	// the presetPrefix value will get prefixed to all the read/writes on the presets file
	CActionParamDialog(QWidget *mainWindow,bool showPresetPanel=true,const string presetPrefix="",CModalDialog::ShowTypes showType=CModalDialog::stRememberSizeAndPosition);
	virtual ~CActionParamDialog();

	// these are used to create new parents for the controls
	// or something to lay other FOX widgets on (but not controls since they won't be saved in presets)
	// 	pass NULL the first time
	QWidget *newHorzPanel(QWidget *parent,bool createMargin=true,bool createFrame=false);
	QWidget *newVertPanel(QWidget *parent,bool createMargin=true,bool createFrame=false);

	CConstantParamValue *addSlider(QWidget *parent,const string name,const string units,AActionParamMapper *valueMapper,f_at_x optRetValueConv,bool showInverseButton);
		CConstantParamValue *getSliderParam(const string name);
	CTextParamValue *addNumericTextEntry(QWidget *parent,const string name,const string units,const double initialValue,const double minValue,const double maxValue,const string unitsTipText="");
	CTextParamValue *addStringTextEntry(QWidget *parent,const string name,const string initialValue,const string tipText="");
		CTextParamValue *getTextParam(const string name);
	CDiskEntityParamValue *addDiskEntityEntry(QWidget *parent,const string name,const string intialEntityName,CDiskEntityParamValue::DiskEntityTypes entityType,const string tipText="");
		CDiskEntityParamValue *getDiskEntityParam(const string name);
		enum ComboParamValueTypes { cpvtAsInteger/* if editable, then atoi of text, else index*/ ,cpvtAsString };
		/* is isEditable then the value is an integer of the actual value, if isEditable is false, then the integer value is the index of the items */
	CComboTextParamValue *addComboTextEntry(QWidget *parent,const string name,const vector<string> &items,ComboParamValueTypes type,const string tipText="",bool isEditable=false);
		CComboTextParamValue *getComboText(const string name); // so a derived class can set the values
	CCheckBoxParamValue *addCheckBoxEntry(QWidget *parent,const string name,const bool checked,const string tipText="");
		CCheckBoxParamValue *getCheckBoxParam(const string name);
	CGraphParamValue *addGraph(QWidget *parent,const string name,const string horzAxisLabel,const string horzUnits,AActionParamMapper *horzValueMapper,const string vertAxisLabel,const string vertUnits,AActionParamMapper *vertValueMapper,f_at_x optRetValueConv);
		CGraphParamValue *getGraphParam(const string name); // so a derived class can set some ranges
	CGraphParamValue *addGraphWithWaveform(QWidget *parent,const string name,const string vertAxisLabel,const string vertUnits,AActionParamMapper *vertValueMapper,f_at_x optRetValueConv);
	CLFOParamValue *addLFO(QWidget *parent,const string name,const string ampUnits,const string ampTitle,const double maxAmp,const string freqUnits,const double maxFreq,const bool hideBipolarLFOs);
		CLFOParamValue *getLFOParam(const string name); 
	CPluginRoutingParamValue *addPluginRoutingParam(QWidget *parent,const string name,const LADSPA_Descriptor *desc);
		CPluginRoutingParamValue *getPluginRoutingParam(const string name); 

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

	void setTitle(const string title) { CModalDialog::setTitle(title.c_str()); }

	void setMargin(int margin) { apdc.leftMargin->setFixedWidth(margin); apdc.rightMargin->setFixedWidth(margin); }

#if 0

protected:
	CActionParamDialog() {}
#endif

	// can be overridden to return an explanation of the action which will cause the appearance of an 'explain' button on the dialog
	virtual const string getExplanation() const { return ""; }

private Q_SLOTS:
	void onPresetUseButton();
	void onPresetSaveButton();
	void onPresetRemoveButton();

	void onExplainButton();

private:
	Ui::CActionParamDialogContent apdc;
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

	// the QWidget * points to either an CConstantParamValue, CTextParamValue, CComboTextParamValue, CCheckBoxParamValue or an CGraphParamValue
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
