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

#include "CPasteChannelsDialog.h"

#include <algorithm>

#include <istring>

#include "settings.h"

#include "../backend/AAction.h"
#include "../backend/CActionSound.h"
#include "../backend/CSound_defs.h"
#include "../backend/ASoundClipboard.h"

CPasteChannelsDialog *gPasteChannelsDialog=NULL;


FXDEFMAP(CPasteChannelsDialog) CPasteChannelsDialogMap[]=
{
//	Message_Type			ID					Message_Handler
	//FXMAPFUNC(SEL_COMMAND,		CPasteChannelsDialog::ID_OKAY_BUTTON,	CPasteChannelsDialog::onOkayButton),
};
		

FXIMPLEMENT(CPasteChannelsDialog,FXModalDialogBox,CPasteChannelsDialogMap,ARRAYNUMBER(CPasteChannelsDialogMap))



// ----------------------------------------

CPasteChannelsDialog::CPasteChannelsDialog(FXWindow *mainWindow) :
	FXModalDialogBox(mainWindow,"Paste Channels",700,340,FXModalDialogBox::ftVertical),

	label(new FXLabel(getFrame(),"Pasting Routing Information:",NULL,LAYOUT_CENTER_X)),
	contents(new FXMatrix(getFrame(),2,MATRIX_BY_COLUMNS | LAYOUT_FILL_X|LAYOUT_FILL_Y))
{
	getFrame()->setVSpacing(1);
	getFrame()->setHSpacing(1);

	new FXLabel(contents,"");
	sourceLabel=new FXLabel(contents,"Source",NULL,LAYOUT_CENTER_X);
	destinationLabel=new FXLabel(contents,"Destination",NULL,LAYOUT_CENTER_Y);
	checkBoxMatrix=new FXMatrix(contents,MAX_CHANNELS+1,MATRIX_BY_COLUMNS | LAYOUT_FILL_X|LAYOUT_FILL_Y);

	// put top source labels 
	new FXLabel(checkBoxMatrix,"");
	for(unsigned x=0;x<MAX_CHANNELS;x++)
		new FXLabel(checkBoxMatrix,("Channel "+istring(x+1)).c_str());

	for(unsigned y=0;y<MAX_CHANNELS;y++)
	{
		// put side destination label
		new FXLabel(checkBoxMatrix,("Channel "+istring(y+1)).c_str(),NULL,LAYOUT_RIGHT);

		// built a row of check boxes
		for(unsigned x=0;x<MAX_CHANNELS;x++)
			checkBoxes[y][x]=new FXCheckButton(checkBoxMatrix,"",NULL,0,CHECKBUTTON_NORMAL | LAYOUT_CENTER_X);
	}
}

bool CPasteChannelsDialog::show(CActionSound *actionSound,CActionParameters *actionParameters)
{
	const ASoundClipboard *clipboard=AAction::clipboards[gWhichClipboard];
	if(clipboard->isEmpty())
		return(false);

	// determine if and which channel is the only channel available in the clipbaord
	bool isSingleClipboardChannel=false;
	size_t singleClipboardChannel;
	for(size_t t=0;t<MAX_CHANNELS;t++)
	{
		if(clipboard->getWhichChannels()[t])
		{
			if(isSingleClipboardChannel)
			{
				isSingleClipboardChannel=false;
				break;
			}
			else
			{
				isSingleClipboardChannel=true;
				singleClipboardChannel=t;
			}
		}
	}

	// don't show the dialog if there is only one channel in both the source and destination
	if(actionSound->sound->getChannelCount()<=1 && isSingleClipboardChannel)
	{
		pasteChannels.clear();
		for(unsigned y=0;y<MAX_CHANNELS;y++)
		{
			pasteChannels.push_back(vector<bool>());
			for(unsigned x=0;x<MAX_CHANNELS;x++)
				pasteChannels[y].push_back(false);
		}

		pasteChannels[0][singleClipboardChannel]=true;
		return(true);
	}


	// uncheck all check boxes and enable only the valid ones where data could be pasted to and from
	for(unsigned y=0;y<MAX_CHANNELS;y++)
	for(unsigned x=0;x<MAX_CHANNELS;x++)
	{
		checkBoxes[y][x]->setCheck(FALSE);

		if(y<actionSound->sound->getChannelCount() && clipboard->getWhichChannels()[x])
			checkBoxes[y][x]->enable();
		else
			checkBoxes[y][x]->disable();
	}

	// by default enable a 1:1 paste mapping
	for(unsigned t=0;t<actionSound->sound->getChannelCount();t++)
	{
		if(checkBoxes[t][t]->isEnabled())
			checkBoxes[t][t]->setCheck(TRUE);
	}

	if(execute(PLACEMENT_CURSOR))
	{
		pasteChannels.clear();

		bool ret=false; // or all the checks together, if they're all false, it's like hitting cancel
		for(unsigned y=0;y<MAX_CHANNELS;y++)
		{
			pasteChannels.push_back(vector<bool>());
			for(unsigned x=0;x<MAX_CHANNELS;x++)
			{
				pasteChannels[y].push_back(checkBoxes[y][x]->getCheck());
				ret|=pasteChannels[y][x];
			}
		}

		return(ret);
	}
	return(false);

}

void *CPasteChannelsDialog::getUserData()
{
	return(&pasteChannels);
}


