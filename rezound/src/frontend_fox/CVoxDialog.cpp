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

#include "CVoxDialog.h"

#include <stdlib.h>

#include <istring>

FXDEFMAP(CVoxDialog) CVoxDialogMap[]=
{
//	Message_Type			ID					Message_Handler
//	FXMAPFUNC(SEL_COMMAND,		CVoxDialog::ID_WHICH_BUTTON,		CVoxDialog::onRadioButton),
};
		

FXIMPLEMENT(CVoxDialog,FXModalDialogBox,CVoxDialogMap,ARRAYNUMBER(CVoxDialogMap))



// ----------------------------------------


CVoxDialog::CVoxDialog(FXWindow *mainWindow) :
	FXModalDialogBox(mainWindow,"Vox Parameters",0,0,FXModalDialogBox::ftVertical)
{
	FXComposite *main=new FXMatrix(getFrame(),2,MATRIX_BY_COLUMNS,LAYOUT_FILL_X|LAYOUT_FILL_Y);
	FXComboBox *combo;

	new FXLabel(main,"Channels:");
	combo=channelsCountComboBox=new FXComboBox(main,10,8,NULL,0,COMBOBOX_NORMAL|FRAME_SUNKEN|FRAME_THICK);
		for(unsigned t=1;t<=MAX_CHANNELS;t++)
			combo->appendItem(istring(t).c_str());
		combo->setCurrentItem(1); // stereo

	new FXLabel(main,"Sample Rate:");
	combo=sampleRateComboBox=new FXComboBox(main,10,9,NULL,0,COMBOBOX_NORMAL|FRAME_SUNKEN|FRAME_THICK);
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
}

CVoxDialog::~CVoxDialog()
{
}

bool CVoxDialog::show(AFrontendHooks::VoxParameters &parameters)
{
	if(execute(PLACEMENT_SCREEN))
	{
		parameters.channelCount=atoi(channelsCountComboBox->getText().text());
		parameters.sampleRate=atoi(sampleRateComboBox->getText().text());

		return(true);
	}
	return(false);
}

