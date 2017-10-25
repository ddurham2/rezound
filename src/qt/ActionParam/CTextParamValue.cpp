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

#include "CTextParamValue.h"

#include <stdlib.h>

#include <istring>

#include <CNestedDataFile/CNestedDataFile.h>

/*
	- This is the text entry widget used over and over by ReZound on action dialogs
	- Its purpose is to select a constant text or double-numeric value for a parameter to an action

	??? it would make more since to break this into CTextParamValue and CDoubleParamValue
*/

CTextParamValue::CTextParamValue(const char *_name,const double _initialValue,const double _minValue,const double _maxValue) :
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

CTextParamValue::CTextParamValue(const char *_name,const string _initialValue) :
	name(_name),

	isNumeric(false),

	initialValue(_initialValue)
{
	setupUi(this);
	valueSpinner->hide();

	titleLabel->setText(gettext(_name));

	setText(_initialValue);
}

CTextParamValue::~CTextParamValue()
{
}

void CTextParamValue::showFrame(bool v)
{
	frame->setFrameShape(v ? QFrame::StyledPanel : QFrame::NoFrame);
}

void CTextParamValue::enableMargin(bool v)
{
	frame->layout()->setContentsMargins(v?6:0, v?6:0, v?6:0, v?6:0);
}

void CTextParamValue::setUnits(const string units,const string tipText)
{
	valueSpinner->setSuffix(units.c_str());
	valueSpinner->setToolTip(tipText.c_str());
}

void CTextParamValue::on_valueSpinner_valueChanged(double v)
{
	Q_EMIT changed();
}

void CTextParamValue::on_valueTextBox_textChanged(const QString &text)
{
	// just verify that a value character was typed???
	Q_EMIT changed();
}

const double CTextParamValue::getValue()
{
	return valueSpinner->value();
}

void CTextParamValue::setValue(const double value)
{
	valueSpinner->setValue(value);
	Q_EMIT changed();
}

const string CTextParamValue::getText()
{
	return valueTextBox->text().toStdString();
}

void CTextParamValue::setText(const string value)
{
	valueTextBox->setText(value.c_str());
	Q_EMIT changed();
}

void CTextParamValue::setRange(const double _minValue,const double _maxValue)
{
	valueSpinner->setRange(_minValue,_maxValue);
	Q_EMIT changed();
}

void CTextParamValue::setStep(const double step)
{
	valueSpinner->setSingleStep(step);
}

const double CTextParamValue::getStep() const
{
	return valueSpinner->singleStep();
}

void CTextParamValue::setDecimals(const int decimals)
{
	valueSpinner->setDecimals(decimals);
}

const int CTextParamValue::getDecimals() const
{
	return valueSpinner->decimals();
}

const string CTextParamValue::getName() const
{
	return name;
}

void CTextParamValue::setTipText(const string &text)
{
	titleLabel->setToolTip(text.c_str());	
	valueTextBox->setToolTip(text.c_str());
	valueSpinner->setToolTip(text.c_str());
}

string CTextParamValue::getTipText() const
{
	return titleLabel->toolTip().toStdString();
}

void CTextParamValue::readFromFile(const string &prefix,CNestedDataFile *f)
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

void CTextParamValue::writeToFile(const string &prefix,CNestedDataFile *f)
{
	const string key=prefix DOT getName();
	if(isNumeric)
		f->setValue<double>(key DOT "value",getValue());
	else
		f->setValue<string>(key DOT "value",getText());
}

