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

#define DOT (CNestedDataFile::delimChar)


FXDEFMAP(CActionParamDialog) CActionParamDialogMap[]=
{
//	Message_Type			ID							Message_Handler

	FXMAPFUNC(SEL_COMMAND,		CActionParamDialog::ID_NATIVE_PRESET_BUTTON,	CActionParamDialog::onPresetUseButton),
	FXMAPFUNC(SEL_DOUBLECLICKED,	CActionParamDialog::ID_NATIVE_PRESET_LIST,	CActionParamDialog::onPresetUseButton),

	FXMAPFUNC(SEL_COMMAND,		CActionParamDialog::ID_USER_PRESET_USE_BUTTON,	CActionParamDialog::onPresetUseButton),
	FXMAPFUNC(SEL_COMMAND,		CActionParamDialog::ID_USER_PRESET_SAVE_BUTTON,	CActionParamDialog::onPresetSaveButton),
	FXMAPFUNC(SEL_COMMAND,		CActionParamDialog::ID_USER_PRESET_REMOVE_BUTTON,CActionParamDialog::onPresetRemoveButton),
	FXMAPFUNC(SEL_DOUBLECLICKED,	CActionParamDialog::ID_USER_PRESET_LIST,	CActionParamDialog::onPresetUseButton),
};
		

FXIMPLEMENT(CActionParamDialog,FXModalDialogBox,CActionParamDialogMap,ARRAYNUMBER(CActionParamDialogMap))


// ----------------------------------------

// ??? TODO Well, I got it to not need the width, it's determined by the 
// widgets' needed widths.. I don't quite know tho, what the height won't 
// work the same what.. I'll work on figuring that out and then both 
// parameters should be unnecessary

