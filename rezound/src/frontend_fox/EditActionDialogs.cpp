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

#include "EditActionDialogs.h"

#include <istring>

#include "../backend/CActionSound.h"
#include "../backend/CSound.h"

#include "ActionParamMappers.h"


// --- insert silence -------------------------

CInsertSilenceDialog::CInsertSilenceDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addNumericTextEntry(p,N_("Length"),_("seconds"),1.0,0,10000);
}



// --- rotate ---------------------------------

CRotateDialog::CRotateDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addNumericTextEntry(p,N_("Amount"),_("seconds"),1.0,0,10000);
}



// --- swap channels --------------------------

CSwapChannelsDialog::CSwapChannelsDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newVertPanel(NULL);
		vector<string> items;
		addComboTextEntry(p,N_("Channel A"),items,"");
		addComboTextEntry(p,N_("Channel B"),items,"");
}

#include "../backend/CActionParameters.h"

bool CSwapChannelsDialog::show(CActionSound *actionSound,CActionParameters *actionParameters)
{
	if(actionSound->sound->getChannelCount()<2)
		return false;
	else if(actionSound->sound->getChannelCount()==2)
	{ // only one possibility of swapping the two channels
		actionParameters->addUnsignedParameter("Channel A",0);
		actionParameters->addUnsignedParameter("Channel B",1);
		return true;
	}
	else
	{
		if(getComboText("Channel A")->getItems().size()!=actionSound->sound->getChannelCount())
		{
			vector<string> items;
			for(size_t t=0;t<actionSound->sound->getChannelCount();t++)
				items.push_back(_("Channel ")+istring(t));

			// set the combo boxes according to actionSound
			getComboText("Channel A")->setItems(items);
			getComboText("Channel B")->setItems(items);
			getComboText("Channel A")->setCurrentItem(0);
			getComboText("Channel B")->setCurrentItem(1);
		}

		return CActionParamDialog::show(actionSound,actionParameters);
	}
}


// --- add channels ---------------------------

CAddChannelsDialog::CAddChannelsDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newVertPanel(NULL);
		addNumericTextEntry(p,N_("Insert Where"),"",0,0,MAX_CHANNELS);
		addNumericTextEntry(p,N_("Insert Count"),"",1,1,MAX_CHANNELS);
}



// --- duplicate channel ----------------------

CDuplicateChannelDialog::CDuplicateChannelDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newVertPanel(NULL);
		vector<string> items;
		addComboTextEntry(p,N_("Which Channel"),items,"");
		addComboTextEntry(p,N_("Insert Where"),items,"");
		addNumericTextEntry(p,N_("How Many Times"),"",1,1,MAX_CHANNELS);
}

bool CDuplicateChannelDialog::show(CActionSound *actionSound,CActionParameters *actionParameters)
{
	if(getComboText("Which Channel")->getItems().size()!=actionSound->sound->getChannelCount())
	{
		vector<string> items;
		for(size_t t=0;t<actionSound->sound->getChannelCount();t++)
			items.push_back(_("Channel ")+istring(t));

		// set the combo boxes according to actionSound
		getComboText("Which Channel")->setItems(items);
		getComboText("Which Channel")->setCurrentItem(0);

		items.push_back(_("Channel ")+istring(actionSound->sound->getChannelCount()));
		getComboText("Insert Where")->setItems(items);
		getComboText("Insert Where")->setCurrentItem(1);
	}

	return CActionParamDialog::show(actionSound,actionParameters);
}



// --- save as multiple files -----------------

#include "../backend/ASoundTranslator.h"
#include "../backend/Edits/CSaveAsMultipleFilesAction.h"
CSaveAsMultipleFilesDialog::CSaveAsMultipleFilesDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p1=newVertPanel(NULL);
		addDiskEntityEntry(p1,N_("Save to Directory"),".",FXDiskEntityParamValue::detDirectory,_("All the files will be saved into this directory"));
		addStringTextEntry(p1,N_("Filename Prefix"),_("Part#"),_("This will be added to the front of the filename"));
		addStringTextEntry(p1,N_("Filename Suffix"),"",_("This will be added to the end of the filename"));
		addComboTextEntry(p1,N_("Format"),ASoundTranslator::getFlatFormatList(),_("The format to save each segment as"),false);
		addNumericTextEntry(p1,N_("Segment Number Start"),"",1,0,1000,_("The Number to Start With When Substituting the Track Number For '#' in the Filenames"));
		addCheckBoxEntry(p1,N_("Open Saved Segments"),false,_("Open the Segments After Saving Them"));
		vector<string> items;
			items.push_back(N_("Entire File"));
			items.push_back(N_("Selection Only"));
		addComboTextEntry(p1,N_("Applies to"),items);
		addCheckBoxEntry(p1,N_("Prompt Only Once for Save Parameters"),false,_("Some formats require parameters from the user (i.e. compression type, audio format, etc).  Checking this checkbox will make it only prompt on the first file if necessary.  All other saved files will use the previous parameters if possible."));
}

