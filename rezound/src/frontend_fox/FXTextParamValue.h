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

#ifndef __FXTextParamValue_H__
#define __FXTextParamValue_H__

#include "../../config/common.h"
#include "fox_compat.h"

#include <string>

#include <fox/fx.h>

class CNestedDataFile;

class FXTextParamValue : public FXHorizontalFrame
{
	FXDECLARE(FXTextParamValue);
public:
	FXTextParamValue(FXComposite *p,int opts,const char *title,const double initialValue,const double minValue,const double maxValue); // make it behave as a numeric entry
	FXTextParamValue(FXComposite *p,int opts,const char *title,const string initialValue); // make it behave like a text entry
	virtual ~FXTextParamValue();

	long onValueTextBoxChange(FXObject *sender,FXSelector sel,void *ptr);

	long onValueSpinnerChange(FXObject *sender,FXSelector sel,void *ptr);

	void setUnits(const FXString units,const FXString tipText="");

	const double getValue();
	void setValue(const double value);

	const string getText();
	void setText(const string text);

	void setRange(const double minValue,const double maxValue);

	const string getTitle() const;

	void setTipText(const FXString &text);
	FXString getTipText() const;

	void readFromFile(const string &prefix,CNestedDataFile *f);
	void writeToFile(const string &prefix,CNestedDataFile *f);

	enum
	{
		ID_VALUE_SPINNER=FXHorizontalFrame::ID_LAST,
		ID_VALUE_TEXTBOX,

		ID_LAST
	};


protected:
	FXTextParamValue() {}

private:
	const bool isNumeric;

	const string initialValue;

	double minValue,maxValue;

	FXLabel *titleLabel;
	FXTextField *valueTextBox;
	FXSpinner *valueSpinner;
	FXLabel *unitsLabel;

	void validateRange();

	FXFont *textFont;
};

#endif
