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

#include "FXCheckBoxParamValue.h"

#include <CNestedDataFile/CNestedDataFile.h>

#include "utils.h"

/*
	- This is the text entry widget used over and over by ReZound on action dialogs
	- Its purpose is to select a boolean value for a parameter to an action
*/

FXDEFMAP(FXCheckBoxParamValue) FXCheckBoxParamValueMap[]=
{
	//Message_Type				ID					Message_Handler

	//FXMAPFUNC(SEL_COMMAND,			FXCheckBoxParamValue::ID_VALUE_SPINNER,	FXCheckBoxParamValue::onValueSpinnerChange),
	FXMAPFUNC(SEL_COMMAND,			FXCheckBoxParamValue::ID_CHECKBOX,	FXCheckBoxParamValue::onCheckBox),
};

FXIMPLEMENT(FXCheckBoxParamValue,FXVerticalFrame,FXCheckBoxParamValueMap,ARRAYNUMBER(FXCheckBoxParamValueMap))

FXCheckBoxParamValue::FXCheckBoxParamValue(FXComposite *p,int opts,const char *_name,const bool checked) :
	FXVerticalFrame(p,opts|FRAME_RAISED | LAYOUT_FILL_X,0,0,0,0, 2,2,2,2, 1,0),

	name(_name),

	checkBox(new FXCheckButton(this,gettext(_name),this,ID_CHECKBOX,CHECKBUTTON_NORMAL)),

	textFont(getApp()->getNormalFont())
{
	// create a smaller font to use 
        FXFontDesc d;
        textFont->getFontDesc(d);
        d.size-=10;
        textFont=new FXFont(getApp(),d);

	checkBox->setCheck(checked);

	//setFontOfAllChildren(this,textFont);
}

FXCheckBoxParamValue::~FXCheckBoxParamValue()
{
	delete textFont;
}

long FXCheckBoxParamValue::onCheckBox(FXObject *sender,FXSelector sel,void *ptr)
{
	return target && target->handle(this,FXSEL(SEL_COMMAND,getSelector()),ptr);
}

const bool FXCheckBoxParamValue::getValue()
{
	return checkBox->getCheck();
}

void FXCheckBoxParamValue::setValue(const bool checked)
{
	checkBox->setCheck(checked);
}

const string FXCheckBoxParamValue::getName() const
{
	return name;
}

void FXCheckBoxParamValue::setTipText(const FXString &text)
{
	checkBox->setTipText(text);
}

FXString FXCheckBoxParamValue::getTipText() const
{
	return checkBox->getTipText();	
}

void FXCheckBoxParamValue::readFromFile(const string &prefix,CNestedDataFile *f)
{
	const string key=prefix DOT getName() DOT "value";
	if(f->keyExists(key))
	{
		const bool v=f->getValue<bool>(key);
		setValue(v);
	}
	else
		setValue(false);

	onCheckBox(NULL,0,NULL);
}

void FXCheckBoxParamValue::writeToFile(const string &prefix,CNestedDataFile *f)
{
	const string key=prefix DOT getName();
	f->createValue<bool>(key DOT "value",getValue());
}


