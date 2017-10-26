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

#ifndef ComboTextParamValue_H__
#define ComboTextParamValue_H__

#include "../../../config/common.h"
#include "../qt_compat.h"

#include "ui_ComboTextParamValue.h"

#include <vector>
#include <string>

/*
 * If isEditable is true, then getValue and setValue are interpreted as the 
 * actual value to put in the combo box field.. if isEditable is false, then 
 * getValue and setValue pertain to the index of the items
 */

class CNestedDataFile;

class ComboTextParamValue : public QWidget, public Ui::ComboTextParamValue
{
	Q_OBJECT
public:
    ComboTextParamValue(const char *name,const vector<string> &items,bool isEditable);
    virtual ~ComboTextParamValue();

	const int getIntegerValue(); // returns the index into the items given at construction of the selected item (or if isEditable it returns the numeric representation of the current string value in the combo box.. this needs to be changed really)
	void setIntegerValue(const int value);

	const string getStringValue(); // returns the text of the selected item
	//void setStringValue(const string value);

	void setItems(const vector<string> &items);
	const vector<string> getItems() const;

	//void setIcons(const vector<FXIcon *> icons); can't do because that's what FXListBox does, but it doesn't allow editing


	void setCurrentItem(const int current);

	const string getName() const;

	void setTipText(const string &text);
	string getTipText() const;

	void enable();
	void disable();

	void readFromFile(const string &prefix,CNestedDataFile *f);
	void writeToFile(const string &prefix,CNestedDataFile *f);

	bool asString;

Q_SIGNALS:
	void changed();

private Q_SLOTS:
	void on_comboBox_currentIndexChanged(const QString &text);
	void on_comboBox_editTextChanged(const QString &text);

private:
	const string name;

	bool isEditable;

	vector<string> items;
};

#endif
