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

#include "StatusComm.h"

#include <stdio.h>
#include <ctype.h>

#include <stdexcept>

#include "ProgressDialog.h"
#include "MainWindow.h"

#include <QApplication>
#include <QMessageBox>

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

	return o;
}

StatusComm::StatusComm(MainWindow *_mainWindow) :
	mainWindow(_mainWindow)
{
	//for(size_t t=0;t<MAX_PROGRESS_DIALOGS;t++)
		//progressDialogs[t]=NULL;
}

StatusComm::~StatusComm()
{
}

/* I really doubt that all these levels of severity are necessary */

void StatusComm::error(const string &message,VSeverity severity,bool reformatIfNeeded)
{
	switch(severity)
	{
	case none:
		fprintf(stderr,"error - %s\n",message.c_str());
		QMessageBox::critical(mainWindow,_("Error"),escapeAmpersand(reformatIfNeeded ? breakIntoLines(message) : message).c_str());
		break;
	case light:
		fprintf(stderr,"light error - %s\n",message.c_str());
		QMessageBox::critical(mainWindow,_("Light Error"),escapeAmpersand(reformatIfNeeded ? breakIntoLines(message) : message).c_str());
		break;
	case medium:
		fprintf(stderr,"medium error - %s\n",message.c_str());
		QMessageBox::critical(mainWindow,_("Medium Error"),escapeAmpersand(reformatIfNeeded ? breakIntoLines(message) : message).c_str());
		break;
	case hard:
		fprintf(stderr,"hard error - %s\n",message.c_str());
		QMessageBox::critical(mainWindow,_("Hard Error"),escapeAmpersand(reformatIfNeeded ? breakIntoLines(message) : message).c_str());
		break;
	case fatal:
		fprintf(stderr,"fatal error - %s\n",message.c_str());
		QMessageBox::critical(mainWindow,_("Fatal Error"),escapeAmpersand(reformatIfNeeded ? breakIntoLines(message) : message).c_str());
		break;
	default:
		fprintf(stderr,"unknwon severity error - %s\n",message.c_str());
		QMessageBox::critical(mainWindow,"Error -- unknown severity",escapeAmpersand(reformatIfNeeded ? breakIntoLines(message) : message).c_str());
		break;
	}
}

void StatusComm::warning(const string &message,bool reformatIfNeeded)
{
	fprintf(stderr,"warning -- %s\n",message.c_str());
	QMessageBox::warning(mainWindow,_("Warning"),escapeAmpersand(reformatIfNeeded ? breakIntoLines(message) : message).c_str());
}

void StatusComm::message(const string &message,bool reformatIfNeeded)
{
	fprintf(stderr,"message -- %s\n",message.c_str());
	QMessageBox::information(mainWindow,_("Note"),escapeAmpersand(reformatIfNeeded ? breakIntoLines(message) : message).c_str());
}

VAnswer StatusComm::question(const string &message,/*VQuestion*/int options,bool reformatIfNeeded)
{
	QMessageBox::StandardButtons buttons;

	if((options&yesnoQues)==yesnoQues)
	{
		buttons=QMessageBox::Yes | QMessageBox::No;
	}
	else if((options&cancelQues)==cancelQues)
	{
		buttons=QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel;
	}
	else
	{
		buttons=QMessageBox::Ok;
	}

	switch(QMessageBox::question(mainWindow,_("Question"),escapeAmpersand(reformatIfNeeded ? breakIntoLines(message) : message).c_str(),buttons))
	{
	case QMessageBox::Yes:
		return yesAns;
	case QMessageBox::No:
		return noAns;
	case QMessageBox::Cancel:
		return cancelAns;
	default:
		return defaultAns;
	}
}

void StatusComm::beep()
{
	QApplication::beep();
}

int StatusComm::beginProgressBar(const string &title,bool showCancelButton)
{
	//printf("begin progress bar: %s\n",title.c_str());

	// find a place in progressDialogs to create a new one
	for(int handle=0;handle<MAX_PROGRESS_DIALOGS;handle++)
	{
		if(progressDialogs[handle]==NULL)
		{
			try
			{
				progressDialogs[handle]=new ProgressDialog(mainWindow,title,showCancelButton);
				progressDialogs[handle]->show();
				return handle;
			}
			catch(exception &e)
			{
				progressDialogs[handle]=NULL;
				Error(e.what());
				return -1;
			}
		}
	}

	return -1;
}

bool StatusComm::updateProgressBar(int handle,int progress,const string timeElapsed,const string timeRemaining)
{
	////printf("update progress bar: %d\n",progress);

	if(handle>=0 && handle<MAX_PROGRESS_DIALOGS && progressDialogs[handle]!=NULL)
	{
		try
		{
			progressDialogs[handle]->setProgress(progress,timeElapsed,timeRemaining);
		// ??? necessary?	progressDialogs[handle]->getApp()->repaint();
			return progressDialogs[handle]->isCancelled;
		}
		catch(exception &e)
		{ // oh well
			fprintf(stderr,"exception caught in %s -- %s\n",__func__,e.what());
		}
	}
	return false;
}

void StatusComm::endProgressBar(int handle)
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

void StatusComm::endAllProgressBars()
{
	//printf("end all progress bars\n");

	for(int t=0;t<MAX_PROGRESS_DIALOGS;t++)
		endProgressBar(t);
}


