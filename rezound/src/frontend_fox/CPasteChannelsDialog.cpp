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
#include "../backend/CActionParameters.h"
#include "../backend/ASoundClipboard.h"

#include "ActionParamMappers.h"

CPasteChannelsDialog *gPasteChannelsDialog=NULL;


FXDEFMAP(CPasteChannelsDialog) CPasteChannelsDialogMap[]=
{
//	Message_Type			ID						Message_Handler
	FXMAPFUNC(SEL_COMMAND,		CPasteChannelsDialog::ID_DEFAULT_BUTTON,	CPasteChannelsDialog::onDefaultButton),
	FXMAPFUNC(SEL_COMMAND,		CPasteChannelsDialog::ID_CLEAR_BUTTON,		CPasteChannelsDialog::onClearButton),

	FXMAPFUNC(SEL_COMMAND,		CPasteChannelsDialog::ID_REPEAT_TYPE_COMBOBOX,	CPasteChannelsDialog::onRepeatTypeChange),
};
		

FXIMPLEMENT(CPasteChannelsDialog,FXModalDialogBox,CPasteChannelsDialogMap,ARRAYNUMBER(CPasteChannelsDialogMap))



// ----------------------------------------

// ??? derive this from CActionDialog if I can so that it can have presets, maybe not possible as it is

#include "FXConstantParamValue.h"

