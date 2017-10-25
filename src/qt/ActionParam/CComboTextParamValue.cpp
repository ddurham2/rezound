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

#include "CComboTextParamValue.h"

#include <stdlib.h>

#include <algorithm> 

#include <istring>

#include <CNestedDataFile/CNestedDataFile.h>

CComboTextParamValue::CComboTextParamValue(const char *_name,const vector<string> &items,bool _isEditable) :
	asString(false),

	name(_name),

	isEditable(_isEditable)
{
	setupUi(this);

	label->setText(gettext(_name));
	comboBox->setEditable(isEditable);

	setItems(items);
}

CComboTextParamValue::~CComboTextParamValue()
{
}

void CComboTextParamValue::on_comboBox_editTextChanged(const QString &text)
{
	Q_EMIT changed();
}

void CComboTextParamValue::on_comboBox_currentIndexChanged(const QString &text)
{
	Q_EMIT changed();
}

const int CComboTextParamValue::getIntegerValue()
{
	if(isEditable)
		return comboBox->currentText().toInt();
	else
		return comboBox->currentIndex();
}

const string CComboTextParamValue::getStringValue()
{
	return comboBox->currentText().toStdString();
}

void CComboTextParamValue::setIntegerValue(const int value)
{
	if(isEditable)
		comboBox->setEditText(istring(value).c_str());
	else
		comboBox->setCurrentIndex(value);
}

void CComboTextParamValue::setItems(const vector<string> &items)
{
	this->items=items;
	int n=comboBox->currentIndex();

	comboBox->clear();
	for(size_t t=0;t<items.size();t++)
		comboBox->addItem(gettext(items[t].c_str()));

	if(n>=0)
		comboBox->setCurrentIndex(min(n,comboBox->count()-1));
	else if(comboBox->count()>0)
		comboBox->setCurrentIndex(0);

	comboBox->setMaxVisibleItems(min((size_t)8,(size_t)items.size()));
}

const vector<string> CComboTextParamValue::getItems() const
{
	/*
	vector<string> items;

	for(int t=0;t<comboBox->count();t++)
		items.push_back(comboBox->getItemText(t).currentText());
	*/
	return items;
}

/*
void CComboTextParamValue::setIcons(const vector<FXIcon *> icons)
{
	if(icons.size()!=(size_t)comboBox->count())
		throw runtime_error(string(__func__)+" -- number of icons does not match number of items");

	for(size_t t=0;t<icons.size();t++)
		comboBox->setItemIcon(t,icons[t]);
}
*/

void CComboTextParamValue::setCurrentItem(const int current)
{
	comboBox->setCurrentIndex(current);
}

const string CComboTextParamValue::getName() const
{
	return name;
}

void CComboTextParamValue::setTipText(const string &text)
{
	label->setToolTip(text.c_str());
	comboBox->setToolTip(text.c_str());
}

string CComboTextParamValue::getTipText() const
{
	return label->toolTip().toStdString();
}

void CComboTextParamValue::enable()
{
	setEnabled(true);
}

void CComboTextParamValue::disable()
{
	setEnabled(false);
}

void CComboTextParamValue::readFromFile(const string &prefix,CNestedDataFile *f)
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
			if(i>=0 && i<comboBox->count())
				setIntegerValue(i);
		}
		else
			setIntegerValue(0); // ??? would use initialIndex if there were such a thing
	}
}

void CComboTextParamValue::writeToFile(const string &prefix,CNestedDataFile *f)
{
	const string key=prefix DOT getName();
	if(asString)
		f->setValue<string>(key DOT "value",getStringValue());
	else
		f->setValue<int>(key DOT "index",getIntegerValue());
}


