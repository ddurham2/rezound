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

#include "FXConstantParamValue.h"

#include <stdlib.h>

#include <istring>

#include <CNestedDataFile/CNestedDataFile.h>

/*
	- This is the slider widget used over and over by ReZound on action dialogs
	- It can also be constructed not to have the slider to serve as a text entry widget
	- Its purpose is to select a constant value for a parameter to an action
*/

// this didn't exist much before fox 1.0
#if FOX_MAJOR < 1
	#define SLIDER_TICKS_RIGHT 0
#endif

FXDEFMAP(FXConstantParamValue) FXConstantParamValueMap[]=
{
	//Message_Type				ID						Message_Handler

	FXMAPFUNC(SEL_CHANGED,			FXConstantParamValue::ID_SLIDER,		FXConstantParamValue::onSliderChange),

	FXMAPFUNC(SEL_COMMAND,			FXConstantParamValue::ID_VALUE_TEXTBOX,		FXConstantParamValue::onValueTextBoxChange),

	FXMAPFUNC(SEL_COMMAND,			FXConstantParamValue::ID_SCALAR_SPINNER,	FXConstantParamValue::onScalarSpinnerChange),

	FXMAPFUNC(SEL_COMMAND,			FXConstantParamValue::ID_INVERSE_BUTTON,	FXConstantParamValue::onInverseButton),
};

FXIMPLEMENT(FXConstantParamValue,FXVerticalFrame,FXConstantParamValueMap,ARRAYNUMBER(FXConstantParamValueMap))

FXConstantParamValue::FXConstantParamValue(f_at_xs _interpretValue,f_at_xs _uninterpretValue,const int minScalar,const int maxScalar,const int _initScalar,bool showInverseButton,FXComposite *p,int opts,const char *title) :
	FXVerticalFrame(p,opts|FRAME_RIDGE | /*LAYOUT_FILL_X|*/LAYOUT_FILL_Y|LAYOUT_CENTER_X,0,0,0,0, 6,6,2,4, 0,2),

	titleLabel(new FXLabel(this,title,NULL,LAYOUT_TOP|LAYOUT_FILL_X | JUSTIFY_CENTER_X)),
	horzSep(new FXHorizontalSeparator(this)),
	middleFrame(new FXHorizontalFrame(this,LAYOUT_FILL_Y|LAYOUT_CENTER_X, 0,0,60,60, 0,0,2,2, 2,0)),
		inverseButtonFrame(showInverseButton ? new FXPacker(middleFrame,LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 0,0) : NULL),
			inverseButton(showInverseButton ? new FXButton(inverseButtonFrame,"Inv",NULL,this,ID_INVERSE_BUTTON,FRAME_RAISED | LAYOUT_CENTER_Y) : NULL),
		slider(new FXSlider(middleFrame,this,ID_SLIDER,  SLIDER_INSIDE_BAR|SLIDER_VERTICAL|SLIDER_TICKS_RIGHT | LAYOUT_FILL_X|LAYOUT_FILL_Y)),
		tickLabelFrame(new FXVerticalFrame(middleFrame,LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 0,0)),
			maxLabel(new FXLabel(tickLabelFrame,"1.0",NULL,JUSTIFY_LEFT|JUSTIFY_CENTER_Y | LAYOUT_FILL_ROW)),
			halfLabel(new FXLabel(tickLabelFrame,"0.5",NULL,JUSTIFY_LEFT|JUSTIFY_CENTER_Y | LAYOUT_FILL_ROW|LAYOUT_FILL_Y)),
			minLabel(new FXLabel(tickLabelFrame,"0.0",NULL,JUSTIFY_LEFT|JUSTIFY_CENTER_Y | LAYOUT_FILL_ROW)),
	valuePanel(new FXHorizontalFrame(this,LAYOUT_FILL_X | JUSTIFY_CENTER_X /*| FRAME_SUNKEN*/, 0,0,0,0, 4,4,4,4, 2,2)),
		//valueLabel(new FXLabel(valuePanel,"Value",NULL, LAYOUT_CENTER_Y)),
		valueTextBox(new FXTextField(valuePanel,6,this,ID_VALUE_TEXTBOX, TEXTFIELD_NORMAL | LAYOUT_CENTER_Y|LAYOUT_FILL_X)),
		unitsLabel(new FXLabel(valuePanel,"x",NULL,LAYOUT_CENTER_Y)),
	scalarPanel(NULL),
		scalarLabel(NULL),
		scalarSpinner(NULL),

	interpretValue(_interpretValue),
	uninterpretValue(_uninterpretValue),
	initScalar(_initScalar)

