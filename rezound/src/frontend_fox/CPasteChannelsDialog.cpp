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
#include "../backend/ASoundClipboard.h"

CPasteChannelsDialog *gPasteChannelsDialog=NULL;


FXDEFMAP(CPasteChannelsDialog) CPasteChannelsDialogMap[]=
{
//	Message_Type			ID						Message_Handler
	FXMAPFUNC(SEL_COMMAND,		CPasteChannelsDialog::ID_DEFAULT_BUTTON,	CPasteChannelsDialog::onDefaultButton),
	FXMAPFUNC(SEL_COMMAND,		CPasteChannelsDialog::ID_CLEAR_BUTTON,		CPasteChannelsDialog::onClearButton),
};
		

FXIMPLEMENT(CPasteChannelsDialog,FXModalDialogBox,CPasteChannelsDialogMap,ARRAYNUMBER(CPasteChannelsDialogMap))



// ----------------------------------------

CPasteChannelsDialog::CPasteChannelsDialog(FXWindow *mainWindow) :
	FXModalDialogBox(mainWindow,"Paste Channels",100,100,FXModalDialogBox::ftVertical),

	label(new FXLabel(getFrame(),"Pasting Routing Information:",NULL,LAYOUT_CENTER_X)),
	contents(new FXMatrix(getFrame(),2,MATRIX_BY_COLUMNS | LAYOUT_FILL_X|LAYOUT_FILL_Y))
{
	getFrame()->setVSpacing(1);
	getFrame()->setHSpacing(1);

	// add clear and default buffers, and the dropdown list for choosing the mix type
	FXPacker *buttonPacker=new FXHorizontalFrame((/*this cast might cause a problem in the future*/FXComposite *)(getFrame()->getParent()),LAYOUT_FILL_X | FRAME_RAISED|FRAME_THICK);
		new FXButton(buttonPacker,"Default",NULL,this,ID_DEFAULT_BUTTON,BUTTON_NORMAL|LAYOUT_CENTER_Y);
		new FXButton(buttonPacker,"Clear",NULL,this,ID_CLEAR_BUTTON,BUTTON_NORMAL|LAYOUT_CENTER_Y);
		mixTypeFrame=new FXHorizontalFrame(buttonPacker,LAYOUT_CENTER_X|LAYOUT_CENTER_Y);
			new FXLabel(mixTypeFrame,"Mix Type:",NULL,LAYOUT_CENTER_Y);
			mixTypeComboBox=new FXComboBox(mixTypeFrame,16,4,NULL,0, COMBOBOX_NORMAL|FRAME_SUNKEN|FRAME_THICK | COMBOBOX_STATIC | LAYOUT_CENTER_Y);
			mixTypeComboBox->appendItem("Add",(void *)mmAdd);
			mixTypeComboBox->appendItem("Subtract",(void *)mmSubtract);
			mixTypeComboBox->appendItem("Multiply",(void *)mmMultiply);
			mixTypeComboBox->appendItem("Average",(void *)mmAverage);
			

	new FXLabel(contents,"");
	sourceLabel=new FXLabel(contents,"Source",NULL,LAYOUT_CENTER_X);
	destinationLabel=new FXLabel(contents,"Destination",NULL,LAYOUT_CENTER_Y);
	checkBoxMatrix=new FXMatrix(contents,MAX_CHANNELS+1,MATRIX_BY_COLUMNS | LAYOUT_FILL_X|LAYOUT_FILL_Y);

	// put top source labels 
	new FXLabel(checkBoxMatrix,"");
	for(unsigned x=0;x<MAX_CHANNELS;x++)
		new FXLabel(checkBoxMatrix,("Channel "+istring(x)).c_str());

	for(unsigned y=0;y<MAX_CHANNELS;y++)
	{
		// put side destination label
		new FXLabel(checkBoxMatrix,("Channel "+istring(y)).c_str(),NULL,LAYOUT_RIGHT);

		// built a row of check boxes
		for(unsigned x=0;x<MAX_CHANNELS;x++)
			checkBoxes[y][x]=new FXCheckButton(checkBoxMatrix,"",NULL,0,CHECKBUTTON_NORMAL | LAYOUT_CENTER_X);
	}

}

CPasteChannelsDialog::~CPasteChannelsDialog()
{
}

bool CPasteChannelsDialog::show(CActionSound *_actionSound,CActionParameters *actionParameters)
{
	actionSound=_actionSound; // save for use in the reset and default handler buttons

	const ASoundClipboard *clipboard=AAction::clipboards[gWhichClipboard];
	if(clipboard->isEmpty())
		return false;

	vector<vector<bool> > &pasteChannels=pasteInfo.second;


	// uncheck all check boxes and enable only the valid ones where data could be pasted to and from
	for(unsigned y=0;y<MAX_CHANNELS;y++)
	for(unsigned x=0;x<MAX_CHANNELS;x++)
	{
		if(y<actionSound->sound->getChannelCount() && clipboard->getWhichChannels()[x])
			checkBoxes[y][x]->enable();
		else
			checkBoxes[y][x]->disable();
	}

	// by default enable a 1:1 paste mapping
	onDefaultButton(NULL,0,NULL);


	if(execute(PLACEMENT_CURSOR))
	{
		pasteChannels.clear();

		// funky, gcc won't let me cast directly from void* to MixMethods.. I have to go thru int
		pasteInfo.first=(MixMethods)(int)(mixTypeComboBox->getItemData(mixTypeComboBox->getCurrentItem()));

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

		return ret;
	}
	return false;

}

void *CPasteChannelsDialog::getUserData()
{
	return(&pasteInfo);
}


long CPasteChannelsDialog::onDefaultButton(FXObject *sender,FXSelector sel,void *ptr)
{
	for(unsigned y=0;y<MAX_CHANNELS;y++)
	for(unsigned x=0;x<MAX_CHANNELS;x++)
		checkBoxes[y][x]->setCheck(FALSE);

	for(unsigned y=0;y<actionSound->sound->getChannelCount();y++)
	{
		if(checkBoxes[y][y]->isEnabled())
			checkBoxes[y][y]->setCheck(TRUE);
	}

	return 1;
}

long CPasteChannelsDialog::onClearButton(FXObject *sender,FXSelector sel,void *ptr)
{
	for(unsigned y=0;y<MAX_CHANNELS;y++)
	for(unsigned x=0;x<MAX_CHANNELS;x++)
		checkBoxes[y][x]->setCheck(FALSE);
	return 1;
}

