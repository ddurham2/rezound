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

#include "CFOXIcons.h"

#define DOT (CNestedDataFile::delimChar)


FXDEFMAP(CActionParamDialog) CActionParamDialogMap[]=
{
//	Message_Type			ID							Message_Handler

	FXMAPFUNC(SEL_COMMAND,		CActionParamDialog::ID_NATIVE_PRESET_BUTTON,	CActionParamDialog::onPresetUseButton),
	FXMAPFUNC(SEL_DOUBLECLICKED,	CActionParamDialog::ID_NATIVE_PRESET_LIST,	CActionParamDialog::onPresetUseButton),

	FXMAPFUNC(SEL_COMMAND,		CActionParamDialog::ID_USER_PRESET_USE_BUTTON,	CActionParamDialog::onPresetUseButton),
	FXMAPFUNC(SEL_COMMAND,		CActionParamDialog::ID_USER_PRESET_SAVE_BUTTON,	CActionParamDialog::onPresetSaveButton),
	FXMAPFUNC(SEL_COMMAND,		CActionParamDialog::ID_USER_PRESET_REMOVE_BUTTON,CActionParamDialog::onPresetRemoveButton),
	FXMAPFUNC(SEL_DOUBLECLICKED,CActionParamDialog::ID_USER_PRESET_LIST,		CActionParamDialog::onPresetUseButton),

	FXMAPFUNC(SEL_COMMAND,		CActionParamDialog::ID_EXPLAIN_BUTTON,			CActionParamDialog::onExplainButton),
};
		

FXIMPLEMENT(CActionParamDialog,FXModalDialogBox,CActionParamDialogMap,ARRAYNUMBER(CActionParamDialogMap))


// ----------------------------------------

// ??? TODO Well, I got it to not need the width, it's determined by the 
// widgets' needed widths.. I don't quite know tho, what the height won't 
// work the same what.. I'll work on figuring that out and then both 
// parameters should be unnecessary

CActionParamDialog::CActionParamDialog(FXWindow *mainWindow,const FXString title,bool showPresetPanel) :
	FXModalDialogBox(mainWindow,title,0,0,FXModalDialogBox::ftVertical),

	explainationButtonCreated(false),
	
	splitter(new FXSplitter(getFrame(),SPLITTER_VERTICAL|SPLITTER_REVERSED | LAYOUT_FILL_X|LAYOUT_FILL_Y)),
		topPanel(new FXHorizontalFrame(splitter,FRAME_RAISED|FRAME_THICK, 0,0,0,0, 0,0,0,0, 0,0)),
			leftMargin(new FXFrame(topPanel,FRAME_NONE|LAYOUT_FILL_Y|LAYOUT_FIX_WIDTH,0,0,0,0, 0,0,0,0)),
			controlsFrame(new FXPacker(topPanel,FRAME_NONE | LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 0,0)),
			rightMargin(new FXFrame(topPanel,FRAME_NONE|LAYOUT_FILL_Y|LAYOUT_FIX_WIDTH,0,0,0,0, 0,0,0,0)),
		presetsFrame(showPresetPanel ? new FXHorizontalFrame(splitter,FRAME_RAISED|FRAME_THICK | LAYOUT_FILL_X, 0,0,0,0, 2,2,2,2) : NULL),
			nativePresetList(NULL),
			userPresetList(NULL)
{
	disableFrameDecor();

	setHeight(getHeight()+100); // since we're adding this presets section, make the dialog taller

	if(showPresetPanel)
	{
		try
		{
			if(gSysPresetsFile->getArraySize((getTitle()+DOT+"names").text())>0)
			{
				// native preset stuff
				FXPacker *listFrame=new FXPacker(presetsFrame,FRAME_SUNKEN|FRAME_THICK|LAYOUT_FIX_WIDTH | LAYOUT_FILL_Y, 0,0,210,0, 0,0,0,0, 0,0); // had to do this because FXList won't take that frame style
					nativePresetList=new FXList(listFrame,4,this,ID_NATIVE_PRESET_LIST,LIST_BROWSESELECT | LAYOUT_FILL_X|LAYOUT_FILL_Y);
				new FXButton(presetsFrame,"&Use\tOr Double-Click an Item in the List",NULL,this,ID_NATIVE_PRESET_BUTTON);
			}
		}
		catch(exception &e)
		{
			nativePresetList=NULL;
			Error(e.what());
		}

		// user preset stuff
		FXPacker *listFrame=new FXPacker(presetsFrame,FRAME_SUNKEN|FRAME_THICK|LAYOUT_FIX_WIDTH | LAYOUT_FILL_Y, 0,0,210,0, 0,0,0,0, 0,0); // had to do this because FXList won't take that frame style
			userPresetList=new FXList(listFrame,4,this,ID_USER_PRESET_LIST,LIST_BROWSESELECT | LAYOUT_FILL_X|LAYOUT_FILL_Y);
		FXPacker *buttonGroup=new FXVerticalFrame(presetsFrame);
			new FXButton(buttonGroup,"&Use\tOr Double-Click an Item in the List",NULL,this,ID_USER_PRESET_USE_BUTTON,BUTTON_NORMAL|LAYOUT_FILL_X);
			new FXButton(buttonGroup,"&Save",NULL,this,ID_USER_PRESET_SAVE_BUTTON,BUTTON_NORMAL|LAYOUT_FILL_X);
			new FXButton(buttonGroup,"&Remove",NULL,this,ID_USER_PRESET_REMOVE_BUTTON,BUTTON_NORMAL|LAYOUT_FILL_X);

		buildPresetLists();
	}

	// make sure the dialog has at least a minimum height and width
	//ASSURE_HEIGHT(this,10);
	//ASSURE_WIDTH(this,200);
}