const string CSaveAsMultipleFilesDialog::getExplanation() const
{
	return CSaveAsMultipleFilesAction::getExplanation();
}



// --- burn to CD -----------------------------

#include <CPath.h>
#include "../backend/Edits/CBurnToCDAction.h"
#include "settings.h"

FXDEFMAP(CBurnToCDDialog) CBurnToCDDialogMap[]=
{
	//Message_Type			ID						Message_Handler
	FXMAPFUNC(SEL_COMMAND,		CBurnToCDDialog::ID_DETECT_DEVICE_BUTTON,	CBurnToCDDialog::onDetectDeviceButton),
};

FXIMPLEMENT(CBurnToCDDialog,CActionParamDialog,CBurnToCDDialogMap,ARRAYNUMBER(CBurnToCDDialogMap))

CBurnToCDDialog::CBurnToCDDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	FXPacker *p1=newVertPanel(NULL);
			// ??? default to file location or fallback work dir
		addDiskEntityEntry(p1,N_("Temp Space Directory"),gFallbackWorkDir,FXDiskEntityParamValue::detDirectory,_("A temporary file for burning will be placed in this location.  Enough space will be needed for CD quality audio of the length of the audio to be burned to the CD"));

			// ??? default to `which cdrdao` or if not found, blank
		FXDiskEntityParamValue *cdrdaoPath=addDiskEntityEntry(p1,N_("Path to cdrdao"),CPath::which("cdrdao"),FXDiskEntityParamValue::detGeneralFilename);

		vector<string> burnSpeeds;
		for(unsigned t=1;t<=50;t++)
			burnSpeeds.push_back(istring(t)+"x");
		FXComboTextParamValue *burnSpeed=addComboTextEntry(p1,N_("Burn Speed"),burnSpeeds);
			burnSpeed->setValue(7);

		vector<string> trackGaps;
		for(unsigned t=0;t<=10;t++)
			trackGaps.push_back(istring(t)+"s");
		FXComboTextParamValue *trackGap=addComboTextEntry(p1,N_("Gap Between Tracks"),trackGaps,_("The Gap of Silence to Place Between Each Track"));
			trackGap->setValue(0);

		vector<string> appliesTo;
			appliesTo.push_back(N_("Entire File"));
			appliesTo.push_back(N_("Selection Only"));
		addComboTextEntry(p1,N_("Applies to"),appliesTo);

		FXVerticalFrame *p2=new FXVerticalFrame(p1,LAYOUT_FILL_X, 0,0,0,0, 0,0,0,0, 0,0);
			FXHorizontalFrame *p3=new FXHorizontalFrame(p2,LAYOUT_FILL_X, 0,0,0,0, 0,0,0,0, 4,0);
				FXComposite *device=addStringTextEntry(p3,N_("Device"),"0,0,0");
					new FXButton(device,_("Detect"),NULL,this,ID_DETECT_DEVICE_BUTTON,BUTTON_NORMAL|LAYOUT_RIGHT);
			new FXLabel(p2,_("In linux >2.6, scanbus may not work and it may be\nnecessary to specify: for example: ATAPI:0,0,0"));
			new FXLabel(p2,_("Also, cdrdao-1.1.9 has much better drive support."));

		addStringTextEntry(p1,N_("Extra cdrdao Options"),"");
		addCheckBoxEntry(p1,N_("Simulate Burn Only"),false,_("Don't Turn on Burn Laser"));
}

long CBurnToCDDialog::onDetectDeviceButton(FXObject *object,FXSelector sel,void *ptr)
{
	getTextParam("Device")->setText(CBurnToCDAction::detectDevice(getDiskEntityParam("Path to cdrdao")->getEntityName()));
	return 0;
}

const string CBurnToCDDialog::getExplanation() const
{
	return CBurnToCDAction::getExplanation();
}



// --- grow or slide selection dialog ---------

CGrowOrSlideSelectionDialog::CGrowOrSlideSelectionDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newVertPanel(NULL);
		addSlider(p,
			N_("Amount"),
			"s",
			new CActionParamMapper_linear(1.0,2,1,3600),
			NULL,
			false
		);
		setTipText("Amount",_("Amount to Affect the Selection in Seconds"));

		vector<string> items;
			items.push_back(N_("Grow Selection to the Left"));
			items.push_back(N_("Grow Selection to the Right"));
			items.push_back(N_("Grow Selection in Both Directions"));
			items.push_back(N_("Slide Selection to the Left"));
			items.push_back(N_("Slide Selection to the Right"));
		addComboTextEntry(p,N_("How"),items);
		getComboText("How")->setCurrentItem(1);
}


