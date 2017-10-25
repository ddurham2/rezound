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

#include "CChannelSelectDialog.h"

#include <QCheckBox>
#include <QVBoxLayout>

#include <istring>

#include "../../backend/CSound.h"


CChannelSelectDialog *gChannelSelectDialog=NULL;


// ----------------------------------------

CChannelSelectDialog::CChannelSelectDialog(QWidget *mainWindow) :
/*
 * having the title be translated is fine, except the fact that I use them in preset names
 * what I need to do is avoid ever calling getTitle() on a widget 
 * I need to implement something like a  getOrigTitle() which stores the original value of
 * the title.  I should use N_(...) when passing a string to be the title except _(...) when
 * actually giving that string to Qt so it can render the translated title, and I should save
 * the original in origTitle for getOrigTitle to return and use in presets
 *
 * this goes for derivations of CModalDialog and all action param value widgets
 *
 * ??? I *think* I can make this _() instead of N_() on the title.. I don't call getTitle() anywhere
 */
	CModalDialog(mainWindow,N_("Channel Select")/*,100,100*/,CModalDialog::ftVertical)
{
	csdc.setupUi(getFrame());

	csdc.label->setText(_("Channels to Which This Action Should Apply:"));

	csdc.checkBoxFrame->setLayout(new QVBoxLayout);
	for(unsigned t=0;t<MAX_CHANNELS;t++)			    // ??? could map it to some name like "Left, Right, Center, Bass... etc"
	{
		checkBoxes[t]=new QCheckBox((_("Channel ")+istring(t)).c_str());
		csdc.checkBoxFrame->layout()->addWidget(checkBoxes[t]);
	}

	connect(csdc.clearButton,SIGNAL(clicked()),this,SLOT(onClearButton()));
	connect(csdc.defaultButton,SIGNAL(clicked()),this,SLOT(onDefaultButton()));
}

CChannelSelectDialog::~CChannelSelectDialog()
{
}

bool CChannelSelectDialog::show(CActionSound *actionSound,CActionParameters *actionParameters)
{
	this->actionSound=actionSound;

	// don't show the dialog if there is only one channel
	if(actionSound->sound->getChannelCount()<=1)
	{
		actionSound->doChannel[0]=true;
		return true;
	}
	
	// uncheck all check boxes
	// only enable the check boxes that there are channels for
	for(unsigned t=0;t<MAX_CHANNELS;t++)
	{
		checkBoxes[t]->setCheckState(Qt::Unchecked);
		checkBoxes[t]->setVisible(t<actionSound->sound->getChannelCount());
	}

	for(unsigned t=0;t<actionSound->sound->getChannelCount();t++)
		checkBoxes[t]->setCheckState(actionSound->doChannel[t] ? Qt::Checked : Qt::Unchecked);

	/*
	// when the number of shown or hidden widgets changes the frame needs to be told to recalc
	getFrame()->recalc();
	*/

	if(exec())
	{
		bool ret=false; // or all the checks together... if it's false, then it's like hitting cancel
		for(unsigned t=0;t<MAX_CHANNELS;t++)
		{
			actionSound->doChannel[t]=checkBoxes[t]->checkState()==Qt::Checked ? true : false;
			ret|=actionSound->doChannel[t];
		}

		return 1;
	}
	return 0;

}

void CChannelSelectDialog::hide()
{
	CModalDialog::hide();
}

void CChannelSelectDialog::onDefaultButton()
{
	onClearButton();

	for(unsigned x=0;x<actionSound->sound->getChannelCount();x++)
		checkBoxes[x]->setCheckState(actionSound->doChannel[x] ? Qt::Checked : Qt::Unchecked);
}

void CChannelSelectDialog::onClearButton()
{
	for(unsigned x=0;x<MAX_CHANNELS;x++)
		checkBoxes[x]->setCheckState(Qt::Unchecked);
}

