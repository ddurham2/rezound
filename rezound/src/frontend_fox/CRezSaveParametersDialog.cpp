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

#include "CRezSaveParametersDialog.h"

#include <stdexcept>
#include <istring>

#include "../backend/CActionParameters.h"

FXDEFMAP(CRezSaveParametersDialog) CRezSaveParametersDialogMap[]=
{
//	Message_Type			ID								Message_Handler
	//FXMAPFUNC(SEL_CHANGED,		CRezSaveParametersDialog::ID_SAMPLE_FORMAT_COMBOBOX,	CRezSaveParametersDialog::onSampleFormatComboBox),
};
		

FXIMPLEMENT(CRezSaveParametersDialog,CActionParamDialog,CRezSaveParametersDialogMap,ARRAYNUMBER(CRezSaveParametersDialogMap))

CRezSaveParametersDialog::CRezSaveParametersDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	setTitle(N_("ReZound Save Parameters"));

	FXPacker *p=newVertPanel(NULL);
		vector<string> sampleFormats;
			sampleFormats.push_back(_("8 Bit Integer"));
			sampleFormats.push_back(_("16 Bit Integer"));
			sampleFormats.push_back(_("24 Bit Integer"));
			sampleFormats.push_back(_("32 Bit Integer"));
			sampleFormats.push_back(_("32 Bit Float"));
		FXComboTextParamValue *sampleFormat=addComboTextEntry(p,N_("Sample Format"),sampleFormats,CActionParamDialog::cpvtAsInteger,"");
}

static AudioEncodingTypes indexToSampleFormat(int index)
{
	switch(index)
	{
	case 0: return aetPCMSigned8BitInteger;
	case 1: return aetPCMSigned16BitInteger;
	case 2: return aetPCMSigned24BitInteger;
	case 3: return aetPCMSigned32BitInteger;
	case 4: return aetPCM32BitFloat;
	default: throw runtime_error(string(__func__)+" -- internal error -- unhandled index: "+istring(index));
	}
}

static int sampleFormatToIndex(AudioEncodingTypes fmt)
{
	switch(fmt)
	{
	case aetPCMSigned8BitInteger: return 0;
	case aetPCMSigned16BitInteger: return 1;
	case aetPCMSigned24BitInteger: return 2;
	case aetPCMSigned32BitInteger: return 3;
	case aetPCM32BitFloat: return 4;
	default: throw runtime_error(string(__func__)+" -- internal error -- unhandled format: "+istring(fmt));
	}
}

bool CRezSaveParametersDialog::show(AFrontendHooks::RezSaveParameters &parameters)
{
	CActionParameters actionParameters(NULL);

	// set from defaults
	getComboText("Sample Format")->setCurrentItem(sampleFormatToIndex(parameters.audioEncodingType));

	if(CActionParamDialog::show(NULL,&actionParameters))
	{
		parameters.audioEncodingType=indexToSampleFormat(actionParameters.getValue<unsigned>("Sample Format"));
		return true;
	}
	else
		return false;
}