CActionParamDialog::~CActionParamDialog()
{
}

void CActionParamDialog::create()
{
	if(!explainationButtonCreated && getExplaination()!="")
		new FXButton(getButtonFrame(),"Explain",FOXIcons->explain,this,ID_EXPLAIN_BUTTON,FRAME_RAISED|FRAME_THICK | JUSTIFY_NORMAL | ICON_ABOVE_TEXT | LAYOUT_FIX_WIDTH, 0,0,60,0, 2,2,2,2);
	explainationButtonCreated=true;
	
	FXModalDialogBox::create();
}

long CActionParamDialog::onExplainButton(FXObject *sender,FXSelector sel,void *ptr)
{
	Message(getExplaination());
	return 1;
}

FXPacker *CActionParamDialog::newHorzPanel(void *parent,bool createBorder)
{
	if(parent==NULL)
	{
		if(controlsFrame->numChildren()>0)
			throw runtime_error(string(__func__)+" -- this method has already been called with a NULL parameter");
		parent=controlsFrame;
	}
	if(createBorder)
		return new FXHorizontalFrame((FXPacker *)parent,FRAME_NONE | LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 2,2,2,2, 0,0);
	else
		return new FXHorizontalFrame((FXPacker *)parent,FRAME_NONE | LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 0,0);
}

FXPacker *CActionParamDialog::newVertPanel(void *parent,bool createBorder)
{
	if(parent==NULL)
	{
		if(controlsFrame->numChildren()>0)
			throw runtime_error(string(__func__)+" -- this method has already been called with a NULL parameter");
		parent=controlsFrame;
	}
	if(createBorder)
		return new FXVerticalFrame((FXPacker *)parent,FRAME_NONE | LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 2,2,2,2, 0,0);
	else
		return new FXVerticalFrame((FXPacker *)parent,FRAME_NONE | LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 0,0);
}

void CActionParamDialog::addSlider(void *parent,const string name,const string units,FXConstantParamValue::f_at_xs interpretValue,FXConstantParamValue::f_at_xs uninterpretValue,f_at_x optRetValueConv,const double initialValue,const int minScalar,const int maxScalar,const int initScalar,bool showInverseButton)
{
	if(parent==NULL)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- used CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
	FXConstantParamValue *slider=new FXConstantParamValue(interpretValue,uninterpretValue,minScalar,maxScalar,initScalar,showInverseButton,(FXPacker *)parent,0,name.c_str());
	slider->setUnits(units.c_str());
	slider->setValue(initialValue);
	parameters.push_back(make_pair(ptConstant,(void *)slider));
	retValueConvs.push_back(optRetValueConv);
}

