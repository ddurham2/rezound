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

#include "utils.h"

/*
	- This is the text entry widget used over and over by ReZound on action dialogs
	- Its purpose is to select a constant value for a parameter to an action
*/

FXDEFMAP(FXComboTextParamValue) FXComboTextParamValueMap[]=
{
	//Message_Type				ID					Message_Handler

	FXMAPFUNC(SEL_COMMAND,			FXComboTextParamValue::ID_COMBOBOX,	FXComboTextParamValue::onComboBoxChange),
	//FXMAPFUNC(SEL_COMMAND,			FXComboTextParamValue::ID_VALUE_TEXTBOX,	FXComboTextParamValue::onValueTextBoxChange),
};

FXIMPLEMENT(FXComboTextParamValue,FXHorizontalFrame,FXComboTextParamValueMap,ARRAYNUMBER(FXComboTextParamValueMap))

FXComboTextParamValue::FXComboTextParamValue(FXComposite *p,int opts,const char *_name,const vector<string> &items,bool _isEditable) :
	FXHorizontalFrame(p,opts|FRAME_RAISED | LAYOUT_FILL_X|LAYOUT_CENTER_Y,0,0,0,0, 2,2,4,4, 0,0),
	asString(false),

	name(_name),

	isEditable(_isEditable),

	titleLabel(new FXLabel(this,gettext(_name),NULL,LABEL_NORMAL|LAYOUT_CENTER_Y)),
	valueComboBox(new FXComboBox(this,8,this,ID_COMBOBOX, COMBOBOX_NORMAL|(!isEditable ? COMBOBOX_STATIC : 0) | FRAME_SUNKEN|FRAME_THICK | LAYOUT_CENTER_Y|LAYOUT_FILL_X)),

	textFont(getApp()->getNormalFont())
{
	// create a smaller font to use 
        FXFontDesc d;
        textFont->getFontDesc(d);
        d.size-=10;
        textFont=new FXFont(getApp(),d);

	setItems(items);

	//setFontOfAllChildren(this,textFont);
}

FXComboTextParamValue::~FXComboTextParamValue()
{
	delete textFont;
}

long FXComboTextParamValue::onComboBoxChange(FXObject *object,FXSelector sel,void *ptr)
{
	return target && target->handle(this,FXSEL(SEL_CHANGED,getSelector()),ptr);
}

const FXint FXComboTextParamValue::getIntegerValue()
{
	if(isEditable)
		return atoi(valueComboBox->getText().text()); // ??? I think I should just return a string.. or perhaps have a getText() method and CActionParamDialog can save both values in CActionParameters one with an altername name 
	else
		return valueComboBox->getCurrentItem();
}

const string FXComboTextParamValue::getStringValue()
{
	return valueComboBox->getText().text();
}

void FXComboTextParamValue::setIntegerValue(const FXint value)
{
	if(isEditable)
		valueComboBox->setText(istring(value).c_str());
	else
		valueComboBox->setCurrentItem(value);
}

void FXComboTextParamValue::setItems(const vector<string> &items)
{
	this->items=items;
	FXint n=valueComboBox->getCurrentItem();

	valueComboBox->clearItems();
	for(size_t t=0;t<items.size();t++)
		valueComboBox->appendItem(gettext(items[t].c_str()));

	if(n>=0)
		valueComboBox->setCurrentItem(min(n,valueComboBox->getNumItems()-1));
	else if(valueComboBox->getNumItems()>0)
		valueComboBox->setCurrentItem(0);

	valueComboBox->setNumVisible(min((size_t)8,(size_t)items.size()));
}

const vector<string> FXComboTextParamValue::getItems() const
{
	/*
	vector<string> items;

	for(int t=0;t<valueComboBox->getNumItems();t++)
		items.push_back(valueComboBox->getItemText(t).text());
	*/
	return items;
}

/*
void FXComboTextParamValue::setIcons(const vector<FXIcon *> icons)
{
	if(icons.size()!=(size_t)valueComboBox->getNumItems())
		throw runtime_error(string(__func__)+" -- number of icons does not match number of items");

	for(size_t t=0;t<icons.size();t++)
		valueComboBox->setItemIcon(t,icons[t]);
}
*/

void FXComboTextParamValue::setCurrentItem(const unsigned current)
{
	valueComboBox->setCurrentItem(current);
}

const string FXComboTextParamValue::getName() const
{
	return name;
}

void FXComboTextParamValue::setTipText(const FXString &text)
{
	titleLabel->setTipText(text);	
	valueComboBox->setTipText(text);
}

FXString FXComboTextParamValue::getTipText() const
{
	return titleLabel->getTipText();
}

void FXComboTextParamValue::enable()
{
	titleLabel->enable();
	valueComboBox->enable();
}

void FXComboTextParamValue::disable()
{
	titleLabel->disable();
	valueComboBox->disable();
}

void FXComboTextParamValue::readFromFile(const string &prefix,CNestedDataFile *f)
{
	if(asString)
	{
		throw runtime_error(string(__func__)+" -- unimplemented");
		/*
		const string key=prefix DOT getName() DOT "value";
		if(f->keyExists(key))
		{
			const string s=f->getValue<string>(key);
			setStringValue(s);
		}
		else
			setStringValue(""); // ??? would use initialIndex if there were such a thing
		*/
	}
	else
	{
		const string key=prefix DOT getName() DOT "index";
		if(f->keyExists(key))
		{
			const int i=f->getValue<int>(key);
			if(i>=0 && i<valueComboBox->getNumItems())
				setIntegerValue(i);
		}
		else
			setIntegerValue(0); // ??? would use initialIndex if there were such a thing
	}
	onComboBoxChange(NULL,0,NULL);
}

void FXComboTextParamValue::writeToFile(const string &prefix,CNestedDataFile *f)
{
	const string key=prefix DOT getName();
	if(asString)
		f->setValue<string>(key DOT "value",getStringValue());
	else
		f->setValue<int>(key DOT "index",getIntegerValue());
}


