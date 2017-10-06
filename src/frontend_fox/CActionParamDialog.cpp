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

#include "CActionParamDialog.h"

#include <stdexcept>

#include <istring>

#include <CNestedDataFile/CNestedDataFile.h>

#include "CStatusComm.h"
#include "settings.h"

#include "../backend/CActionParameters.h"
#include "../backend/CActionSound.h"
#include "../backend/AAction.h" // for EUserMessage

#include "CFOXIcons.h"

FXDEFMAP(CActionParamDialog) CActionParamDialogMap[]=
{
//	Message_Type			ID							Message_Handler

	FXMAPFUNC(SEL_COMMAND,		CActionParamDialog::ID_NATIVE_PRESET_BUTTON,	CActionParamDialog::onPresetUseButton),
	FXMAPFUNC(SEL_DOUBLECLICKED,	CActionParamDialog::ID_NATIVE_PRESET_LIST,	CActionParamDialog::onPresetUseButton),

	FXMAPFUNC(SEL_COMMAND,		CActionParamDialog::ID_USER_PRESET_USE_BUTTON,	CActionParamDialog::onPresetUseButton),
	FXMAPFUNC(SEL_COMMAND,		CActionParamDialog::ID_USER_PRESET_SAVE_BUTTON,	CActionParamDialog::onPresetSaveButton),
	FXMAPFUNC(SEL_COMMAND,		CActionParamDialog::ID_USER_PRESET_REMOVE_BUTTON,CActionParamDialog::onPresetRemoveButton),
	FXMAPFUNC(SEL_DOUBLECLICKED,	CActionParamDialog::ID_USER_PRESET_LIST,	CActionParamDialog::onPresetUseButton),

	FXMAPFUNC(SEL_COMMAND,		CActionParamDialog::ID_EXPLAIN_BUTTON,		CActionParamDialog::onExplainButton),
};
		

FXIMPLEMENT(CActionParamDialog,FXModalDialogBox,CActionParamDialogMap,ARRAYNUMBER(CActionParamDialogMap))


// ----------------------------------------

// ??? TODO Well, I got it to not need the width, it's determined by the 
// widgets' needed widths.. I don't quite know tho, what the height won't 
// work the same what.. I'll work on figuring that out and then both 
// parameters should be unnecessary

CActionParamDialog::CActionParamDialog(FXWindow *mainWindow,bool _showPresetPanel,const string _presetPrefix,FXModalDialogBox::ShowTypes showType) :
	FXModalDialogBox(mainWindow,"",0,0,FXModalDialogBox::ftVertical,showType),

	showPresetPanel(_showPresetPanel),

	explanationButtonCreated(false),
	
	splitter(new FXSplitter(getFrame(),SPLITTER_VERTICAL|SPLITTER_REVERSED | LAYOUT_FILL_X|LAYOUT_FILL_Y)),
		topPanel(new FXHorizontalFrame(splitter,FRAME_RAISED|FRAME_THICK, 0,0,0,0, 0,0,0,0, 0,0)),
			leftMargin(new FXFrame(topPanel,FRAME_NONE|LAYOUT_FILL_Y|LAYOUT_FIX_WIDTH,0,0,0,0, 0,0,0,0)),
			controlsFrame(new FXPacker(topPanel,FRAME_NONE | LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 0,0)),
			rightMargin(new FXFrame(topPanel,FRAME_NONE|LAYOUT_FILL_Y|LAYOUT_FIX_WIDTH,0,0,0,0, 0,0,0,0)),
		presetsFrame(NULL),
			nativePresetList(NULL),
			userPresetList(NULL),

	presetPrefix(_presetPrefix=="" ? "" : _presetPrefix DOT ""),
	firstShowing(true)
{
	disableFrameDecor();

	setHeight(getHeight()+100); // since we're adding this presets section, make the dialog taller

	// make sure the dialog has at least a minimum height and width
	//ASSURE_HEIGHT(this,10);
	//ASSURE_WIDTH(this,200);
}

CActionParamDialog::~CActionParamDialog()
{
}

void CActionParamDialog::create()
{
	if(!explanationButtonCreated && getExplanation()!="")
		new FXButton(getButtonFrame(),_("Explain"),FOXIcons->explain,this,ID_EXPLAIN_BUTTON,FRAME_RAISED|FRAME_THICK | JUSTIFY_NORMAL | ICON_ABOVE_TEXT | LAYOUT_FIX_WIDTH, 0,0,60,0, 2,2,2,2);
	explanationButtonCreated=true;
	
	FXModalDialogBox::create();
}

long CActionParamDialog::onExplainButton(FXObject *sender,FXSelector sel,void *ptr)
{
	Message(getExplanation());
	return 1;
}

