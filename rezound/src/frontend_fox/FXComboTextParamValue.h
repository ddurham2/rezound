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

#ifndef __FXComboTextParamValue_H__
#define __FXComboTextParamValue_H__

#include "../../config/common.h"
#include "fox_compat.h"

#include <vector>
#include <string>

/*
 * If isEditable is true, then getValue and setValue are interpreted as the 
 * actual value to put in the combo box field.. if isEditable is false, then 
 * getValue and setValue pertain to the index of the items
 */

class CNestedDataFile;

class FXComboTextParamValue : public FXHorizontalFrame
{
	FXDECLARE(FXComboTextParamValue);
public:
	FXComboTextParamValue(FXComposite *p,int opts,const char *name,const vector<string> &items,bool isEditable);
	virtual ~FXComboTextParamValue();

	enum {
		ID_COMBOBOX=FXHorizontalFrame::ID_LAST,
		ID_LAST,
	};

	long onComboBoxChange(FXObject *object,FXSelector sel,void *ptr);

	const FXint getValue(); // returns the index into the items given at construction of the selected item (or if isEditable it returns the numeric representation of the current string value in the combo box.. this needs to be changed really)
	void setValue(const FXint value);

	void setItems(const vector<string> &items);
	const vector<string> getItems() const;

	//void setIcons(const vector<FXIcon *> icons); can't do because that's what FXListBox does, but it doesn't allow editing


	void setCurrentItem(const unsigned current);

	const string getName() const;

	void setTipText(const FXString &text);
	FXString getTipText() const;

	void readFromFile(const string &prefix,CNestedDataFile *f);
	void writeToFile(const string &prefix,CNestedDataFile *f);

protected:
	FXComboTextParamValue() {}

private:
	const string name;

	bool isEditable;

	FXLabel *titleLabel;
	FXComboBox *valueComboBox;

	FXFont *textFont;

	vector<string> items;

};

#endif
