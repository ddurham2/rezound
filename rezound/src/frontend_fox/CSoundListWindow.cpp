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

#include "CSoundListWindow.h"

#include "rememberShow.h"

#include "CSoundWindow.h"
#include "../backend/CLoadedSound.h"

#include "settings.h"

#include <CPath.h>

CSoundListWindow *gSoundListWindow=NULL;

/* TODO:
 * - make a better way of deciding on the position of the tool bar.. it would be nice if I knew I could dock it with the main window
 */

FXDEFMAP(CSoundListWindow) CSoundListWindowMap[]=
{
	//	  Message_Type			ID					Message_Handler
	FXMAPFUNC(SEL_CHANGED,			CSoundListWindow::ID_SOUND_LIST,	CSoundListWindow::onSoundListChange),

	FXMAPFUNC(SEL_CLOSE,			0,					CSoundListWindow::onCloseWindow),
};

FXIMPLEMENT(CSoundListWindow,FXTopWindow,CSoundListWindowMap,ARRAYNUMBER(CSoundListWindowMap))


CSoundListWindow::CSoundListWindow(FXWindow *mainWindow) :
	FXTopWindow(mainWindow,"Opened",NULL,NULL,DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE,358,18,360,mainWindow->getHeight(), 0,0,0,0, 0,0),

	contents(new FXPacker(this,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_RAISED)),
		soundListFrame(new FXPacker(contents,LAYOUT_FILL_X|LAYOUT_FILL_Y | FRAME_SUNKEN|FRAME_THICK, 0,0,0,0, 0,0,0,0, 0,0)), // had to do this because FXList won't take that frame style
			soundList(new FXList(soundListFrame,0,this,ID_SOUND_LIST,LIST_BROWSESELECT | LAYOUT_FILL_X|LAYOUT_FILL_Y))
{
	delete getAccelTable();
	setAccelTable(mainWindow->getAccelTable());
}

CSoundListWindow::~CSoundListWindow()
{
	setAccelTable(NULL);
}

void CSoundListWindow::show()
{
	// put this if here because it would show just after adding items to the list
	if(soundList->getNumItems()>1)
	{
		rememberShow(this);
		FXTopWindow::show();
	}
}

void CSoundListWindow::hide()
{
	rememberHide(this);
	FXTopWindow::hide();
}

long CSoundListWindow::onSoundListChange(FXObject *sender,FXSelector sel,void *ptr)
{
	FXint index=(FXint)ptr;

	if(index!=-1)
		((CSoundWindow *)soundList->getItemData(index))->setActiveState(true);

	return 1;
}

void CSoundListWindow::addSoundWindow(CSoundWindow *window)
{
	CPath p(window->loadedSound->getFilename().c_str());

	// if I could, i'd like a two column list where the basename was in the first column and path in the second???
	soundList->appendItem((p.baseName()+"  "+p.dirName()).c_str(),NULL,window);
	soundList->setCurrentItem(soundList->getNumItems()-1);
	soundList->makeItemVisible(soundList->getNumItems()-1);

	hideOrShow();
}

void CSoundListWindow::removeSoundWindow(CSoundWindow *window)
{
	for(FXint t=0;t<soundList->getNumItems();t++)
	{
		if(soundList->getItemData(t)==window)
		{
			soundList->removeItem(t);
			if(soundList->getNumItems()>0)
			{
				soundList->setCurrentItem(0);
				soundList->makeItemVisible(0);
			}
			break;
		}
	}

	hideOrShow();
}

void CSoundListWindow::updateWindowName(CSoundWindow *window)
{
	for(FXint t=0;t<soundList->getNumItems();t++)
	{
		if(soundList->getItemData(t)==window)
		{
			CPath p(window->loadedSound->getFilename().c_str());
			soundList->setItemText(t,(p.baseName()+"  "+p.dirName()).c_str());
			break;
		}
	}

}

void CSoundListWindow::hideOrShow()
{
	if(soundList->getNumItems()>1)
		show();
	else if(shown())
		hide();
}

long CSoundListWindow::onCloseWindow(FXObject *sender,FXSelector sel,void *ptr)
{
	return 1;
}


