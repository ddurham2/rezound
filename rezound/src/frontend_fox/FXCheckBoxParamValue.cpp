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

#include <istring>

#include <CNestedDataFile/CNestedDataFile.h>
#define DOT (CNestedDataFile::delimChar)

#include "utils.h"

/*
	- This is the text entry widget used over and over by ReZound on action dialogs
	- Its purpose is to select a boolean value for a parameter to an action
*/

FXDEFMAP(FXCheckBoxParamValue) FXCheckBoxParamValueMap[]=
{
	//Message_Type				ID					Message_Handler

	//FXMAPFUNC(SEL_COMMAND,			FXCheckBoxParamValue::ID_VALUE_SPINNER,	FXCheckBoxParamValue::onValueSpinnerChange),
	//FXMAPFUNC(SEL_COMMAND,			FXCheckBoxParamValue::ID_VALUE_TEXTBOX,	FXCheckBoxParamValue::onValueTextBoxChange),
};

FXIMPLEMENT(FXCheckBoxParamValue,FXVerticalFrame,FXCheckBoxParamValueMap,ARRAYNUMBER(FXCheckBoxParamValueMap))

FXCheckBoxParamValue::FXCheckBoxParamValue(FXComposite *p,int opts,const char *_name,const bool checked) :
	FXVerticalFrame(p,opts|FRAME_RAISED | LAYOUT_FILL_X,0,0,0,0, 2,2,2,2, 1,0),

	name(_name),

	checkBox(new FXCheckButton(this,gettext(_name),NULL,0,CHECKBUTTON_NORMAL)),

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
	const string key=prefix+DOT+getName()+DOT+"value";
	if(f->keyExists(key.c_str()))
	{
		const bool v= f->getValue(key.c_str())=="true" ? true : false;
		setValue(v);
	}
	else
		setValue(false);
}

void FXCheckBoxParamValue::writeToFile(const string &prefix,CNestedDataFile *f)
{
	const string key=prefix+DOT+getName()+DOT;
	f->createKey((key+"value").c_str(),getValue() ? "true" : "false");
}


