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

#include "CNewSoundDialog.h"

#include <string.h>
#include <stdlib.h>

#include <cc++/path.h>

#include <istring>

#include "settings.h"
#include "CFrontendHooks.h"
#include "../backend/AStatusComm.h"

CNewSoundDialog *gNewSoundDialog=NULL;


FXDEFMAP(CNewSoundDialog) CNewSoundDialogMap[]=
{
//	Message_Type			ID					Message_Handler
	FXMAPFUNC(SEL_COMMAND,		CNewSoundDialog::ID_BROWSE_BUTTON,	CNewSoundDialog::onBrowseButton)
};
		

FXIMPLEMENT(CNewSoundDialog,FXModalDialogBox,CNewSoundDialogMap,ARRAYNUMBER(CNewSoundDialogMap))



// ----------------------------------------

CNewSoundDialog::CNewSoundDialog(FXWindow *mainWindow) :
	FXModalDialogBox(mainWindow,"New Sound",395,210,FXModalDialogBox::ftVertical),
	
	filenameFrame(new FXHorizontalFrame(getFrame(),LAYOUT_FILL_X)),
		filenameLabel(new FXLabel(filenameFrame,"(.rez)Filename:")),
		filenameTextBox(new FXTextField(filenameFrame,30,NULL,0,TEXTFIELD_NORMAL | LAYOUT_FILL_X)),
		browseButton(new FXButton(filenameFrame,"&Browse",NULL,this,ID_BROWSE_BUTTON)),
	channelsFrame(new FXHorizontalFrame(getFrame(),LAYOUT_CENTER_X)),
		channelsLabel(new FXLabel(channelsFrame,"      Channels:")),
		channelsComboBox(new FXComboBox(channelsFrame,10,8,NULL,0,COMBOBOX_NORMAL|FRAME_SUNKEN|FRAME_THICK|COMBOBOX_STATIC)),
	sampleRateFrame(new FXHorizontalFrame(getFrame(),LAYOUT_CENTER_X)),
		sampleRateLabel(new FXLabel(sampleRateFrame,"Sample Rate:")),
		sampleRateComboBox(new FXComboBox(sampleRateFrame,10,8,NULL,0,COMBOBOX_NORMAL|FRAME_SUNKEN|FRAME_THICK)),
	lengthFrame(new FXHorizontalFrame(getFrame(),LAYOUT_CENTER_X)),
		lengthLabel(new FXLabel(lengthFrame,"         Length:")),
		lengthComboBox(new FXComboBox(lengthFrame,10,8,NULL,0,COMBOBOX_NORMAL|FRAME_SUNKEN|FRAME_THICK)),
		lengthUnitsLabel(new FXLabel(lengthFrame,"second(s)"))
{
	for(size_t t=0;t<MAX_CHANNELS;t++)
		channelsComboBox->appendItem(istring(t+1).c_str());
	channelsComboBox->setCurrentItem(1); // stereo

	sampleRateComboBox->appendItem("4000");
	sampleRateComboBox->appendItem("8000");
	sampleRateComboBox->appendItem("11025");
	sampleRateComboBox->appendItem("22050");
	sampleRateComboBox->appendItem("44100");
	sampleRateComboBox->appendItem("48000");
	sampleRateComboBox->appendItem("88200");
	sampleRateComboBox->appendItem("96000");
	sampleRateComboBox->setCurrentItem(4);


	lengthComboBox->appendItem("0.25");
	lengthComboBox->appendItem("0.5");
	lengthComboBox->appendItem("0.75");
	lengthComboBox->appendItem("1");
	lengthComboBox->appendItem("15");
	lengthComboBox->appendItem("30");
	lengthComboBox->appendItem("60");
	lengthComboBox->appendItem("120");
	lengthComboBox->appendItem("300");
	lengthComboBox->appendItem("600");
	lengthComboBox->setCurrentItem(3);
}

long CNewSoundDialog::onBrowseButton(FXObject *sender,FXSelector sel,void *ptr)
{
	FXString filename=FXFileDialog::getSaveFilename(this,"New file",gPromptDialogDirectory.c_str(),((CFrontendHooks *)gFrontendHooks)->getFOXFileTypes().c_str(),0);
	if(filename!="")
	{
		// save directory to open the opendialog to next time
		gPromptDialogDirectory=ost::Path(filename.text()).DirName();
		gPromptDialogDirectory.append(&ost::Path::dirDelim,1);

		filenameTextBox->setText(filename);
	}

	return(1);
}

void CNewSoundDialog::show(FXuint placement)
{
	filenameTextBox->setText(filename.c_str());
	FXModalDialogBox::show(placement);
}

void CNewSoundDialog::hideFilename(bool hide)
{
	if(hide)
		filenameFrame->hide();
	else
		filenameFrame->show();
}

void CNewSoundDialog::hideLength(bool hide)
{
	if(hide)
		lengthFrame->hide();
	else
		lengthFrame->show();
}

bool CNewSoundDialog::validateOnOkay()
{
	if(filenameFrame->shown())
	{
		istring filename=filenameTextBox->getText().text();
		filename.trim();

		if(filename=="")
		{
			Warning("Please supply a filename");
			return(false);
		}


		// make sure filename has some extension at the end
		if(ost::Path(filename).Extension()=="")
			filename.append(".rez");

		if(ost::Path(filename).Exists())
		{
			if(Question(string("Are you sure you want to overwrite the existing file:\n   ")+filename.c_str(),yesnoQues)!=yesAns)
				return(false);
		}
		this->filename=filename;
	}
	else
		this->filename="";

	int channelCount=atoi(channelsComboBox->getText().text());
	if(channelCount<0 || channelCount>=MAX_CHANNELS)
	{
		Error("Invalid number of channels");
		return(false);
	}
	this->channelCount=channelCount;

	
	int sampleRate=atoi(sampleRateComboBox->getText().text());
	if(sampleRate<1000 || sampleRate>1000000)
	{
		Error("Invalid sample rate");
		return(false);
	}
	this->sampleRate=sampleRate;

	if(lengthFrame->shown())
	{
		double length=atof(lengthComboBox->getText().text());
		if(length<0 || MAX_LENGTH/sampleRate<length)
		{
			Error("Invalid length");
			return(false);
		} 
		this->length=(int)(length*sampleRate);
	}
	else
		this->length=1;

	return(true);
}

