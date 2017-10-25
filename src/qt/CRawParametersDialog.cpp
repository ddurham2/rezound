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

#include "CRawParametersDialog.h"

#include <stdexcept>
#include <istring>

#include "../backend/CActionParameters.h"

CRawParametersDialog::CRawParametersDialog(QWidget *mainWindow) :
	CActionParamDialog(mainWindow)
{
	setTitle(N_("Raw Parameters"));

	QWidget *p=newVertPanel(NULL);
		vector<string> channels;
		for(int t=1;t<=MAX_CHANNELS;t++)
			channels.push_back(istring(t));
		channelsCombo=addComboTextEntry(p,N_("Channels:"),channels,CActionParamDialog::cpvtAsInteger,"",true);
		channelsCombo->setCurrentItem(1);

		vector<string> sampleRates;
			sampleRates.push_back(_("4000"));
			sampleRates.push_back(_("8000"));
			sampleRates.push_back(_("11025"));
			sampleRates.push_back(_("22050"));
			sampleRates.push_back(_("32000"));
			sampleRates.push_back(_("44100"));
			sampleRates.push_back(_("48000"));
			sampleRates.push_back(_("88200"));
			sampleRates.push_back(_("96000"));
		sampleRateCombo=addComboTextEntry(p,N_("Sample Rate:"),sampleRates,CActionParamDialog::cpvtAsInteger,"",true);
		sampleRateCombo->setCurrentItem(5);

		vector<string> sampleFormats;
			sampleFormats.push_back(_("8bit Signed PCM"));
			sampleFormats.push_back(_("8bit Unsigned PCM"));
			sampleFormats.push_back(_("16bit Signed PCM"));
			sampleFormats.push_back(_("16bit Unsigned PCM"));
			sampleFormats.push_back(_("24bit Signed PCM"));
			sampleFormats.push_back(_("24bit Unsigned PCM"));
			sampleFormats.push_back(_("32bit Signed PCM"));
			sampleFormats.push_back(_("32bit Unsigned PCM"));
			sampleFormats.push_back(_("32bit Floating Point PCM"));
			sampleFormats.push_back(_("64bit Floating Point PCM"));
		sampleFormatCombo=addComboTextEntry(p,N_("Sample Format"),sampleFormats,CActionParamDialog::cpvtAsInteger,"");
		sampleFormatCombo->setCurrentItem(2);

		vector<string> endians;
			endians.push_back(_("Little Endian (Intel)"));
			endians.push_back(_("Big Endian (non-Intel)"));
		endianCombo=addComboTextEntry(p,N_("Byte Order:"),endians,CActionParamDialog::cpvtAsInteger,"");

		dataStartEdit=addNumericTextEntry(p,N_("Data Start:"),_("bytes"),0,0,MAX_LENGTH-1);
		dataStartEdit->setDecimals(0);
		dataLengthEdit=addNumericTextEntry(p,N_("Data length:"),_("frames"),0,0,MAX_LENGTH-1,_("normally leave this 0"));
		dataLengthEdit->setDecimals(0);
}

bool CRawParametersDialog::show(AFrontendHooks::RawParameters &parameters,bool showLoadRawParameters)
{
	if(showLoadRawParameters)
	{
		channelsCombo->show();
		sampleRateCombo->show();
		dataStartEdit->show();
		dataLengthEdit->show();
		
	}
	else
	{
		channelsCombo->hide();
		sampleRateCombo->hide();
		dataStartEdit->hide();
		dataLengthEdit->hide();
	}

	CActionParameters actionParameters(NULL);

	// ??? set controls according to given CRawParameters?

	if(CActionParamDialog::show(NULL,&actionParameters))
	{
		parameters.channelCount=channelsCombo->getIntegerValue();
		parameters.sampleRate=sampleRateCombo->getIntegerValue();

		switch(sampleFormatCombo->getIntegerValue())
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
				throw runtime_error(string(__func__)+" -- unhandled index for sampleFormatCombo: "+istring(sampleFormatCombo->getIntegerValue()));
		}
		
		parameters.endian= endianCombo->getIntegerValue()==0 ? AFrontendHooks::RawParameters::eLittleEndian : AFrontendHooks::RawParameters::eBigEndian;

		parameters.dataOffset=(unsigned)dataStartEdit->getValue();
		parameters.dataLength=(unsigned)dataLengthEdit->getValue();

		return true;
	}
	else
		return false;
}