FXPacker *CActionParamDialog::newHorzPanel(void *parent,bool createMargin,bool createFrame)
{
	if(parent==NULL)
	{
		if(controlsFrame->numChildren()>0)
			throw runtime_error(string(__func__)+" -- this method has already been called with a NULL parameter");
		parent=controlsFrame;
	}
	if(createMargin)
		return new FXHorizontalFrame((FXPacker *)parent,(createFrame ? FRAME_RAISED : FRAME_NONE) | LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 2,2,2,2, 0,0);
	else
		return new FXHorizontalFrame((FXPacker *)parent,(createFrame ? FRAME_RAISED : FRAME_NONE) | LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 0,0);
}

FXPacker *CActionParamDialog::newVertPanel(void *parent,bool createMargin,bool createFrame)
{
	if(parent==NULL)
	{
		if(controlsFrame->numChildren()>0)
			throw runtime_error(string(__func__)+" -- this method has already been called with a NULL parameter");
		parent=controlsFrame;
	}
	if(createMargin)
		return new FXVerticalFrame((FXPacker *)parent,(createFrame ? FRAME_RAISED : FRAME_NONE) | LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 2,2,2,2, 0,0);
	else
		return new FXVerticalFrame((FXPacker *)parent,(createFrame ? FRAME_RAISED : FRAME_NONE) | LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 0,0);
}

FXConstantParamValue *CActionParamDialog::addSlider(void *parent,const string name,const string units,AActionParamMapper *valueMapper,f_at_x optRetValueConv,bool showInverseButton)
{
	if(parent==NULL)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- use CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
	FXConstantParamValue *slider=new FXConstantParamValue(valueMapper,showInverseButton,(FXPacker *)parent,0,name.c_str());
	slider->setUnits(units.c_str());
	slider->setValue(valueMapper->getDefaultValue());
	parameters.push_back(make_pair(ptConstant,slider));
	retValueConvs.push_back(optRetValueConv);

	return slider;
}

FXConstantParamValue *CActionParamDialog::getSliderParam(const string name)
{
	const unsigned index=findParamByName(name);
	if(parameters[index].first==ptConstant);
		return (FXConstantParamValue *)parameters[index].second;
	throw runtime_error(string(__func__)+" -- widget with name, "+name+", is not a slider");
}

FXTextParamValue *CActionParamDialog::addNumericTextEntry(void *parent,const string name,const string units,const double initialValue,const double minValue,const double maxValue,const string unitsTipText)
{
	if(parent==NULL)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- use CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
	FXTextParamValue *textEntry=new FXTextParamValue((FXPacker *)parent,0,name.c_str(),initialValue,minValue,maxValue);
	textEntry->setUnits(units.c_str(),unitsTipText.c_str());
	parameters.push_back(make_pair(ptNumericText,textEntry));
	retValueConvs.push_back(NULL);

	return textEntry;
}

FXTextParamValue *CActionParamDialog::addStringTextEntry(void *parent,const string name,const string initialValue,const string tipText)
{
	if(parent==NULL)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- use CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
	FXTextParamValue *textEntry=new FXTextParamValue((FXPacker *)parent,0,name.c_str(),initialValue);
	textEntry->setTipText(tipText.c_str());
	parameters.push_back(make_pair(ptStringText,textEntry));
	retValueConvs.push_back(NULL);

	return textEntry;
}

FXTextParamValue *CActionParamDialog::getTextParam(const string name)
{
	for(size_t t=0;t<parameters.size();t++)
	{
		if((parameters[t].first==ptStringText || parameters[t].first==ptNumericText) && ((FXTextParamValue *)parameters[t].second)->getName()==name)
			return (FXTextParamValue *)parameters[t].second;
	}
	throw runtime_error(string(__func__)+" -- no text param found named "+name);
}

FXDiskEntityParamValue *CActionParamDialog::addDiskEntityEntry(void *parent,const string name,const string initialEntityName,FXDiskEntityParamValue::DiskEntityTypes entityType,const string tipText)
{
	if(parent==NULL)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- use CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
	FXDiskEntityParamValue *diskEntityEntry=new FXDiskEntityParamValue((FXPacker *)parent,0,name.c_str(),initialEntityName,entityType);
	diskEntityEntry->setTipText(tipText.c_str());
	parameters.push_back(make_pair(ptDiskEntity,diskEntityEntry));
	retValueConvs.push_back(NULL);

	return diskEntityEntry;
}

FXDiskEntityParamValue *CActionParamDialog::getDiskEntityParam(const string name)
{
	const unsigned index=findParamByName(name);
	if(parameters[index].first==ptDiskEntity)
		return (FXDiskEntityParamValue *)parameters[index].second;
	throw runtime_error(string(__func__)+" -- widget with name, "+name+", is not a disk entity");
}

