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

#include <CPath.h>

#include <istring>

#include "CFrontendHooks.h"
#include "../backend/AStatusComm.h"


FXDEFMAP(CNewSoundDialog) CNewSoundDialogMap[]=
{
//	Message_Type			ID					Message_Handler
	FXMAPFUNC(SEL_COMMAND,		CNewSoundDialog::ID_BROWSE_BUTTON,	CNewSoundDialog::onBrowseButton)
};
		

FXIMPLEMENT(CNewSoundDialog,FXModalDialogBox,CNewSoundDialogMap,ARRAYNUMBER(CNewSoundDialogMap))



// ----------------------------------------

CNewSoundDialog::CNewSoundDialog(FXWindow *mainWindow) :
	FXModalDialogBox(mainWindow,N_("New Sound"),395,210,FXModalDialogBox::ftVertical),
	
	filenameFrame(new FXHorizontalFrame(getFrame(),LAYOUT_FILL_X)),
		filenameLabel(new FXLabel(filenameFrame,_("Filename:"))),
		filenameTextBox(new FXTextField(filenameFrame,30,NULL,0,TEXTFIELD_NORMAL | LAYOUT_FILL_X)),
		browseButton(new FXButton(filenameFrame,_("&Browse"),NULL,this,ID_BROWSE_BUTTON)),
	rawFormatFrame(new FXHorizontalFrame(getFrame(),LAYOUT_FILL_X)),
		rawFormatCheckButton(new FXCheckButton(rawFormatFrame,_("Raw Format"),NULL,0,CHECKBUTTON_NORMAL|LAYOUT_CENTER_X)),
	matrix(new FXMatrix(getFrame(),2,MATRIX_BY_COLUMNS|LAYOUT_CENTER_X)),

		channelsLabel(new FXLabel(matrix,_("Channels:"),NULL,LABEL_NORMAL|LAYOUT_RIGHT)),
		channelsComboBox(new FXComboBox(matrix,10,8,NULL,0,COMBOBOX_NORMAL|FRAME_SUNKEN|FRAME_THICK|COMBOBOX_STATIC)),

		sampleRateLabel(new FXLabel(matrix,_("Sample Rate:"),NULL,LABEL_NORMAL|LAYOUT_RIGHT)),
		sampleRateComboBox(new FXComboBox(matrix,10,8,NULL,0,COMBOBOX_NORMAL|FRAME_SUNKEN|FRAME_THICK)),

		lengthLabel(new FXLabel(matrix,_("Length:"),NULL,LABEL_NORMAL|LAYOUT_RIGHT)),
		lengthFrame(new FXHorizontalFrame(matrix,LAYOUT_CENTER_X,0,0,0,0, 0,0,0,0, 0,0)),
			lengthComboBox(new FXComboBox(lengthFrame,10,8,NULL,0,COMBOBOX_NORMAL|FRAME_SUNKEN|FRAME_THICK)),
			lengthUnitsLabel(new FXLabel(lengthFrame,_("second(s)")))
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

CNewSoundDialog::~CNewSoundDialog()
{
}

long CNewSoundDialog::onBrowseButton(FXObject *sender,FXSelector sel,void *ptr)
{
	string filename;
	bool saveAsRaw=false;
	if(gFrontendHooks->promptForSaveSoundFilename(filename,saveAsRaw))
	{
		filenameTextBox->setText(filename.c_str());
		rawFormatCheckButton->setCheck(saveAsRaw);
	}


	return 1;
}

void CNewSoundDialog::show(FXuint placement)
{
	filenameTextBox->setText(filename.c_str());
	rawFormatCheckButton->setCheck(rawFormat);
	FXModalDialogBox::show(placement);
}

void CNewSoundDialog::hideFilename(bool hide)
{
	if(hide)
	{
		filenameFrame->hide();
		rawFormatCheckButton->hide();
	}
	else
	{
		filenameFrame->show();
		rawFormatCheckButton->show();
	}
	recalc();
}

void CNewSoundDialog::hideLength(bool hide)
{
	if(hide)
	{
		lengthLabel->hide();
		lengthFrame->hide();
	}
	else
	{
		lengthLabel->show();
		lengthFrame->show();
	}
	recalc();
}

void CNewSoundDialog::hideSampleRate(bool hide)
{
	if(hide)
	{
		sampleRateLabel->hide();
		sampleRateComboBox->hide();
	}
	else
	{
		sampleRateLabel->show();
		sampleRateComboBox->show();
	}
	recalc();
}

bool CNewSoundDialog::validateOnOkay()
{
	if(filenameFrame->shown())
	{
		istring filename=filenameTextBox->getText().text();
		filename.trim();

		if(filename=="")
		{
			Warning(_("Please supply a filename"));
			return false;
		}


		// make sure filename has some extension at the end
		if(CPath(filename).extension()=="")
			filename.append(".rez");

		if(CPath(filename).exists())
		{
			if(Question(_("Are you sure you want to overwrite the existing file:\n   ")+filename,yesnoQues)!=yesAns)
				return false;
		}
		this->filename=filename;
	}
	else
		this->filename="";

	this->rawFormat=rawFormatCheckButton->getCheck();

	int channelCount=atoi(channelsComboBox->getText().text());
	if(channelCount<0 || channelCount>=MAX_CHANNELS)
	{
		Error(_("Invalid number of channels"));
		return false;
	}
	this->channelCount=channelCount;

	
	int sampleRate=atoi(sampleRateComboBox->getText().text());
	if(sampleRate<1000 || sampleRate>1000000)
	{
		Error(_("Invalid sample rate"));
		return false;
	}
	this->sampleRate=sampleRate;

	if(lengthFrame->shown())
	{
		double length=atof(lengthComboBox->getText().text());
		if(length<0 || MAX_LENGTH/sampleRate<length)
		{
			Error(_("Invalid length"));
			return false;
		} 
		this->length=(int)(length*sampleRate);
	}
	else
		this->length=1;

	return true;
}

