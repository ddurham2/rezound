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

#include "FXFilenameParamValue.h"

#include <stdlib.h>

#include <istring>

#include "CFrontendHooks.h"
#include "settings.h"

#include <CNestedDataFile/CNestedDataFile.h>
#define DOT (CNestedDataFile::delimChar)

/*
	- This is the text entry widget used over and over by ReZound on action dialogs
	- Its purpose is to select a constant value for a parameter to an action
*/

FXDEFMAP(FXFilenameParamValue) FXFilenameParamValueMap[]=
{
	//Message_Type				ID					Message_Handler

	FXMAPFUNC(SEL_COMMAND,			FXFilenameParamValue::ID_FILENAME_TEXTBOX,	FXFilenameParamValue::onFilenameTextBoxChange),
	FXMAPFUNC(SEL_COMMAND,			FXFilenameParamValue::ID_BROWSE_BUTTON,		FXFilenameParamValue::onBrowseButton),
};

FXIMPLEMENT(FXFilenameParamValue,FXHorizontalFrame,FXFilenameParamValueMap,ARRAYNUMBER(FXFilenameParamValueMap))

FXFilenameParamValue::FXFilenameParamValue(FXComposite *p,int opts,const char *title,const string filename) :
	FXHorizontalFrame(p,opts|FRAME_RIDGE | LAYOUT_FILL_X|LAYOUT_CENTER_Y,0,0,0,0, 6,6,2,4, 2,0),

	titleLabel(new FXLabel(this,title,NULL,LABEL_NORMAL|LAYOUT_CENTER_Y)),
	filenameTextBox(new FXTextField(this,8,this,ID_FILENAME_TEXTBOX, TEXTFIELD_NORMAL | LAYOUT_CENTER_Y|LAYOUT_FILL_X)),
	browseButton(new FXButton(this,"&Browse",NULL,this,ID_BROWSE_BUTTON))
{
	filenameTextBox->setText(filename.c_str());
}

long FXFilenameParamValue::onFilenameTextBoxChange(FXObject *sender,FXSelector sel,void *ptr)
{
	// just verify something ???
	return 1;
}

long FXFilenameParamValue::onBrowseButton(FXObject *sender,FXSelector sel,void *ptr)
{
	string filename;
	bool dummy=false;
	if(gFrontendHooks->promptForOpenSoundFilename(filename,dummy))
		setFilename(filename);

	return 1;
}

const string FXFilenameParamValue::getFilename()
{
	return decodeFilenamePresetParameter(filenameTextBox->getText().text());
}

void FXFilenameParamValue::setFilename(const string filename)
{
	filenameTextBox->setText(filename.c_str());
}

const string FXFilenameParamValue::getTitle() const
{
	return(titleLabel->getText().text());
}

void FXFilenameParamValue::setTipText(const FXString &text)
{
	titleLabel->setTipText(text);	
	filenameTextBox->setTipText(text);
}

FXString FXFilenameParamValue::getTipText() const
{
	return(titleLabel->getTipText());	
}

void FXFilenameParamValue::readFromFile(const string &prefix,CNestedDataFile *f)
{
	// translate env vars?

	const string key=prefix+DOT+getTitle();
	const string v=f->getValue(key.c_str());
	setFilename(v);
}

void FXFilenameParamValue::writeToFile(const string &prefix,CNestedDataFile *f)
{
	const string key=prefix+DOT+getTitle();
	f->createKey(key.c_str(),encodeFilenamePresetParameter(filenameTextBox->getText().text()));
}


