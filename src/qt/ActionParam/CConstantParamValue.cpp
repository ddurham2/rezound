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

// ??? I might want to reset the range on the scalar each time an action dialog is shown so I'd need a method to update the widget

#include "CConstantParamValue.h"

#include <QMouseEvent>

#include <stdlib.h>

#include <istring>
#include <algorithm>

#include <CNestedDataFile/CNestedDataFile.h>

//#include "../utils.h"

/*
	- This is the slider widget used over and over by ReZound on action dialogs
	- It can also be constructed not to have the slider to serve as a text entry widget
	- Its purpose is to select a constant value for a parameter to an action
*/

CConstantParamValue::CConstantParamValue(AActionParamMapper *_valueMapper,bool showInverseButton,const char *_name) :
	name(_name),

	valueMapper(_valueMapper),

	minWidth(0),
	minHeight(196)
{
	setupUi(this);

	slider->installEventFilter(this);
	halfLabel->installEventFilter(this);

	titleLabel->setText(gettext(_name));

	inverseButton->setText(_("I"));
	inverseButton->setToolTip(_("Invert Value"));

	if(valueMapper->getMinScalar()==valueMapper->getMaxScalar())
	{
		scalarLabel->hide();
		scalarSpinner->hide();
	}
	else
	{
		scalarLabel->setText(_("Scalar"));
		scalarSpinner->blockSignals(true);
		scalarSpinner->setRange(valueMapper->getMinScalar(),valueMapper->getMaxScalar());
		scalarSpinner->setValue(valueMapper->getScalar());
		scalarSpinner->blockSignals(false);
	}

	slider->setRange(0,10000);
	//slider->setHeadSize(25);
	slider->setTickInterval(10000/8);
	slider->setValue(0);

	setValue(valueMapper->getDefaultValue());

	updateNumbers();
}

CConstantParamValue::~CConstantParamValue()
{
}

void CConstantParamValue::setMinSize(int minWidth,int minHeight)
{
	setMinimumSize(minWidth,minHeight);
}

void CConstantParamValue::setUnits(const string units)
{
	unitsLabel->setText(units.c_str());
	updateNumbers();
}

#define GET_SCALAR_VALUE ( scalarSpinner==NULL ? valueMapper->getDefaultScalar() : scalarSpinner->value()  )
#define SET_SCALAR_VALUE(v) { if(scalarSpinner!=NULL) scalarSpinner->setValue(v); }

void CConstantParamValue::on_slider_valueChanged(int value)
{
	retValue=valueMapper->interpretValue((double)slider->value()/10000.0);
	if(retValue==floor(retValue))
		valueTextBox->setText((istring(floor(retValue))).c_str());
	else
		valueTextBox->setText((istring(retValue,7,4)).c_str());
}

void CConstantParamValue::on_scalarSpinner_valueChanged(int value)
{
	valueMapper->setScalar(GET_SCALAR_VALUE);
	slider->setValue((int)round(valueMapper->uninterpretValue(retValue)*10000.0));
	retValue=valueMapper->interpretValue((double)slider->value()/10000.0);
	valueTextBox->setText((istring(retValue,7,4)).c_str());
	updateNumbers();
}

void CConstantParamValue::on_valueTextBox_editingFinished()
{
		// check range if uninterpretValue does that
	retValue=valueMapper->interpretValue(valueMapper->uninterpretValue(atof(valueTextBox->text().toUtf8().constData())));
	slider->setValue((int)(valueMapper->uninterpretValue(retValue)*10000.0));
}

void CConstantParamValue::on_inverseButton_clicked()
{
	slider->setValue(slider->maximum()-slider->value()+slider->minimum());
}

bool CConstantParamValue::eventFilter(QObject *w,QEvent *e)
{
	if(
		(w==slider && e->type()==QEvent::MouseButtonPress && ((QMouseEvent*)e)->button()==Qt::MidButton) || 
		(w==halfLabel && e->type()==QEvent::MouseButtonPress && ((QMouseEvent*)e)->button()==Qt::LeftButton)
	)
	{ // middle button pressed on slider or middle value label clicked
		slider->setValue((slider->minimum()+slider->maximum())/2);
		return true;
	}
	return QWidget::eventFilter(w,e);
}

void CConstantParamValue::updateNumbers()
{
	minLabel->setText(istring(valueMapper->interpretValue(0.0),3,2).c_str()+unitsLabel->text());
	halfLabel->setText(istring(valueMapper->interpretValue(0.5),3,2).c_str()+unitsLabel->text());
	maxLabel->setText(istring(valueMapper->interpretValue(1.0),3,2).c_str()+unitsLabel->text());

	on_slider_valueChanged(slider->value());
}

const double CConstantParamValue::getValue() const
{
	return retValue;
}

void CConstantParamValue::setValue(const double value)
{
	defaultValue=value;
	prvSetValue(value);
}

void CConstantParamValue::prvSetValue(const double value)
{
	retValue=value;
	slider->setValue((int)(valueMapper->uninterpretValue(value)*10000.0));
	if(retValue==floor(retValue))
		valueTextBox->setText((istring(floor(retValue))).c_str());
	else
		valueTextBox->setText((istring(retValue,7,4)).c_str());
}

const int CConstantParamValue::getScalar() const
{
	return valueMapper->getScalar();
}

void CConstantParamValue::setScalar(const int scalar)
{
	valueMapper->setScalar(scalar);
	SET_SCALAR_VALUE(valueMapper->getScalar());
	updateNumbers();
}

const int CConstantParamValue::getMinScalar() const
{
	return scalarSpinner->minimum();
}

const int CConstantParamValue::getMaxScalar() const
{
	return scalarSpinner->maximum();
}

const string CConstantParamValue::getName() const
{
	return name;
}

void CConstantParamValue::setTipText(const string &_text)
{
	QString text=_text.c_str();

	setToolTip(text);
	titleLabel->setToolTip(text);	
	//inverseButtonFrame->setToolTip(text);	
	if(inverseButton)
		inverseButton->setToolTip(text);	
	slider->setToolTip(text);	
	//tickLableFrame->setToolTip(text);	
		maxLabel->setToolTip(text);	
		halfLabel->setToolTip(text);	
		minLabel->setToolTip(text);	
	//valuePanel->setToolTip(text);
		valueTextBox->setToolTip(text);
		unitsLabel->setToolTip(text);
	//scalarPanel->setToolTip(text);
		scalarLabel->setToolTip(text);
		scalarSpinner->setToolTip(text);
}

string CConstantParamValue::getTipText() const
{
	return toolTip().toStdString();
}

void CConstantParamValue::enable()
{
	setEnabled(true);
}

void CConstantParamValue::disable()
{
	setEnabled(false);
}

void CConstantParamValue::readFromFile(const string &prefix,CNestedDataFile *f)
{
	const string key=prefix DOT getName();

	const double value=f->keyExists(key DOT "value") ? f->getValue<double>(key DOT "value") : valueMapper->getDefaultValue();
	const int scalar=f->keyExists(key DOT "scalar") ? f->getValue<int>(key DOT "scalar") : valueMapper->getDefaultScalar();

	setScalar(scalar);
	prvSetValue(value);

}

void CConstantParamValue::writeToFile(const string &prefix,CNestedDataFile *f) const
{
	const string key=prefix DOT getName();

	f->setValue<double>(key DOT "value",getValue());
	if(getMinScalar()!=getMaxScalar())
		f->setValue<int>(key DOT "scalar",getScalar());
}


