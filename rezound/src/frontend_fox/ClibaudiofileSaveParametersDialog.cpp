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

#include "ClibaudiofileSaveParametersDialog.h"

#include <stdexcept>
#include <istring>

#include "../backend/CActionParameters.h"

FXDEFMAP(ClibaudiofileSaveParametersDialog) ClibaudiofileSaveParametersDialogMap[]=
{
//	Message_Type			ID								Message_Handler
	FXMAPFUNC(SEL_CHANGED,		ClibaudiofileSaveParametersDialog::ID_SAMPLE_FORMAT_COMBOBOX,	ClibaudiofileSaveParametersDialog::onSampleFormatComboBox),
};
		

FXIMPLEMENT(ClibaudiofileSaveParametersDialog,CActionParamDialog,ClibaudiofileSaveParametersDialogMap,ARRAYNUMBER(ClibaudiofileSaveParametersDialogMap))

ClibaudiofileSaveParametersDialog::ClibaudiofileSaveParametersDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	setTitle(N_("Save Parameters"));

	FXPacker *p=newVertPanel(NULL);
		formatNameLabel=new FXLabel(p,"",NULL,LABEL_NORMAL | LAYOUT_CENTER_X|LAYOUT_TOP);

		vector<string> sampleFormats;
			sampleFormats.push_back("Signed");
			sampleFormats.push_back("Unsigned");
			sampleFormats.push_back("32bit float");
			sampleFormats.push_back("64bit double");
		FXComboTextParamValue *sampleFormat=addComboTextEntry(p,N_("Sample Format"),sampleFormats,CActionParamDialog::cpvtAsInteger,"");
			sampleFormat->setTarget(this);
			sampleFormat->setSelector(ID_SAMPLE_FORMAT_COMBOBOX);
		

			// when Sample Format is either float or double I need to disable this control
		vector<string> sampleWidths;
			// audiofile might translate into 1-32 bits not just these values
			sampleWidths.push_back("8"); 
			sampleWidths.push_back("16");
			sampleWidths.push_back("24");
			sampleWidths.push_back("32");
		addComboTextEntry(p,N_("Sample Width"),sampleWidths,CActionParamDialog::cpvtAsInteger,"");

		vector<string> compressionTypes;
			// will be populated with values later
		addComboTextEntry(p,N_("Compression Type"),compressionTypes,CActionParamDialog::cpvtAsInteger,"");
}

#ifdef HAVE_LIBAUDIOFILE
#include <audiofile.h>
static int indexTo_AF_SAMPFMT_xxx(int index)
{
	switch(index)
	{
	case 0: return AF_SAMPFMT_TWOSCOMP;
	case 1: return AF_SAMPFMT_UNSIGNED;
	case 2: return AF_SAMPFMT_FLOAT;
	case 3: return AF_SAMPFMT_DOUBLE;
	default: throw runtime_error(string(__func__)+" -- internal error -- unhandled index: "+istring(index));
	}
}

#else

static int indexTo_AF_SAMPFMT_xxx(int index){ return 0; }

#endif

static int indexToSampleWidth(int index)
{
	switch(index)
	{
	case 0: return 8;
	case 1: return 16;
	case 2: return 24;
	case 3: return 32;
	default: throw runtime_error(string(__func__)+" -- internal error -- unhandled index: "+istring(index));
	}
}

bool ClibaudiofileSaveParametersDialog::show(AFrontendHooks::libaudiofileSaveParameters &parameters,const string formatName)
{
	formatNameLabel->setText(formatName.c_str());

	CActionParameters actionParameters(NULL);

	// set the compression types that are supported
	{
		vector<string> compressionTypes;
		FXComboTextParamValue *comboBox=getComboText("Compression Type");
		for(size_t t=0;t<parameters.supportedCompressionTypes.size();t++)
			compressionTypes.push_back(parameters.supportedCompressionTypes[t].first);
		comboBox->setItems(compressionTypes);
	}

	// set from defaults
	if((size_t)parameters.defaultSampleFormatIndex<getComboText("Sample Format")->getItems().size())
		getComboText("Sample Format")->setCurrentItem(parameters.defaultSampleFormatIndex);

	if((size_t)parameters.defaultSampleWidthIndex<getComboText("Sample Width")->getItems().size())
		getComboText("Sample Width")->setCurrentItem(parameters.defaultSampleWidthIndex);
		
	if((size_t)parameters.defaultCompressionTypeIndex<getComboText("Compression Type")->getItems().size())
		getComboText("Compression Type")->setCurrentItem(parameters.defaultCompressionTypeIndex);


	onSampleFormatComboBox(NULL,0,NULL);

	if(CActionParamDialog::show(NULL,&actionParameters))
	{
		parameters.sampleFormat=indexTo_AF_SAMPFMT_xxx(actionParameters.getUnsignedParameter("Sample Format"));
		parameters.sampleWidth=indexToSampleWidth(actionParameters.getUnsignedParameter("Sample Width"));
		parameters.compressionType=parameters.supportedCompressionTypes[actionParameters.getUnsignedParameter("Compression Type")].second;
		return true;
	}
	else
		return false;
}


long ClibaudiofileSaveParametersDialog::onSampleFormatComboBox(FXObject *sender,FXSelector sel,void *ptr)
{
	int selected=getComboText("Sample Format")->getIntegerValue();
	if(selected==0 || selected==1)
	{ // signed or unsigned
		getComboText("Sample Width")->enable();
		getComboText("Compression Type")->enable();
	}
	else
	{ // float or double
		getComboText("Sample Width")->disable();
		getComboText("Compression Type")->disable();
	}
	return 1;
}

const string ClibaudiofileSaveParametersDialog::getExplanation() const
{
	return _("Some combinations of values may cause issues.\nThis is because ReZound cannot tell enough from the libaudiofile library what is valid.");
}
