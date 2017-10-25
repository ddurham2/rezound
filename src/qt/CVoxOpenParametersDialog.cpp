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

#include "CVoxOpenParametersDialog.h"

#include <stdexcept>
#include <istring>

#include "../backend/CActionParameters.h"

CVoxOpenParametersDialog::CVoxOpenParametersDialog(QWidget *mainWindow) :
	CActionParamDialog(mainWindow)
{
	setTitle(N_("Ogg Compression Parameters"));

	QWidget *p=newVertPanel(NULL);


	vector<string> ch;
	for(int t=1;t<=MAX_CHANNELS;t++)
		ch.push_back(istring(t));

	m_channels=addComboTextEntry(p,N_("Channels:"),ch,CActionParamDialog::cpvtAsInteger,"",false);
	m_channels->setIntegerValue(1);

	vector<string> br;
		br.push_back("4000");
		br.push_back("8000");
		br.push_back("11025");
		br.push_back("22050");
		br.push_back("32000");
		br.push_back("44100");
		br.push_back("48000");
		br.push_back("88200");
		br.push_back("96000");

	m_sampleRate=addComboTextEntry(p,N_("Sample Rate:"),br,CActionParamDialog::cpvtAsInteger,"",true);
	m_sampleRate->setIntegerValue(44100);
			
}

bool CVoxOpenParametersDialog::show(AFrontendHooks::VoxParameters &parameters)
{
	CActionParameters actionParameters(NULL);

	// set from defaults ??

	if(CActionParamDialog::show(NULL,&actionParameters))
	{
		parameters.channelCount=m_channels->getIntegerValue()+1;
		parameters.sampleRate=m_sampleRate->getIntegerValue();
		return true;
	}
	else
		return false;
}

