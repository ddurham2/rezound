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

#ifndef __CActionMenuCommand_H__
#define __CActionMenuCommand_H__

#include "../../config/common.h"
#include "fox_compat.h"

/*
 * This class is a normal FXMenuCommand except that the window it's
 * on doesn't need to setup an event handler for it.  The event
 * is always performing the action (given by the first parameter
 * to the constructor) on the active sound window
 *
 * So it can be used to conventiently add many buttons to a toolbar
 * and only specify the AActionFactorys on construction, and that's 
 * it
 * 
 */

#include <string>

class AActionFactory;

class CActionMenuCommand : public FXMenuCommand
{
	FXDECLARE(CActionMenuCommand)
public:
	CActionMenuCommand(AActionFactory *_actionFactory,FXComposite* p, const FXString& accelKeyText, FXIcon* ic=NULL, FXuint opts=0);
	CActionMenuCommand(FXComposite *p,const CActionMenuCommand &src);

	virtual ~CActionMenuCommand();

	const string getUntranslatedText() const;

	long onMouseClick(FXObject *sender,FXSelector sel,void *ptr);
	long onKeyClick(FXObject *sender,FXSelector sel,void *ptr);

	long onCommand(FXObject *sender,FXSelector sel,void *ptr);

	long onQueryTip(FXObject* sender,FXSelector sel,void *ptr);

	enum
	{
		ID_HOTKEY=FXMenuCommand::ID_LAST,
		ID_LAST
	};

	AActionFactory *getActionFactory() { return actionFactory; }

protected:
	CActionMenuCommand() : actionFactory(NULL) {}

private:
	AActionFactory * const actionFactory;
	FXString tip;

	FXEvent prevEvent;

};

#endif