{
	if(minScalar!=maxScalar)
	{
		scalarPanel=new FXHorizontalFrame(this,LAYOUT_BOTTOM|LAYOUT_FILL_X | JUSTIFY_CENTER_X | FRAME_SUNKEN, 0,0,0,0, 4,4,4,4, 2,2);
			scalarLabel=new FXLabel(scalarPanel,"Scalar",NULL, LAYOUT_CENTER_Y);
			scalarSpinner=new FXSpinner(scalarPanel,5,this,ID_SCALAR_SPINNER,SPIN_NORMAL | LAYOUT_CENTER_Y);
			//scalarSpinner->isEditable(false);

		scalarSpinner->setRange(minScalar,maxScalar);
		scalarSpinner->setValue(initScalar);
	}

	slider->setRange(0,10000);
	slider->setHeadSize(30);
#if FOX_MAJOR >= 1 // this did not exist much before fox 1.0
	slider->setTickDelta(10000/8);
#endif
	slider->setValue(0);

	updateNumbers();
}

//FXConstantParamValue::FXConstantParamValue(FXComposite *p,int opts,const char *title,int x,int y,int w,int h) :
	//FXVerticalFrame(p,opts|FRAME_RIDGE | LAYOUT_FILL_Y|LAYOUT_CENTER_X|LAYOUT_FIX_HEIGHT,x,y,w,h, 6,6,2,4, 0,2),
FXConstantParamValue::FXConstantParamValue(FXComposite *p,int opts,const char *title) :
	FXVerticalFrame(p,opts|FRAME_RIDGE | LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0, 6,6,2,4, 0,2),

	titleLabel(new FXLabel(this,title,NULL,LAYOUT_TOP|LAYOUT_FILL_X | JUSTIFY_CENTER_X)),
	horzSep(NULL),
	middleFrame(NULL),
		inverseButtonFrame(NULL),
			inverseButton(NULL),
		slider(NULL),
		tickLabelFrame(NULL),
			maxLabel(NULL),
			halfLabel(NULL),
			minLabel(NULL),
	valuePanel(new FXHorizontalFrame(this,LAYOUT_FILL_X | JUSTIFY_CENTER_X /*| FRAME_SUNKEN*/, 0,0,0,0, 4,4,4,4, 2,2)),
		//valueLabel(new FXLabel(valuePanel,"Value",NULL, LAYOUT_CENTER_Y)),
		valueTextBox(new FXTextField(valuePanel,6,this,ID_VALUE_TEXTBOX, TEXTFIELD_NORMAL | LAYOUT_CENTER_Y|LAYOUT_FILL_X)),
		unitsLabel(new FXLabel(valuePanel,"x",NULL,LAYOUT_CENTER_Y)),
	scalarPanel(NULL),
		scalarLabel(NULL),
		scalarSpinner(NULL),

	interpretValue(NULL),
	uninterpretValue(NULL)
{
	updateNumbers();
}

void FXConstantParamValue::setUnits(FXString _units)
{
	units=_units;

	unitsLabel->setText(units);
	updateNumbers();
}

#define GET_SCALAR_VALUE ( scalarSpinner==NULL ? initScalar : scalarSpinner->getValue()  )
#define SET_SCALAR_VALUE(v) { if(scalarSpinner!=NULL) scalarSpinner->setValue(v); }

