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

#include "CActionButton.h"

#include <stdexcept>
#include <string>

#include "CSoundFileManager.h"

#include "../backend/CActionParameters.h"
#include "../backend/AAction.h"


FXDEFMAP(CActionButton) CActionButtonMap[]=
{
	//Message_Type				ID				Message_Handler
	FXMAPFUNC(SEL_LEFTBUTTONRELEASE,	0,				CActionButton::onMouseClick),
	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,	0,				CActionButton::onMouseClick),
	FXMAPFUNC(SEL_KEYRELEASE,		0,				CActionButton::onKeyClick),

	FXMAPFUNC(SEL_COMMAND,			CActionButton::ID_ACCEL_KEY,	CActionButton::onKeyClick),
};

FXIMPLEMENT(CActionButton,FXButton,CActionButtonMap,ARRAYNUMBER(CActionButtonMap))

CActionButton::CActionButton(AActionFactory *_actionFactory,FXComposite* p, const FXString& text, FXIcon* ic, FXuint opts, FXint x, FXint y, FXint w, FXint h, FXint pl, FXint pr, FXint pt, FXint pb) :
	FXButton(p,text+("\t"+_actionFactory->getDescription()).c_str(),ic,NULL,0,opts,x,y,w,h,pl,pr,pt),
	actionFactory(_actionFactory)
{
}

long CActionButton::onMouseClick(FXObject *sender,FXSelector sel,void *ptr)
{
	if(!FXButton::handle(sender,sel,ptr))
		return(0);

	FXEvent *ev=((FXEvent *)ptr);

	CLoadedSound *activeSound=gSoundFileManager->getActive();
	if(activeSound)
	{
		if((ev->click_button==LEFTBUTTON || ev->click_button==RIGHTBUTTON) && underCursor())
		{
			CActionParameters actionParameters;
			if(actionFactory->performAction(activeSound,&actionParameters,ev->state&SHIFTMASK,ev->click_button==RIGHTBUTTON))
				gSoundFileManager->updateAfterEdit();
		}
	}
	else
		getApp()->beep();

	return(1);
}

/*
	This method handles either just any keypress in which case it asks 
	the base class to handle it and depending on the return value proceeds 
	to do the action or not...   Or it handles an accelerator key press.
*/
long CActionButton::onKeyClick(FXObject *sender,FXSelector sel,void *ptr)
{
	if(SELID(sel)!=ID_ACCEL_KEY)
	{
		if(!FXButton::handle(sender,sel,ptr))
			return(0);
	}

	FXEvent *ev=((FXEvent *)ptr);

	CLoadedSound *activeSound=gSoundFileManager->getActive();
	if(activeSound)
	{
		CActionParameters actionParameters;
		if(actionFactory->performAction(activeSound,&actionParameters,ev->state&SHIFTMASK,false))
			gSoundFileManager->updateAfterEdit();
	}
	else
		getApp()->beep();

	return(1);
}




