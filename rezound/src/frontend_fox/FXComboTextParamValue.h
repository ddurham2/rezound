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

#include <fox/fx.h>

class CNestedDataFile;

class FXComboTextParamValue : public FXVerticalFrame
{
	FXDECLARE(FXComboTextParamValue);
public:
	FXComboTextParamValue(FXComposite *p,int opts,const char *title,const vector<string> &items);

	const FXint getValue(); // returns the index into the items given at construction of the selected item
	void setValue(const FXint value);

	void setItems(const vector<string> &items);

	const string getTitle() const;

	void setHelpText(const FXString &text);
	FXString getHelpText() const;

	void readFromFile(const string &prefix,CNestedDataFile &f);
	void writeToFile(const string &prefix,CNestedDataFile &f);

/*
	enum
	{
		ID_SLIDER=FXPacker::ID_LAST,

		ID_VALUE_TEXTBOX,
		ID_VALUE_SPINNER,

		ID_LAST
	};
*/


protected:
	FXComboTextParamValue() {}

private:

	FXLabel *titleLabel;
	FXComboBox *valueComboBox;
};

#endif
