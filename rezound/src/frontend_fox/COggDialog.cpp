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

#include "COggDialog.h"

#include <stdlib.h>

FXDEFMAP(COggDialog) COggDialogMap[]=
{
//	Message_Type			ID					Message_Handler
	FXMAPFUNC(SEL_COMMAND,		COggDialog::ID_WHICH_BUTTON,		COggDialog::onRadioButton),
};
		

FXIMPLEMENT(COggDialog,FXModalDialogBox,COggDialogMap,ARRAYNUMBER(COggDialogMap))



// ----------------------------------------


static void fillBitRateComboBox(FXComboBox *c)
{
	c->appendItem("8000");
	c->appendItem("16000");
	c->appendItem("32000");
	c->appendItem("64000");
	c->appendItem("128000");
	c->appendItem("160000");
	c->appendItem("192000");
	c->appendItem("256000");
	c->appendItem("320000");

	c->setCurrentItem(4);
	c->setNumVisible(c->getNumItems());
}

COggDialog::COggDialog(FXWindow *mainWindow) :
	FXModalDialogBox(mainWindow,N_("Ogg Compression Parameters"),0,0,FXModalDialogBox::ftVertical)//,

	//notesFrame(new FXPacker(getFrame(),FRAME_SUNKEN|FRAME_THICK | LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 0,0)),
		//notesTextBox(new FXText(notesFrame,NULL,0,TEXT_WORDWRAP | LAYOUT_FILL_X|LAYOUT_FILL_Y))
{
	FXComposite *main=new FXGroupBox(getFrame(),"",GROUPBOX_NORMAL|LAYOUT_FILL_X|LAYOUT_FILL_Y);

	qualityButton=new FXRadioButton(main,_("Quality Setting"),this,ID_WHICH_BUTTON);
		qualityFrame=new FXMatrix(main,2,MATRIX_BY_COLUMNS|FRAME_RAISED|LAYOUT_FILL_X);
			new FXLabel(qualityFrame,_("Quality: "));
			qualityTextBox=new FXTextField(qualityFrame,10,NULL,0,TEXTFIELD_REAL|TEXTFIELD_NORMAL);
			qualityTextBox->setText("0.7");
			new FXLabel(qualityFrame,"");
			new FXLabel(qualityFrame,_("0 (lowest) to 1 (highest)"));

	
	new FXFrame(main,LAYOUT_FIX_HEIGHT,0,0,1,10);
	CBRButton=new FXRadioButton(main,_("Constant Bit Rate"),this,ID_WHICH_BUTTON);
		CBRFrame=new FXMatrix(main,2,MATRIX_BY_COLUMNS|FRAME_RAISED|LAYOUT_FILL_X);
			new FXLabel(CBRFrame,_("Bit Rate: "));
			bitRateComboBox=new FXComboBox(CBRFrame,10,8,NULL,0,COMBOBOX_NORMAL|FRAME_SUNKEN|FRAME_THICK|COMBOBOX_STATIC);
			fillBitRateComboBox(bitRateComboBox);


	new FXFrame(main,LAYOUT_FIX_HEIGHT,0,0,1,10);
	VBRButton=new FXRadioButton(main,_("Variable Bit Rate"),this,ID_WHICH_BUTTON);
		VBRFrame=new FXMatrix(main,2,MATRIX_BY_COLUMNS|FRAME_RAISED|LAYOUT_FILL_X);
			new FXLabel(VBRFrame,_("Minimum Bit Rate: "));
			minRateComboBox=new FXComboBox(VBRFrame,10,8,NULL,0,COMBOBOX_NORMAL|FRAME_SUNKEN|FRAME_THICK|COMBOBOX_STATIC);
			fillBitRateComboBox(minRateComboBox);
			new FXLabel(VBRFrame,_("Normal Bit Rate: "));
			normRateComboBox=new FXComboBox(VBRFrame,10,8,NULL,0,COMBOBOX_NORMAL|FRAME_SUNKEN|FRAME_THICK|COMBOBOX_STATIC);
			fillBitRateComboBox(normRateComboBox);
			new FXLabel(VBRFrame,_("Maximum Bit Rate: "));
			maxRateComboBox=new FXComboBox(VBRFrame,10,8,NULL,0,COMBOBOX_NORMAL|FRAME_SUNKEN|FRAME_THICK|COMBOBOX_STATIC);
			fillBitRateComboBox(maxRateComboBox);


	onRadioButton(qualityButton,0,(void *)1); // to disable all but Quality
	

}

COggDialog::~COggDialog()
{
}

bool COggDialog::show(AFrontendHooks::OggCompressionParameters &parameters)
{
	if(execute(PLACEMENT_SCREEN))
	{
		parameters.method=qualityFrame->isEnabled() ? AFrontendHooks::OggCompressionParameters::brQuality : AFrontendHooks::OggCompressionParameters::brVBR;
		parameters.quality=atof(qualityTextBox->getText().text());

		if(CBRFrame->isEnabled())
		{
			parameters.minBitRate=parameters.normBitRate=parameters.maxBitRate=atoi(bitRateComboBox->getText().text());
		}
		else
		{
			parameters.minBitRate=atoi(minRateComboBox->getText().text());
			parameters.normBitRate=atoi(normRateComboBox->getText().text());
			parameters.maxBitRate=atoi(maxRateComboBox->getText().text());
		}

		return(true);
	}
	return(false);
}

template <class type> void setEnable(type *t,bool enabled)
{
	if(enabled)
		t->enable();
	else
		t->disable();

	for(FXint i=0;i<t->numChildren();i++)
		setEnable(t->childAtIndex(i),enabled);
}

long COggDialog::onRadioButton(FXObject *sender,FXSelector sel,void *ptr)
{
	if((int)ptr==0) // only act when ptr==1 when it's getting checked
		return 1;

	// turn off all buttons
	CBRButton->setCheck(FALSE);
	setEnable((FXWindow *)CBRFrame,false);

	VBRButton->setCheck(FALSE);
	setEnable((FXWindow *)VBRFrame,false);

	qualityButton->setCheck(FALSE);
	setEnable((FXWindow *)qualityFrame,false);

	// enable button that was clicked
	if(sender==CBRButton)
	{
	       	CBRButton->setCheck(TRUE);
		setEnable((FXWindow *)CBRFrame,true);
	}
	else if(sender==VBRButton)
	{
		VBRButton->setCheck(TRUE);
		setEnable((FXWindow *)VBRFrame,true);
	}
	else if(sender==qualityButton)
	{
		qualityButton->setCheck(TRUE);
		setEnable((FXWindow *)qualityFrame,true);
	}

	return 0;
}
