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

//#include <Global.h>
#include "file.h"

#include <exception>

#include "AStatusComm.h"
#include "ASoundFileManager.h"

void openSound(ASoundFileManager *soundFileManager,const string filename)
{
	try
	{
		soundFileManager->open(filename);
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}
//---------------------------------------------------------------------------

void newSound(ASoundFileManager *soundFileManager)
{
	try
	{
		soundFileManager->createNew();
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}

//---------------------------------------------------------------------------


void closeSound(ASoundFileManager *soundFileManager)
{
	try
	{
		soundFileManager->close(ASoundFileManager::ctSaveYesNoCancel);
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}

//---------------------------------------------------------------------------

void saveSound(ASoundFileManager *soundFileManager)
{
	try
	{
		soundFileManager->save();
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}

//---------------------------------------------------------------------------

void saveAsSound(ASoundFileManager *soundFileManager)
{
	try
	{
		soundFileManager->saveAs();
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}

//---------------------------------------------------------------------------

void revertSound(ASoundFileManager *soundFileManager)
{
	try
	{
		soundFileManager->revert();
	}
	catch(exception &e)
	{
		Error(e.what());
	}
}

//---------------------------------------------------------------------------

const bool exitReZound(ASoundFileManager *soundFileManager)
{
/*
	if(Question("Are you sure you want to exit ReZound?",yesnoQues)!=yesAns)
		return(false);
*/
	try
	{
		while(soundFileManager->getActive()!=NULL)
			soundFileManager->close(ASoundFileManager::ctSaveYesNoStop);
		return(true);
	}
	catch(EStopClosing &e)
	{
		return(false);
	}
	catch(exception &e)
	{
		Error(e.what());
		return(false);
	}
}