FXComboTextParamValue *CActionParamDialog::addComboTextEntry(void *parent,const string name,const vector<string> &items,ComboParamValueTypes type,const string tipText,bool isEditable)
{
	if(parent==NULL)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- use CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
	FXComboTextParamValue *comboTextEntry=new FXComboTextParamValue((FXPacker *)parent,0,name.c_str(),items,isEditable);
	if(type==cpvtAsString)
		comboTextEntry->asString=true;
	else
		comboTextEntry->asString=false;
	comboTextEntry->setTipText(tipText.c_str());
	parameters.push_back(make_pair(ptComboText,comboTextEntry));
	retValueConvs.push_back(NULL);

	return comboTextEntry;
}

FXComboTextParamValue *CActionParamDialog::getComboText(const string name)
{
	const unsigned index=findParamByName(name);
	if(parameters[index].first==ptComboText);
		return (FXComboTextParamValue *)parameters[index].second;
	throw runtime_error(string(__func__)+" -- widget with name, "+name+", is not a combobox");
}


FXCheckBoxParamValue *CActionParamDialog::addCheckBoxEntry(void *parent,const string name,const bool checked,const string tipText)
{
	if(parent==NULL)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- use CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
	FXCheckBoxParamValue *checkBoxEntry=new FXCheckBoxParamValue((FXPacker *)parent,0,name.c_str(),checked);
	checkBoxEntry->setTipText(tipText.c_str());
	parameters.push_back(make_pair(ptCheckBox,checkBoxEntry));
	retValueConvs.push_back(NULL);

	return checkBoxEntry;
}

FXCheckBoxParamValue *CActionParamDialog::getCheckBoxParam(const string name)
{
	const unsigned index=findParamByName(name);
	if(parameters[index].first==ptCheckBox)
		return (FXCheckBoxParamValue *)parameters[index].second;
	throw runtime_error(string(__func__)+" -- widget with name, "+name+", is not a checkbox");
}

FXGraphParamValue *CActionParamDialog::addGraph(void *parent,const string name,const string horzAxisLabel,const string horzUnits,AActionParamMapper *horzValueMapper,const string vertAxisLabel,const string vertUnits,AActionParamMapper *vertValueMapper,f_at_x optRetValueConv)
{
	if(parent==NULL)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- use CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
	FXGraphParamValue *graph=new FXGraphParamValue(name.c_str(),(FXPacker *)parent,LAYOUT_FILL_X|LAYOUT_FILL_Y);
	graph->setHorzParameters(horzAxisLabel,horzUnits,horzValueMapper);
	graph->setVertParameters(vertAxisLabel,vertUnits,vertValueMapper);
	parameters.push_back(make_pair(ptGraph,graph));
	retValueConvs.push_back(optRetValueConv);

	return graph;
}

FXGraphParamValue *CActionParamDialog::addGraphWithWaveform(void *parent,const string name,const string vertAxisLabel,const string vertUnits,AActionParamMapper *vertValueMapper,f_at_x optRetValueConv)
{
	if(parent==NULL)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- use CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
	FXGraphParamValue *graph=new FXGraphParamValue(name.c_str(),(FXPacker *)parent,LAYOUT_FILL_X|LAYOUT_FILL_Y);
	graph->setVertParameters(vertAxisLabel,vertUnits,vertValueMapper);
	parameters.push_back(make_pair(ptGraphWithWaveform,graph));
	retValueConvs.push_back(optRetValueConv);

	return graph;
}

FXGraphParamValue *CActionParamDialog::getGraphParam(const string name)
{
	for(size_t t=0;t<parameters.size();t++)
	{
		if((parameters[t].first==ptGraph || parameters[t].first==ptGraphWithWaveform) && ((FXGraphParamValue *)parameters[t].second)->getName()==name)
			return (FXGraphParamValue *)parameters[t].second;
	}
	throw runtime_error(string(__func__)+" -- no graph param found named "+name);
}

FXLFOParamValue *CActionParamDialog::addLFO(void *parent,const string name,const string ampUnits,const string ampTitle,const double maxAmp,const string freqUnits,const double maxFreq,const bool hideBipolarLFOs)
{
	if(parent==NULL)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- use CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
	FXLFOParamValue *LFOEntry=new FXLFOParamValue((FXPacker *)parent,0,name.c_str(),ampUnits,ampTitle,maxAmp,freqUnits,maxFreq,hideBipolarLFOs);
	//LFOEntry->setTipText(tipText.c_str());
	parameters.push_back(make_pair(ptLFO,LFOEntry));
	retValueConvs.push_back(NULL);

	return LFOEntry;
}

FXLFOParamValue *CActionParamDialog::getLFOParam(const string name)
{
	const unsigned index=findParamByName(name);
	if(parameters[index].first==ptLFO);
		return (FXLFOParamValue *)parameters[index].second;
	throw runtime_error(string(__func__)+" -- widget with name, "+name+", is not an LFO");
}

