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

#ifndef __CStatusComm_H__
#define __CStatusComm_H__

#include "../../config/common.h"
#include "fox_compat.h"

#include "../backend/AStatusComm.h"

class CProgressDialog;


#ifdef FOX_NO_NAMESPACE
	class FXWindow;
#else
	namespace FX { class FXWindow; }
	using namespace FX;
#endif

#define MAX_PROGRESS_DIALOGS 8

class CStatusComm : public AStatusComm
{
public:
	CStatusComm(FXWindow *mainWindow);
	virtual ~CStatusComm();

	virtual void error(const string &message,VSeverity severity=none,bool reformatIfNeeded=true);
	virtual void warning(const string &message,bool reformatIfNeeded=true);
	virtual void message(const string &message,bool reformatIfNeeded=true);
	virtual VAnswer question(const string &message,VQuestion options,bool reformatIfNeeded=true);

	virtual void beep();

	virtual int beginProgressBar(const string &title,bool showCancelButton=false);
	virtual bool updateProgressBar(int handle,int progress,const string timeElapsed,const string timeRemaining);
	virtual void endProgressBar(int handle);
	virtual void endAllProgressBars();

private:
	FXWindow *mainWindow;
	CProgressDialog *progressDialogs[MAX_PROGRESS_DIALOGS];

};

#endif
