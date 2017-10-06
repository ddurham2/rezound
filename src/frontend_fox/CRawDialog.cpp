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

#include "CRawDialog.h"

#include <stdlib.h>

#include <stdexcept>

#include <istring>

#include "CStatusComm.h"

FXDEFMAP(CRawDialog) CRawDialogMap[]=
{
//	Message_Type			ID					Message_Handler
//	FXMAPFUNC(SEL_COMMAND,		CRawDialog::ID_WHICH_BUTTON,		CRawDialog::onRadioButton),
};
		

FXIMPLEMENT(CRawDialog,FXModalDialogBox,CRawDialogMap,ARRAYNUMBER(CRawDialogMap))



// ----------------------------------------


CRawDialog::CRawDialog(FXWindow *mainWindow) :
	FXModalDialogBox(mainWindow,N_("Raw Parameters"),0,0,FXModalDialogBox::ftVertical)
{
	FXComposite *main=new FXMatrix(getFrame(),2,MATRIX_BY_COLUMNS,LAYOUT_FILL_X|LAYOUT_FILL_Y);
	FXComboBox *combo;

	channelsCountLabel=new FXLabel(main,_("Channels:"),NULL,LABEL_NORMAL|LAYOUT_RIGHT);
	combo=channelsCountComboBox=new FXComboBox(main,10,NULL,0,COMBOBOX_NORMAL|FRAME_SUNKEN|FRAME_THICK);
	combo->setNumVisible(8);
		for(unsigned t=1;t<=MAX_CHANNELS;t++)
			combo->appendItem(istring(t).c_str());
		combo->setCurrentItem(1); // stereo

	sampleRateLabel=new FXLabel(main,_("Sample Rate:"),NULL,LABEL_NORMAL|LAYOUT_RIGHT);
	combo=sampleRateComboBox=new FXComboBox(main,10,NULL,0,COMBOBOX_NORMAL|FRAME_SUNKEN|FRAME_THICK);
	combo->setNumVisible(9);
		combo->appendItem("4000");
		combo->appendItem("8000");
		combo->appendItem("11025");
		combo->appendItem("22050");
		combo->appendItem("32000");
		combo->appendItem("44100");
		combo->appendItem("48000");
		combo->appendItem("88200");
		combo->appendItem("96000");
		combo->setCurrentItem(5);

	new FXLabel(main,_("Sample Format:"),NULL,LABEL_NORMAL|LAYOUT_RIGHT);
	combo=sampleFormatComboBox=new FXComboBox(main,25,NULL,0,COMBOBOX_STATIC|FRAME_SUNKEN|FRAME_THICK);
	combo->setNumVisible(10);
		combo->appendItem(_("8bit Signed PCM"));
		combo->appendItem(_("8bit Unsigned PCM"));
		combo->appendItem(_("16bit Signed PCM"));
		combo->appendItem(_("16bit Unsigned PCM"));
		combo->appendItem(_("24bit Signed PCM"));
		combo->appendItem(_("24bit Unsigned PCM"));
		combo->appendItem(_("32bit Signed PCM"));
		combo->appendItem(_("32bit Unsigned PCM"));
		combo->appendItem(_("32bit Floating Point PCM"));
		combo->appendItem(_("64bit Floating Point PCM"));
		combo->setCurrentItem(2);

	new FXLabel(main,_("Byte Order:"),NULL,LABEL_NORMAL|LAYOUT_RIGHT);
	byteOrderToggleButton=new FXToggleButton(main,_("Little Endian (Intel)"),_("Big Endian (non-Intel)"));

	FXComposite *t;

	offsetLabel=new FXLabel(main,_("Data Start:"),NULL,LABEL_NORMAL|LAYOUT_RIGHT);
	offsetFrame=new FXHorizontalFrame(main,0, 0,0,0,0, 0,0,0,0);
		dataOffsetTextBox=new FXTextField(offsetFrame,10,NULL,0,TEXTFIELD_NORMAL|TEXTFIELD_INTEGER);
		dataOffsetTextBox->setText("0");
		new FXLabel(offsetFrame,_("in bytes"));

	lengthLabel=new FXLabel(main,_("Data Length:"),NULL,LABEL_NORMAL|LAYOUT_RIGHT);
		lengthLabel->setTipText(_("normally leave this 0"));
	lengthFrame=new FXHorizontalFrame(main,0, 0,0,0,0, 0,0,0,0);
		dataLengthTextBox=new FXTextField(lengthFrame,10,NULL,0,TEXTFIELD_NORMAL|TEXTFIELD_INTEGER);
		dataLengthTextBox->setText("0");
		dataLengthTextBox->setTipText(_("normally leave this 0"));
		(new FXLabel(lengthFrame,_("in audio frames")))->setTipText(_("normally leave this 0"));
}