FXPluginRoutingParamValue *CActionParamDialog::addPluginRoutingParam(void *parent,const string name,const LADSPA_Descriptor *desc)
{
	if(parent==NULL)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- use CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
	FXPluginRoutingParamValue *pluginRoutingEntry=new FXPluginRoutingParamValue((FXPacker *)parent,0,name.c_str(),desc);
	parameters.push_back(make_pair(ptPluginRouting,pluginRoutingEntry));
	retValueConvs.push_back(NULL);

	return pluginRoutingEntry;
}

FXPluginRoutingParamValue *CActionParamDialog::getPluginRoutingParam(const string name)
{
	const unsigned index=findParamByName(name);
	if(parameters[index].first==ptPluginRouting);
		return (FXPluginRoutingParamValue *)parameters[index].second;
	throw runtime_error(string(__func__)+" -- widget with name, "+name+", is not a Plugin Routing");
}

void CActionParamDialog::setMargin(FXint margin)
{
	leftMargin->setWidth(margin);
	rightMargin->setWidth(margin);
}

void CActionParamDialog::setValue(size_t index,const double value)
{
	switch(parameters[index].first)
	{
	case ptConstant:
		((FXConstantParamValue *)parameters[index].second)->setValue(value);
		break;

	case ptNumericText:
		((FXTextParamValue *)parameters[index].second)->setValue(value);
		break;

	case ptStringText:
		((FXTextParamValue *)parameters[index].second)->setText(istring(value));
		break;

	case ptComboText:
		((FXComboTextParamValue *)parameters[index].second)->setIntegerValue((FXint)value);
		break;

	case ptCheckBox:
		((FXCheckBoxParamValue *)parameters[index].second)->setValue((bool)value);
		break;

	case ptGraph:
	case ptGraphWithWaveform:
		/*
		((FXGraphParamValue *)parameters[index].second)->setValue(value);
		break;
		*/

	case ptLFO:
		/*
		((FXGraphParamValue *)parameters[index].second)->setValue(value);
		break;
		*/

	case ptPluginRouting:
		/*
		((FXGraphParamValue *)parameters[index].second)->setValue(value);
		 break;
		 */

	default:
		throw runtime_error(string(__func__)+" -- unhandled or unimplemented parameter type: "+istring(parameters[index].first));
	}
}

void CActionParamDialog::setTipText(const string name,const string tipText)
{
	const unsigned index=findParamByName(name);
	
	switch(parameters[index].first)
	{
	case ptConstant:
		((FXConstantParamValue *)parameters[index].second)->setTipText(tipText.c_str());
		break;

	case ptNumericText:
	case ptStringText:
		((FXTextParamValue *)parameters[index].second)->setTipText(tipText.c_str());
		break;

	case ptDiskEntity:
		((FXDiskEntityParamValue *)parameters[index].second)->setTipText(tipText.c_str());
		break;

	case ptComboText:
		((FXComboTextParamValue *)parameters[index].second)->setTipText(tipText.c_str());
		break;

	case ptCheckBox:
		((FXCheckBoxParamValue *)parameters[index].second)->setTipText(tipText.c_str());
		break;

	case ptGraph:
	case ptGraphWithWaveform:
/*
		((FXGraphParamValue *)parameters[index].second)->setTipText(tipText.c_str());
		break;
*/

	case ptLFO:
/*
		((FXLFOParamValue *)parameters[index].second)->setTipText(tipText.c_str());
		break;
*/

	case ptPluginRouting:
/*
		((FXPluginRoutingParamValue *)parameters[index].second)->setTipText(tipText.c_str());
		break;
*/

	default:
		throw runtime_error(string(__func__)+" -- unhandled or unimplemented parameter type: "+istring(parameters[index].first));
	}
}

void CActionParamDialog::showControl(const string name,bool show)
{
	const unsigned index=findParamByName(name);
	
	switch(parameters[index].first)
	{
	case ptConstant:
		if(show)
			((FXConstantParamValue *)parameters[index].second)->show();
		else
			((FXConstantParamValue *)parameters[index].second)->hide();
		break;

	case ptNumericText:
	case ptStringText:
		if(show)
			((FXTextParamValue *)parameters[index].second)->show();
		else
			((FXTextParamValue *)parameters[index].second)->hide();
		break;

	case ptDiskEntity:
		if(show)
			((FXDiskEntityParamValue *)parameters[index].second)->show();
		else
			((FXDiskEntityParamValue *)parameters[index].second)->hide();
		break;

	case ptComboText:
		if(show)
			((FXComboTextParamValue *)parameters[index].second)->show();
		else
			((FXComboTextParamValue *)parameters[index].second)->hide();
		break;

	case ptCheckBox:
		if(show)
			((FXCheckBoxParamValue *)parameters[index].second)->show();
		else
			((FXCheckBoxParamValue *)parameters[index].second)->hide();
		break;

	case ptGraph:
	case ptGraphWithWaveform:
		if(show)
			((FXGraphParamValue *)parameters[index].second)->show();
		else
			((FXGraphParamValue *)parameters[index].second)->hide();
		break;

	case ptLFO:
		if(show)
			((FXLFOParamValue *)parameters[index].second)->show();
		else
			((FXLFOParamValue *)parameters[index].second)->hide();
		break;

	case ptPluginRouting:
		if(show)
			((FXPluginRoutingParamValue *)parameters[index].second)->show();
		else
			((FXPluginRoutingParamValue *)parameters[index].second)->hide();
		break;

	default:
		throw runtime_error(string(__func__)+" -- unhandled or unimplemented parameter type: "+istring(parameters[index].first));
	}
	
	// tell it to recalc if the number of shown or hidden children changes
	parameters[index].second->getParent()->recalc();
}

