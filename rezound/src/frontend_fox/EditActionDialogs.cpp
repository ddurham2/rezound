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
		vector<string> items;
		for(size_t t=0;t<actionSound->sound->getChannelCount();t++)
				items.push_back("Channel "+istring(t));

		// set the combo boxes according to actionSound
		getComboText("Channel A")->setItems(items);
		getComboText("Channel B")->setItems(items);

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
CSaveAsMultipleFilesDialog::CSaveAsMultipleFilesDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,"Save As Multiple Files")
{
	void *p=newVertPanel(NULL,false);
		void *p1=newVertPanel(p);
			addDiskEntityEntry(p1,"Save to Directory",".",FXDiskEntityParamValue::detDirectory,"All the files will be saved into this directory");
			addStringTextEntry(p1,"Filename Prefix","","This will be added to the front of the filename");
			addStringTextEntry(p1,"Filename Suffix","","This will be added to the end of the filename");
			addComboTextEntry(p1,"Format",ASoundTranslator::getFlatFormatList(),"The format to save each segment as",false);
		p1=newVertPanel(p);
			addNumericTextEntry(p1,"Segment Number Start","",1,0,1000,"The Number to Start With When Substituting the Track Number For '#' in the Filenames");
		p1=newVertPanel(p);
			addCheckBoxEntry(p1,"Open Saved Segments",false,"Open the Segments After Saving Them");
}

const string CSaveAsMultipleFilesDialog::getExplaination() const
{
return "
To save a large file as several smaller segments you can create cues that define the segments and then click on \"Save As Multiple Files\" under the \"File\" menu.

In general, cues can be named '(' and ')' to define the beginning and end of each segment to be saved.
However, a ')' cue can be ommitted if a previous segment is to end at the beginning of the next segment.
The very last ')' cue can also be ommitted if the last defined segment is to end at the end of the original audio file.
Furthermore, the '(' cue can optionally be named '(xyz' if 'xyz' is to be included in the segment's filename.

There are several parameters in the dialog is displayed after selecting \"Save As Multiple Files\" under the \"File\" menu.
The \"Save to Directory\" parameter that is the directory to place each segment file.
The \"Filename Prefix\" will be placed in front of the optional 'xyz' from the '(xyz' cue name.
The \"Filename Suffix\" will be placed after of the optional 'xyz' from the '(xyz' cue name.
The \"Format\" parameter in specifies what audio format the segments should be saved.
After a segment's filename is formed by putting together, [directory]/[prefix][xyz][suffix].[format extension], if it contains '#' then the '#' will be replaced with a number based on the order that the segments are defined.
    For instance: \"/home/john/sounds/track #.wav\" will be changed to \"/home/john/sounds/track 1.wav\" for the first segment that is saved, and all the subsequent segments will contain an increasing number.
    Thus, you should use a '#' in either the save to directory, filename prefix, xyz, or the filename suffix to create unique filenames when saving the segments.
The \"Segment Number Start\" parameter can be changed from '1' to start the '#' substitutions at something different.
The \"Open Saved Segments\" can be selected simply if you want to open the segments after they have been saved.
";
	
}
