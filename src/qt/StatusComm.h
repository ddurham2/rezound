/* 
 * Copyright (C) 2006 - David W. Durham
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

#ifndef StatusComm_H__
#define StatusComm_H__

#include "../../config/common.h"
#include "qt_compat.h"

#include "../backend/AStatusComm.h"

class MainWindow;
class ProgressDialog;


#define MAX_PROGRESS_DIALOGS 8

class StatusComm : public AStatusComm
{
public:
    StatusComm(MainWindow *mainWindow);
    virtual ~StatusComm();

	virtual void error(const string &message,VSeverity severity=none,bool reformatIfNeeded=true);
	virtual void warning(const string &message,bool reformatIfNeeded=true);
	virtual void message(const string &message,bool reformatIfNeeded=true);
	virtual VAnswer question(const string &message,/*VQuestion*/int options,bool reformatIfNeeded=true);

	virtual void beep();

	virtual int beginProgressBar(const string &title,bool showCancelButton=false);
	virtual bool updateProgressBar(int handle,int progress,const string timeElapsed,const string timeRemaining);
	virtual void endProgressBar(int handle);
	virtual void endAllProgressBars();

	void noMainWindow() { mainWindow=NULL; }

private:
	MainWindow *mainWindow;
	ProgressDialog *progressDialogs[MAX_PROGRESS_DIALOGS];

};

#endif