CRawDialog::~CRawDialog()
{
}

bool CRawDialog::show(AFrontendHooks::RawParameters &parameters,bool showLoadRawParameters)
{
	if(showLoadRawParameters)
	{
		channelsCountLabel->show();
		channelsCountComboBox->show();
		sampleRateLabel->show();
		sampleRateComboBox->show();
		offsetLabel->show();
		offsetFrame->show();
		lengthLabel->show();
		lengthFrame->show();
		
	}
	else
	{
		channelsCountLabel->hide();
		channelsCountComboBox->hide();
		sampleRateLabel->hide();
		sampleRateComboBox->hide();
		offsetLabel->hide();
		offsetFrame->hide();
		lengthLabel->hide();
		lengthFrame->hide();
	}
	recalc();

	if(execute(PLACEMENT_SCREEN))
	{
		// don't apply values of hidden GUI controls
		if (showLoadRawParameters) {
			parameters.channelCount=atoi(channelsCountComboBox->getText().text());
			parameters.sampleRate=atoi(sampleRateComboBox->getText().text());
		}

		switch(sampleFormatComboBox->getCurrentItem())
		{
			case 0: parameters.sampleFormat=AFrontendHooks::RawParameters::f8BitSignedPCM; break;
			case 1: parameters.sampleFormat=AFrontendHooks::RawParameters::f8BitUnsignedPCM; break;
			case 2: parameters.sampleFormat=AFrontendHooks::RawParameters::f16BitSignedPCM; break;
			case 3: parameters.sampleFormat=AFrontendHooks::RawParameters::f16BitUnsignedPCM; break;
			case 4: parameters.sampleFormat=AFrontendHooks::RawParameters::f24BitSignedPCM; break;
			case 5: parameters.sampleFormat=AFrontendHooks::RawParameters::f24BitUnsignedPCM; break;
			case 6: parameters.sampleFormat=AFrontendHooks::RawParameters::f32BitSignedPCM; break;
			case 7: parameters.sampleFormat=AFrontendHooks::RawParameters::f32BitUnsignedPCM; break;
			case 8: parameters.sampleFormat=AFrontendHooks::RawParameters::f32BitFloatPCM; break;
			case 9: parameters.sampleFormat=AFrontendHooks::RawParameters::f64BitFloatPCM; break;
			default:
				throw runtime_error(string(__func__)+" -- unhandled index for sampleFormatComboBox: "+istring(sampleFormatComboBox->getCurrentItem()));
		}

		parameters.endian= byteOrderToggleButton->getState() ? AFrontendHooks::RawParameters::eBigEndian : AFrontendHooks::RawParameters::eLittleEndian;

		// don't apply values of hidden GUI controls
		if (showLoadRawParameters) {
			if(atoi(dataOffsetTextBox->getText().text())<0)
			{
				Error(_("invalid negative data offset"));
				return(false);
			}
			parameters.dataOffset=atoi(dataOffsetTextBox->getText().text());

			if(atoi(dataLengthTextBox->getText().text())<0)
			{
				Error(_("invalid negative data length"));
				return(false);
			}
			parameters.dataLength=atoi(dataLengthTextBox->getText().text());
		}

		return(true);
	}
	return(false);
}

