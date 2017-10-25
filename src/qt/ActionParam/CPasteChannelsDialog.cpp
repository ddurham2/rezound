/* 
 * Copyright (C) 2007 - David W. Durham
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

#include <QGridLayout>

#include <algorithm>

#include <istring>

#include "../settings.h"

#include "../../backend/AAction.h"
#include "../../backend/CActionSound.h"
#include "../../backend/CActionParameters.h"
#include "../../backend/ASoundClipboard.h"

#include "../../backend/ActionParamMappers.h"

CPasteChannelsDialog *gPasteChannelsDialog=NULL;


// ----------------------------------------

// ??? derive this from CActionDialog if I can so that it can have presets, maybe not possible as it is

#include "CConstantParamValue.h"

CPasteChannelsDialog::CPasteChannelsDialog(QWidget *mainWindow) :
	CModalDialog(mainWindow,N_("Paste Channels")/*,100,100*/,CModalDialog::ftVertical)
{
	pcdc.setupUi(getFrame());

	pcdc.checkBoxFrame->setLayout(new QGridLayout);

	// put top source (clipboard) labels 
	for(unsigned t=0;t<MAX_CHANNELS;t++) // clipboard
	{
		srcLabels[t]=new QLabel((_("Channel ")+istring(t)).c_str());
		((QGridLayout *)(pcdc.checkBoxFrame->layout()))->addWidget(srcLabels[t],0,t+1);
	}

	// put top destination (clipboard) labels 
	for(unsigned t=0;t<MAX_CHANNELS;t++) // destination channel
	{
		destLabels[t]=new QLabel((_("Channel ")+istring(t)).c_str());
		((QGridLayout *)(pcdc.checkBoxFrame->layout()))->addWidget(destLabels[t],t+1,0);
	}

	// build the grid of checkboxes
	for(unsigned row=0;row<MAX_CHANNELS;row++)
	for(unsigned col=0;col<MAX_CHANNELS;col++)
	{
		checkBoxes[row][col]=new QCheckBox;
		((QGridLayout *)(pcdc.checkBoxFrame->layout()))->addWidget(checkBoxes[row][col],row+1,col+1);
	}

	// setup the repeat type controls

	while(pcdc.repeatStackedWidget->count()>0)
	{	
		QWidget *w=pcdc.repeatStackedWidget->widget(0);
		pcdc.repeatStackedWidget->removeWidget(w);
		delete w;
	}

	repeatCountSlider=new CConstantParamValue(
		new CActionParamMapper_linear_bipolar(1.0,4,100),
		false,
		_("Repeat Count")
	);
	repeatCountSlider->setUnits("x");

	pcdc.repeatStackedWidget->addWidget(repeatCountSlider);
	pcdc.repeatTypeComboBox->addItem(_("Repeat X Times"));


	repeatTimeSlider=new CConstantParamValue(
		new CActionParamMapper_linear_bipolar(10.0,60,3600),
		false,
		_("Repeat Time")
	);
	repeatTimeSlider->setUnits("s");
	
	pcdc.repeatStackedWidget->addWidget(repeatTimeSlider);
	pcdc.repeatTypeComboBox->addItem(_("Repeat for X Seconds"));

	pcdc.repeatTypeComboBox->setMaxVisibleItems(2);
	connect(pcdc.repeatTypeComboBox,SIGNAL(activated(int)),pcdc.repeatStackedWidget,SLOT(setCurrentIndex(int)));

	
	connect(pcdc.clearButton,SIGNAL(clicked()),this,SLOT(onClearButton()));

	connect(pcdc.defaultButton,SIGNAL(clicked()),this,SLOT(onDefaultButton()));

		// ??? this could be done in designer
	pcdc.mixTypeComboBox->setMaxVisibleItems(4);
	pcdc.mixTypeComboBox->addItem(_("Add"),(int)mmAdd);
	pcdc.mixTypeComboBox->addItem(_("Subtract"),(int)mmSubtract);
	pcdc.mixTypeComboBox->addItem(_("Multiply"),(int)mmMultiply);
	pcdc.mixTypeComboBox->addItem(_("Average"),(int)mmAverage);
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
		QCheckBox *b=checkBoxes[row][col];

		b->setEnabled(true);

		if(row<actionSound->sound->getChannelCount() && col<clipboard->getChannelCount())
			b->show();
		else
			b->hide();

		// let it show, but disabled since there is no data for that channel in the clipboard
		if(!clipboard->getWhichChannels()[col])
			b->setEnabled(false);
	}

	// show/hide labels across the top that matter
	for(unsigned t=0;t<MAX_CHANNELS;t++)
		srcLabels[t]->setVisible(t<clipboard->getChannelCount());

	// show/hide labels on the left that matter
	for(unsigned t=0;t<MAX_CHANNELS;t++)
		destLabels[t]->setVisible(t<actionSound->sound->getChannelCount());

	// by default check a 1:1 paste mapping
	onDefaultButton();

	// if there is only one checkbox, then make it checked and hide it and the labels
	if(actionSound->sound->getChannelCount()==1 && clipboard->getChannelCount()==1)
	{
		checkBoxes[0][0]->setCheckState(Qt::Checked);
		pcdc.vertLabel->hide();
		pcdc.horzLabel->hide();
		pcdc.checkBoxFrame->hide();
		pcdc.vertLine->hide();
	}
	else
	{
		pcdc.vertLabel->show();
		pcdc.horzLabel->show();
		pcdc.checkBoxFrame->show();
		pcdc.vertLine->show();
	}


	if(exec())
	{
		pasteChannels.clear();

		actionParameters->setValue<unsigned>(_("MixMethod"),(pcdc.mixTypeComboBox->itemData(pcdc.mixTypeComboBox->currentIndex()).toUInt()));
		
		if(pcdc.repeatTypeComboBox->currentIndex()==0)
		{ // repeating it a given number of times
			actionParameters->setValue<double>(_("Repeat Count"),repeatCountSlider->getValue());
		}
		else
		{ // repeating it for a given number of seconds
			actionParameters->setValue<double>(_("Repeat Count"),
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
				QCheckBox *b=checkBoxes[row][col];
				pasteChannels[row].push_back(!b->isHidden() ? b->checkState()==Qt::Checked : false);
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
	CModalDialog::hide();
}

void *CPasteChannelsDialog::getUserData()
{
	return &pasteChannels;
}


void CPasteChannelsDialog::onDefaultButton()
{
	onClearButton();

	for(unsigned t=0;t<actionSound->sound->getChannelCount();t++)
	{
		QCheckBox *b=checkBoxes[t][t];
		if(!b->isHidden() && b->isEnabled())
			b->setCheckState(Qt::Checked);
	}
}

void CPasteChannelsDialog::onClearButton()
{
	for(unsigned row=0;row<MAX_CHANNELS;row++)
	for(unsigned col=0;col<MAX_CHANNELS;col++)
		checkBoxes[row][col]->setCheckState(Qt::Unchecked);
}

