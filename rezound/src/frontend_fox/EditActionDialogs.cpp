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

// --- insert silence -------------------------

CInsertSilenceDialog::CInsertSilenceDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Insert Silence")
{
	void *p=newHorzPanel(NULL);
		addNumericTextEntry(p,"Length","seconds",1.0,0,10000);
}



// --- rotate ---------------------------------

CRotateDialog::CRotateDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Rotate")
{
	void *p=newHorzPanel(NULL);
		addNumericTextEntry(p,"Amount","seconds",1.0,0,10000);
}



// --- swap channels --------------------------

CSwapChannelsDialog::CSwapChannelsDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Swap Channels A & B")
{
	void *p=newHorzPanel(NULL);
		vector<string> items;
		addComboTextEntry(p,"Channel A",items,"");
		addComboTextEntry(p,"Channel B",items,"");
}

#include "../backend/CActionParameters.h"

bool CSwapChannelsDialog::show(CActionSound *actionSound,CActionParameters *actionParameters)
{
	if(actionSound->sound->getChannelCount()<2)
		return(false);
	else if(actionSound->sound->getChannelCount()==2)
	{ // only one possibility of swapping the two channels
		actionParameters->addUnsignedParameter("Channel A",0);
		actionParameters->addUnsignedParameter("Channel B",1);
		return(true);
	}
	else
	{
		if(getComboText("Channel A")->getItems().size()!=actionSound->sound->getChannelCount())
		{
			vector<string> items;
			for(size_t t=0;t<actionSound->sound->getChannelCount();t++)
					items.push_back("Channel "+istring(t));

			// set the combo boxes according to actionSound
			getComboText("Channel A")->setItems(items);
			getComboText("Channel B")->setItems(items);
			getComboText("Channel A")->setCurrentItem(0);
			getComboText("Channel B")->setCurrentItem(1);
		}

		return(CActionParamDialog::show(actionSound,actionParameters));
	}
}


// --- add channels ---------------------------

CAddChannelsDialog::CAddChannelsDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Add Channels")
{
	void *p=newHorzPanel(NULL);
		addNumericTextEntry(p,"Insert Where","",0,0,MAX_CHANNELS);
		addNumericTextEntry(p,"Insert Count","",1,1,MAX_CHANNELS);
}



// --- save as multiple files -----------------

#include "../backend/ASoundTranslator.h"
#include "../backend/Edits/CSaveAsMultipleFilesAction.h"
CSaveAsMultipleFilesDialog::CSaveAsMultipleFilesDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Save As Multiple Files")
{
	void *p1=newVertPanel(NULL);
		addDiskEntityEntry(p1,"Save to Directory",".",FXDiskEntityParamValue::detDirectory,"All the files will be saved into this directory");
		addStringTextEntry(p1,"Filename Prefix","Part#","This will be added to the front of the filename");
		addStringTextEntry(p1,"Filename Suffix","","This will be added to the end of the filename");
		addComboTextEntry(p1,"Format",ASoundTranslator::getFlatFormatList(),"The format to save each segment as",false);
		addNumericTextEntry(p1,"Segment Number Start","",1,0,1000,"The Number to Start With When Substituting the Track Number For '#' in the Filenames");
		addCheckBoxEntry(p1,"Open Saved Segments",false,"Open the Segments After Saving Them");
		vector<string> items;
			items.push_back("Entire File");
			items.push_back("Selection Only");
		addComboTextEntry(p1,"Applies to",items);
}

const string CSaveAsMultipleFilesDialog::getExplaination() const
{
	return CSaveAsMultipleFilesAction::getExplaination();
}
