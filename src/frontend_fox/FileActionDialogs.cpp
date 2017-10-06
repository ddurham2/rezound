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

#include "FileActionDialogs.h"

#include "../backend/CActionParameters.h"
#include "../backend/ASoundFileManager.h"
#include "../backend/CLoadedSound.h"
#include "../backend/CActionSound.h"
#include "settings.h"
#include "CFrontendHooks.h"


// --- create new audio file ------------------------

CNewAudioFileActionDialog::CNewAudioFileActionDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
}

bool CNewAudioFileActionDialog::show(CActionSound *actionSound,CActionParameters *actionParameters)
{
	string filename=actionParameters->getSoundFileManager()->getUntitledFilename(gPromptDialogDirectory,"rez");
	bool rawFormat=false;
	unsigned channelCount=0;
	unsigned sampleRate=0;
	sample_pos_t length=0;
	
	if(gFrontendHooks->promptForNewSoundParameters(filename,rawFormat,false,channelCount,false,sampleRate,false,length,false))
	{
		actionParameters->setValue<string>("filename",filename);
		actionParameters->setValue<bool>("rawFormat",rawFormat);
		actionParameters->setValue<unsigned>("channelCount",channelCount);
		actionParameters->setValue<unsigned>("sampleRate",sampleRate);
		actionParameters->setValue<sample_pos_t>("length",length);
		return true;
	}
	return false;
}


// --- open audio file ------------------------

COpenAudioFileActionDialog::COpenAudioFileActionDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
}

bool COpenAudioFileActionDialog::show(CActionSound *actionSound,CActionParameters *actionParameters)
{
	vector<string> filenames;
	bool readOnly=false;
	bool openAsRaw=false;
	if(gFrontendHooks->promptForOpenSoundFilenames(filenames,readOnly,openAsRaw))
	{
		actionParameters->setValue<vector<string> >("filenames",filenames);
		actionParameters->setValue<bool>("readOnly",readOnly);
		actionParameters->setValue<bool>("openAsRaw",openAsRaw);
		return true;
	}
	return false;
}


// --- save audio file as ---------------------

CSaveAsAudioFileActionDialog::CSaveAsAudioFileActionDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
}

bool CSaveAsAudioFileActionDialog::show(CActionSound *actionSound,CActionParameters *actionParameters)
{
	// find the CLoadedSound object in the ASoundFileManager object (??? would be nice if this were just passed in as some of the info)
	ASoundFileManager *sfm=actionParameters->getSoundFileManager();
	CLoadedSound *loaded=NULL;
	for(size_t t=0;t<sfm->getOpenedCount();t++)
	{
		if(sfm->getSound(t)->sound==actionSound->sound)
		{
			loaded=sfm->getSound(t);
			break;
		}
	}

	string filename;
	if(loaded)
		filename=loaded->getFilename();
	bool saveAsRaw=false;
	if(gFrontendHooks->promptForSaveSoundFilename(filename,saveAsRaw))
	{
		actionParameters->setValue<string>("filename",filename);
		actionParameters->setValue<bool>("saveAsRaw",saveAsRaw);
		
		return true;
	}
	return false;
}



// --- save as multiple files -----------------

