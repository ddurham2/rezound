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

#include "LoopingActionDialogs.h"

// --- add cues -------------------------------

CAddNCuesDialog::CAddNCuesDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,N_("Add N Cues"))
{
	void *p=newVertPanel(NULL);
		addStringTextEntry(p,N_("Cue Name"),"(",_("What to Name the New Cues"));
		addNumericTextEntry(p,N_("Cue Count"),"",4,1,100,_("How Many Cues to Place Equally Spaced within the Selection"));
		addCheckBoxEntry(p,N_("Anchor Cues in Time"),false,_("Set the Cues to be Anchored in Time or Not"));
}

static const double interpretValue_addTimedCues(const double x,const int s) { return x*s; }
static const double uninterpretValue_addTimedCues(const double x,const int s) { return x/s; }

CAddTimedCuesDialog::CAddTimedCuesDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,N_("Add Cues Every X Seconds"))
{
	void *p=newVertPanel(NULL);
		addStringTextEntry(p,N_("Cue Name"),"(",_("What to Name the New Cues"));
		addSlider(p,N_("Time Interval"),"s",interpretValue_addTimedCues,uninterpretValue_addTimedCues,NULL,10,1,3600,60,false);
		addCheckBoxEntry(p,N_("Anchor Cues in Time"),false,_("Set the Cues to be Anchored in Time or Not"));
}



