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

#include "CCueListDialog.h"

#include <string.h>

#include <string>
#include <map>

#include "CFOXIcons.h"

#include "rememberShow.h"

#include "../backend/CLoadedSound.h"
#include "../backend/Edits/CCueAction.h"
#include "../backend/CActionParameters.h"

#include "CCueDialog.h" // ??? this is going to have to rename to CCueDialog.h

/* TODO:
	Color Red the cues that are passed the end of the sound (if I can change the color, or flag as such if not)
 */

CCueListDialog *gCueListDialog;

FXDEFMAP(CCueListDialog) CCueListDialogMap[]=
{
//	Message_Type			ID					Message_Handler
	FXMAPFUNC(SEL_COMMAND,		CCueListDialog::ID_CLOSE_BUTTON,	CCueListDialog::onCloseButton),

	FXMAPFUNC(SEL_COMMAND,		CCueListDialog::ID_ADD_CUE_BUTTON,	CCueListDialog::onAddCueButton),
	FXMAPFUNC(SEL_COMMAND,		CCueListDialog::ID_REMOVE_CUE_BUTTON,	CCueListDialog::onRemoveCueButton),
	FXMAPFUNC(SEL_COMMAND,		CCueListDialog::ID_CHANGE_CUE_BUTTON,	CCueListDialog::onEditCueButton),
	FXMAPFUNC(SEL_DOUBLECLICKED,	CCueListDialog::ID_CUE_LIST,		CCueListDialog::onEditCueButton),
};
		

FXIMPLEMENT(CCueListDialog,FXDialogBox,CCueListDialogMap,ARRAYNUMBER(CCueListDialogMap))



// ----------------------------------------

CCueListDialog::CCueListDialog(FXWindow *owner) :
	FXDialogBox(owner,"Cue List",DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE, 0,0,350,350, 0,0,0,0, 0,0),

	contents(new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 0,0)),
		upperFrame(new FXVerticalFrame(contents,FRAME_RAISED|FRAME_THICK | LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 5,5,5,5)),
			cueListFrame(new FXPacker(upperFrame,LAYOUT_FILL_X|LAYOUT_FILL_Y | FRAME_SUNKEN|FRAME_THICK, 0,0,0,0, 0,0,0,0, 0,0)),
				cueList(new FXList(cueListFrame,0,this,ID_CUE_LIST,LIST_BROWSESELECT | LAYOUT_FILL_X|LAYOUT_FILL_Y)),
		lowerFrame(new FXHorizontalFrame(contents,FRAME_RAISED|FRAME_THICK | LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT, 0,0,0,65)),
			buttonPacker(new FXHorizontalFrame(lowerFrame,LAYOUT_CENTER_X|LAYOUT_CENTER_Y, 0,0,0,0, 0,0,0,0, 12)),
				closeButton(new FXButton(buttonPacker,"&Close",FOXIcons->GreenCheck1,this,ID_CLOSE_BUTTON,FRAME_RAISED|FRAME_THICK | JUSTIFY_NORMAL | ICON_ABOVE_TEXT | LAYOUT_FIX_WIDTH, 0,0,60,0, 2,2,2,2)),

				addCueButton(new FXButton(buttonPacker,"&Add",NULL,this,ID_ADD_CUE_BUTTON,FRAME_RAISED|FRAME_THICK | JUSTIFY_NORMAL | ICON_ABOVE_TEXT ,0,0,60,0, 2,2,2,2)),
				removeCueButton(new FXButton(buttonPacker,"&Remove",NULL,this,ID_REMOVE_CUE_BUTTON,FRAME_RAISED|FRAME_THICK | JUSTIFY_NORMAL | ICON_ABOVE_TEXT , 0,0,60,0, 2,2,2,2)),
				editCueButton(new FXButton(buttonPacker,"&Edit",NULL,this,ID_CHANGE_CUE_BUTTON,FRAME_RAISED|FRAME_THICK | JUSTIFY_NORMAL | ICON_ABOVE_TEXT , 0,0,60,0, 2,2,2,2)),

	loadedSound(NULL),

	addCueActionFactory(NULL),
	removeCueActionFactory(NULL),
	replaceCueActionFactory(NULL)
{
	// make cueList have a fixed width font
	FXFont *listFont=cueList->getFont();
        FXFontDesc d;
        listFont->getFontDesc(d);
	strcpy(d.face,"courier");
        listFont=new FXFont(getApp(),d);
	cueList->setFont(listFont);


		// ??? this doesn't seem to be having any effect... ask mailing list
	closeButton->setDefault(TRUE);

	// keep the dialog from getting too narrow
	//ASSURE_WIDTH(contents,160);
	
	addCueActionFactory=new CAddCueActionFactory(gCueDialog);
	removeCueActionFactory=new CRemoveCueActionFactory;
	replaceCueActionFactory=new CReplaceCueActionFactory(gCueDialog);
}