bool CActionParamDialog::show(CActionSound *actionSound,CActionParameters *actionParameters)
{
	bool retval=false;

	if(getOrigTitle()=="")
		throw runtime_error(string(__func__)+" -- title was never set");

	buildPresetLists();

	// restore the splitter's position
	const FXint h=gSettingsRegistry->getValue<int>("FOX" DOT "SplitterPositions" DOT presetPrefix+getOrigTitle());
	if(presetsFrame!=NULL)
		presetsFrame->setHeight(h);


	// initialize all the graphs to this sound
	for(size_t t=0;t<parameters.size();t++)
	{
		if(parameters[t].first==ptGraphWithWaveform)
		{
			((FXGraphParamValue *)parameters[t].second)->setSound(actionSound->sound,actionSound->start,actionSound->stop);
			if(firstShowing)
				((FXGraphParamValue *)parameters[t].second)->clearNodes();
		}
		else if(parameters[t].first==ptPluginRouting)
			((FXPluginRoutingParamValue *)parameters[t].second)->setSound(actionSound->sound);
	}

	firstShowing=false;

reshow:

	if(execute(PLACEMENT_CURSOR))
	{
		vector<string> addedParameters;
		try
		{
			for(unsigned t=0;t<parameters.size();t++)
			{
				switch(parameters[t].first)
				{
				case ptConstant:
					{
						FXConstantParamValue *slider=(FXConstantParamValue *)parameters[t].second;
						double ret=slider->getValue();

						if(retValueConvs[t]!=NULL)
							ret=retValueConvs[t](ret);

						actionParameters->setValue<double>(slider->getName(),ret);
						addedParameters.push_back(slider->getName());
					}
					break;

				case ptNumericText:
					{
						FXTextParamValue *textEntry=(FXTextParamValue *)parameters[t].second;
						double ret=textEntry->getValue();

						if(retValueConvs[t]!=NULL)
							ret=retValueConvs[t](ret);

						actionParameters->setValue<double>(textEntry->getName(),ret);	
						addedParameters.push_back(textEntry->getName());
					}
					break;

				case ptStringText:
					{
						FXTextParamValue *textEntry=(FXTextParamValue *)parameters[t].second;
						const string ret=textEntry->getText();
						actionParameters->setValue<string>(textEntry->getName(),ret);	
						addedParameters.push_back(textEntry->getName());
					}
					break;

				case ptDiskEntity:
					{
						FXDiskEntityParamValue *diskEntityEntry=(FXDiskEntityParamValue *)parameters[t].second;
						const string ret=diskEntityEntry->getEntityName();

						actionParameters->setValue<string>(diskEntityEntry->getName(),ret);	

						if(diskEntityEntry->getEntityType()==FXDiskEntityParamValue::detAudioFilename)
							actionParameters->setValue<bool>(diskEntityEntry->getName()+" OpenAsRaw",diskEntityEntry->getOpenAsRaw());	
						addedParameters.push_back(diskEntityEntry->getName());
					}
					break;

				case ptComboText:
					{
						FXComboTextParamValue *comboTextEntry=(FXComboTextParamValue *)parameters[t].second;
						if(comboTextEntry->asString)
						{ // return the text of the item selected
							actionParameters->setValue<string>(comboTextEntry->getName(),comboTextEntry->getStringValue());
						}
						else
						{ // return values as integer of the index that was selected
							FXint ret=comboTextEntry->getIntegerValue();
							actionParameters->setValue<unsigned>(comboTextEntry->getName(),(unsigned)ret);	
						}
						addedParameters.push_back(comboTextEntry->getName());
					}
					break;

				case ptCheckBox:
					{
						FXCheckBoxParamValue *checkBoxEntry=(FXCheckBoxParamValue *)parameters[t].second;
						bool ret=checkBoxEntry->getValue();

						actionParameters->setValue<bool>(checkBoxEntry->getName(),ret);	
						addedParameters.push_back(checkBoxEntry->getName());
					}
					break;

				case ptGraph:
				case ptGraphWithWaveform:
					{
						FXGraphParamValue *graph=(FXGraphParamValue *)parameters[t].second;
						CGraphParamValueNodeList nodes=graph->getNodes();

						if(retValueConvs[t]!=NULL)
						{
							for(size_t i=0;i<nodes.size();i++)
								nodes[i].y=retValueConvs[t](nodes[i].y);
						}

						actionParameters->setValue<CGraphParamValueNodeList>(graph->getName(),nodes);
						addedParameters.push_back(graph->getName());
					}
					break;

				case ptLFO:
					{
						FXLFOParamValue *LFOEntry=(FXLFOParamValue *)parameters[t].second;
						actionParameters->setValue<CLFODescription>(LFOEntry->getName(),LFOEntry->getValue());
						addedParameters.push_back(LFOEntry->getName());
					}
					break;

				case ptPluginRouting:
					{
						FXPluginRoutingParamValue *pluginRoutingEntry=(FXPluginRoutingParamValue *)parameters[t].second;
						actionParameters->setValue<CPluginMapping>(pluginRoutingEntry->getName(),pluginRoutingEntry->getValue());
						addedParameters.push_back(pluginRoutingEntry->getName());
					}
					break;

				default:
					throw runtime_error(string(__func__)+" -- unhandled parameter type: "+istring(parameters[t].first));
				}
			}
			retval=true;
		}
		catch(EUserMessage &e)
		{
			if(e.what()[0])
			{
				Message(e.what());
				for(size_t t=0;t<addedParameters.size();t++)
					actionParameters->removeKey(addedParameters[t]);
				goto reshow;
			}
			else
				retval=false;;
		}
	}

	// save the splitter's position
	if(presetsFrame!=NULL)
	{
		FXint h2=presetsFrame->getHeight();
		gSettingsRegistry->setValue<string>("FOX" DOT "SplitterPositions" DOT presetPrefix+getOrigTitle(),istring(h2));
	}

	hide(); // hide now and ... 
#if FOX_MAJOR>0 // stupid debian's old crap
	getApp()->repaint(); // force redraws from disappearing dialogs now
#endif

	return retval;
}

