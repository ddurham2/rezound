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

#include <string>

#include <fox/fx.h>

#include "../backend/CSound_defs.h"

class FXConstantParamNode;
class CNestedDataFile;

class FXConstantParamValue : public FXVerticalFrame
{
	FXDECLARE(FXConstantParamValue);
public:
	typedef const double (*f_at_xs)(const double x,const int scalar);

	// display as a slider and a value entry (with optional scalar control)
	// interpretValue should return the value of the slider at the given x where 0<=x<=1 and uninterpretValue should do the inverse
	// minScalar and maxScalar are the min and max values of the scalar spinner, if they are equal, the scalar spinner will not be shown
	//FXConstantParamValue(f_at_xs interpretValue,f_at_xs uninterpretValue,const int minScalar,const int maxScalar,const int initScalar,bool showInverseButton,FXComposite *p,int opts,const char *title,int x=0,int y=0,int w=0,int h=0);
	FXConstantParamValue(f_at_xs interpretValue,f_at_xs uninterpretValue,const int minScalar,const int maxScalar,const int initScalar,bool showInverseButton,FXComposite *p,int opts,const char *title);

	// display only as a value entry
	//FXConstantParamValue(FXComposite *p,int opts,const char *title,int x=0,int y=0,int w=0,int h=0);
	FXConstantParamValue(FXComposite *p,int opts,const char *title);


	long onSliderChange(FXObject *sender,FXSelector sel,void *ptr);

	long onValueTextBoxChange(FXObject *sender,FXSelector sel,void *ptr);

	long onScalarSpinnerChange(FXObject *sender,FXSelector sel,void *ptr);

	long onInverseButton(FXObject *sender,FXSelector sel,void *ptr);

	// call if the tick labels, slider position, or edit box value need to be reevaluated
	void updateNumbers();

	void setUnits(FXString units);

	const double &getValue() const;
	void setValue(const double value);

	const int getScalar() const;
	void setScalar(const int scalar);

	const int getMinScalar() const;
	const int getMaxScalar() const;

	const string getTitle() const;

	void setHelpText(const FXString &text);
	FXString getHelpText() const;

	void readFromFile(const string &prefix,CNestedDataFile &f);
	void writeToFile(const string &prefix,CNestedDataFile &f) const;

	enum
	{
		ID_SLIDER=FXPacker::ID_LAST,

		ID_VALUE_TEXTBOX,
		ID_SCALAR_SPINNER,

		ID_INVERSE_BUTTON,

		ID_LAST
	};


protected:
	FXConstantParamValue() {}

private:
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
			FXLabel *halfLabel;
			FXLabel *minLabel;
	FXHorizontalFrame *valuePanel;
		//FXLabel *valueLabel;
		FXTextField *valueTextBox;
		FXLabel *unitsLabel;
	FXHorizontalFrame *scalarPanel;
		FXLabel *scalarLabel;
		FXSpinner *scalarSpinner;

	f_at_xs interpretValue;
	f_at_xs uninterpretValue;
	const int initScalar;

	mutable double retValue;

};

#endif