void CActionParamDialog::addNumericTextEntry(void *parent,const string name,const string units,const double initialValue,const double minValue,const double maxValue,const string unitsTipText)
{
	if(parent==NULL)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- used CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
	FXTextParamValue *textEntry=new FXTextParamValue((FXPacker *)parent,0,name.c_str(),initialValue,minValue,maxValue);
	textEntry->setUnits(units.c_str(),unitsTipText.c_str());
	parameters.push_back(make_pair(ptNumericText,(void *)textEntry));
	retValueConvs.push_back(NULL);
}

void CActionParamDialog::addStringTextEntry(void *parent,const string name,const string initialValue,const string unitsTipText)
{
	if(parent==NULL)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- used CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
	FXTextParamValue *textEntry=new FXTextParamValue((FXPacker *)parent,0,name.c_str(),initialValue);
	parameters.push_back(make_pair(ptStringText,(void *)textEntry));
	retValueConvs.push_back(NULL);
}

FXTextParamValue *CActionParamDialog::getTextParam(const string name)
{
	for(size_t t=0;t<parameters.size();t++)
	{
		if((parameters[t].first==ptStringText || parameters[t].first==ptNumericText) && ((FXTextParamValue *)parameters[t].second)->getTitle()==name)
			return (FXTextParamValue *)parameters[t].second;
	}
	throw runtime_error(string(__func__)+" -- no text param found named "+name);
}

void CActionParamDialog::addDiskEntityEntry(void *parent,const string name,const string initialEntityName,FXDiskEntityParamValue::DiskEntityTypes entityType,const string tipText)
{
	if(parent==NULL)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- used CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
	FXDiskEntityParamValue *diskEntityEntry=new FXDiskEntityParamValue((FXPacker *)parent,0,name.c_str(),initialEntityName,entityType);
	diskEntityEntry->setTipText(tipText.c_str());
	parameters.push_back(make_pair(ptDiskEntity,(void *)diskEntityEntry));
	retValueConvs.push_back(NULL);
}


void CActionParamDialog::addComboTextEntry(void *parent,const string name,const vector<string> &items,const string tipText,bool isEditable)
{
	if(parent==NULL)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- used CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
	FXComboTextParamValue *comboTextEntry=new FXComboTextParamValue((FXPacker *)parent,0,name.c_str(),items,isEditable);
	comboTextEntry->setTipText(tipText.c_str());
	parameters.push_back(make_pair(ptComboText,(void *)comboTextEntry));
	retValueConvs.push_back(NULL);
}

FXComboTextParamValue *CActionParamDialog::getComboText(const string name)
{
	for(size_t t=0;t<parameters.size();t++)
	{
		if(parameters[t].first==ptComboText && ((FXComboTextParamValue *)parameters[t].second)->getTitle()==name)
			return (FXComboTextParamValue *)parameters[t].second;
	}
	throw runtime_error(string(__func__)+" -- no combo text param found named "+name);
}


void CActionParamDialog::addCheckBoxEntry(void *parent,const string name,const bool checked,const string tipText)
{
	if(parent==NULL)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- used CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
	FXCheckBoxParamValue *checkBoxEntry=new FXCheckBoxParamValue((FXPacker *)parent,0,name.c_str(),checked);
	checkBoxEntry->setTipText(tipText.c_str());
	parameters.push_back(make_pair(ptCheckBox,(void *)checkBoxEntry));
	retValueConvs.push_back(NULL);
}

void CActionParamDialog::addGraph(void *parent,const string name,const string horzAxisLabel,const string horzUnits,FXGraphParamValue::f_at_xs horzInterpretValue,FXGraphParamValue::f_at_xs horzUninterpretValue,const string vertAxisLabel,const string vertUnits,FXGraphParamValue::f_at_xs vertInterpretValue,FXGraphParamValue::f_at_xs vertUninterpretValue,f_at_x optRetValueConv,const int minScalar,const int maxScalar,const int initialScalar)
{
	if(parent==NULL)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- used CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
	FXGraphParamValue *graph=new FXGraphParamValue(name.c_str(),minScalar,maxScalar,initialScalar,(FXPacker *)parent,LAYOUT_FILL_X|LAYOUT_FILL_Y);
	graph->setHorzParameters(horzAxisLabel,horzUnits,horzInterpretValue,horzUninterpretValue);
	graph->setVertParameters(vertAxisLabel,vertUnits,vertInterpretValue,vertUninterpretValue);
	parameters.push_back(make_pair(ptGraph,(void *)graph));
	retValueConvs.push_back(optRetValueConv);
}

