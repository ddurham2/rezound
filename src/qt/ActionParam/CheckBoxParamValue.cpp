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

#include "CheckBoxParamValue.h"

#include <CNestedDataFile/CNestedDataFile.h>

/*
	- This is the text entry widget used over and over by ReZound on action dialogs
	- Its purpose is to select a boolean value for a parameter to an action
*/

CheckBoxParamValue::CheckBoxParamValue(const char *_name,const bool checked) :
	name(_name)
{
	setupUi(this);

	checkBox->setText(gettext(_name));

	checkBox->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
}

CheckBoxParamValue::~CheckBoxParamValue()
{
}

void CheckBoxParamValue::showFrame(bool v)
{
	frame->setFrameShape(v ? QFrame::StyledPanel : QFrame::NoFrame);
}

void CheckBoxParamValue::enableMargin(bool v)
{
	frame->layout()->setContentsMargins(v?6:0, v?6:0, v?6:0, v?6:0);
}

void CheckBoxParamValue::on_checkBox_stateChanged(int state)
{
	Q_EMIT changed();
}

const bool CheckBoxParamValue::getValue()
{
	return checkBox->checkState()==Qt::Checked;
}

void CheckBoxParamValue::setValue(const bool checked)
{
	checkBox->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
}

const string CheckBoxParamValue::getName() const
{
	return name;
}

void CheckBoxParamValue::setTipText(const string &text)
{
	checkBox->setToolTip(text.c_str());
}

string CheckBoxParamValue::getTipText() const
{
	return checkBox->toolTip().toStdString();
}

void CheckBoxParamValue::readFromFile(const string &prefix,CNestedDataFile *f)
{
	const string key=prefix DOT getName() DOT "value";
	if(f->keyExists(key))
	{
		const bool v=f->getValue<bool>(key);
		setValue(v);
	}
	else
		setValue(false);

	Q_EMIT changed();
}

void CheckBoxParamValue::writeToFile(const string &prefix,CNestedDataFile *f)
{
	const string key=prefix DOT getName();
	f->setValue<bool>(key DOT "value",getValue());
}

