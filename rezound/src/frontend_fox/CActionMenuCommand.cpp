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
#include "CFOXIcons.h"
#include "CMainWindow.h" // for CMainWindow::actionMenuCommandTriggered()

FXDEFMAP(CActionMenuCommand) CActionMenuCommandMap[]=
{
	//Message_Type				ID				Message_Handler

	FXMAPFUNC(SEL_LEFTBUTTONRELEASE,	0,				CActionMenuCommand::onMouseClick),
	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,	0,				CActionMenuCommand::onMouseClick),

	FXMAPFUNC(SEL_KEYRELEASE,		0,				CActionMenuCommand::onKeyClick),

	FXMAPFUNC(SEL_COMMAND,			CActionMenuCommand::ID_HOTKEY,	CActionMenuCommand::onCommand),

	FXMAPFUNC(SEL_UPDATE,			FXWindow::ID_QUERY_TIP,		CActionMenuCommand::onQueryTip)
};

FXIMPLEMENT(CActionMenuCommand,FXMenuCommand,CActionMenuCommandMap,ARRAYNUMBER(CActionMenuCommandMap))

CActionMenuCommand::CActionMenuCommand(AActionFactory *_actionFactory,FXComposite* p, const FXString& accelKeyText, FXIcon* ic, FXuint opts) :
	FXMenuCommand(
		p,
		(_actionFactory->getName()+"\t"+accelKeyText.text()).c_str(),
		(ic==NULL ? FOXIcons->normal_action_buff : ic),
		this,
		ID_HOTKEY,
		opts
		),
	actionFactory(_actionFactory)
{
	if(actionFactory->getName()!=actionFactory->getDescription()) // don't be stupid (no need for a popup hint)
		tip=actionFactory->getDescription().c_str();

	prevEvent.state=0;
	prevEvent.click_button=0;
}

CActionMenuCommand::CActionMenuCommand(FXComposite *p,const CActionMenuCommand &src) :
	FXMenuCommand(
		p,
		src.actionFactory->getName().c_str(),
		src.getIcon(),
		this,
		src.getSelector(),
		src.getLayoutHints() /* I can't get the original options from src */
		),
	actionFactory(src.actionFactory),
	tip(src.tip),
	prevEvent(src.prevEvent)
{
}

CActionMenuCommand::~CActionMenuCommand()
{
}

long CActionMenuCommand::onMouseClick(FXObject *sender,FXSelector sel,void *ptr)
{
	prevEvent=*((FXEvent *)ptr);
	return FXMenuCommand::handle(sender,sel,ptr);
}

long CActionMenuCommand::onKeyClick(FXObject *sender,FXSelector sel,void *ptr)
{
	prevEvent=*((FXEvent *)ptr);
	return FXMenuCommand::handle(sender,sel,ptr);
}

long CActionMenuCommand::onCommand(FXObject *sender,FXSelector sel,void *ptr)
{
	CLoadedSound *activeSound=gSoundFileManager->getActive();
	if(activeSound)
	{
		gSoundFileManager->getMainWindow()->actionMenuCommandTriggered(this);

			// ??? let action parameters contain actionSound and the two bool parameters
			// they should have some flag which says that they would not be streamed to disk (if that were ever something I do with action parameters)
		CActionParameters actionParameters(gSoundFileManager);
		actionFactory->performAction(activeSound,&actionParameters,prevEvent.state&SHIFTMASK);

		gSoundFileManager->updateAfterEdit();

		// now, in case a newly created window has taken the place of the previously active window, it still may be necessary to update the previous active sound window 
		if(gSoundFileManager->getActive()!=activeSound && gSoundFileManager->isValidLoadedSound(activeSound))
			gSoundFileManager->updateAfterEdit(activeSound);

		prevEvent.state=0;
		prevEvent.click_button=0;
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

