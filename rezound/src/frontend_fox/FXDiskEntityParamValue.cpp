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

#include "FXDiskEntityParamValue.h"

#include <stdlib.h>

#include <stdexcept>

#include <istring>

#include "CFrontendHooks.h"
#include "settings.h"

#include <CNestedDataFile/CNestedDataFile.h>
#define DOT (CNestedDataFile::delimChar)

#include "utils.h"

/*
	- This is the text entry widget used over and over by ReZound on action dialogs
	- Its purpose is to select a constant value for a parameter to an action
*/

FXDEFMAP(FXDiskEntityParamValue) FXDiskEntityParamValueMap[]=
{
	//Message_Type				ID					Message_Handler

	FXMAPFUNC(SEL_COMMAND,			FXDiskEntityParamValue::ID_ENTITYNAME_TEXTBOX,	FXDiskEntityParamValue::onEntityNameTextBoxChange),
	FXMAPFUNC(SEL_COMMAND,			FXDiskEntityParamValue::ID_BROWSE_BUTTON,		FXDiskEntityParamValue::onBrowseButton),
};

FXIMPLEMENT(FXDiskEntityParamValue,FXVerticalFrame,FXDiskEntityParamValueMap,ARRAYNUMBER(FXDiskEntityParamValueMap))

FXDiskEntityParamValue::FXDiskEntityParamValue(FXComposite *p,int opts,const char *_name,const string initialEntityName,DiskEntityTypes _entityType) :
	FXVerticalFrame(p,opts|FRAME_RAISED |LAYOUT_FILL_X, 0,0,0,0, 2,2,2,2, 0,0),

	name(_name),
	
	entityType(_entityType),

	hFrame(new FXHorizontalFrame(this,LAYOUT_FILL_X,0,0,0,0, 0,0,0,0, 2,0)),
		titleLabel(new FXLabel(hFrame,gettext(_name),NULL,LABEL_NORMAL|LAYOUT_CENTER_Y)),
		entityNameTextBox(new FXTextField(hFrame,8,this,ID_ENTITYNAME_TEXTBOX, TEXTFIELD_NORMAL | LAYOUT_CENTER_Y|LAYOUT_FILL_X)),
		browseButton(new FXButton(hFrame,"&Browse",NULL,this,ID_BROWSE_BUTTON)),
			// ??? if this widget is ever going to be used for anything other than sound files, then I need to conditionally show this checkButton
	openAsRawCheckButton(entityType==detAudioFilename ? new FXCheckButton(this,"Open as &Raw") : NULL),

	textFont(getApp()->getNormalFont())
{
	// create a smaller font to use 
        FXFontDesc d;
        textFont->getFontDesc(d);
        d.size-=10;
        textFont=new FXFont(getApp(),d);

	entityNameTextBox->setText(initialEntityName.c_str());

	if(entityType==detAudioFilename)
		setFontOfAllChildren(this,textFont);
}

FXDiskEntityParamValue::~FXDiskEntityParamValue()
{
	delete textFont;
}

long FXDiskEntityParamValue::onEntityNameTextBoxChange(FXObject *sender,FXSelector sel,void *ptr)
{
	// just verify something ???
	return 1;
}

long FXDiskEntityParamValue::onBrowseButton(FXObject *sender,FXSelector sel,void *ptr)
{
	switch(entityType)
	{
	case detAudioFilename:
		{
			string filename;
			bool dummy=false;
			bool openAsRaw=false;
			if(gFrontendHooks->promptForOpenSoundFilename(filename,dummy,openAsRaw))
				setEntityName(filename,openAsRaw);
		}
		break;

	case detGeneralFilename:
		throw runtime_error(string(__func__)+" -- general filename not yet implemented because it is not yet needed");
		break;

	case detDirectory:
		{
			FXDirDialog d(this,"Select Directory");
			d.setDirectory(getEntityName().c_str());
			if(d.execute())
				setEntityName(d.getDirectory().text());
		}
		break;

	default:
		throw runtime_error(string(__func__)+" -- internal error -- unhandled entityType: "+istring(entityType));
	}
	
	return 1;
}

const string FXDiskEntityParamValue::getEntityName() const
{
	return decodeFilenamePresetParameter(entityNameTextBox->getText().text());
}

const bool FXDiskEntityParamValue::getOpenAsRaw() const
{
	if(openAsRawCheckButton!=NULL)
		return openAsRawCheckButton->getCheck();
	else
		return false;
}

void FXDiskEntityParamValue::setEntityName(const string entityName,bool openAsRaw)
{
	entityNameTextBox->setText(entityName.c_str());
	if(openAsRawCheckButton!=NULL)
		openAsRawCheckButton->setCheck(openAsRaw);
}

const string FXDiskEntityParamValue::getName() const
{
	return name;
}

void FXDiskEntityParamValue::setTipText(const FXString &text)
{
	titleLabel->setTipText(text);	
	entityNameTextBox->setTipText(text);
}

FXString FXDiskEntityParamValue::getTipText() const
{
	return titleLabel->getTipText();
}

const FXDiskEntityParamValue::DiskEntityTypes FXDiskEntityParamValue::getEntityType() const
{
	return entityType;
}

void FXDiskEntityParamValue::readFromFile(const string &prefix,CNestedDataFile *f)
{
	const string key=prefix+DOT+getName();
	const string v=f->getValue(key.c_str());
	setEntityName(v,f->getValue((key+" AsRaw").c_str())=="true");
}

void FXDiskEntityParamValue::writeToFile(const string &prefix,CNestedDataFile *f)
{
	const string key=prefix+DOT+getName();
	f->createKey(key.c_str(),encodeFilenamePresetParameter(entityNameTextBox->getText().text()));

	if(openAsRawCheckButton!=NULL)
		f->createKey((key+" AsRaw").c_str(),openAsRawCheckButton->getCheck() ? "true" : "false");
}


