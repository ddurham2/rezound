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

#include "CActionMenuCommand.h"

#include <stdexcept>
#include <string>

#include "CSoundFileManager.h"

#include "../backend/CActionParameters.h"
#include "../backend/AAction.h"
#include "../images/images.h"

FXDEFMAP(CActionMenuCommand) CActionMenuCommandMap[]=
{
	//Message_Type				ID				Message_Handler

	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,	0,				CActionMenuCommand::onMouseClick),
	FXMAPFUNC(SEL_LEFTBUTTONRELEASE,	0,				CActionMenuCommand::onMouseClick),

	FXMAPFUNC(SEL_KEYRELEASE,		0,				CActionMenuCommand::onKeyClick),

	FXMAPFUNC(SEL_COMMAND,			CActionMenuCommand::ID_HOTKEY,	CActionMenuCommand::onCommand),

	FXMAPFUNC(SEL_UPDATE,			FXWindow::ID_QUERY_TIP,		CActionMenuCommand::onQueryTip)
};

FXIMPLEMENT(CActionMenuCommand,FXMenuCommand,CActionMenuCommandMap,ARRAYNUMBER(CActionMenuCommandMap))

CActionMenuCommand::CActionMenuCommand(AActionFactory *_actionFactory,FXComposite* p, const FXString& accelKeyText, FXIcon* ic, FXuint opts) :
	FXMenuCommand(
		p,
		(_actionFactory->getName()+"\t"+accelKeyText.text()).c_str(),
		(ic==NULL ? new FXGIFIcon(p->getApp(),_actionFactory->hasAdvancedMode() ? advanced_action_gif : normal_action_gif) : ic),
		this,
		ID_HOTKEY,
		opts
		),
	actionFactory(_actionFactory)
{
	if(actionFactory->getName()!=actionFactory->getDescription()) // don't be stupid (no need for a popup hint)
		tip=actionFactory->getDescription().c_str();
}

long CActionMenuCommand::onMouseClick(FXObject *sender,FXSelector sel,void *ptr)
{
	if(!FXMenuCommand::handle(sender,sel,ptr))
		return(0);

	if(ptr==NULL || ptr==((void *)1))
		throw(runtime_error(string(__func__)+" -- ptr was NULL"));

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

long CActionMenuCommand::onKeyClick(FXObject *sender,FXSelector sel,void *ptr)
{
	if(!FXMenuCommand::handle(sender,sel,ptr))
		return(0);

	if(ptr==NULL || ptr==((void *)1))
		throw(runtime_error(string(__func__)+" -- ptr was NULL"));

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

long CActionMenuCommand::onCommand(FXObject *sender,FXSelector sel,void *ptr)
{
	CLoadedSound *activeSound=gSoundFileManager->getActive();
	if(activeSound)
	{
		CActionParameters actionParameters;
		//if(actionFactory->performAction(activeSound,&actionParameters,keyboardState&SHIFTMASK,false))
		if(actionFactory->performAction(activeSound,&actionParameters,false,false))
			gSoundFileManager->updateAfterEdit();
	}
	else
		getApp()->beep();

	return(1);
}


long CActionMenuCommand::onQueryTip(FXObject* sender,FXSelector sel,void *ptr)
{
	if(!tip.empty() && (flags&FLAG_TIP))
	{
		sender->handle(this,MKUINT(ID_SETSTRINGVALUE,SEL_COMMAND),(void*)&tip);
		return 1;
	}
	return 0;
}