void CActionParamDialog::hide()
{
	FXModalDialogBox::hide();
}

long CActionParamDialog::onPresetUseButton(FXObject *sender,FXSelector sel,void *ptr)
{
	CNestedDataFile *presetsFile;
	FXList *listBox;
	if(FXSELID(sel)==ID_NATIVE_PRESET_BUTTON || FXSELID(sel)==ID_NATIVE_PRESET_LIST)
	{
		presetsFile=gSysPresetsFile;
		listBox=nativePresetList;
	}
	else //if(FXSELID(sel)==ID_USER_PRESET_BUTTON || FXSELID(sel)==ID_USER_PRESET_LIST)
	{
		presetsFile=gUserPresetsFile;
		listBox=userPresetList;
	}

	try
	{
		if(listBox->getCurrentItem()<0)
		{
			gStatusComm->beep();
			return 1;
		}
		
		const string name=string(listBox->getItemText(listBox->getCurrentItem()).text()).substr(4);
		const string title=presetPrefix+getOrigTitle() DOT name;

		for(unsigned t=0;t<parameters.size();t++)
		{
			switch(parameters[t].first)
			{
			case ptConstant:
				((FXConstantParamValue *)parameters[t].second)->readFromFile(title,presetsFile);
				break;

			case ptNumericText:
			case ptStringText:
				((FXTextParamValue *)parameters[t].second)->readFromFile(title,presetsFile);
				break;

			case ptDiskEntity:
				((FXDiskEntityParamValue *)parameters[t].second)->readFromFile(title,presetsFile);
				break;

			case ptComboText:
				((FXComboTextParamValue *)parameters[t].second)->readFromFile(title,presetsFile);
				break;

			case ptCheckBox:
				((FXCheckBoxParamValue *)parameters[t].second)->readFromFile(title,presetsFile);
				break;

			case ptGraph:
			case ptGraphWithWaveform:
				((FXGraphParamValue *)parameters[t].second)->readFromFile(title,presetsFile);
				break;

			case ptLFO:
				((FXLFOParamValue *)parameters[t].second)->readFromFile(title,presetsFile);
				break;

			case ptPluginRouting:
				((FXPluginRoutingParamValue *)parameters[t].second)->readFromFile(title,presetsFile);
				break;

			default:
				throw runtime_error(string(__func__)+" -- unhandled parameter type: "+istring(parameters[t].first));
			}
		}
	}
	catch(exception &e)
	{
		Error(e.what());
	}

	return 1;
}

