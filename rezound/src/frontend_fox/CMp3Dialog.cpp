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

#include "CMp3Dialog.h"

#include <stdlib.h>

#include <istring>


FXDEFMAP(CMp3Dialog) CMp3DialogMap[]=
{
//	Message_Type			ID					Message_Handler
	FXMAPFUNC(SEL_COMMAND,		CMp3Dialog::ID_WHICH_BUTTON,		CMp3Dialog::onRadioButton),
};
		

FXIMPLEMENT(CMp3Dialog,FXModalDialogBox,CMp3DialogMap,ARRAYNUMBER(CMp3DialogMap))



// ----------------------------------------


static void fillBitRateComboBox(FXComboBox *c,bool t=false)
{
	c->appendItem("16000");
	c->appendItem("32000");
	c->appendItem("40000");
	c->appendItem("48000");
	c->appendItem("56000");
	c->appendItem("64000");
	c->appendItem("80000");
	c->appendItem("96000");
	c->appendItem("112000");
	c->appendItem("128000");
	c->appendItem("160000");
	c->appendItem("192000");
	c->appendItem("224000");
	c->appendItem("256000");
	if(!t)
		c->appendItem("320000");
	else
		c->appendItem("310000");

	c->setCurrentItem(9);
	c->setNumVisible(c->getNumItems());
}

CMp3Dialog::CMp3Dialog(FXWindow *mainWindow) :
	FXModalDialogBox(mainWindow,N_("mp3 Compression Parameters"),0,0,FXModalDialogBox::ftVertical)//,

	//notesFrame(new FXPacker(getFrame(),FRAME_SUNKEN|FRAME_THICK | LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 0,0)),
		//notesTextBox(new FXText(notesFrame,NULL,0,TEXT_WORDWRAP | LAYOUT_FILL_X|LAYOUT_FILL_Y))
{
	FXComposite *main=new FXGroupBox(getFrame(),"",GROUPBOX_NORMAL|LAYOUT_FILL_X|LAYOUT_FILL_Y);

	CBRButton=new FXRadioButton(main,_("Constant Bit Rate"),this,ID_WHICH_BUTTON);
		CBRFrame=new FXMatrix(main,2,MATRIX_BY_COLUMNS|FRAME_RAISED|LAYOUT_FILL_X);
			new FXLabel(CBRFrame,_("Bit Rate: "));
			bitRateComboBox=new FXComboBox(CBRFrame,10,8,NULL,0,COMBOBOX_NORMAL|FRAME_SUNKEN|FRAME_THICK|COMBOBOX_STATIC);
			fillBitRateComboBox(bitRateComboBox);


	new FXFrame(main,LAYOUT_FIX_HEIGHT,0,0,1,10);
	ABRButton=new FXRadioButton(main,_("Average Bit Rate"),this,ID_WHICH_BUTTON);
		ABRFrame=new FXMatrix(main,2,MATRIX_BY_COLUMNS|FRAME_RAISED|LAYOUT_FILL_X);
			new FXLabel(ABRFrame,_("Minimum Bit Rate: "));
			minRateComboBox=new FXComboBox(ABRFrame,10,8,NULL,0,COMBOBOX_NORMAL|FRAME_SUNKEN|FRAME_THICK|COMBOBOX_STATIC);
			fillBitRateComboBox(minRateComboBox);
			new FXLabel(ABRFrame,_("Average Bit Rate: "));
			normRateComboBox=new FXComboBox(ABRFrame,10,8,NULL,0,COMBOBOX_NORMAL|FRAME_SUNKEN|FRAME_THICK|COMBOBOX_STATIC);
			fillBitRateComboBox(normRateComboBox,true);
			new FXLabel(ABRFrame,_("Maximum Bit Rate: "));
			maxRateComboBox=new FXComboBox(ABRFrame,10,8,NULL,0,COMBOBOX_NORMAL|FRAME_SUNKEN|FRAME_THICK|COMBOBOX_STATIC);
			fillBitRateComboBox(maxRateComboBox);


	new FXFrame(main,LAYOUT_FIX_HEIGHT,0,0,1,10);
	qualityButton=new FXRadioButton(main,_("Variable Bit Rate Quality Setting"),this,ID_WHICH_BUTTON);
		qualityFrame=new FXMatrix(main,3,MATRIX_BY_COLUMNS|FRAME_RAISED|LAYOUT_FILL_X);
			new FXLabel(qualityFrame,_("Quality: "));
			qualityComboBox=new FXComboBox(qualityFrame,10,10,NULL,0,COMBOBOX_NORMAL|FRAME_SUNKEN|FRAME_THICK|COMBOBOX_STATIC);
			for(int t=0;t<10;t++)
				qualityComboBox->appendItem(istring(t).c_str());
			qualityComboBox->setCurrentItem(4);
			new FXLabel(qualityFrame,_("0(Highest Quality -> 9(Lowest Quality)"));

	
	new FXFrame(main,LAYOUT_FIX_HEIGHT,0,0,1,10);
		flagsFrame=new FXMatrix(main,3,MATRIX_BY_COLUMNS|FRAME_RAISED|LAYOUT_FILL_X);
			useOnlyFlagsButton=new FXCheckButton(flagsFrame,_("Use Only These Flags"));
			new FXFrame(flagsFrame,FRAME_NONE); // place holder
			new FXFrame(flagsFrame,FRAME_NONE); // place holder

			new FXLabel(flagsFrame,_("Additional Flags: "));
			additionalFlagsTextBox=new FXTextField(flagsFrame,25,NULL,0,TEXTFIELD_NORMAL);
			additionalFlagsTextBox->setText("");
			new FXLabel(flagsFrame,_("To lame"));

	
	CBRButton->setCheck(TRUE);
	onRadioButton(NULL,0,(void *)1); // to disable all but CBR
	

}

