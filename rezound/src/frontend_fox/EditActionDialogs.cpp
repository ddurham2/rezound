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

#include "EditActionDialogs.h"

#include <istring>

#include "../backend/CActionSound.h"
#include "../backend/CSound.h"
#include "settings.h"

#include "ActionParamMappers.h"


// --- insert silence -------------------------

CInsertSilenceDialog::CInsertSilenceDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addNumericTextEntry(p,N_("Length"),_("seconds"),1.0,0,10000);
}



// --- rotate ---------------------------------

CRotateDialog::CRotateDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addNumericTextEntry(p,N_("Amount"),_("seconds"),1.0,0,10000);
}



// --- swap channels --------------------------

CSwapChannelsDialog::CSwapChannelsDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newVertPanel(NULL);
		vector<string> items;
		addComboTextEntry(p,N_("Channel A"),items,CActionParamDialog::cpvtAsInteger,"");
		addComboTextEntry(p,N_("Channel B"),items,CActionParamDialog::cpvtAsInteger,"");
}

#include "../backend/CActionParameters.h"

bool CSwapChannelsDialog::show(CActionSound *actionSound,CActionParameters *actionParameters)
{
	if(actionSound->sound->getChannelCount()<2)
		return false;
	else if(actionSound->sound->getChannelCount()==2)
	{ // only one possibility of swapping the two channels
		actionParameters->setValue<unsigned>("Channel A",0);
		actionParameters->setValue<unsigned>("Channel B",1);
		return true;
	}
	else
	{
		if(getComboText("Channel A")->getItems().size()!=actionSound->sound->getChannelCount())
		{
			vector<string> items;
			for(size_t t=0;t<actionSound->sound->getChannelCount();t++)
				items.push_back(_("Channel ")+istring(t));

			// set the combo boxes according to actionSound
			getComboText("Channel A")->setItems(items);
			getComboText("Channel B")->setItems(items);
			getComboText("Channel A")->setCurrentItem(0);
			getComboText("Channel B")->setCurrentItem(1);
		}

		return CActionParamDialog::show(actionSound,actionParameters);
	}
}


// --- add channels ---------------------------

CAddChannelsDialog::CAddChannelsDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newVertPanel(NULL);
		addNumericTextEntry(p,N_("Insert Where"),"",0,0,MAX_CHANNELS);
		addNumericTextEntry(p,N_("Insert Count"),"",1,1,MAX_CHANNELS);
}



// --- duplicate channel ----------------------

CDuplicateChannelDialog::CDuplicateChannelDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newVertPanel(NULL);
		vector<string> items;
		addComboTextEntry(p,N_("Which Channel"),items,CActionParamDialog::cpvtAsInteger,"");
		addComboTextEntry(p,N_("Insert Where"),items,CActionParamDialog::cpvtAsInteger,"");
		addNumericTextEntry(p,N_("How Many Times"),"",1,1,MAX_CHANNELS);
}

bool CDuplicateChannelDialog::show(CActionSound *actionSound,CActionParameters *actionParameters)
{
	if(getComboText("Which Channel")->getItems().size()!=actionSound->sound->getChannelCount())
	{
		vector<string> items;
		for(size_t t=0;t<actionSound->sound->getChannelCount();t++)
			items.push_back(_("Channel ")+istring(t));

		// set the combo boxes according to actionSound
		getComboText("Which Channel")->setItems(items);
		getComboText("Which Channel")->setCurrentItem(0);

		items.push_back(_("Channel ")+istring(actionSound->sound->getChannelCount()));
		getComboText("Insert Where")->setItems(items);
		getComboText("Insert Where")->setCurrentItem(1);
	}

	return CActionParamDialog::show(actionSound,actionParameters);
}
// --- grow or slide selection dialog ---------

CGrowOrSlideSelectionDialog::CGrowOrSlideSelectionDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newVertPanel(NULL);
		addSlider(p,
			N_("Amount"),
			"s",
			new CActionParamMapper_linear(1.0,2,1,3600),
			NULL,
			false
		);
		setTipText("Amount",_("Amount to Affect the Selection in Seconds"));

		vector<string> items;
			items.push_back(N_("Grow Selection to the Left"));
			items.push_back(N_("Grow Selection to the Right"));
			items.push_back(N_("Grow Selection in Both Directions"));
			items.push_back(N_("Slide Selection to the Left"));
			items.push_back(N_("Slide Selection to the Right"));
		addComboTextEntry(p,N_("How"),items,CActionParamDialog::cpvtAsInteger);
		getComboText("How")->setCurrentItem(1);
}

