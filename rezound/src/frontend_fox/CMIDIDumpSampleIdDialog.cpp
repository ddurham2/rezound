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

#include "CMIDIDumpSampleIdDialog.h"

#include "CStatusComm.h"

#include <stdlib.h>

#include <istring>


FXDEFMAP(CMIDIDumpSampleIdDialog) CMIDIDumpSampleIdDialogMap[]=
{
//	Message_Type			ID						Message_Handler
	FXMAPFUNC(SEL_COMMAND,		CMIDIDumpSampleIdDialog::ID_WAIT_CHECKBUTTON,	CMIDIDumpSampleIdDialog::onWaitCheckButton),
};
		

FXIMPLEMENT(CMIDIDumpSampleIdDialog,FXModalDialogBox,CMIDIDumpSampleIdDialogMap,ARRAYNUMBER(CMIDIDumpSampleIdDialogMap))



// ----------------------------------------


CMIDIDumpSampleIdDialog::CMIDIDumpSampleIdDialog(FXWindow *mainWindow) :
	FXModalDialogBox(mainWindow,_("MIDI Sample Dump"),0,0,FXModalDialogBox::ftVertical,FXModalDialogBox::stShrinkWrap)
{
	FXComposite *t;
	
	waitCheckButton=new FXCheckButton(getFrame(),_("Wait for Dump"),this,ID_WAIT_CHECKBUTTON);

	t=new FXHorizontalFrame(getFrame());
		new FXLabel(t,_("Sample ID"));
		waveformIdEdit=new FXTextField(t,10,NULL,0,TEXTFIELD_NORMAL|TEXTFIELD_INTEGER | LAYOUT_FILL_X);
		waveformIdEdit->setText("0");

	t=new FXHorizontalFrame(getFrame());
		new FXLabel(t,_("SysEx Channel"));
		sysExChannelSpinner=new FXSpinner(t,4,NULL,0,SPIN_NORMAL | SPIN_CYCLIC | FRAME_SUNKEN|FRAME_THICK);
		sysExChannelSpinner->setRange(0,127);

	loopTypeFrame=t=new FXHorizontalFrame(getFrame());
		new FXLabel(t,_("Loop Type"));
		loopTypeEdit=new FXTextField(t,10,NULL,0,TEXTFIELD_NORMAL|TEXTFIELD_INTEGER | LAYOUT_FILL_X);
		loopTypeEdit->setText("0");
}

CMIDIDumpSampleIdDialog::~CMIDIDumpSampleIdDialog()
{
}

bool CMIDIDumpSampleIdDialog::showForOpen(int &sysExChannel,int &waveformId)
{
	waitCheckButton->setCheck(waveformId<0);
	waitCheckButton->show();

	if(waveformId>=0)
		waveformIdEdit->setText(istring(waveformId).c_str());

	if(sysExChannel>=0)
		sysExChannelSpinner->setValue(sysExChannel);

	loopTypeFrame->hide();

	onWaitCheckButton(NULL,0,NULL);

	recalc();

	redo:
	if(execute(PLACEMENT_SCREEN))
	{

		if(waitCheckButton->getCheck())
		{
			waveformId=-1;
			sysExChannel=-1;
		}
		else
		{
			waveformId=atoi(waveformIdEdit->getText().text());
			if(waveformId<0 || waveformId>16384)
			{
				Error(_("Waveform IDs must be from 0 to 16384"));
				goto redo;
			}

			sysExChannel=sysExChannelSpinner->getValue();
		}
		return true;
	}
	return false;
}

bool CMIDIDumpSampleIdDialog::showForSave(int &sysExChannel,int &waveformId,int &loopType)
{
	waitCheckButton->setCheck(false);
	waitCheckButton->hide();

	if(waveformId>=0)
		waveformIdEdit->setText(istring(waveformId).c_str());

	if(sysExChannel>=0)
		sysExChannelSpinner->setValue(sysExChannel);

	if(loopType>=0)
		loopTypeEdit->setText(istring(loopType).c_str());
	loopTypeFrame->show();

	onWaitCheckButton(NULL,0,NULL);

	recalc();


	redo:
	if(execute(PLACEMENT_SCREEN))
	{
		waveformId=atoi(waveformIdEdit->getText().text());
		if(waveformId<0 || waveformId>16384)
		{
			Error(_("Waveform IDs must be from 0 to 16384"));
			goto redo;
		}

		sysExChannel=sysExChannelSpinner->getValue();

		loopType=atoi(loopTypeEdit->getText().text());
		if(loopType<0 || loopType>127)
		{
			Error(_("Loop type must be from 0 to 127"));
			goto redo;
		}

		return true;
	}
	return false;
}

long CMIDIDumpSampleIdDialog::onWaitCheckButton(FXObject *sender,FXuint sel,void *ptr)
{
	if(waitCheckButton->getCheck())
	{
		waveformIdEdit->disable();
		sysExChannelSpinner->disable();
	}
	else
	{
		waveformIdEdit->enable();
		sysExChannelSpinner->enable();
	}

	return 1;
}