long FXConstantParamValue::onSliderChange(FXObject *sender,FXSelector sel,void *ptr)
{
	retValue=interpretValue((double)slider->getValue()/10000.0,GET_SCALAR_VALUE);
	valueTextBox->setText((istring(retValue,7,4)).c_str());
	return(1);
}

long FXConstantParamValue::onScalarSpinnerChange(FXObject *sender,FXSelector sel,void *ptr)
{
	slider->setValue((int)(uninterpretValue(retValue,GET_SCALAR_VALUE)*10000.0));
	retValue=interpretValue((double)slider->getValue()/10000.0,GET_SCALAR_VALUE);
	valueTextBox->setText((istring(retValue,7,4)).c_str());
	updateNumbers();
	return(1);
}

long FXConstantParamValue::onValueTextBoxChange(FXObject *sender,FXSelector sel,void *ptr)
{
	if(slider!=NULL)
	{
			// check range if uninterpretValue does that
		retValue=interpretValue(uninterpretValue(atof(valueTextBox->getText().text()),GET_SCALAR_VALUE),GET_SCALAR_VALUE);
		slider->setValue((int)(uninterpretValue(retValue,GET_SCALAR_VALUE)*10000.0));
	}
	else
		retValue=atof(valueTextBox->getText().text());

	return(1);
}

long FXConstantParamValue::onInverseButton(FXObject *sender,FXSelector sel,void *ptr)
{
	slider->setValue(10000-slider->getValue());
	return(onSliderChange(sender,sel,ptr));
}

void FXConstantParamValue::updateNumbers()
{
	if(slider!=NULL)
	{
		minLabel->setText(istring(interpretValue(0.0,GET_SCALAR_VALUE),3,2).c_str()+units);
		halfLabel->setText(istring(interpretValue(0.5,GET_SCALAR_VALUE),3,2).c_str()+units);
		maxLabel->setText(istring(interpretValue(1.0,GET_SCALAR_VALUE),3,2).c_str()+units);

		onSliderChange(NULL,0,NULL);
	}
}

const double &FXConstantParamValue::getValue() const
{
	return(retValue);
}

void FXConstantParamValue::setValue(const double value)
{
	retValue=value;
	if(slider!=NULL)
		slider->setValue((int)(uninterpretValue(value,GET_SCALAR_VALUE)*10000.0));
	valueTextBox->setText((istring(retValue,7,4)).c_str());
}

const int FXConstantParamValue::getScalar() const
{
	return(GET_SCALAR_VALUE);
}

void FXConstantParamValue::setScalar(const int scalar)
{
	SET_SCALAR_VALUE(scalar);
	updateNumbers();
}

const int FXConstantParamValue::getMinScalar() const
{
	FXint lo=0,hi=0;
	if(scalarSpinner!=NULL)
		scalarSpinner->getRange(lo,hi);
	return(lo);
}

const int FXConstantParamValue::getMaxScalar() const
{
	FXint lo=0,hi=0;
	if(scalarSpinner!=NULL)
		scalarSpinner->getRange(lo,hi);
	return(hi);
}

const string FXConstantParamValue::getTitle() const
{
	return(titleLabel->getText().text());
}

void FXConstantParamValue::readFromFile(const string &prefix,CNestedDataFile &f)
{
	const string key=prefix+"."+getTitle()+".";
	const string v=f.getValue((key+"value").c_str());
	const string s=f.getValue((key+"scalar").c_str());

	setScalar(atoi(s.c_str()));
	setValue(atof(v.c_str()));
}

void FXConstantParamValue::writeToFile(const string &prefix,CNestedDataFile &f) const
{
	const string key=prefix+"."+getTitle()+".";

	f.createKey((key+"value").c_str(),istring(getValue()));
	if(getMinScalar()!=getMaxScalar())
		f.createKey((key+"scalar").c_str(),istring(getScalar()));
}


