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

#include "CConstantParamActionDialog.h"


FXDEFMAP(CConstantParamActionDialog) CConstantParamActionDialogMap[]=
{
//	Message_Type			ID					Message_Handler
	//FXMAPFUNC(SEL_COMMAND,		CConstantParamActionDialog::ID_OKAY_BUTTON,	CConstantParamActionDialog::onOkayButton),
};
		

FXIMPLEMENT(CConstantParamActionDialog,FXModalDialogBox,CConstantParamActionDialogMap,ARRAYNUMBER(CConstantParamActionDialogMap))


// ----------------------------------------

CConstantParamActionDialog::CConstantParamActionDialog(FXWindow *mainWindow,const FXString title,int w,int h) :
	FXModalDialogBox(mainWindow,title,w,h)
{
	// make sure the dialog has at least a minimum height and width
	//ASSURE_HEIGHT(this,10);
	//ASSURE_WIDTH(this,200);
}

void CConstantParamActionDialog::addSlider(const FXString name,const FXString units,FXConstantParamValue::f_at_xs interpretValue,FXConstantParamValue::f_at_xs uninterpretValue,f_at_x optRetValueConv,const double initialValue,const int minScalar,const int maxScalar,const int initScalar,bool showInverseButton)
{
	FXConstantParamValue *slider=new FXConstantParamValue(interpretValue,uninterpretValue,minScalar,maxScalar,initScalar,showInverseButton,getFrame(),0,name.text());
	slider->setUnits(units);
	slider->setValue(initialValue);
	parameters.push_back(slider);
	retValueConvs.push_back(optRetValueConv);
}

void CConstantParamActionDialog::addValueEntry(const FXString name,const FXString units,const double initialValue)
{
	FXConstantParamValue *valueEntry=new FXConstantParamValue(getFrame(),0,name.text());
	valueEntry->setUnits(units);
	valueEntry->setValue(initialValue);
	parameters.push_back(valueEntry);
	retValueConvs.push_back(NULL);
}

bool CConstantParamActionDialog::show(CActionSound *actionSound,CActionParameters *actionParameters)
{
	if(FXDialogBox::execute(PLACEMENT_CURSOR))
	{
		for(unsigned t=0;t<parameters.size();t++)
		{
			double ret=parameters[t]->getValue();

			if(retValueConvs[t]!=NULL)
				ret=retValueConvs[t](ret);

			actionParameters->addDoubleParameter(ret);	
		}
		return(true);
	}
	return(false);
}


