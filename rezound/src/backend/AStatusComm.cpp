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

#include "AStatusComm.h"

#include <ctype.h>

AStatusComm *gStatusComm=NULL;

const string AStatusComm::breakIntoLines(const string _s)
{
	// break into lines when a line has become to long, break at the next space.
	#define BREAK_AFTER 100

	// and also break at each ' -- '
	
	string s=_s;
	const size_t len=s.length();
	size_t lineLen=0;
	for(size_t t=0;t<len;t++)
	{
		if(s[t]=='\n')
			lineLen=0;
		else if(
			(lineLen++>BREAK_AFTER) 
			||
			((len-t)>3 && s[t+1]=='-' && s[t+2]=='-' && s[t+3]==' ') // "-- " is in the future
		)
		{
			if(isspace(s[t]))
			{
				lineLen=0;
				s[t]='\n';
			}
		}
	}
	return(s);
}

void Error(const string &message,VSeverity severity,bool reformatIfNeeded)
{
	gStatusComm->error(message,severity,reformatIfNeeded);
}

void Warning(const string &message,bool reformatIfNeeded)
{
	gStatusComm->warning(message,reformatIfNeeded);
}

void Message(const string &message,bool reformatIfNeeded)
{
	gStatusComm->message(message,reformatIfNeeded);
}

VAnswer Question(const string &message,VQuestion options,bool reformatIfNeeded)
{
	return(gStatusComm->question(message,options,reformatIfNeeded));
}



int beginProgressBar(const string &title,bool showCancelButton)
{
	return(gStatusComm->beginProgressBar(title,showCancelButton));
}

void endProgressBar(int handle)
{
	gStatusComm->endProgressBar(handle);
}

void endAllProgressBars()
{
	gStatusComm->endAllProgressBars();
}


// --- CStatusBar --------------------------------------

#include <sys/timeb.h>
static unsigned long getCurrentMilliseconds()
{
	struct timeb tb;
	ftime(&tb);
	return (unsigned long)tb.time*1000UL+(unsigned long)tb.millitm;
}

CStatusBar::CStatusBar(const string title,const sample_pos_t firstValue,const sample_pos_t lastValue,const bool showCancelButton) :
	handle(gStatusComm->beginProgressBar(title,showCancelButton)),
	sub(firstValue),
	valueDiff(lastValue-firstValue),
	div( valueDiff<100 ? 1 : ((valueDiff+100-1)/100) ),
	mul( valueDiff<100 ? (100.0/valueDiff) : 100.0/(valueDiff/div) ),
	lastProgress(0),
	// set initial time in milliseconds since epoch
	initialTime(getCurrentMilliseconds())
{
}

CStatusBar::~CStatusBar()
{
	hide();
}

void CStatusBar::reset()
{
	lastProgress=0;
	initialTime=getCurrentMilliseconds();
	gStatusComm->updateProgressBar(handle,0,"","");
}

void CStatusBar::hide()
{
	if(handle!=-1)
	{
		gStatusComm->endProgressBar(handle);
		handle=-1;
	}
}

#include "unit_conv.h" // for seconds_to_string

const string CStatusBar::getTimeRemaining()
{
	if(lastProgress>0)
	{
		/* NOTE: lastProgress will have been set to the current progress by the time this function is called */

		unsigned long millisecondsPassed=getCurrentMilliseconds()-initialTime;
	
		//                   remaining progress * seconds per progress unit
		unsigned long millisecondsRemaining=(100-lastProgress) * millisecondsPassed/lastProgress;

		//printf("remaining: %s\n",seconds_to_string(millisecondsRemaining/1000,0,false).c_str());

		//printf("estimated total: %s (currentProgress: %d)\n",seconds_to_string((millisecondsPassed+millisecondsRemaining)/1000,0,false).c_str(),lastProgress);
		return seconds_to_string(millisecondsRemaining/1000,0,false);
	}
	else
		return "";
}

const string CStatusBar::getTimeElapsed()
{
	unsigned long millisecondsPassed=getCurrentMilliseconds()-initialTime;
	//printf("elapsed: %s\n",seconds_to_string(millisecondsPassed/1000,0,false).c_str());
	return seconds_to_string(millisecondsPassed/1000,0,false);
}

