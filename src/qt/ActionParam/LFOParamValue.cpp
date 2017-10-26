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

#include "LFOParamValue.h"

#include <QHBoxLayout>
#include <QListView>

#include <CNestedDataFile/CNestedDataFile.h>

#include "ConstantParamValue.h"

#include "../../backend/ActionParamMappers.h"
#include "../settings.h"

/*
	- This is the LFO selection widget used over and over by ReZound on action dialogs

	- NOTE: I am using FXListBox's itemData point to store indexes into the LFO 
	  registry rather than just in the index of the item in the FXListBox because 
	  sometimes I exclude bipolar LFOs, which makes the index off.
*/

LFOParamValue::LFOParamValue(const char *_name,const string ampUnits,const string ampTitle,const double maxAmp,const string freqUnits,const double maxFreq,const bool hideBipolarLFOs) :
	name(_name),

	amplitudeSlider(new ConstantParamValue(
		new CActionParamMapper_linear(1.0,min((int)maxAmp,1),min((int)maxAmp,1),(int)maxAmp),
		false,
		gettext(ampTitle.c_str())
	)),

	frequencySlider(new ConstantParamValue(
		new CActionParamMapper_linear(1.0,min((int)maxFreq,1),min((int)maxFreq,1),(int)maxFreq),
		false,
		N_("Frequency")
	)),

	phaseSlider(new ConstantParamValue(
		new CActionParamMapper_linear(90.0,360,0,0),
		true,
		N_("Phase")
	))
{
	setupUi(this);

	label->setText(gettext(_name));

	amplitudeSlider->setMinSize(0,0);
	frequencySlider->setMinSize(0,0);
	phaseSlider->setMinSize(0,0);

	sliderBox->setLayout(new QHBoxLayout());
	sliderBox->layout()->setMargin(0);
	sliderBox->layout()->setSpacing(4);
	sliderBox->layout()->addWidget(amplitudeSlider);
	sliderBox->layout()->addWidget(frequencySlider);
	sliderBox->layout()->addWidget(phaseSlider);

	// allow this to be hidden (cause varied repeat doesn't need it)
	if(ampTitle=="")
		amplitudeSlider->hide();

	amplitudeSlider->setUnits(ampUnits.c_str());
	frequencySlider->setUnits(freqUnits.c_str());
	phaseSlider->setUnits("deg");

	LFOTypeComboBox->setIconSize(QSize(45,25));
	dynamic_cast<QListView *>(LFOTypeComboBox->view())->setSpacing(3);
	for(size_t t=0;t<gLFORegistry.getCount();t++)
	{
		if(!hideBipolarLFOs || !gLFORegistry.isBipolar(t))
		{
			LFOTypeComboBox->addItem(loadIcon(gLFORegistry.getName(t)+".png"),gettext(gLFORegistry.getName(t).c_str()),QVariant((int)t));
		}
	}
	LFOTypeComboBox->setCurrentIndex(0);
}

LFOParamValue::~LFOParamValue()
{
}

void LFOParamValue::setMinSize(int _minWidth,int _minHeight)
{
	setMinimumSize(_minWidth,_minHeight);
}

void LFOParamValue::on_LFOTypeComboBox_currentIndexChanged(int index)
{
	if((LFOTypeComboBox->itemData(LFOTypeComboBox->currentIndex())).toInt()==3)
	{ // constant needs only to have an amplitude setting
		amplitudeSlider->enable();
		frequencySlider->disable();
		phaseSlider->disable();
	}
	else
	{
		amplitudeSlider->enable();
		frequencySlider->enable();
		phaseSlider->enable();
	}
}

const CLFODescription LFOParamValue::getValue()
{
	//validateRange();
	return CLFODescription(
		amplitudeSlider->getValue(),
		frequencySlider->getValue(),
		phaseSlider->getValue(),
		LFOTypeComboBox->itemData(LFOTypeComboBox->currentIndex()).toInt()
	);
}

const string LFOParamValue::getName() const
{
	return name;
}

/*
void LFOParamValue::setTipText(const FXString &text)
{
	titleLabel->setTipText(text);	
	valueTextBox->setTipText(text);
	valueSpinner->setTipText(text);
}

QString LFOParamValue::getTipText() const
{
	return titleLabel->getTipText();
}
*/

void LFOParamValue::readFromFile(const string &prefix,CNestedDataFile *f)
{
	const string key=prefix DOT getName();
	const string LFOName=f->keyExists(key DOT "name") ? f->getValue<string>(key DOT "name") : "Constant";
	try
	{
		const int LFOIndex=gLFORegistry.getIndexByName(LFOName);
		for(int t=0;t<LFOTypeComboBox->count();t++)
		{
			if((LFOTypeComboBox->itemData(t)).toInt()==LFOIndex)
			{
				LFOTypeComboBox->setCurrentIndex(t);
				break;
			}
		}
	}
	catch(...)
	{
		LFOTypeComboBox->setCurrentIndex(0); // default to the first position which should be sine
	}

	if(amplitudeSlider->getName()!="")
		amplitudeSlider->readFromFile(key,f);
	frequencySlider->readFromFile(key,f);
	phaseSlider->readFromFile(key,f);
}

void LFOParamValue::writeToFile(const string &prefix,CNestedDataFile *f)
{
	const string key=prefix DOT getName();
	f->setValue<string>(key DOT "name", gLFORegistry.getName(LFOTypeComboBox->itemData(LFOTypeComboBox->currentIndex()).toInt()) );

	if(amplitudeSlider->getName()!="")
		amplitudeSlider->writeToFile(key,f);
	frequencySlider->writeToFile(key,f);
	phaseSlider->writeToFile(key,f);
}


