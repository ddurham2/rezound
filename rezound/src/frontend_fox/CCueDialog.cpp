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

#include "CCueDialog.h"

#include <istring>

#include "CStatusComm.h"

#include "../backend/CActionParameters.h"
#include "../backend/CActionSound.h"
#include "../backend/CSound.h"

CCueDialog *gCueDialog=NULL;


FXDEFMAP(CCueDialog) CCueDialogMap[]=
{
//	Message_Type			ID					Message_Handler
	//FXMAPFUNC(SEL_COMMAND,		CCueDialog::ID_BROWSE_BUTTON,	CCueDialog::onBrowseButton)
};
		

FXIMPLEMENT(CCueDialog,FXModalDialogBox,CCueDialogMap,ARRAYNUMBER(CCueDialogMap))



// ----------------------------------------

CCueDialog::CCueDialog(FXWindow *mainWindow) :
	FXModalDialogBox(mainWindow,N_("Cue Properties"),300,175,FXModalDialogBox::ftVertical),
	
	cueNamePacker(new FXHorizontalFrame(getFrame(),LAYOUT_FILL_X)),
		cueNameLabel(new FXLabel(cueNamePacker,_("Cue Name:"),NULL,LABEL_NORMAL | LAYOUT_CENTER_X|LAYOUT_CENTER_Y)),
		cueNameTextBox(new FXTextField(cueNamePacker,20,NULL,0,TEXTFIELD_NORMAL | LAYOUT_CENTER_X|LAYOUT_CENTER_Y)),

	cueTimePacker(new FXHorizontalFrame(getFrame(),LAYOUT_FILL_X)),
		cueTimeLabel(new FXLabel(cueTimePacker,_(" Cue Time:"),NULL,LABEL_NORMAL | LAYOUT_CENTER_X|LAYOUT_CENTER_Y)),
		cueTimeTextBox(new FXTextField(cueTimePacker,20,NULL,0,TEXTFIELD_NORMAL | LAYOUT_CENTER_X|LAYOUT_CENTER_Y)),

	isAnchoredPacker(new FXHorizontalFrame(getFrame(),LAYOUT_FILL_X)),
		isAnchoredCheckButton(new FXCheckButton(isAnchoredPacker,_("Anchored in Time\tMeaning That This Cue's Position Will Not Be Affected by Adding and Removing Space Within the Audio"),NULL,0,CHECKBUTTON_NORMAL | LAYOUT_CENTER_X|LAYOUT_CENTER_Y))

{
}

CCueDialog::~CCueDialog()
{
}

bool CCueDialog::show(CActionSound *_actionSound,CActionParameters *actionParameters)
{
	actionSound=_actionSound;

	cueTime=actionParameters->getSamplePosParameter("position");
	isAnchored=actionParameters->getBoolParameter("isAnchored");

	cueNameTextBox->setText(actionParameters->getStringParameter("name").c_str());
	cueTimeTextBox->setText(actionSound->sound->getTimePosition(actionParameters->getSamplePosParameter("position"),5,false).c_str());
	isAnchoredCheckButton->setCheck(actionParameters->getBoolParameter("isAnchored"));

	if(execute(PLACEMENT_CURSOR))
	{
		actionParameters->setStringParameter("name",cueName);
		actionParameters->setSamplePosParameter("position",cueTime);
		actionParameters->setBoolParameter("isAnchored",isAnchored);
		return(true);
	}
	return(false);
}

void CCueDialog::hide()
{
	FXModalDialogBox::hide();
}

bool CCueDialog::validateOnOkay()
{
	cueName=istring(cueNameTextBox->getText().text()).trim();
	if(cueName=="")
		return(false);
	if(cueName.size()>=MAX_SOUND_CUE_NAME_LENGTH-1)
	{
		Error(_("Cue Name Too Long"));
		return(false);
	}

	bool wasInvalid;
	cueTime=actionSound->sound->getPositionFromTime(cueTimeTextBox->getText().text(),wasInvalid);
	if(wasInvalid)
	{
		Error(_("Invalid Cue Time.  Format: HH:MM:SS.sssss or MM:SS.sssss"));
		return(false);
	}

	isAnchored=isAnchoredCheckButton->getCheck();

	return(true);
}

