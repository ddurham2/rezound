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

#include "CConstantParamActionDialog.h"
#include "CStatusComm.h"

#include <CNestedDataFile/CNestedDataFile.h>

#include "settings.h"
#include "CStatusComm.h"


FXDEFMAP(CConstantParamActionDialog) CConstantParamActionDialogMap[]=
{
//	Message_Type			ID							Message_Handler

	FXMAPFUNC(SEL_COMMAND,		CConstantParamActionDialog::ID_NATIVE_PRESET_BUTTON,	CConstantParamActionDialog::onPresetUseButton),
	FXMAPFUNC(SEL_DOUBLECLICKED,	CConstantParamActionDialog::ID_NATIVE_PRESET_LIST,	CConstantParamActionDialog::onPresetUseButton),

	FXMAPFUNC(SEL_COMMAND,		CConstantParamActionDialog::ID_USER_PRESET_USE_BUTTON,	CConstantParamActionDialog::onPresetUseButton),
	FXMAPFUNC(SEL_COMMAND,		CConstantParamActionDialog::ID_USER_PRESET_SAVE_BUTTON,	CConstantParamActionDialog::onPresetSaveButton),
	FXMAPFUNC(SEL_COMMAND,		CConstantParamActionDialog::ID_USER_PRESET_REMOVE_BUTTON,CConstantParamActionDialog::onPresetRemoveButton),
	FXMAPFUNC(SEL_DOUBLECLICKED,	CConstantParamActionDialog::ID_USER_PRESET_LIST,	CConstantParamActionDialog::onPresetUseButton),


	//FXMAPFUNC(SEL_COMMAND,	CConstantParamActionDialog::ID_OKAY_BUTTON,	CConstantParamActionDialog::onOkayButton),
};
		

FXIMPLEMENT(CConstantParamActionDialog,FXModalDialogBox,CConstantParamActionDialogMap,ARRAYNUMBER(CConstantParamActionDialogMap))


// ----------------------------------------

