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

#include "CSoundFileManager.h"

#include <stdexcept>
#include <string>

FXDEFMAP(CActionButton) CActionButtonMap[]=
{
	//Message_Type				ID				Message_Handler
	FXMAPFUNC(SEL_LEFTBUTTONRELEASE,	CActionButton::ID_PRESS,	CActionButton::onButtonClick),
	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,	CActionButton::ID_PRESS,	CActionButton::onButtonClick),
};

FXIMPLEMENT(CActionButton,FXButton,CActionButtonMap,ARRAYNUMBER(CActionButtonMap))

CActionButton::CActionButton(AActionFactory *_actionFactory,FXComposite* p, const FXString& text, FXIcon* ic, FXuint opts, FXint x, FXint y, FXint w, FXint h, FXint pl, FXint pr, FXint pt, FXint pb) :
	FXButton(p,text+("\t"+_actionFactory->getDescription()).c_str(),ic,this,ID_PRESS,opts,x,y,w,h,pl,pr,pt),
	actionFactory(_actionFactory)
{
}

long CActionButton::onButtonClick(FXObject *sender,FXSelector sel,void *ptr)
{
	// ??? I think I need to heed FXWindow's implementation of onBtnRelease because it does some stuff with grab and ungrab...
	// right now, if you press the button, then move out of the window, and release it, it still clicks which it shouldn't probably do

	if(ptr==NULL || ptr==((void *)1))
		throw(runtime_error(string(__func__)+" -- ptr was NULL"));

	FXEvent *ev=((FXEvent *)ptr);
	if((ev->click_button==LEFTBUTTON || (ev->click_button==RIGHTBUTTON)) && underCursor())
	{
		CLoadedSound *activeSound=gSoundFileManager->getActive();
		if(activeSound)
		{
			CActionParameters actionParameters;
			if(actionFactory->performAction(activeSound,&actionParameters,ev->state&CONTROLMASK,ev->click_button==RIGHTBUTTON))
				gSoundFileManager->updateAfterEdit();
		}
		else
			getApp()->beep();
	}

	return(0); // since this is currently the button_release event, we have to return 0 so that FXButton will draw the button in an up state
}




