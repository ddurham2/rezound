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

#include <istring>

#include <CNestedDataFile/CNestedDataFile.h>
#define DOT (CNestedDataFile::delimChar)

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

FXTextParamValue::FXTextParamValue(FXComposite *p,int opts,const char *title,const double _minValue,const double _maxValue) :
	FXHorizontalFrame(p,opts|FRAME_RIDGE | LAYOUT_FILL_X|LAYOUT_CENTER_Y,0,0,0,0, 6,6,2,4, 2,0),

	minValue(_minValue),
	maxValue(_maxValue),

	titleLabel(new FXLabel(this,title,NULL,LABEL_NORMAL|LAYOUT_CENTER_Y)),
	valueTextBox(new FXTextField(this,8,this,ID_VALUE_TEXTBOX, TEXTFIELD_NORMAL | LAYOUT_CENTER_Y|LAYOUT_FILL_X)),
	valueSpinner(new FXSpinner(this,0,this,ID_VALUE_SPINNER, SPIN_NORMAL|SPIN_NOTEXT | LAYOUT_FILL_Y)),
	unitsLabel(new FXLabel(this,"",NULL,LABEL_NORMAL|LAYOUT_CENTER_Y))
{
	valueSpinner->setRange(-10,10);
}

void FXTextParamValue::setUnits(const FXString units,const FXString helpText)
{
	unitsLabel->setText(units);
	unitsLabel->setTipText(helpText);
}

long FXTextParamValue::onValueSpinnerChange(FXObject *sender,FXSelector sel,void *ptr)
{
	double v=atof(valueTextBox->getText().text());
	v+=valueSpinner->getValue();
	valueSpinner->setValue(0);
	valueTextBox->setText(istring(v).c_str());

	validateRange();
	return(1);
}

long FXTextParamValue::onValueTextBoxChange(FXObject *sender,FXSelector sel,void *ptr)
{
	// just verity that a value character was typed???
	
	validateRange();
	return(1);
}

const double FXTextParamValue::getValue()
{
	validateRange();
	return(atof(valueTextBox->getText().text()));
}

void FXTextParamValue::setValue(const double value)
{
	valueTextBox->setText(istring(value).c_str());
	validateRange();
}

void FXTextParamValue::setRange(const double _minValue,const double _maxValue)
{
	minValue=_minValue;
	maxValue=_maxValue;
	validateRange();
}

void FXTextParamValue::validateRange()
{
	double v=atof(valueTextBox->getText().text());

	if(v<minValue)
		v=minValue;
	else if(v>maxValue)
		v=maxValue;

	valueTextBox->setText(istring(v).c_str());
}

const string FXTextParamValue::getTitle() const
{
	return(titleLabel->getText().text());
}

void FXTextParamValue::setHelpText(const FXString &text)
{
	titleLabel->setTipText(text);	
	valueTextBox->setTipText(text);
	valueSpinner->setTipText(text);
}

FXString FXTextParamValue::getHelpText() const
{
	return(titleLabel->getHelpText());	
}

void FXTextParamValue::readFromFile(const string &prefix,CNestedDataFile &f)
{
	const string key=prefix+DOT+getTitle()+DOT;
	const string v=f.getValue((key+"value").c_str());
	setValue(atof(v.c_str()));
}

void FXTextParamValue::writeToFile(const string &prefix,CNestedDataFile &f)
{
	const string key=prefix+DOT+getTitle()+DOT;
	f.createKey((key+"value").c_str(),istring(getValue()));
}


