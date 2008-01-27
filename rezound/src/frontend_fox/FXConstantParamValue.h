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

#ifndef __FXConstantParamValue_H__
#define __FXConstantParamValue_H__

#include "../../config/common.h"
#include "fox_compat.h"

#include <string>

#include "../backend/CSound_defs.h"

#include "../backend/AActionParamMapper.h"

class CNestedDataFile;

class FXConstantParamValue : public FXVerticalFrame
{
	FXDECLARE(FXConstantParamValue);
public:
	// display as a slider and a value entry (with optional scalar control)
	// interpretValue should return the value of the slider at the given x where 0<=x<=1 and uninterpretValue should do the inverse
	// minScalar and maxScalar are the min and max values of the scalar spinner, if they are equal, the scalar spinner will not be shown
	FXConstantParamValue(AActionParamMapper *valueMapper,bool showInverseButton,FXComposite *p,int opts,const char *name);
	virtual ~FXConstantParamValue();

	FXint getDefaultWidth();
	FXint getDefaultHeight();
	void setMinSize(FXint minWidth,FXint minHeight);

	long onSliderChange(FXObject *sender,FXSelector sel,void *ptr);

	long onValueTextBoxChange(FXObject *sender,FXSelector sel,void *ptr);

	long onScalarSpinnerChange(FXObject *sender,FXSelector sel,void *ptr);

	long onInverseButton(FXObject *sender,FXSelector sel,void *ptr);

	long onMiddleLabelClick(FXObject *sender,FXSelector sel,void *ptr);

	// call if the tick labels, slider position, or edit box value need to be reevaluated
	void updateNumbers();

	void setUnits(const FXString units);

	const double getValue() const;
	void setValue(const double value);

	const int getScalar() const;
	void setScalar(const int scalar);

	const int getMinScalar() const;
	const int getMaxScalar() const;

	const string getName() const;

	void setTipText(const FXString &text);
	FXString getTipText() const;

	void enable();
	void disable();

	void readFromFile(const string &prefix,CNestedDataFile *f);
	void writeToFile(const string &prefix,CNestedDataFile *f) const;

	enum
	{
		ID_SLIDER=FXVerticalFrame::ID_LAST,

		ID_VALUE_TEXTBOX,
		ID_SCALAR_SPINNER,

		ID_INVERSE_BUTTON,

		ID_MIDDLE_LABEL,

		ID_LAST
	};


protected:
	FXConstantParamValue() {}

private:
	const string name;

	FXString units;

	//
	//  -------------------------------------------  <---- this
	// | ----------------------------------------- |
	// | |               titleLabel              | |
	// | ----------------------------------------- |
	// | ----------------------------------------- |
	// ||    (optional) middleFrame               ||
	// || -----  --------------  ---------------  ||
	// |||     ||              || ------------- | ||
	// |||     ||              || | maxLabel  | | ||
	// |||     ||              || ------------- | ||
	// |||     ||              ||          <----|-||---- tickLabelFrame
	// ||| --- ||    slider    || ------------- | ||
	// ||||Inv|||              || | halfLabel | | ||
	// ||| --- ||              || ------------- | ||
	// |||  <---|----------------------------------|---- inverseButtonFrame (optional)
	// |||     ||              || ------------- | ||
	// |||     ||              || | minLabel  | | ||
	// |||     ||              || ------------- | ||
	// || -----  --------------  ---------------  ||
	// | ----------------------------------------- |
	// | ----------------------------------------- |
	// | |                 valuePanel            | |
	// | ----------------------------------------- |
	// | ----------------------------------------- |
	// | |       (optional) scalarPanel          | |
	// | ----------------------------------------- |
	//  ------------------------------------------- 
	// 

	FXLabel *titleLabel;
	FXHorizontalSeparator *horzSep;
	FXHorizontalFrame *middleFrame;
		FXPacker *inverseButtonFrame;
			FXButton *inverseButton;
		FXSlider *slider;
		FXVerticalFrame *tickLabelFrame;
			FXLabel *maxLabel;
			FXPacker *middleTickLabelFrame;
				FXLabel *halfLabel;
			FXLabel *minLabel;
	FXHorizontalFrame *valuePanel;
		FXTextField *valueTextBox;
		FXLabel *unitsLabel;
	FXHorizontalFrame *scalarPanel;
		FXLabel *scalarLabel;
		FXSpinner *scalarSpinner;

	AActionParamMapper *valueMapper;

	mutable double retValue;

	void prvSetValue(const double value);
	double defaultValue;

	FXFont *textFont;

	FXint minWidth,minHeight;
};

#endif