CMp3Dialog::~CMp3Dialog()
{
}

bool CMp3Dialog::show(AFrontendHooks::Mp3CompressionParameters &parameters)
{
	if(execute(PLACEMENT_SCREEN))
	{
		if(CBRFrame->isEnabled())
		{
			parameters.method=AFrontendHooks::Mp3CompressionParameters::brCBR;
			parameters.constantBitRate=atoi(bitRateComboBox->getText().text());
		}
		else if(ABRFrame->isEnabled())
		{
			parameters.method=AFrontendHooks::Mp3CompressionParameters::brABR;
			parameters.minBitRate=atoi(minRateComboBox->getText().text());
			parameters.normBitRate=atoi(normRateComboBox->getText().text());
			parameters.maxBitRate=atoi(maxRateComboBox->getText().text());
		}
		else if(qualityFrame->isEnabled())
		{
			parameters.method=AFrontendHooks::Mp3CompressionParameters::brQuality;
			parameters.quality=atoi(qualityComboBox->getText().text());
		}

		parameters.useFlagsOnly= useOnlyFlagsButton->getCheck()==TRUE;

		parameters.additionalFlags= additionalFlagsTextBox->getText().text();

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

long CMp3Dialog::onRadioButton(FXObject *sender,FXSelector sel,void *ptr)
{
	if((int)ptr==0) // only act when ptr==1 when it's getting checked
		return 1;

	setEnable((FXWindow *)CBRFrame,false);
	setEnable((FXWindow *)ABRFrame,false);
	setEnable((FXWindow *)qualityFrame,false);

	if(CBRButton->getCheck()==TRUE)
		setEnable((FXWindow *)CBRFrame,true);
	else if(ABRButton->getCheck()==TRUE)
		setEnable((FXWindow *)ABRFrame,true);
	else if(qualityButton->getCheck()==TRUE)
		setEnable((FXWindow *)qualityFrame,true);

	return 1;
}
