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

AStatusComm *gStatusComm=NULL;

void Error(const string &message,VSeverity severity=none)
{
	gStatusComm->error(message,severity);
}

void Warning(const string &message)
{
	gStatusComm->warning(message);
}

void Message(const string &message)
{
	gStatusComm->message(message);
}

VAnswer Question(const string &message,VQuestion options)
{
	return(gStatusComm->question(message,options));
}



int beginProgressBar(const string &title)
{
	return(gStatusComm->beginProgressBar(title));
}

void endProgressBar(int handle)
{
	gStatusComm->endProgressBar(handle);
}

void endAllProgressBars()
{
	gStatusComm->endAllProgressBars();
}