CActionParamDialog::CActionParamDialog(FXWindow *mainWindow,const FXString title,FXModalDialogBox::FrameTypes frameType) :
	FXModalDialogBox(mainWindow,title,0,0,FXModalDialogBox::ftVertical),
	
	splitter(new FXSplitter(getFrame(),SPLITTER_VERTICAL|SPLITTER_REVERSED | LAYOUT_FILL_X|LAYOUT_FILL_Y)),
		topPanel(new FXHorizontalFrame(splitter,FRAME_RAISED|FRAME_THICK, 0,0,0,0, 0,0,0,0, 0,0)),
			leftMargin(new FXFrame(topPanel,FRAME_NONE|LAYOUT_FILL_Y|LAYOUT_FIX_WIDTH,0,0,0,0, 0,0,0,0)),
			controlsFrame(
				frameType==FXModalDialogBox::ftHorizontal 
					?
					(FXPacker *)new FXHorizontalFrame(topPanel,FRAME_NONE | LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 5,5,5,5)
					:
					(FXPacker *)new FXVerticalFrame(topPanel,FRAME_NONE | LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 5,5,5,5)
			),
			rightMargin(new FXFrame(topPanel,FRAME_NONE|LAYOUT_FILL_Y|LAYOUT_FIX_WIDTH,0,0,0,0, 0,0,0,0)),
		presetsFrame(new FXHorizontalFrame(splitter,FRAME_RAISED|FRAME_THICK | LAYOUT_FILL_X, 0,0,0,0, 5,5,5,5)),
			nativePresetList(NULL),
			userPresetList(NULL)
{
	disableFrameDecor();

	setHeight(getHeight()+100); // since we're adding this presets section, make the dialog taller

	try
	{
		if(gSysPresetsFile->getArraySize((getTitle()+DOT+"names").text())>0)
		{
			// native preset stuff
			FXPacker *listFrame=new FXPacker(presetsFrame,FRAME_SUNKEN|FRAME_THICK|LAYOUT_FIX_WIDTH | LAYOUT_FILL_Y, 0,0,200,0, 0,0,0,0, 0,0); // had to do this because FXList won't take that frame style
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
	FXPacker *listFrame=new FXPacker(presetsFrame,FRAME_SUNKEN|FRAME_THICK|LAYOUT_FIX_WIDTH | LAYOUT_FILL_Y, 0,0,200,0, 0,0,0,0, 0,0); // had to do this because FXList won't take that frame style
		userPresetList=new FXList(listFrame,4,this,ID_USER_PRESET_LIST,LIST_BROWSESELECT | LAYOUT_FILL_X|LAYOUT_FILL_Y);
	FXPacker *buttonGroup=new FXVerticalFrame(presetsFrame);
		new FXButton(buttonGroup,"&Use\tOr Double-Click an Item in the List",NULL,this,ID_USER_PRESET_USE_BUTTON,BUTTON_NORMAL|LAYOUT_FILL_X);
		new FXButton(buttonGroup,"&Save",NULL,this,ID_USER_PRESET_SAVE_BUTTON,BUTTON_NORMAL|LAYOUT_FILL_X);
		new FXButton(buttonGroup,"&Remove",NULL,this,ID_USER_PRESET_REMOVE_BUTTON,BUTTON_NORMAL|LAYOUT_FILL_X);

	buildPresetLists();

	// make sure the dialog has at least a minimum height and width
	//ASSURE_HEIGHT(this,10);
	//ASSURE_WIDTH(this,200);
}

void CActionParamDialog::addSlider(const string name,const string units,FXConstantParamValue::f_at_xs interpretValue,FXConstantParamValue::f_at_xs uninterpretValue,f_at_x optRetValueConv,const double initialValue,const int minScalar,const int maxScalar,const int initScalar,bool showInverseButton)
{
	FXConstantParamValue *slider=new FXConstantParamValue(interpretValue,uninterpretValue,minScalar,maxScalar,initScalar,showInverseButton,controlsFrame,0,name.c_str());
	slider->setUnits(units.c_str());
	slider->setValue(initialValue);
	parameters.push_back(pair<ParamTypes,void *>(ptConstant,(void *)slider));
	retValueConvs.push_back(optRetValueConv);
}

void CActionParamDialog::addTextEntry(const string name,const string units,const double initialValue,const double minValue,const double maxValue,const string unitsHelpText)
{
	FXTextParamValue *textEntry=new FXTextParamValue(controlsFrame,0,name.c_str(),minValue,maxValue);
	textEntry->setUnits(units.c_str(),unitsHelpText.c_str());
	textEntry->setValue(initialValue);
	parameters.push_back(pair<ParamTypes,void *>(ptText,(void *)textEntry));
	retValueConvs.push_back(NULL);
}


void CActionParamDialog::addComboTextEntry(const string name,const vector<string> &items,const string helpText)
{
	FXComboTextParamValue *comboTextEntry=new FXComboTextParamValue(controlsFrame,0,name.c_str(),items);
	comboTextEntry->setHelpText(helpText.c_str());
	parameters.push_back(pair<ParamTypes,void *>(ptComboText,(void *)comboTextEntry));
	retValueConvs.push_back(NULL);
}


void CActionParamDialog::addCheckBoxEntry(const string name,const bool checked,const string helpText)
{
	FXCheckBoxParamValue *checkBoxEntry=new FXCheckBoxParamValue(controlsFrame,0,name.c_str(),checked);
	checkBoxEntry->setHelpText(helpText.c_str());
	parameters.push_back(pair<ParamTypes,void *>(ptCheckBox,(void *)checkBoxEntry));
	retValueConvs.push_back(NULL);
}


void CActionParamDialog::addGraph(const string name,const string units,FXGraphParamValue::f_at_xs interpretValue,FXGraphParamValue::f_at_xs uninterpretValue,f_at_x optRetValueConv,const int minScalar,const int maxScalar,const int initialScalar)
{
		// ??? there is still a question of how quite to lay out the graph if there are graph(s) and sliders
	FXGraphParamValue *graph=new FXGraphParamValue(name.c_str(),interpretValue,uninterpretValue,minScalar,maxScalar,initialScalar,controlsFrame,LAYOUT_FILL_X|LAYOUT_FILL_Y);
	graph->setUnits(units.c_str());
	parameters.push_back(pair<ParamTypes,void *>(ptGraph,(void *)graph));
	retValueConvs.push_back(optRetValueConv);
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

	case ptText:
		((FXTextParamValue *)parameters[index].second)->setValue(value);
		break;

	case ptComboText:
		((FXComboTextParamValue *)parameters[index].second)->setValue((FXint)value);
		break;

	case ptCheckBox:
		((FXCheckBoxParamValue *)parameters[index].second)->setValue((bool)value);
		break;

	case ptGraph:
		/*
		((FXGraphParamValue *)parameters[index].second)->setValue(value);
		break;
		*/

	default:
		throw(runtime_error(string(__func__)+" -- unhandled or unimplemented parameter type: "+istring(parameters[index].first)));
	}
}

bool CActionParamDialog::show(CActionSound *actionSound,CActionParameters *actionParameters)
{
	bool retval=false;

	// restore the splitter's position
	const FXint h=atoi(gSettingsRegistry->getValue((FXString("SplitterPositions")+DOT+getTitle()).text()).c_str());
	presetsFrame->setHeight(h);


	// initialize all the graphs to this sound
	for(size_t t=0;t<parameters.size();t++)
	{
		if(parameters[t].first==ptGraph)
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

					actionParameters->addDoubleParameter(ret);	
				}
				break;

			case ptText:
				{
					FXTextParamValue *textEntry=(FXTextParamValue *)parameters[t].second;
					double ret=textEntry->getValue();

					if(retValueConvs[t]!=NULL)
						ret=retValueConvs[t](ret);

					actionParameters->addDoubleParameter(ret);	
				}
				break;

			case ptComboText:
				{
					FXComboTextParamValue *comboTextEntry=(FXComboTextParamValue *)parameters[t].second;
					FXint ret=comboTextEntry->getValue();

					actionParameters->addUnsignedParameter((unsigned)ret);	
				}
				break;

			case ptCheckBox:
				{
					FXCheckBoxParamValue *checkBoxEntry=(FXCheckBoxParamValue *)parameters[t].second;
					bool ret=checkBoxEntry->getValue();

					actionParameters->addBoolParameter(ret);	
				}
				break;

			case ptGraph:
				{
					FXGraphParamValue *graph=(FXGraphParamValue *)parameters[t].second;
					CGraphParamValueNodeList nodes=graph->getNodes();

					if(retValueConvs[t]!=NULL)
					{
						for(size_t i=0;i<nodes.size();i++)
							nodes[i].value=retValueConvs[t](nodes[i].value);
					}

					actionParameters->addGraphParameter(nodes);
				}
				break;

			default:
				throw(runtime_error(string(__func__)+" -- unhandled parameter type: "+istring(parameters[t].first)));
			}
		}

		retval=true;
	}

	// save the splitter's position
	FXint h2=presetsFrame->getHeight();
	gSettingsRegistry->createKey((FXString("SplitterPositions")+DOT+getTitle()).text(),istring(h2));

	return(retval);
}