CPasteChannelsDialog::CPasteChannelsDialog(FXWindow *mainWindow) :
	FXModalDialogBox(mainWindow,N_("Paste Channels"),100,100,FXModalDialogBox::ftVertical,FXModalDialogBox::stShrinkWrap),

	label(new FXLabel(getFrame(),_("Pasting Parameters:"),NULL,LAYOUT_CENTER_X)),
	horzSeparator(new FXHorizontalSeparator(getFrame())),
	topFrame(new FXHorizontalFrame(getFrame())),
		routingContents(new FXMatrix(topFrame,2,MATRIX_BY_COLUMNS | LAYOUT_FILL_X|LAYOUT_FILL_Y)),
		vertSeparator(new FXVerticalSeparator(topFrame)),
		repeatFrame(new FXVerticalFrame(topFrame,LAYOUT_CENTER_X))
{
	// setup the repeat type controls

	repeatTypeSwitcher=new FXSwitcher(repeatFrame,LAYOUT_CENTER_X);

		repeatCountSlider=new FXConstantParamValue(
			new CActionParamMapper_linear_bipolar(1.0,4,100),
			false,
			repeatTypeSwitcher,
			LAYOUT_CENTER_X,_("Repeat Count")
		);
		repeatCountSlider->setUnits("x");

		repeatTimeSlider=new FXConstantParamValue(
			new CActionParamMapper_linear_bipolar(10.0,60,3600),
			false,
			repeatTypeSwitcher,
			LAYOUT_CENTER_X,_("Repeat Time")
		);
		repeatTimeSlider->setUnits("s");

		repeatTypeSwitcher->setCurrent(0);

	repeatTypeComboBox=new FXComboBox(repeatFrame,0,this,ID_REPEAT_TYPE_COMBOBOX,COMBOBOX_NORMAL|COMBOBOX_STATIC | FRAME_SUNKEN|FRAME_THICK | LAYOUT_CENTER_X);
		repeatTypeComboBox->setNumVisible(2);
		repeatTypeComboBox->appendItem(_("Repeat X Times"));
		repeatTypeComboBox->appendItem(_("Repeat for X Seconds"));
		repeatTypeComboBox->setCurrentItem(0);



	getFrame()->setVSpacing(1);
	getFrame()->setHSpacing(1);

	// add clear and default buffers, and the dropdown list for choosing the mix type
	FXPacker *buttonPacker=new FXHorizontalFrame((/*this cast might cause a problem in the future*/FXComposite *)(getFrame()->getParent()),LAYOUT_FILL_X | FRAME_RAISED|FRAME_THICK);
		new FXButton(buttonPacker,_("Default"),NULL,this,ID_DEFAULT_BUTTON,BUTTON_NORMAL|LAYOUT_CENTER_Y);
		new FXButton(buttonPacker,_("Clear"),NULL,this,ID_CLEAR_BUTTON,BUTTON_NORMAL|LAYOUT_CENTER_Y);
		mixTypeFrame=new FXHorizontalFrame(buttonPacker,LAYOUT_CENTER_X|LAYOUT_CENTER_Y);
			new FXLabel(mixTypeFrame,_("Mix Type:"),NULL,LAYOUT_CENTER_Y);
			mixTypeComboBox=new FXComboBox(mixTypeFrame,16,NULL,0, COMBOBOX_NORMAL|FRAME_SUNKEN|FRAME_THICK | COMBOBOX_STATIC | LAYOUT_CENTER_Y);
			mixTypeComboBox->setNumVisible(4);
			mixTypeComboBox->appendItem(_("Add"),(void *)mmAdd);
			mixTypeComboBox->appendItem(_("Subtract"),(void *)mmSubtract);
			mixTypeComboBox->appendItem(_("Multiply"),(void *)mmMultiply);
			mixTypeComboBox->appendItem(_("Average"),(void *)mmAverage);


	new FXLabel(routingContents,"");
	sourceLabel=new FXLabel(routingContents,_("Clipboard"),NULL,LAYOUT_CENTER_X);
	destinationLabel=new FXLabel(routingContents,_("Destination"),NULL,LAYOUT_CENTER_Y);
	checkBoxMatrix=new FXMatrix(routingContents,MAX_CHANNELS+1,MATRIX_BY_COLUMNS | LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 0,0);

	// put top source labels 
	new FXLabel(checkBoxMatrix,"");
	for(unsigned t=0;t<MAX_CHANNELS;t++)
		new FXLabel(checkBoxMatrix,(_("Channel ")+istring(t)).c_str());

	for(unsigned t=0;t<MAX_CHANNELS;t++)
	{
		// put side destination label
		new FXLabel(checkBoxMatrix,(_("Channel ")+istring(t)).c_str(),NULL,LAYOUT_RIGHT);

		// built a row of check boxes
		for(unsigned col=0;col<MAX_CHANNELS;col++)
			new FXCheckButton(checkBoxMatrix,"",NULL,0,CHECKBUTTON_NORMAL | LAYOUT_CENTER_X);
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

	// only show the checkboxes that matter
	for(unsigned row=0;row<MAX_CHANNELS;row++)
	for(unsigned col=0;col<MAX_CHANNELS;col++)
	{
		FXCheckButton *cb=(FXCheckButton *)checkBoxMatrix->childAtRowCol(row+1,col+1);

		cb->enable();

		if(row<actionSound->sound->getChannelCount() && col<clipboard->getChannelCount())
			cb->show();
		else
			cb->hide();

		// let it show, but disabled since there is no data for that channel in the clipboard
		if(!clipboard->getWhichChannels()[col])
			cb->disable();
	}

	// show/hide labels across the top that matter
	for(unsigned col=0;col<MAX_CHANNELS;col++)
	{
		if(col<clipboard->getChannelCount())
			checkBoxMatrix->childAtRowCol(0,col+1)->show();
		else
			checkBoxMatrix->childAtRowCol(0,col+1)->hide();
	}

	// show/hide labels on the left that matter
	for(unsigned row=0;row<MAX_CHANNELS;row++)
	{
		if(row<actionSound->sound->getChannelCount())
			checkBoxMatrix->childAtRowCol(row+1,0)->show();
		else
			checkBoxMatrix->childAtRowCol(row+1,0)->hide();
	}

	// by default check a 1:1 paste mapping
	onDefaultButton(NULL,0,NULL);

	// if there is only one checkbox, then make it checked and hide it and the labels
	if(actionSound->sound->getChannelCount()==1 && clipboard->getChannelCount()==1)
	{
		((FXCheckButton *)(checkBoxMatrix->childAtRowCol(1,1)))->setCheck(true);
		routingContents->hide();
		vertSeparator->hide();
	}
	else
	{
		routingContents->show();
		vertSeparator->show();
	}

	// when the number of hidden and shown widgets change, the matrix needs to be told to recalc
	checkBoxMatrix->recalc();

	if(execute(PLACEMENT_CURSOR))
	{
		pasteChannels.clear();

		actionParameters->addUnsignedParameter(_("MixMethod"),(unsigned)(mixTypeComboBox->getItemData(mixTypeComboBox->getCurrentItem())));
		
		if(repeatTypeComboBox->getCurrentItem()==0)
		{ // repeating it a given number of times
			actionParameters->addDoubleParameter(_("Repeat Count"),repeatCountSlider->getValue());
		}
		else
		{ // repeating it for a given number of seconds
			actionParameters->addDoubleParameter(_("Repeat Count"),
				(double)( (sample_fpos_t)repeatTimeSlider->getValue() * actionSound->sound->getSampleRate() / clipboard->getLength(actionSound->sound->getSampleRate()) )
			);
		}

		bool ret=false; // or all the checks together, if they're all false, it's like hitting cancel
		for(unsigned row=0;row<MAX_CHANNELS;row++)
		{
			pasteChannels.push_back(vector<bool>());
			actionSound->doChannel[row]=false; // set doChannel for backend's crossfading's sake
			for(unsigned col=0;col<MAX_CHANNELS;col++)
			{
				FXCheckButton *cb=(FXCheckButton *)checkBoxMatrix->childAtRowCol(row+1,col+1);
				pasteChannels[row].push_back(cb->shown() ? cb->getCheck() : false);
				ret|=pasteChannels[row][col];
				actionSound->doChannel[row]|=pasteChannels[row][col]; // set doChannel for backend's crossfading's sake
			}
		}

		return ret;
	}
	return false;

}

void CPasteChannelsDialog::hide()
{
	FXModalDialogBox::hide();
}

void *CPasteChannelsDialog::getUserData()
{
	return(&pasteChannels);
}


long CPasteChannelsDialog::onDefaultButton(FXObject *sender,FXSelector sel,void *ptr)
{
	for(unsigned row=0;row<MAX_CHANNELS;row++)
	for(unsigned col=0;col<MAX_CHANNELS;col++)
	{
		FXCheckButton *cb=(FXCheckButton *)checkBoxMatrix->childAtRowCol(row+1,col+1);
		cb->setCheck(FALSE);
	}

	for(unsigned t=0;t<actionSound->sound->getChannelCount();t++)
	{
		FXCheckButton *cb=(FXCheckButton *)checkBoxMatrix->childAtRowCol(t+1,t+1);
		if(cb->shown() && cb->isEnabled())
			cb->setCheck(TRUE);
	}

	return 1;
}

long CPasteChannelsDialog::onClearButton(FXObject *sender,FXSelector sel,void *ptr)
{
	for(unsigned row=0;row<MAX_CHANNELS;row++)
	for(unsigned col=0;col<MAX_CHANNELS;col++)
	{
		FXCheckButton *cb=(FXCheckButton *)checkBoxMatrix->childAtRowCol(row+1,col+1);
		cb->setCheck(FALSE);
	}

	return 1;
}

long CPasteChannelsDialog::onRepeatTypeChange(FXObject *sender,FXSelector sel,void *ptr)
{
	repeatTypeSwitcher->setCurrent(repeatTypeComboBox->getCurrentItem());
	return 1;
}
