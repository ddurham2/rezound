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

#include "CStatusComm.h"

#include <stdio.h>
#include <ctype.h>

#include <stdexcept>

#include <fox/fx.h>

#include "CProgressDialog.h"

const string escapeAmpersand(const string i)
{
	string o;

	// escape '&' for the reopen menu
	for(size_t t=0;t<i.size();t++)
	{
		const char c=i[t];
		if(c=='&')
			o.append("&");
		o.append(&c,1);
	}

	return(o);
}

CStatusComm::CStatusComm(FXWindow *_mainWindow) :
	mainWindow(_mainWindow)
{
	for(size_t t=0;t<MAX_PROGRESS_DIALOGS;t++)
		progressDialogs[t]=NULL;
}

CStatusComm::~CStatusComm()
{
}

/* I really doubt that all these levels of severity are necessary */

void CStatusComm::error(const string &message,VSeverity severity)
{
	switch(severity)
	{
	case none:
		fprintf(stderr,"error - %s\n",message.c_str());
		FXMessageBox::error(mainWindow,MBOX_OK,"Error",escapeAmpersand(breakIntoLines(message)).c_str());
		break;
	case light:
		fprintf(stderr,"light error - %s\n",message.c_str());
		FXMessageBox::error(mainWindow,MBOX_OK,"Light Error",escapeAmpersand(breakIntoLines(message)).c_str());
		break;
	case medium:
		fprintf(stderr,"medium error - %s\n",message.c_str());
		FXMessageBox::error(mainWindow,MBOX_OK,"Medium Error",escapeAmpersand(breakIntoLines(message)).c_str());
		break;
	case hard:
		fprintf(stderr,"hard error - %s\n",message.c_str());
		FXMessageBox::error(mainWindow,MBOX_OK,"Hard Error",escapeAmpersand(breakIntoLines(message)).c_str());
		break;
	case fatal:
		fprintf(stderr,"fatal error - %s\n",message.c_str());
		FXMessageBox::error(mainWindow,MBOX_OK,"Fatal Error!",escapeAmpersand(breakIntoLines(message)).c_str());
		break;
	default:
		fprintf(stderr,"unknwon severity error - %s\n",message.c_str());
		FXMessageBox::error(mainWindow,MBOX_OK,"Error -- unknown severity",escapeAmpersand(breakIntoLines(message)).c_str());
		break;
	}
}

void CStatusComm::warning(const string &message)
{
	fprintf(stderr,"warning -- %s\n",message.c_str());
	FXMessageBox::warning(mainWindow,MBOX_OK,"Warning",escapeAmpersand(breakIntoLines(message)).c_str());
}

void CStatusComm::message(const string &message)
{
	FXMessageBox::information(mainWindow,MBOX_OK,"Note",escapeAmpersand(breakIntoLines(message)).c_str());
}

VAnswer CStatusComm::question(const string &message,VQuestion options)
{
	FXint flags=0;

	if((options&yesnoQues)==yesnoQues)
		flags|=MBOX_YES_NO;
	if((options&cancelQues)==cancelQues)
		flags|=MBOX_YES_NO_CANCEL;

	if(flags==0)
		flags=MBOX_OK;

	switch(FXMessageBox::question(mainWindow,flags,"Question",escapeAmpersand(breakIntoLines(message)).c_str()))
	{
	case MBOX_CLICKED_YES:
		return(yesAns);
	case MBOX_CLICKED_NO:
		return(noAns);
	case MBOX_CLICKED_CANCEL:
		return(cancelAns);
	default:
		return(defaultAns);
	}
}

void CStatusComm::beep()
{
	mainWindow->getApp()->beep();
}

int CStatusComm::beginProgressBar(const string &title,bool showCancelButton)
{
	//printf("begin progress bar: %s\n",title.c_str());

	// find a place in progressDialogs to create a new one
	for(int handle=0;handle<MAX_PROGRESS_DIALOGS;handle++)
	{
		if(progressDialogs[handle]==NULL)
		{
			try
			{
				progressDialogs[handle]=new CProgressDialog(mainWindow,title.c_str(),showCancelButton);
				progressDialogs[handle]->create();
				progressDialogs[handle]->show();
				return(handle);
			}
			catch(exception &e)
			{
				progressDialogs[handle]=NULL;
				Error(e.what());
				return(-1);
			}
		}
	}

	return(-1);
}

bool CStatusComm::updateProgressBar(int handle,int progress)
{
	////printf("update progress bar: %d\n",progress);

	if(handle>=0 && handle<MAX_PROGRESS_DIALOGS && progressDialogs[handle]!=NULL)
	{
		try
		{
			progressDialogs[handle]->setProgress(progress);
			progressDialogs[handle]->getApp()->repaint();
			return progressDialogs[handle]->isCancelled;
		}
		catch(exception &e)
		{ // oh well
			fprintf(stderr,"exception caught in %s -- %s\n",__func__,e.what());
		}
	}
	return false;
}

void CStatusComm::endProgressBar(int handle)
{
	//printf("end progress bar\n");

	if(handle>=0 && handle<MAX_PROGRESS_DIALOGS && progressDialogs[handle]!=NULL)
	{
		try
		{
			progressDialogs[handle]->hide();
			delete progressDialogs[handle];
			progressDialogs[handle]=NULL;
		}
		catch(exception &e)
		{
			progressDialogs[handle]=NULL;
			Error(e.what());
		}
	}
}

void CStatusComm::endAllProgressBars()
{
	//printf("end all progress bars\n");

	for(int t=0;t<MAX_PROGRESS_DIALOGS;t++)
		endProgressBar(t);
}