long CActionParamDialog::onPresetUseButton(FXObject *sender,FXSelector sel,void *ptr)
{
	CNestedDataFile *presetsFile;
	FXList *listBox;
	if(SELID(sel)==ID_NATIVE_PRESET_BUTTON || SELID(sel)==ID_NATIVE_PRESET_LIST)
	{
		presetsFile=gSysPresetsFile;
		listBox=nativePresetList;
	}
	else //if(SELID(sel)==ID_USER_PRESET_BUTTON || SELID(sel)==ID_USER_PRESET_LIST)
	{
		presetsFile=gUserPresetsFile;
		listBox=userPresetList;
	}

	try
	{
		if(listBox->getCurrentItem()<0)
		{
			gStatusComm->beep();
			return(1);
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

			case ptText:
				((FXTextParamValue *)parameters[t].second)->readFromFile(title,presetsFile);
				break;

			case ptComboText:
				((FXComboTextParamValue *)parameters[t].second)->readFromFile(title,presetsFile);
				break;

			case ptCheckBox:
				((FXCheckBoxParamValue *)parameters[t].second)->readFromFile(title,presetsFile);
				break;

			case ptGraph:
				((FXGraphParamValue *)parameters[t].second)->readFromFile(title,presetsFile);
				break;

			default:
				throw(runtime_error(string(__func__)+" -- unhandled parameter type: "+istring(parameters[t].first)));
			}
		}
	}
	catch(exception &e)
	{
		Error(e.what());
	}

	return(1);
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
					return(1);
			}

			for(unsigned t=0;t<parameters.size();t++)
			{
				switch(parameters[t].first)
				{
				case ptConstant:
					((FXConstantParamValue *)parameters[t].second)->writeToFile(title,presetsFile);
					break;

				case ptText:
					((FXTextParamValue *)parameters[t].second)->writeToFile(title,presetsFile);
					break;

				case ptComboText:
					((FXComboTextParamValue *)parameters[t].second)->writeToFile(title,presetsFile);
					break;

				case ptCheckBox:
					((FXCheckBoxParamValue *)parameters[t].second)->writeToFile(title,presetsFile);
					break;

				case ptGraph:
					((FXGraphParamValue *)parameters[t].second)->writeToFile(title,presetsFile);
					break;

				default:
					throw(runtime_error(string(__func__)+" -- unhandled parameter type: "+istring(parameters[t].first)));
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

	return(1);
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

	return(1);
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


