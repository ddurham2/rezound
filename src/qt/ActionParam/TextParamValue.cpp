/* 
 * Copyright (C) 2007 - David W. Durham
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

#include "TextParamValue.h"

#include <stdlib.h>

#include <istring>

#include <CNestedDataFile/CNestedDataFile.h>

/*
	- This is the text entry widget used over and over by ReZound on action dialogs
	- Its purpose is to select a constant text or double-numeric value for a parameter to an action

	??? it would make more since to break this into TextParamValue and CDoubleParamValue
*/

TextParamValue::TextParamValue(const char *_name,const double _initialValue,const double _minValue,const double _maxValue) :
	name(_name),

	isNumeric(true),
	
	initialValue(istring(_initialValue))
{
	setupUi(this);
	valueTextBox->hide();

	titleLabel->setText(gettext(_name));
	
	valueSpinner->setDecimals(4);
	valueSpinner->setRange(_minValue,_maxValue);
	valueSpinner->setAccelerated(true);

	setValue(_initialValue);
}

TextParamValue::TextParamValue(const char *_name,const string _initialValue) :
	name(_name),

	isNumeric(false),

	initialValue(_initialValue)
{
	setupUi(this);
	valueSpinner->hide();

	titleLabel->setText(gettext(_name));

	setText(_initialValue);
}

TextParamValue::~TextParamValue()
{
}

void TextParamValue::showFrame(bool v)
{
	frame->setFrameShape(v ? QFrame::StyledPanel : QFrame::NoFrame);
}

void TextParamValue::enableMargin(bool v)
{
	frame->layout()->setContentsMargins(v?6:0, v?6:0, v?6:0, v?6:0);
}

void TextParamValue::setUnits(const string units,const string tipText)
{
	valueSpinner->setSuffix(units.c_str());
	valueSpinner->setToolTip(tipText.c_str());
}

void TextParamValue::on_valueSpinner_valueChanged(double v)
{
	Q_EMIT changed();
}

void TextParamValue::on_valueTextBox_textChanged(const QString &text)
{
	// just verify that a value character was typed???
	Q_EMIT changed();
}

const double TextParamValue::getValue()
{
	return valueSpinner->value();
}

void TextParamValue::setValue(const double value)
{
	valueSpinner->setValue(value);
	Q_EMIT changed();
}

const string TextParamValue::getText()
{
	return valueTextBox->text().toStdString();
}

void TextParamValue::setText(const string value)
{
	valueTextBox->setText(value.c_str());
	Q_EMIT changed();
}

void TextParamValue::setRange(const double _minValue,const double _maxValue)
{
	valueSpinner->setRange(_minValue,_maxValue);
	Q_EMIT changed();
}

void TextParamValue::setStep(const double step)
{
	valueSpinner->setSingleStep(step);
}

const double TextParamValue::getStep() const
{
	return valueSpinner->singleStep();
}

void TextParamValue::setDecimals(const int decimals)
{
	valueSpinner->setDecimals(decimals);
}

const int TextParamValue::getDecimals() const
{
	return valueSpinner->decimals();
}

const string TextParamValue::getName() const
{
	return name;
}

void TextParamValue::setTipText(const string &text)
{
	titleLabel->setToolTip(text.c_str());	
	valueTextBox->setToolTip(text.c_str());
	valueSpinner->setToolTip(text.c_str());
}

string TextParamValue::getTipText() const
{
	return titleLabel->toolTip().toStdString();
}

void TextParamValue::readFromFile(const string &prefix,CNestedDataFile *f)
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
	Q_EMIT changed();
}

void TextParamValue::writeToFile(const string &prefix,CNestedDataFile *f)
{
	const string key=prefix DOT getName();
	if(isNumeric)
		f->setValue<double>(key DOT "value",getValue());
	else
		f->setValue<string>(key DOT "value",getText());
}