void CActionParamDialog::addGraphWithWaveform(void *parent,const string name,const string vertAxisLabel,const string vertUnits,FXGraphParamValue::f_at_xs vertInterpretValue,FXGraphParamValue::f_at_xs vertUninterpretValue,f_at_x optRetValueConv,const int minScalar,const int maxScalar,const int initialScalar)
{
	if(parent==NULL)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- used CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
	FXGraphParamValue *graph=new FXGraphParamValue(name.c_str(),minScalar,maxScalar,initialScalar,(FXPacker *)parent,LAYOUT_FILL_X|LAYOUT_FILL_Y);
	graph->setVertParameters(vertAxisLabel,vertUnits,vertInterpretValue,vertUninterpretValue);
	parameters.push_back(make_pair(ptGraphWithWaveform,(void *)graph));
	retValueConvs.push_back(optRetValueConv);
}

FXGraphParamValue *CActionParamDialog::getGraphParam(const string name)
{
	for(size_t t=0;t<parameters.size();t++)
	{
		if((parameters[t].first==ptGraph || parameters[t].first==ptGraphWithWaveform) && ((FXGraphParamValue *)parameters[t].second)->getTitle()==name)
			return (FXGraphParamValue *)parameters[t].second;
	}
	throw runtime_error(string(__func__)+" -- no graph param found named "+name);
}

void CActionParamDialog::addLFO(void *parent,const string name,const string ampUnits,const string ampTitle,const double maxAmp,const string freqUnits,const double maxFreq,const bool hideBipolarLFOs)
{
	if(parent==NULL)
		throw runtime_error(string(__func__)+" -- parent was passed NULL -- used CActionParameValue::newHorzPanel() or newVertPanel() to obtain a parent parameter to pass");
	FXLFOParamValue *LFOEntry=new FXLFOParamValue((FXPacker *)parent,0,name.c_str(),ampUnits,ampTitle,maxAmp,freqUnits,maxFreq,hideBipolarLFOs);
	//LFOEntry->setTipText(tipText.c_str());
	parameters.push_back(make_pair(ptLFO,(void *)LFOEntry));
	retValueConvs.push_back(NULL);
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
		((FXComboTextParamValue *)parameters[index].second)->setValue((FXint)value);
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

	default:
		throw runtime_error(string(__func__)+" -- unhandled or unimplemented parameter type: "+istring(parameters[index].first));
	}
}

#if 0
void CActionParamDialog::setControlHeight(size_t index,const size_t height)
{
	switch(parameters[index].first)
	{
	case ptConstant:
		((FXConstantParamValue *)parameters[index].second)->setHeight(height);
		break;

	case ptNumericText:
	case ptStringText:
		((FXTextParamValue *)parameters[index].second)->setHeight(height);
		break;

	case ptDiskEntity:
		((FXDiskEntityParamValue *)parameters[index].second)->setHeight(height);
		break;

	case ptComboText:
		((FXComboTextParamValue *)parameters[index].second)->setHeight(height);
		break;

	case ptCheckBox:
		((FXCheckBoxParamValue *)parameters[index].second)->setHeight(height);
		break;

	case ptGraph:
	case ptGraphWithWaveform:
		((FXGraphParamValue *)parameters[index].second)->setHeight(height);
		break;

	case ptLFO:
		((FXGraphParamValue *)parameters[index].second)->setHeight(height);
		break;

	default:
		throw runtime_error(string(__func__)+" -- unhandled or unimplemented parameter type: "+istring(parameters[index].first));
	}
}

