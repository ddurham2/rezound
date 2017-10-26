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

#include "Mp3SaveParametersDialog.h"

#include <stdexcept>
#include <istring>

#include "../backend/CActionParameters.h"

Mp3SaveParametersDialog::Mp3SaveParametersDialog(QWidget *mainWindow) :
	ActionParamDialog(mainWindow)
{
	setTitle(N_("MP3 Compression Parameters"));

	vector<string> br;
		br.push_back("16000");
		br.push_back("32000");
		br.push_back("40000");
		br.push_back("48000");
		br.push_back("56000");
		br.push_back("64000");
		br.push_back("80000");
		br.push_back("96000");
		br.push_back("112000");
		br.push_back("128000");
		br.push_back("160000");
		br.push_back("192000");
		br.push_back("224000");
		br.push_back("256000");
		br.push_back("320000");

	QWidget *p=newVertPanel(NULL);

		// quality
		qualityGroupBox=new QGroupBox(_("Quality Setting"));
		qualityGroupBox->setCheckable(true);
		connect(qualityGroupBox,SIGNAL(toggled(bool)),this,SLOT(onGroupBoxToggled(bool)));
		qualityGroupBox->setLayout(new QVBoxLayout);
		p->layout()->addWidget(qualityGroupBox);
			qualityEdit=addNumericTextEntry(qualityGroupBox,N_("Quality:"),"",0.7,0,1);
			qualityEdit->setStep(0.1);
				// ??? key: "0 (lowest) to 1 (highest)"


		// constant bit rate
		CBRGroupBox=new QGroupBox(_("Constant Bit Rate"));
		CBRGroupBox->setCheckable(true);
		connect(CBRGroupBox,SIGNAL(toggled(bool)),this,SLOT(onGroupBoxToggled(bool)));
		CBRGroupBox->setLayout(new QVBoxLayout);
		p->layout()->addWidget(CBRGroupBox);
			CBRCombo=addComboTextEntry(CBRGroupBox,N_("Bit Rate:"),br,ActionParamDialog::cpvtAsInteger,"");
			CBRCombo->setIntegerValue(9);
			
		// variable bit rate
		ABRGroupBox=new QGroupBox(_("Average Bit Rate"));
		ABRGroupBox->setLayout(new QVBoxLayout);
		ABRGroupBox->setCheckable(true);
		connect(ABRGroupBox,SIGNAL(toggled(bool)),this,SLOT(onGroupBoxToggled(bool)));
		p->layout()->addWidget(ABRGroupBox);
			minABRCombo=addComboTextEntry(ABRGroupBox,N_("Minimum Bit Rate:"),br,ActionParamDialog::cpvtAsInteger,"");
			minABRCombo->setIntegerValue(9);
			normalABRCombo=addComboTextEntry(ABRGroupBox,N_("Normal Bit Rate:"),br,ActionParamDialog::cpvtAsInteger,"");
			normalABRCombo->setIntegerValue(9);
			maxABRCombo=addComboTextEntry(ABRGroupBox,N_("Maximum Bit Rate:"),br,ActionParamDialog::cpvtAsInteger,"");
			maxABRCombo->setIntegerValue(9);

		QWidget *frame=newVertPanel(p,true,true);
		flagsEdit=addStringTextEntry(frame,_("Additional Flags to lame"),"");
		flagsEdit->showFrame(false);
		flagsEdit->enableMargin(false);
		useOnlyFlagsCheckBox=addCheckBoxEntry(frame,_("Use Only These Flags"),false);
		useOnlyFlagsCheckBox->showFrame(false);
		useOnlyFlagsCheckBox->enableMargin(false);

		connect(useOnlyFlagsCheckBox,SIGNAL(changed()),this,SLOT(onUseOnlyFlagsCheckBox_changed()));

		qualityGroupBox->setChecked(true);
		CBRGroupBox->setChecked(false);
		ABRGroupBox->setChecked(false);
}

void Mp3SaveParametersDialog::onGroupBoxToggled(bool on)
{
	if(!on)
		return; // only care about on events


	qualityGroupBox->blockSignals(true);
	CBRGroupBox->blockSignals(true);
	ABRGroupBox->blockSignals(true);

		qualityGroupBox->setChecked(false);
		CBRGroupBox->setChecked(false);
		ABRGroupBox->setChecked(false);
		
		((QGroupBox *)sender())->setChecked(true);

	qualityGroupBox->blockSignals(false);
	CBRGroupBox->blockSignals(false);
	ABRGroupBox->blockSignals(false);
}

void Mp3SaveParametersDialog::onUseOnlyFlagsCheckBox_changed()
{
	bool v=!(useOnlyFlagsCheckBox->getValue());
	qualityGroupBox->setEnabled(v);
	CBRGroupBox->setEnabled(v);
	ABRGroupBox->setEnabled(v);
}

bool Mp3SaveParametersDialog::show(AFrontendHooks::Mp3CompressionParameters &parameters)
{
	CActionParameters actionParameters(NULL);

	// set from defaults ??

	if(ActionParamDialog::show(NULL,&actionParameters))
	{
		parameters.method=qualityGroupBox->isChecked() ? AFrontendHooks::Mp3CompressionParameters::brQuality : AFrontendHooks::Mp3CompressionParameters::brABR;

		if(CBRGroupBox->isChecked())
		{
			parameters.minBitRate=parameters.normBitRate=parameters.maxBitRate=istring(CBRCombo->getStringValue()).to<int>();
		}
		else
		{
			parameters.minBitRate=istring(minABRCombo->getStringValue()).to<int>();
			parameters.normBitRate=istring(normalABRCombo->getStringValue()).to<int>();
			parameters.maxBitRate=istring(maxABRCombo->getStringValue()).to<int>();
		}

		parameters.additionalFlags=flagsEdit->getText();
		parameters.useFlagsOnly=useOnlyFlagsCheckBox->getValue();

		return true;
	}
	else
		return false;
}

