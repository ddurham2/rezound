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

#include "FXTextParamValue.h"

#include <stdlib.h>
#include <math.h> // for log10

#include <istring>

#include <CNestedDataFile/CNestedDataFile.h>

#include "utils.h"

/*
	- This is the text entry widget used over and over by ReZound on action dialogs
	- Its purpose is to select a constant value for a parameter to an action
*/

FXDEFMAP(FXTextParamValue) FXTextParamValueMap[]=
{
	//Message_Type				ID					Message_Handler

	FXMAPFUNC(SEL_COMMAND,			FXTextParamValue::ID_VALUE_SPINNER,	FXTextParamValue::onValueSpinnerChange),
	FXMAPFUNC(SEL_COMMAND,			FXTextParamValue::ID_VALUE_TEXTBOX,	FXTextParamValue::onValueTextBoxChange),
};

FXIMPLEMENT(FXTextParamValue,FXHorizontalFrame,FXTextParamValueMap,ARRAYNUMBER(FXTextParamValueMap))

FXTextParamValue::FXTextParamValue(FXComposite *p,int opts,const char *_name,const double _initialValue,const double _minValue,const double _maxValue) :
	FXHorizontalFrame(p,opts|FRAME_RAISED | LAYOUT_FILL_X|LAYOUT_CENTER_Y,0,0,0,0, 2,2,4,4, 0,0),

	name(_name),

	isNumeric(true),
	
	initialValue(istring(_initialValue)),
	minValue(_minValue),
	maxValue(_maxValue),

	panel(new FXHorizontalFrame(this,FRAME_NONE|LAYOUT_FILL_Y|LAYOUT_CENTER_X,0,0,0,0, 0,0,0,0, 0,0)),
		titleLabel(new FXLabel(panel,gettext(_name),NULL,LABEL_NORMAL|LAYOUT_CENTER_Y)),
		valueTextBox(new FXTextField(panel,8,this,ID_VALUE_TEXTBOX, TEXTFIELD_NORMAL | LAYOUT_CENTER_Y)),
		valueSpinner(new FXSpinner(panel,0,this,ID_VALUE_SPINNER, SPIN_NORMAL|SPIN_NOTEXT | LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0)),
		unitsLabel(new FXLabel(panel,"",NULL,LABEL_NORMAL|LAYOUT_CENTER_Y)),

	textFont(getApp()->getNormalFont())
{
	valueTextBox->setNumColumns((FXint)(log10(maxValue)+1));

	// create a smaller font to use 
        FXFontDesc d;
        textFont->getFontDesc(d);
        d.size-=10;
        textFont=new FXFont(getApp(),d);

	valueSpinner->setRange(-10,10);
	setValue(_initialValue);

	//setFontOfAllChildren(this,textFont);
}

FXTextParamValue::FXTextParamValue(FXComposite *p,int opts,const char *_name,const string _initialValue) :
	FXHorizontalFrame(p,opts|FRAME_RAISED | LAYOUT_FILL_X|LAYOUT_CENTER_Y,0,0,0,0, 2,2,4,4, 0,0),

	name(_name),

	isNumeric(false),

	initialValue(_initialValue),

	titleLabel(new FXLabel(this,gettext(_name),NULL,LABEL_NORMAL|LAYOUT_CENTER_Y)),
	valueTextBox(new FXTextField(this,8,this,ID_VALUE_TEXTBOX, TEXTFIELD_NORMAL | LAYOUT_CENTER_Y|LAYOUT_FILL_X)),
	valueSpinner(NULL),
	unitsLabel(NULL),

	textFont(getApp()->getNormalFont())
{
	// create a smaller font to use 
        FXFontDesc d;
        textFont->getFontDesc(d);
        d.size-=10;
        textFont=new FXFont(getApp(),d);

	//setFontOfAllChildren(this,textFont);
	setText(_initialValue);
}

FXTextParamValue::~FXTextParamValue()
{
	delete textFont;
}

void FXTextParamValue::setUnits(const FXString units,const FXString tipText)
{
	if(isNumeric)
	{
		unitsLabel->setText(units);
		unitsLabel->setTipText(tipText);
	}
}

long FXTextParamValue::onValueSpinnerChange(FXObject *sender,FXSelector sel,void *ptr)
{
	double v=atof(valueTextBox->getText().text());
	v+=valueSpinner->getValue();
	valueSpinner->setValue(0);
	valueTextBox->setText(istring(v).c_str());

	validateRange();
	
	changed();
	return 1;
}

long FXTextParamValue::onValueTextBoxChange(FXObject *sender,FXSelector sel,void *ptr)
{
	// just verity that a value character was typed???
	
	validateRange();
	changed();
	return 1;
}

const double FXTextParamValue::getValue()
{
	validateRange();
	return atof(valueTextBox->getText().text());
}

void FXTextParamValue::setValue(const double value)
{
	valueTextBox->setText(istring(value).c_str());
	validateRange();
	changed();
}

const string FXTextParamValue::getText()
{
	validateRange();
	return valueTextBox->getText().text();
}

void FXTextParamValue::setText(const string value)
{
	validateRange();
	valueTextBox->setText(value.c_str());
	changed();
}

void FXTextParamValue::setRange(const double _minValue,const double _maxValue)
{
	minValue=_minValue;
	maxValue=_maxValue;
	valueTextBox->setNumColumns((FXint)(log10(maxValue)+1));
	validateRange();
	changed();
}

void FXTextParamValue::validateRange()
{
	if(!isNumeric)
		return;

	double v=atof(valueTextBox->getText().text());

	if(v<minValue)
		v=minValue;
	else if(v>maxValue)
		v=maxValue;

	valueTextBox->setText(istring(v).c_str());
}

const string FXTextParamValue::getName() const
{
	return name;
}

void FXTextParamValue::setTipText(const FXString &text)
{
	titleLabel->setTipText(text);	
	valueTextBox->setTipText(text);
	if(isNumeric)
		valueSpinner->setTipText(text);
}

FXString FXTextParamValue::getTipText() const
{
	return titleLabel->getTipText();
}

void FXTextParamValue::readFromFile(const string &prefix,CNestedDataFile *f)
{
	const string key=prefix DOT getName() DOT "value";
	if(f->keyExists(key))
	{
		if(isNumeric)
			setValue(f->getValue<double>(key));
		else
			setText(f->getValue<string>(key));
	}
	else
	{
		if(isNumeric)
			setValue(atof(initialValue.c_str())); // ??? not i18n safe (i.e. if the locale was ru and the initial value was "1.234")
		else
			setText(initialValue);
	}
	changed();
}

void FXTextParamValue::writeToFile(const string &prefix,CNestedDataFile *f)
{
	const string key=prefix DOT getName();
	if(isNumeric)
		f->createValue<double>(key DOT "value",getValue());
	else
		f->createValue<string>(key DOT "value",getText());
}

void FXTextParamValue::changed()
{
	if(target)
		target->handle(this,FXSEL(SEL_COMMAND,getSelector()),NULL);
}