long CActionParamDialog::onPresetSaveButton(FXObject *sender,FXSelector sel,void *ptr)
{
	FXString _name=userPresetList->getCurrentItem()>=0 ? (userPresetList->getItemText(userPresetList->getCurrentItem())).mid(4,255) : "";

	askAgain:
	if(FXInputDialog::getString(_name,this,_("Preset Name"),_("Preset Name")))
	{
		if(_name.trim()=="")
		{
			Error(_("Invalid Preset Name"));
			goto askAgain;
		}

		string name=_name.text();

		// make sure it doesn't contain DOT
		if(name.find(CNestedDataFile::delim)!=string::npos)
		{
			Error(_("Preset Name cannot contain")+string(" '")+string(CNestedDataFile::delim)+"'");
			goto askAgain;
		}

		try
		{
			CNestedDataFile *presetsFile=gUserPresetsFile;


			const string title=presetPrefix+getOrigTitle() DOT name;

			bool alreadyExists=false;
			if(presetsFile->keyExists(title))
			{
				alreadyExists=true;
				if(Question(_("Overwrite Existing Preset")+string(" '")+name+"'",yesnoQues)!=yesAns)
					return 1;
			}

			for(unsigned t=0;t<parameters.size();t++)
			{
				switch(parameters[t].first)
				{
				case ptConstant:
					((FXConstantParamValue *)parameters[t].second)->writeToFile(title,presetsFile);
					break;

				case ptNumericText:
				case ptStringText:
					((FXTextParamValue *)parameters[t].second)->writeToFile(title,presetsFile);
					break;

				case ptDiskEntity:
					((FXDiskEntityParamValue *)parameters[t].second)->writeToFile(title,presetsFile);
					break;

				case ptComboText:
					((FXComboTextParamValue *)parameters[t].second)->writeToFile(title,presetsFile);
					break;

				case ptCheckBox:
					((FXCheckBoxParamValue *)parameters[t].second)->writeToFile(title,presetsFile);
					break;

				case ptGraph:
				case ptGraphWithWaveform:
					((FXGraphParamValue *)parameters[t].second)->writeToFile(title,presetsFile);
					break;

				case ptLFO:
					((FXLFOParamValue *)parameters[t].second)->writeToFile(title,presetsFile);
					break;

				case ptPluginRouting:
					((FXPluginRoutingParamValue *)parameters[t].second)->writeToFile(title,presetsFile);
					break;

				default:
					throw runtime_error(string(__func__)+" -- unhandled parameter type: "+istring(parameters[t].first));
				}
			}

			if(!alreadyExists)
			{
				const string key=presetPrefix+getOrigTitle() DOT "names";
				vector<string> names=presetsFile->getValue<vector<string> >(key);
				names.push_back(name);
				presetsFile->setValue<vector<string> >(key,names);
				buildPresetList(presetsFile,userPresetList);
			}
				
			presetsFile->save();
		}
		catch(exception &e)
		{
			Error(e.what());
		}
	}

	return 1;
}

long CActionParamDialog::onPresetRemoveButton(FXObject *sender,FXSelector sel,void *ptr)
{
	if(userPresetList->getCurrentItem()>=0)
	{
		string name=(userPresetList->getItemText(userPresetList->getCurrentItem())).mid(4,255).text();
		if(Question(_("Remove Preset")+string(" '")+name+"'",yesnoQues)==yesAns)
		{
			CNestedDataFile *presetsFile=gUserPresetsFile;

			const string key=presetPrefix+getOrigTitle() DOT name;

			// remove preset's values
			presetsFile->removeKey(key);

			// remove preset's name from the names array
			vector<string> names=presetsFile->getValue<vector<string> >(presetPrefix+getOrigTitle() DOT "names");
			names.erase(names.begin()+userPresetList->getCurrentItem());
			presetsFile->setValue<vector<string> >(presetPrefix+getOrigTitle() DOT "names",names);

			buildPresetList(presetsFile,userPresetList);
			presetsFile->save();
		}
	}
	else
		gStatusComm->beep();

	return 1;
}

