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

#include "CDuplicateChannelEdit.h"

#include "../CActionSound.h"
#include "../CActionParameters.h"


CDuplicateChannelEdit::CDuplicateChannelEdit(const AActionFactory *factory,const CActionSound *actionSound,unsigned _whichChannel,unsigned _insertWhere,unsigned _insertCount) :
	AAction(factory,actionSound),

	whichChannel(_whichChannel),
	insertWhere(_insertWhere),
	insertCount(_insertCount)
{
}

CDuplicateChannelEdit::~CDuplicateChannelEdit()
{
}

bool CDuplicateChannelEdit::doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo)
{
	if(insertWhere>actionSound->sound->getChannelCount())
		throw runtime_error(string(__func__)+" -- insertWhere ("+istring(insertWhere)+") is out of range");
	if(whichChannel>=actionSound->sound->getChannelCount())
		throw runtime_error(string(__func__)+" -- whichChannel ("+istring(whichChannel)+") is out of range");

	if(actionSound->sound->getChannelCount()+insertCount>MAX_CHANNELS)
		throw EUserMessage(string(_("Inserting this many more channels will add more than the maximum allowed channels."))+"\n"+istring(actionSound->sound->getChannelCount())+"+"+istring(insertCount)+" > "+istring(MAX_CHANNELS));

	unsigned whereSrcChannelWillBe;
	if(insertWhere>whichChannel)
		whereSrcChannelWillBe=whichChannel;
	else
		whereSrcChannelWillBe=whichChannel+insertCount;
	actionSound->sound->addChannels(insertWhere,insertCount,false);

	const CRezPoolAccesser src=actionSound->sound->getAudio(whereSrcChannelWillBe);
	for(unsigned t=insertWhere;t<insertWhere+insertCount;t++)
		actionSound->sound->mixSound(t,0,src,0,actionSound->sound->getSampleRate(),actionSound->sound->getLength(),mmOverwrite,sftNone,false,true);

	return true;
}

AAction::CanUndoResults CDuplicateChannelEdit::canUndo(const CActionSound *actionSound) const
{
	return curYes;
}

void CDuplicateChannelEdit::undoActionSizeSafe(const CActionSound *actionSound)
{
	actionSound->sound->removeChannels(insertWhere,insertCount);
}



// ------------------------------

CDuplicateChannelEditFactory::CDuplicateChannelEditFactory(AActionDialog *dialog) :
	AActionFactory(N_("Duplicate Channel"),_("Duplicate Channel"),NULL,dialog,true,false)
{
	selectionPositionsAreApplicable=false;
}

CDuplicateChannelEditFactory::~CDuplicateChannelEditFactory()
{
}

CDuplicateChannelEdit *CDuplicateChannelEditFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return new CDuplicateChannelEdit(
		this,
		actionSound,
		actionParameters->getUnsignedParameter("Which Channel"),
		actionParameters->getUnsignedParameter("Insert Where"),
		actionParameters->getUnsignedParameter("How Many Times")
	);
}

