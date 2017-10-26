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

#include "OggSaveParametersDialog.h"

#include <stdexcept>
#include <istring>

#include "../backend/CActionParameters.h"

OggSaveParametersDialog::OggSaveParametersDialog(QWidget *mainWindow) :
	ActionParamDialog(mainWindow)
{
	setTitle(N_("Ogg Compression Parameters"));

	vector<string> br;
		br.push_back("8000");
		br.push_back("16000");
		br.push_back("32000");
		br.push_back("64000");
		br.push_back("128000");
		br.push_back("160000");
		br.push_back("192000");
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
			CBRCombo->setIntegerValue(4);
			
		// variable bit rate
		VBRGroupBox=new QGroupBox(_("Variable Bit Rate"));
		VBRGroupBox->setLayout(new QVBoxLayout);
		VBRGroupBox->setCheckable(true);
		connect(VBRGroupBox,SIGNAL(toggled(bool)),this,SLOT(onGroupBoxToggled(bool)));
		p->layout()->addWidget(VBRGroupBox);
			minVBRCombo=addComboTextEntry(VBRGroupBox,N_("Minimum Bit Rate:"),br,ActionParamDialog::cpvtAsInteger,"");
			minVBRCombo->setIntegerValue(4);
			normalVBRCombo=addComboTextEntry(VBRGroupBox,N_("Normal Bit Rate:"),br,ActionParamDialog::cpvtAsInteger,"");
			normalVBRCombo->setIntegerValue(4);
			maxVBRCombo=addComboTextEntry(VBRGroupBox,N_("Maximum Bit Rate:"),br,ActionParamDialog::cpvtAsInteger,"");
			maxVBRCombo->setIntegerValue(4);

		qualityGroupBox->setChecked(true);
		CBRGroupBox->setChecked(false);
		VBRGroupBox->setChecked(false);
}

void OggSaveParametersDialog::onGroupBoxToggled(bool on)
{
	if(!on)
		return; // only care about on events


	qualityGroupBox->blockSignals(true);
	CBRGroupBox->blockSignals(true);
	VBRGroupBox->blockSignals(true);

		qualityGroupBox->setChecked(false);
		CBRGroupBox->setChecked(false);
		VBRGroupBox->setChecked(false);
		
		((QGroupBox *)sender())->setChecked(true);

	qualityGroupBox->blockSignals(false);
	CBRGroupBox->blockSignals(false);
	VBRGroupBox->blockSignals(false);
}

bool OggSaveParametersDialog::show(AFrontendHooks::OggCompressionParameters &parameters)
{
	CActionParameters actionParameters(NULL);

	// set from defaults ??

	if(ActionParamDialog::show(NULL,&actionParameters))
	{
		parameters.method=qualityGroupBox->isChecked() ? AFrontendHooks::OggCompressionParameters::brQuality : AFrontendHooks::OggCompressionParameters::brVBR;

		if(CBRGroupBox->isChecked())
		{
			parameters.minBitRate=parameters.normBitRate=parameters.maxBitRate=istring(CBRCombo->getStringValue()).to<int>();
		}
		else
		{
			parameters.minBitRate=istring(minVBRCombo->getStringValue()).to<int>();
			parameters.normBitRate=istring(normalVBRCombo->getStringValue()).to<int>();
			parameters.maxBitRate=istring(maxVBRCombo->getStringValue()).to<int>();
		}

		return true;
	}
	else
		return false;
}

