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


#include "../backend/AStatusComm.h"

class CProgressDialog;
class FXWindow;

#define MAX_PROGRESS_DIALOGS 8

class CStatusComm : public AStatusComm
{
public:
	CStatusComm(FXWindow *mainWindow);
	virtual ~CStatusComm();

	virtual void error(const string &message,VSeverity severity=none);
	virtual void warning(const string &message);
	virtual void message(const string &message);
	virtual VAnswer question(const string &message,VQuestion options);

	virtual void beep();

	virtual int beginProgressBar(const string &title);
	virtual void updateProgressBar(int handle,int progress);
	virtual void endProgressBar(int handle);
	virtual void endAllProgressBars();

private:

	static const string breakIntoLines(const string s);
	
	FXWindow *mainWindow;
	CProgressDialog *progressDialogs[MAX_PROGRESS_DIALOGS];
};

#endif