#include "../backend/ASoundTranslator.h"
#include "../backend/File/CSaveAsMultipleFilesAction.h"
CSaveAsMultipleFilesDialog::CSaveAsMultipleFilesDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p1=newVertPanel(NULL);
		addDiskEntityEntry(p1,N_("Save to Directory"),".",FXDiskEntityParamValue::detDirectory,_("All the files will be saved into this directory"));
		addStringTextEntry(p1,N_("Filename Prefix"),_("Part#"),_("This will be added to the front of the filename"));
		addStringTextEntry(p1,N_("Filename Suffix"),"",_("This will be added to the end of the filename"));
		addComboTextEntry(p1,N_("Format"),ASoundTranslator::getFlatFormatList(),CActionParamDialog::cpvtAsInteger,_("The format to save each segment as"),false);
		addNumericTextEntry(p1,N_("Segment Number Start"),"",1,0,1000,_("The Number to Start With When Substituting the Track Number For '#' in the Filenames"));
		addCheckBoxEntry(p1,N_("Open Saved Segments"),false,_("Open the Segments After Saving Them"));
		vector<string> items;
			items.push_back(N_("Entire File"));
			items.push_back(N_("Selection Only"));
		addComboTextEntry(p1,N_("Applies to"),items,CActionParamDialog::cpvtAsInteger);
		addCheckBoxEntry(p1,N_("Prompt Only Once for Save Parameters"),false,_("Some formats require parameters from the user (i.e. compression type, audio format, etc).  Checking this checkbox will make it only prompt on the first file if necessary.  All other saved files will use the previous parameters if possible."));
}

const string CSaveAsMultipleFilesDialog::getExplanation() const
{
	return CSaveAsMultipleFilesAction::getExplanation();
}



// --- burn to CD -----------------------------

#include <CPath.h>
#include "../backend/File/CBurnToCDAction.h"

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
		FXComboTextParamValue *burnSpeed=addComboTextEntry(p1,N_("Burn Speed"),burnSpeeds,CActionParamDialog::cpvtAsInteger);
			burnSpeed->setIntegerValue(7);

		vector<string> trackGaps;
		for(unsigned t=0;t<=10;t++)
			trackGaps.push_back(istring(t)+"s");
		FXComboTextParamValue *trackGap=addComboTextEntry(p1,N_("Gap Between Tracks"),trackGaps,CActionParamDialog::cpvtAsInteger,_("The Gap of Silence to Place Between Each Track"));
			trackGap->setIntegerValue(0);

		vector<string> appliesTo;
			appliesTo.push_back(N_("Entire File"));
			appliesTo.push_back(N_("Selection Only"));
		addComboTextEntry(p1,N_("Applies to"),appliesTo,CActionParamDialog::cpvtAsInteger);

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



// --- run macro dialog -----------------------

#include "../backend/CMacroRecorder.h"
#include <CNestedDataFile/CNestedDataFile.h>
#include <CPath.h>

FXDEFMAP(CRunMacroDialog) CRunMacroDialogMap[]=
{
	//Message_Type			ID						Message_Handler
	FXMAPFUNC(SEL_COMMAND,		CRunMacroDialog::ID_REMOVE_BUTTON,	CRunMacroDialog::onRemoveButton),
};

FXIMPLEMENT(CRunMacroDialog,CActionParamDialog,CRunMacroDialogMap,ARRAYNUMBER(CRunMacroDialogMap))

CRunMacroDialog::CRunMacroDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,false)
{
	FXPacker *p=newVertPanel(NULL);
		vector<string> items;
		addComboTextEntry(p,N_("Macro Name"),items,CActionParamDialog::cpvtAsString);
		new FXButton(p,_("Remove Macro"),NULL,this,ID_REMOVE_BUTTON,BUTTON_NORMAL|LAYOUT_LEFT);
}

bool CRunMacroDialog::show(CActionSound *actionSound,CActionParameters *actionParameters)
{
	vector<string> items=gUserMacroStore->getValue<vector<string> >("MacroNames");
	getComboText("Macro Name")->setItems(items);

	return CActionParamDialog::show(actionSound,actionParameters);
}

long CRunMacroDialog::onRemoveButton(FXObject *object,FXSelector sel,void *ptr)
{
	FXComboTextParamValue *cb=getComboText("Macro Name");
	const string macroName=cb->getStringValue();
	if(Question(_("Are you sure you want to delete the macro: ")+macroName,yesnoQues)==yesAns)
	{
		CMacroRecorder::removeMacro(gUserMacroStore,macroName);

		vector<string> items=gUserMacroStore->getValue<vector<string> >("MacroNames");
		getComboText("Macro Name")->setItems(items);
	}
	return 0;
}