CConstantParamActionDialog::CConstantParamActionDialog(FXWindow *mainWindow,const FXString title,int w,int h) :
	FXModalDialogBox(mainWindow,title,w,h,FXModalDialogBox::ftVertical),
	
	splitter(new FXSplitter(getFrame(),SPLITTER_VERTICAL|SPLITTER_REVERSED | LAYOUT_FILL_X|LAYOUT_FILL_Y)),
		controlsFrame(new FXHorizontalFrame(splitter,FRAME_RAISED|FRAME_THICK | LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 5,5,5,5)),
		presetsFrame(new FXHorizontalFrame(splitter,FRAME_RAISED|FRAME_THICK | LAYOUT_FILL_X, 0,0,0,0, 5,5,5,5)),
			nativePresetList(NULL),
			userPresetList(NULL)
{
	disableFrameDecor();

	setHeight(getHeight()+100); // since we're adding this presets section, make the dialog taller

	// ??? it would be nice if we remembered the height of presetsFrame so that it would remember from run to run where the splitter location was

	try
	{
		if(CNestedDataFile(gSysDataDirectory+"/presets.dat").getArraySize((getTitle()+".names").text())>0)
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

void CConstantParamActionDialog::addSlider(const FXString name,const FXString units,FXConstantParamValue::f_at_xs interpretValue,FXConstantParamValue::f_at_xs uninterpretValue,f_at_x optRetValueConv,const double initialValue,const int minScalar,const int maxScalar,const int initScalar,bool showInverseButton)
{
	FXConstantParamValue *slider=new FXConstantParamValue(interpretValue,uninterpretValue,minScalar,maxScalar,initScalar,showInverseButton,controlsFrame,0,name.text());
	slider->setUnits(units);
	slider->setValue(initialValue);
	parameters.push_back(slider);
	retValueConvs.push_back(optRetValueConv);
}

void CConstantParamActionDialog::addValueEntry(const FXString name,const FXString units,const double initialValue)
{
	FXConstantParamValue *valueEntry=new FXConstantParamValue(controlsFrame,0,name.text());
	valueEntry->setUnits(units);
	valueEntry->setValue(initialValue);
	parameters.push_back(valueEntry);
	retValueConvs.push_back(NULL);
}

bool CConstantParamActionDialog::show(CActionSound *actionSound,CActionParameters *actionParameters)
{
	if(FXDialogBox::execute(PLACEMENT_CURSOR))
	{
		for(unsigned t=0;t<parameters.size();t++)
		{
			double ret=parameters[t]->getValue();

			if(retValueConvs[t]!=NULL)
				ret=retValueConvs[t](ret);

			actionParameters->addDoubleParameter(ret);	
		}
		return(true);
	}
	return(false);
}

long CConstantParamActionDialog::onPresetUseButton(FXObject *sender,FXSelector sel,void *ptr)
{
	string filename;
	FXList *listBox;
	if(SELID(sel)==ID_NATIVE_PRESET_BUTTON || SELID(sel)==ID_NATIVE_PRESET_LIST)
	{
		filename=gSysDataDirectory+"/presets.dat";
		listBox=nativePresetList;
	}
	else //if(SELID(sel)==ID_USER_PRESET_BUTTON || SELID(sel)==ID_USER_PRESET_LIST)
	{
		filename=gUserDataDirectory+"/presets.dat";
		listBox=userPresetList;
	}

	try
	{
		CNestedDataFile f(filename);

		if(listBox->getCurrentItem()<0)
		{
			gStatusComm->beep();
			return(1);
		}
		
		const string name=string(listBox->getItemText(listBox->getCurrentItem()).text()).substr(4);
		const string title=string(getTitle().text())+"."+name+".";

		for(unsigned t=0;t<parameters.size();t++)
		{
			// perhaps I should make FXConstantParamValue be able to read and write itself to a CNestedDataFile given a prefix of course
			const string key=title+parameters[t]->getTitle()+".";
			const string v=f.getValue((key+"value").c_str());
			const string s=f.getValue((key+"scalar").c_str());

			parameters[t]->setScalar(atoi(s.c_str()));
			parameters[t]->setValue(atof(v.c_str()));
		}
	}
	catch(exception &e)
	{
		Error(e.what());
	}

	return(1);
}

long CConstantParamActionDialog::onPresetSaveButton(FXObject *sender,FXSelector sel,void *ptr)
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
		try
		{
			const string filename=gUserDataDirectory+"/presets.dat";
			CNestedDataFile f(filename);


			const string title=string(getTitle().text())+"."+name+".";

			bool alreadyExists=false;
			if(f.keyExists(title.substr(0,title.length()-1).c_str()))
			{
				alreadyExists=true;
				if(Question("Overwrite Existing Preset '"+name+"'",yesnoQues)!=yesAns)
					return(1);
			}

			for(unsigned t=0;t<parameters.size();t++)
			{
				// perhaps I should make FXConstantParamValue be able to read and write itself to a CNestedDataFile given a prefix of course
				const string key=title+parameters[t]->getTitle()+".";

				f.createKey((key+"value").c_str(),istring(parameters[t]->getValue()));
				if(parameters[t]->getMinScalar()!=parameters[t]->getMaxScalar())
					f.createKey((key+"scalar").c_str(),istring(parameters[t]->getScalar()));
			}

			if(!alreadyExists)
			{
				const string key=(getTitle()+".names").text();
				f.createArrayKey(key.c_str(),f.getArraySize(key.c_str()),name);
				buildPresetList(f,userPresetList);
			}
				
			f.writeFile(filename);

		}
		catch(exception &e)
		{
			Error(e.what());
		}
	}

	return(1);
}

long CConstantParamActionDialog::onPresetRemoveButton(FXObject *sender,FXSelector sel,void *ptr)
{
	if(userPresetList->getCurrentItem()>=0)
	{
		string name=(userPresetList->getItemText(userPresetList->getCurrentItem())).mid(4,255).text();
		if(Question("Remove Preset '"+name+"'",yesnoQues)==yesAns)
		{
			const string filename=gUserDataDirectory+"/presets.dat";
			CNestedDataFile f(filename);

			const string key=string(getTitle().text())+"."+name;

			f.removeKey(key.c_str());
			f.removeArrayKey((getTitle()+".names").text(),userPresetList->getCurrentItem());

			buildPresetList(f,userPresetList);
			f.writeFile(filename);
		}
	}
	else
		gStatusComm->beep();

	return(1);
}

void CConstantParamActionDialog::buildPresetLists()
{
	if(nativePresetList!=NULL)
	{
		try
		{
			CNestedDataFile f1(gSysDataDirectory+"/presets.dat");
			buildPresetList(f1,nativePresetList);
		}
		catch(exception &e)
		{
			Error(e.what());
			nativePresetList->clearItems();
		}
	}

	try
	{
		CNestedDataFile f2(gUserDataDirectory+"/presets.dat");
		buildPresetList(f2,userPresetList);
	}
	catch(exception &e)
	{
		Error(e.what());
		userPresetList->clearItems();
	}
	
}

void CConstantParamActionDialog::buildPresetList(CNestedDataFile &f,FXList *list)
{
	const size_t presetCount=f.getArraySize((getTitle()+".names").text());
	list->clearItems();
	for(size_t t=0;t<presetCount;t++)
	{
		const string name=f.getArrayValue((getTitle()+".names").text(),t);
		list->appendItem((istring(t,3,true)+" "+name).c_str());
	}
}