const size_t CActionParamDialog::getControlHeight(size_t index) const
{
	switch(parameters[index].first)
	{
	case ptConstant:
		return ((FXConstantParamValue *)parameters[index].second)->getHeight();

	case ptNumericText:
	case ptStringText:
		return ((FXTextParamValue *)parameters[index].second)->getHeight();

	case ptDiskEntity:
		return ((FXDiskEntityParamValue *)parameters[index].second)->getHeight();

	case ptComboText:
		return ((FXComboTextParamValue *)parameters[index].second)->getHeight();

	case ptCheckBox:
		return ((FXCheckBoxParamValue *)parameters[index].second)->getHeight();

	case ptGraph:
	case ptGraphWithWaveform:
		return ((FXGraphParamValue *)parameters[index].second)->getHeight();

	case ptLFO:
		return ((FXGraphParamValue *)parameters[index].second)->getHeight();

	default:
		throw runtime_error(string(__func__)+" -- unhandled or unimplemented parameter type: "+istring(parameters[index].first));
	}
}
#endif

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
		((FXGraphParamValue *)parameters[index].second)->setTipText(tipText.c_str());
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
			((FXGraphParamValue *)parameters[index].second)->show();
		else
			((FXGraphParamValue *)parameters[index].second)->hide();
		break;

	default:
		throw runtime_error(string(__func__)+" -- unhandled or unimplemented parameter type: "+istring(parameters[index].first));
	}
}

bool CActionParamDialog::show(CActionSound *actionSound,CActionParameters *actionParameters)
{
	bool retval=false;

	// restore the splitter's position
	const FXint h=atoi(gSettingsRegistry->getValue((FXString("SplitterPositions")+DOT+getTitle()).text()).c_str());
	if(presetsFrame!=NULL)
		presetsFrame->setHeight(h);


	// initialize all the graphs to this sound
	for(size_t t=0;t<parameters.size();t++)
	{
		if(parameters[t].first==ptGraphWithWaveform)
			((FXGraphParamValue *)parameters[t].second)->setSound(actionSound->sound,actionSound->start,actionSound->stop);
	}

	if(execute(PLACEMENT_CURSOR))
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

					actionParameters->addDoubleParameter(slider->getTitle(),ret);
				}
				break;

			case ptNumericText:
				{
					FXTextParamValue *textEntry=(FXTextParamValue *)parameters[t].second;
					double ret=textEntry->getValue();

					if(retValueConvs[t]!=NULL)
						ret=retValueConvs[t](ret);

					actionParameters->addDoubleParameter(textEntry->getTitle(),ret);	
				}
				break;

			case ptStringText:
				{
					FXTextParamValue *textEntry=(FXTextParamValue *)parameters[t].second;
					const string ret=textEntry->getText();
					actionParameters->addStringParameter(textEntry->getTitle(),ret);	
				}
				break;

			case ptDiskEntity:
				{
					FXDiskEntityParamValue *diskEntityEntry=(FXDiskEntityParamValue *)parameters[t].second;
					const string ret=diskEntityEntry->getEntityName();

					actionParameters->addStringParameter(diskEntityEntry->getTitle(),ret);	

					if(diskEntityEntry->getEntityType()==FXDiskEntityParamValue::detAudioFilename)
						actionParameters->addBoolParameter(diskEntityEntry->getTitle()+" OpenAsRaw",diskEntityEntry->getOpenAsRaw());	
				}
				break;

			case ptComboText:
				{
					FXComboTextParamValue *comboTextEntry=(FXComboTextParamValue *)parameters[t].second;
					FXint ret=comboTextEntry->getValue();

					actionParameters->addUnsignedParameter(comboTextEntry->getTitle(),(unsigned)ret);	
				}
				break;

			case ptCheckBox:
				{
					FXCheckBoxParamValue *checkBoxEntry=(FXCheckBoxParamValue *)parameters[t].second;
					bool ret=checkBoxEntry->getValue();

					actionParameters->addBoolParameter(checkBoxEntry->getTitle(),ret);	
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

					actionParameters->addGraphParameter(graph->getTitle(),nodes);
				}
				break;

			case ptLFO:
				{
					FXLFOParamValue *LFOEntry=(FXLFOParamValue *)parameters[t].second;
					actionParameters->addLFODescription(LFOEntry->getTitle(),LFOEntry->getValue());
				}
				break;

			default:
				throw runtime_error(string(__func__)+" -- unhandled parameter type: "+istring(parameters[t].first));
			}
		}

		retval=true;
	}

	// save the splitter's position
	if(presetsFrame!=NULL)
	{
		FXint h2=presetsFrame->getHeight();
		gSettingsRegistry->createKey((FXString("SplitterPositions")+DOT+getTitle()).text(),istring(h2));
	}

	hide(); // hide now and ... 