CCueListDialog::~CCueListDialog()
{
	delete addCueActionFactory;
	delete removeCueActionFactory;
	delete replaceCueActionFactory;
}


long CCueListDialog::onCloseButton(FXObject *sender,FXSelector sel,void *ptr)
{
	FXDialogBox::handle(this,MKUINT(ID_ACCEPT,SEL_COMMAND),ptr);

	return(1);
}


long CCueListDialog::onAddCueButton(FXObject *sender,FXSelector sel,void *ptr)
{
	CActionParameters actionParameters;

	if(cueList->getCurrentItem()!=-1)
	{
		size_t cueIndex=(size_t)cueList->getItemData(cueList->getCurrentItem());

		actionParameters.addStringParameter("name",loadedSound->getSound()->getCueName(cueIndex));
		actionParameters.addSamplePosParameter("position",loadedSound->getSound()->getCueTime(cueIndex));
		actionParameters.addBoolParameter("isAnchored",loadedSound->getSound()->isCueAnchored(cueIndex));

	}
	else
	{	
		actionParameters.addStringParameter("name","Cue1");
		actionParameters.addSamplePosParameter("position",0);
		actionParameters.addBoolParameter("isAnchored",false);
	}

	addCueActionFactory->performAction(loadedSound,&actionParameters,false,false);
	rebuildCueList();

	return(1);
}

long CCueListDialog::onRemoveCueButton(FXObject *sender,FXSelector sel,void *ptr)
{
	if(cueList->getCurrentItem()!=-1)
	{
		CActionParameters actionParameters;
		actionParameters.addUnsignedParameter("index",(size_t)cueList->getItemData(cueList->getCurrentItem()));
		removeCueActionFactory->performAction(loadedSound,&actionParameters,false,false);

		rebuildCueList();
	}

	return(1);
}

long CCueListDialog::onEditCueButton(FXObject *sender,FXSelector sel,void *ptr)
{
	if(cueList->getCurrentItem()!=-1)
	{
		CActionParameters actionParameters;
		size_t cueIndex=(size_t)cueList->getItemData(cueList->getCurrentItem());

		actionParameters.addUnsignedParameter("index",cueIndex);
		actionParameters.addStringParameter("name",loadedSound->getSound()->getCueName(cueIndex));
		actionParameters.addSamplePosParameter("position",loadedSound->getSound()->getCueTime(cueIndex));
		actionParameters.addBoolParameter("isAnchored",loadedSound->getSound()->isCueAnchored(cueIndex));

		replaceCueActionFactory->performAction(loadedSound,&actionParameters,false,false);

		rebuildCueList();
	}
	return(1);
}

void CCueListDialog::rebuildCueList()
{
	// save the selected item
	FXint currentItem=cueList->getCurrentItem();

	cueList->clearItems();

	// insert into a map and then extract (sorted) into the list box
	map<string,size_t> cues;
	CSound *sound=loadedSound->getSound();
	for(size_t t=0;t<loadedSound->getSound()->getCueCount();t++)
	{
		cues.insert(
			map<string,size_t>::value_type(
				sound->getTimePosition(sound->getCueTime(t),5)+(sound->isCueAnchored(t) ? " + " : " - ")+sound->getCueName(t),
				t
			)
		);
	}

	for(map<string,size_t>::iterator i=cues.begin();i!=cues.end();i++)
		cueList->appendItem(i->first.c_str(),NULL,(void *)i->second);


	// restore the selected item
	if(currentItem<cueList->getNumItems())
		cueList->setCurrentItem(currentItem);
	else
		cueList->setCurrentItem(cueList->getNumItems()-1);
}

void CCueListDialog::show(FXuint placement)
{
	if(loadedSound==NULL)
		throw(runtime_error(string(__func__)+" -- setSound() was not called with a non-NULL value"));

	rebuildCueList();
	if(cueList->getNumItems()>0)
		cueList->setCurrentItem(0);

	rememberShow(this);
	FXDialogBox::show(placement);
}

void CCueListDialog::hide()
{
	loadedSound=NULL;
	rememberHide(this);
	FXDialogBox::hide();
}

