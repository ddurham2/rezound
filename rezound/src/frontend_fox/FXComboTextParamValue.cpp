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

#include "FXComboTextParamValue.h"

#include <stdlib.h>

#include <algorithm> 

#include <istring>

#include <CNestedDataFile/CNestedDataFile.h>

/*
	- This is the text entry widget used over and over by ReZound on action dialogs
	- Its purpose is to select a constant value for a parameter to an action
*/

FXDEFMAP(FXComboTextParamValue) FXComboTextParamValueMap[]=
{
	//Message_Type				ID					Message_Handler

	//FXMAPFUNC(SEL_COMMAND,			FXComboTextParamValue::ID_VALUE_SPINNER,	FXComboTextParamValue::onValueSpinnerChange),
	//FXMAPFUNC(SEL_COMMAND,			FXComboTextParamValue::ID_VALUE_TEXTBOX,	FXComboTextParamValue::onValueTextBoxChange),
};

FXIMPLEMENT(FXComboTextParamValue,FXVerticalFrame,FXComboTextParamValueMap,ARRAYNUMBER(FXComboTextParamValueMap))

FXComboTextParamValue::FXComboTextParamValue(FXComposite *p,int opts,const char *title,const vector<string> &items) :
	FXVerticalFrame(p,opts|FRAME_RIDGE | LAYOUT_FILL_X|LAYOUT_CENTER_Y,0,0,0,0, 6,6,2,4, 2,0),

	titleLabel(new FXLabel(this,title,NULL,LABEL_NORMAL|LAYOUT_CENTER_Y|LAYOUT_CENTER_X)),
	valueComboBox(new FXComboBox(this,8,min((size_t)items.size(),(size_t)8),NULL,0, COMBOBOX_NORMAL|COMBOBOX_STATIC | FRAME_SUNKEN|FRAME_THICK | LAYOUT_CENTER_Y|LAYOUT_FILL_X))
{
	setItems(items);
}

const FXint FXComboTextParamValue::getValue()
{
	return(valueComboBox->getCurrentItem());
}

void FXComboTextParamValue::setValue(const FXint value)
{
	valueComboBox->setCurrentItem(value);
}

void FXComboTextParamValue::setItems(const vector<string> &items)
{
	valueComboBox->clearItems();
	for(size_t t=0;t<items.size();t++)
		valueComboBox->appendItem(items[t].c_str());
	valueComboBox->setCurrentItem(0);
}

const string FXComboTextParamValue::getTitle() const
{
	return(titleLabel->getText().text());
}

void FXComboTextParamValue::setHelpText(const FXString &text)
{
	titleLabel->setTipText(text);	
	valueComboBox->setTipText(text);
}

FXString FXComboTextParamValue::getHelpText() const
{
	return(titleLabel->getHelpText());	
}

void FXComboTextParamValue::readFromFile(const string &prefix,CNestedDataFile &f)
{
	const string key=prefix+"."+getTitle()+".index";
	if(f.keyExists(key.c_str()))
	{
		const string v=f.getValue(key.c_str());
		setValue(atoi(v.c_str()));
	}
	else
		setValue(0); // ??? would use initialIndex if there were such a thing
}

void FXComboTextParamValue::writeToFile(const string &prefix,CNestedDataFile &f)
{
	const string key=prefix+"."+getTitle()+".";
	f.createKey((key+"index").c_str(),istring(getValue()));
}


