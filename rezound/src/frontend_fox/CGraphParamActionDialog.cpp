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

#include "CGraphParamActionDialog.h"

CGraphParamActionDialog *gGraphParamActionDialog=NULL;


FXDEFMAP(CGraphParamActionDialog) CGraphParamActionDialogMap[]=
{
//	Message_Type			ID					Message_Handler
	//FXMAPFUNC(SEL_COMMAND,		CGraphParamActionDialog::ID_OKAY_BUTTON,	CGraphParamActionDialog::onOkayButton),
};
		

FXIMPLEMENT(CGraphParamActionDialog,FXModalDialogBox,CGraphParamActionDialogMap,ARRAYNUMBER(CGraphParamActionDialogMap))


// ----------------------------------------

CGraphParamActionDialog::CGraphParamActionDialog(FXWindow *mainWindow,const FXString title,const FXString units,FXGraphParamValue::f_at_xs interpretValue,FXGraphParamValue::f_at_xs uninterpretValue,f_at_x optRetValueConv,const int minScalar,const int maxScalar,const int initialScalar) :
	FXModalDialogBox(mainWindow,title.text(),600,400),
	graph(new FXGraphParamValue(interpretValue,uninterpretValue,minScalar,maxScalar,initialScalar,getFrame(),LAYOUT_FILL_X|LAYOUT_FILL_Y)),
	retValueConv(optRetValueConv)
{
	graph->setUnits(units);
}

bool CGraphParamActionDialog::show(CActionSound *actionSound,CActionParameters *actionParameters)
{
	graph->setSound(actionSound->sound,actionSound->start,actionSound->stop);
	if(FXDialogBox::execute(PLACEMENT_CURSOR))
	{
		CGraphParamValueNodeList nodes=graph->getNodes();

		if(retValueConv!=NULL)
		{
			for(size_t i=0;i<nodes.size();i++)
				nodes[i].value=retValueConv(nodes[i].value);
		}

		actionParameters->addGraphParameter(nodes);

		return(true);
	}
	return(false);
}