#if FOX_MAJOR>0 // stupid debian's old crap
	getApp()->repaint(); // force redraws from disappearing dialogs now
#endif

	return retval;
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
		const string title=string(getTitle().text())+DOT+name;

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
	if(FXInputDialog::getString(_name,this,"Preset Name","Preset Name"))
	{
		if(_name.trim()=="")
		{
			Error("Invalid Preset Name");
			goto askAgain;
		}

		string name=_name.text();

		// make sure it doesn't contain DOT
		if(name.find(*DOT)!=string::npos)
		{
			Error("Preset Name cannot contain '"+string(DOT)+"'");
			goto askAgain;
		}

		try
		{
			CNestedDataFile *presetsFile=gUserPresetsFile;


			const string title=string(getTitle().text())+DOT+name;

			bool alreadyExists=false;
			if(presetsFile->keyExists(title.c_str()))
			{
				alreadyExists=true;
				if(Question("Overwrite Existing Preset '"+name+"'",yesnoQues)!=yesAns)
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

				default:
					throw runtime_error(string(__func__)+" -- unhandled parameter type: "+istring(parameters[t].first));
				}
			}

			if(!alreadyExists)
			{
				const string key=(getTitle()+DOT+"names").text();
				presetsFile->createArrayKey(key.c_str(),presetsFile->getArraySize(key.c_str()),name);
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
		if(Question("Remove Preset '"+name+"'",yesnoQues)==yesAns)
		{
			CNestedDataFile *presetsFile=gUserPresetsFile;

			const string key=string(getTitle().text())+DOT+name;

			presetsFile->removeKey(key.c_str());
			presetsFile->removeArrayKey((getTitle()+DOT+"names").text(),userPresetList->getCurrentItem());

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
	if(nativePresetList!=NULL)
	{
		try
		{
			buildPresetList(gSysPresetsFile,nativePresetList);
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
	}
	catch(exception &e)
	{
		Error(e.what());
		userPresetList->clearItems();
	}
	
}

void CActionParamDialog::buildPresetList(CNestedDataFile *f,FXList *list)
{
	const size_t presetCount=f->getArraySize((getTitle()+DOT+"names").text());
	list->clearItems();
	for(size_t t=0;t<presetCount;t++)
	{
		const string name=f->getArrayValue((getTitle()+DOT+"names").text(),t);
		list->appendItem((istring(t,3,true)+" "+name).c_str());
	}
}


unsigned CActionParamDialog::findParamByName(const string name) const
{
	for(unsigned t=0;t<parameters.size();t++)
	{
		switch(parameters[t].first)
		{
		case ptConstant:
			if(((FXConstantParamValue *)parameters[t].second)->getTitle()==name)
				return t;
			break;

		case ptNumericText:
		case ptStringText:
			if(((FXTextParamValue *)parameters[t].second)->getTitle()==name)
				return t;
			break;

		case ptDiskEntity:
			if(((FXDiskEntityParamValue *)parameters[t].second)->getTitle()==name)
				return t;
			break;

		case ptComboText:
			if(((FXComboTextParamValue *)parameters[t].second)->getTitle()==name)
				return t;
			break;

		case ptCheckBox:
			if(((FXCheckBoxParamValue *)parameters[t].second)->getTitle()==name)
				return t;
			break;

		case ptGraph:
		case ptGraphWithWaveform:
			if(((FXGraphParamValue *)parameters[t].second)->getTitle()==name)
				return t;
			break;

		case ptLFO:
			if(((FXLFOParamValue *)parameters[t].second)->getTitle()==name)
				return t;
			break;

		default:
			throw runtime_error(string(__func__)+" -- unhandled parameter type: "+istring(parameters[t].first));
		}
	}

	throw runtime_error(string(__func__)+" -- no param found named "+name);
}

