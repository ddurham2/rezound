/* 
 * Copyright (C) 2005 - David W. Durham
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

#include "CMacroActionParamsDialog.h"

#include "CFOXIcons.h"
#include "utils.h"

#include "../backend/AAction.h"
#include "../backend/CLoadedSound.h"
#include "../backend/CSound.h"
#include "../backend/CSoundPlayerChannel.h"

FXDEFMAP(CMacroActionParamsDialog) CMacroActionParamsDialogMap[]=
{
//	Message_Type			ID						Message_Handler
	//FXMAPFUNC(SEL_COMMAND,		CMacroActionParamsDialog::ID_START_BUTTON,			CMacroActionParamsDialog::onStartButton),
	FXMAPFUNC(SEL_COMMAND,		CMacroActionParamsDialog::ID_RADIO_BUTTON,			CMacroActionParamsDialog::onRadioButton),
};
		

FXIMPLEMENT(CMacroActionParamsDialog,FXModalDialogBox,CMacroActionParamsDialogMap,ARRAYNUMBER(CMacroActionParamsDialogMap))



// ----------------------------------------

CMacroActionParamsDialog::CMacroActionParamsDialog(FXWindow *mainWindow) :
	FXModalDialogBox(mainWindow,N_("Macro Action Parameters"),350,100,FXModalDialogBox::ftVertical)
{
	//setIcon(FOXIcons->small_record_macro);

	getFrame()->setHSpacing(0);
	getFrame()->setVSpacing(5);


	FXPacker *frame1;

#warning need to remove the cancel button
		
		actionNameLabel=new FXLabel(getFrame(),"",0,LABEL_NORMAL);

	//frame1=new FXHorizontalFrame(getFrame(),FRAME_RAISED | LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 1,1);
		new FXLabel(getFrame(),_("When the Macro is Played Back and This Action is to be Performed..."),0,LABEL_NORMAL|LAYOUT_CENTER_X);

	//frame1=new FXHorizontalFrame(getFrame(),FRAME_RAISED | LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 1,1);
		askToPromptForActionParametersAtPlaybackCheckButton=new FXCheckButton(getFrame(),"Ask to Prompt For Action Parameters");
		askToPromptForActionParametersAtPlaybackCheckButton->setCheck(FALSE);

	// ??? there might be a better way of wording this (especially the "percentage" term)
	// ??? if the start or stop positions are on exact cue positions, then we could ask to set the position to the same cue names

	frame1=startPosPositioningGroupBox=new FXGroupBox(getFrame(),_("Set the Start Position..."),GROUPBOX_NORMAL | FRAME_GROOVE);
		startPosRadioButton4=new FXRadioButton(frame1,_("Same Proportionate Time"),this,ID_RADIO_BUTTON);
		startPosRadioButton2=new FXRadioButton(frame1,_("Same Absolute Time From the Beginning of the Audio File"),this,ID_RADIO_BUTTON);
		startPosRadioButton3=new FXRadioButton(frame1,_("Same Absolute Time From the End of the Audio File"),this,ID_RADIO_BUTTON);
		startPosRadioButton7=new FXRadioButton(frame1,_("Same Cue Name"),this,ID_RADIO_BUTTON);
		startPosRadioButton1=new FXRadioButton(frame1,_("Leave in the Same Position From Previous Action"),this,ID_RADIO_BUTTON);
		startPosRadioButton4->setCheck(TRUE);

	frame1=stopPosPositioningGroupBox=new FXGroupBox(getFrame(),_("Set the Stop Position..."),GROUPBOX_NORMAL | FRAME_GROOVE);
		stopPosRadioButton4=new FXRadioButton(frame1,_("Same Proportionate Time"),this,ID_RADIO_BUTTON);
		stopPosRadioButton2=new FXRadioButton(frame1,_("Same Absolute Time From the Beginning of the Audio File"),this,ID_RADIO_BUTTON);
		stopPosRadioButton3=new FXRadioButton(frame1,_("Same Absolute Time From the End of the Audio File"),this,ID_RADIO_BUTTON);
		stopPosRadioButton5=new FXRadioButton(frame1,_("Same Absolute Time After the Start Position"),this,ID_RADIO_BUTTON);
		stopPosRadioButton6=new FXRadioButton(frame1,_("Same Proportionate Time After the Start Position"),this,ID_RADIO_BUTTON);
		stopPosRadioButton7=new FXRadioButton(frame1,_("Same Cue Name"),this,ID_RADIO_BUTTON);
		stopPosRadioButton1=new FXRadioButton(frame1,_("Leave in the Same Position From Previous Action"),this,ID_RADIO_BUTTON);
		stopPosRadioButton4->setCheck(TRUE);
}

CMacroActionParamsDialog::~CMacroActionParamsDialog()
{
}

bool CMacroActionParamsDialog::showIt(const AActionFactory *actionFactory,AFrontendHooks::MacroActionParameters &macroActionParameters,CLoadedSound *loadedSound)
{
	const string actionName=actionFactory->getName();
	const bool selectionPositionsAreApplicable=actionFactory->selectionPositionsAreApplicable;
	const bool hasDialog=actionFactory->hasDialog();

	if(!hasDialog && !selectionPositionsAreApplicable)
	{
		macroActionParameters.askToPromptForActionParametersAtPlayback=false;
		return false; // don't bother even showing the dialog
	}

	if(hasDialog)
		askToPromptForActionParametersAtPlaybackCheckButton->enable();
	else
		askToPromptForActionParametersAtPlaybackCheckButton->disable();

	string startPositionCueName;
	string stopPositionCueName;
	if(loadedSound && selectionPositionsAreApplicable)
	{
		enableAllChildren(startPosPositioningGroupBox);
		enableAllChildren(stopPosPositioningGroupBox);

		size_t index;

		if(loadedSound->sound->findCue(loadedSound->channel->getStartPosition(),index))
		{
			startPositionCueName=loadedSound->sound->getCueName(index);
			startPosRadioButton7->enable();
		}
		else
		{
			startPosRadioButton7->disable();
			if(startPosRadioButton7->getCheck()==TRUE)
			{
				startPosRadioButton1->setCheck(TRUE);
				startPosRadioButton7->setCheck(FALSE);
			}
		}

		if(loadedSound->sound->findCue(loadedSound->channel->getStopPosition(),index))
		{
			stopPositionCueName=loadedSound->sound->getCueName(index);
			stopPosRadioButton7->enable();
		}
		else
		{
			stopPosRadioButton7->disable();
			if(stopPosRadioButton7->getCheck()==TRUE)
			{
				stopPosRadioButton1->setCheck(TRUE);
				stopPosRadioButton7->setCheck(FALSE);
			}
		}
	}
	else
	{
		disableAllChildren(startPosPositioningGroupBox);
		disableAllChildren(stopPosPositioningGroupBox);
	}

	actionNameLabel->setText(("Action: "+actionName).c_str());

	// this dialog may not even need to be shown depending on the action.. probably macroRecord can exclude a list of names, or it can be part of the action factory's info to know that
	
	if(execute(PLACEMENT_CURSOR))
	{
		macroActionParameters.askToPromptForActionParametersAtPlayback=(askToPromptForActionParametersAtPlaybackCheckButton->getCheck()==TRUE);

		if(startPosRadioButton1->getCheck()==TRUE)
			macroActionParameters.startPosPositioning=AFrontendHooks::MacroActionParameters::spLeaveAlone;
		else if(startPosRadioButton2->getCheck()==TRUE)
			macroActionParameters.startPosPositioning=AFrontendHooks::MacroActionParameters::spAbsoluteTimeFromBeginning;
		else if(startPosRadioButton3->getCheck()==TRUE)
			macroActionParameters.startPosPositioning=AFrontendHooks::MacroActionParameters::spAbsoluteTimeFromEnd;
		else if(startPosRadioButton4->getCheck()==TRUE)
			macroActionParameters.startPosPositioning=AFrontendHooks::MacroActionParameters::spProportionateTimeFromBeginning;
		else if(startPosRadioButton7->getCheck()==TRUE)
			macroActionParameters.startPosPositioning=AFrontendHooks::MacroActionParameters::spSameCueName;
		macroActionParameters.startPosCueName=startPositionCueName;

		if(stopPosRadioButton1->getCheck()==TRUE)
			macroActionParameters.stopPosPositioning=AFrontendHooks::MacroActionParameters::spLeaveAlone;
		else if(stopPosRadioButton2->getCheck()==TRUE)
			macroActionParameters.stopPosPositioning=AFrontendHooks::MacroActionParameters::spAbsoluteTimeFromBeginning;
		else if(stopPosRadioButton3->getCheck()==TRUE)
			macroActionParameters.stopPosPositioning=AFrontendHooks::MacroActionParameters::spAbsoluteTimeFromEnd;
		else if(stopPosRadioButton4->getCheck()==TRUE)
			macroActionParameters.stopPosPositioning=AFrontendHooks::MacroActionParameters::spProportionateTimeFromBeginning;
		else if(stopPosRadioButton5->getCheck()==TRUE)
			macroActionParameters.stopPosPositioning=AFrontendHooks::MacroActionParameters::spAbsoluteTimeFromStartPosition;
		else if(stopPosRadioButton6->getCheck()==TRUE)
			macroActionParameters.stopPosPositioning=AFrontendHooks::MacroActionParameters::spProportionateTimeFromStartPosition;
		else if(stopPosRadioButton7->getCheck()==TRUE)
			macroActionParameters.stopPosPositioning=AFrontendHooks::MacroActionParameters::spSameCueName;
		macroActionParameters.stopPosCueName=stopPositionCueName;

		return true;
	}
	return false;
}

long CMacroActionParamsDialog::onRadioButton(FXObject *sender,FXSelector sel,void *ptr)
{
	if(ptr)
	{
		FXGroupBox *p=(FXGroupBox *)(((FXRadioButton *)sender)->getParent());
		for(int t=0;t<p->numChildren();t++)
			((FXRadioButton *)p->childAtIndex(t))->setCheck(FALSE);
		((FXRadioButton *)sender)->setCheck(TRUE);
	}

	return 0;
}