void CActionParamDialog::buildPresetLists()
{
	if(showPresetPanel)
	{
		bool firstTime=presetsFrame==NULL;

		FXint currentNativePresetItem=nativePresetList ? nativePresetList->getCurrentItem() : 0;
		FXint currentUserPresetItem=userPresetList ? userPresetList->getCurrentItem() : 0;

		// delete previous stuff
		delete presetsFrame;
			nativePresetList=NULL;
			userPresetList=NULL;

		// (re)create GUI widgets
		presetsFrame=new FXHorizontalFrame(splitter,FRAME_RAISED|FRAME_THICK | LAYOUT_FILL_X, 0,0,0,0, 2,2,2,2);
		try
		{
			if(gSysPresetsFile->getValue<vector<string> >(presetPrefix+getOrigTitle() DOT "names").size()>0)
			{
				// native preset stuff
				FXPacker *listFrame=new FXPacker(presetsFrame,FRAME_SUNKEN|FRAME_THICK|LAYOUT_FIX_WIDTH | LAYOUT_FILL_Y, 0,0,210,0, 0,0,0,0, 0,0); // had to do this because FXList won't take that frame style
					nativePresetList=new FXList(listFrame,this,ID_NATIVE_PRESET_LIST,LIST_BROWSESELECT | LAYOUT_FILL_X|LAYOUT_FILL_Y);
					nativePresetList->setNumVisible(4);
				new FXButton(presetsFrame,_("&Use\tOr Double-Click an Item in the List"),NULL,this,ID_NATIVE_PRESET_BUTTON);
			}
		}
		catch(exception &e)
		{
			nativePresetList=NULL;
			Error(e.what());
		}

		// user preset stuff
		FXPacker *listFrame=new FXPacker(presetsFrame,FRAME_SUNKEN|FRAME_THICK|LAYOUT_FIX_WIDTH | LAYOUT_FILL_Y, 0,0,210,0, 0,0,0,0, 0,0); // had to do this because FXList won't take that frame style
			userPresetList=new FXList(listFrame,this,ID_USER_PRESET_LIST,LIST_BROWSESELECT | LAYOUT_FILL_X|LAYOUT_FILL_Y);
			userPresetList->setNumVisible(4);
		FXPacker *buttonGroup=new FXVerticalFrame(presetsFrame);
			new FXButton(buttonGroup,_("&Use\tOr Double-Click an Item in the List"),NULL,this,ID_USER_PRESET_USE_BUTTON,BUTTON_NORMAL|LAYOUT_FILL_X);
			new FXButton(buttonGroup,_("&Save"),NULL,this,ID_USER_PRESET_SAVE_BUTTON,BUTTON_NORMAL|LAYOUT_FILL_X);
			new FXButton(buttonGroup,_("&Remove"),NULL,this,ID_USER_PRESET_REMOVE_BUTTON,BUTTON_NORMAL|LAYOUT_FILL_X);




		// load presets from file

		if(nativePresetList!=NULL)
		{
			try
			{
				buildPresetList(gSysPresetsFile,nativePresetList);
				if(currentNativePresetItem<nativePresetList->getNumItems())
					nativePresetList->setCurrentItem(currentNativePresetItem);
			}
			catch(exception &e)
			{
				Error(e.what());
				nativePresetList->clearItems();
			}
		}
	
		try
		{
			buildPresetList(gUserPresetsFile,userPresetList);
			if(currentUserPresetItem<userPresetList->getNumItems())
				userPresetList->setCurrentItem(currentUserPresetItem);
		}
		catch(exception &e)
		{
			Error(e.what());
			userPresetList->clearItems();
		}

		// have to do this after the first showing, otherwise the presets won't show up until you resize the dialog
		if(!firstTime)
			presetsFrame->create();
	}
}

void CActionParamDialog::buildPresetList(CNestedDataFile *f,FXList *list)
{
	const vector<string> names=f->getValue<vector<string> >(presetPrefix+getOrigTitle() DOT "names");
	list->clearItems();
	for(size_t t=0;t<names.size();t++)
		list->appendItem((istring(t,3,true)+" "+names[t]).c_str());
}


unsigned CActionParamDialog::findParamByName(const string name) const
{
	for(unsigned t=0;t<parameters.size();t++)
	{
		switch(parameters[t].first)
		{
		case ptConstant:
			if(((FXConstantParamValue *)parameters[t].second)->getName()==name)
				return t;
			break;

		case ptNumericText:
		case ptStringText:
			if(((FXTextParamValue *)parameters[t].second)->getName()==name)
				return t;
			break;

		case ptDiskEntity:
			if(((FXDiskEntityParamValue *)parameters[t].second)->getName()==name)
				return t;
			break;

		case ptComboText:
			if(((FXComboTextParamValue *)parameters[t].second)->getName()==name)
				return t;
			break;

		case ptCheckBox:
			if(((FXCheckBoxParamValue *)parameters[t].second)->getName()==name)
				return t;
			break;

		case ptGraph:
		case ptGraphWithWaveform:
			if(((FXGraphParamValue *)parameters[t].second)->getName()==name)
				return t;
			break;

		case ptLFO:
			if(((FXLFOParamValue *)parameters[t].second)->getName()==name)
				return t;
			break;

		case ptPluginRouting:
			if(((FXPluginRoutingParamValue *)parameters[t].second)->getName()==name)
				return t;
			break;

		default:
			throw runtime_error(string(__func__)+" -- unhandled parameter type: "+istring(parameters[t].first));
		}
	}

	throw runtime_error(string(__func__)+" -- no param found named "+name);
}

