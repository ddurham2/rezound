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

#include "ASoundTranslator.h"
#include "CSound.h"
#include "AStatusComm.h"

#include <stdexcept>

vector<const ASoundTranslator *> ASoundTranslator::registeredTranslators;

ASoundTranslator::ASoundTranslator()
{
}

ASoundTranslator::~ASoundTranslator()
{
}

bool ASoundTranslator::loadSound(const string filename,CSound *sound) const
{
	sound->lockForResize();
	try
	{
		// use working file if it exists
		if(!sound->createFromWorkingPoolFileIfExists(filename))
		{
			if(!onLoadSound(filename,sound))
			{
				sound->unlockForResize();
				sound->closeSound(); // this removes the working poolfile
				return false; // cancelled
			}

			sound->setIsModified(false);
		}

		// ??? could check various other things
		if(!sound->poolFile.isOpen())
			throw(runtime_error(string(__func__)+" -- internal error -- no pool file was open after loading file"));

		sound->unlockForResize();
	}
	catch(...)
	{
		sound->unlockForResize();
		endAllProgressBars();
		throw;
	}
	return true;
}

bool ASoundTranslator::saveSound(const string filename,CSound *sound) const
{
	sound->lockSize();
	try
	{
		/*
		 * ??? A nice feature that would be good now that saving a file can be cancelled
		 * is for onSaveSound to be passed two files.. one to display in error msgs and on dialogs and such
		 * and another to actually write to.. then this code would do a move after everything was successful.
		 * The only problem might be running out of space... Perhaps if space is low, or there isn't
		 * enough for raw PCM size on that directory, then I should prompt the user
		 *
		 * Another solution is to rename the current file that exists and then do the same.. when the 
		 * save is successful, delete the original, or if the save is cancelled, then rename the original
		 * back again.
		 *
		 * ESPECIALLY since saving a file can be cancelled thus killing the original
		 */
		bool ret=onSaveSound(filename,sound);
		sound->unlockSize();
		return ret;
	}
	catch(...)
	{
		sound->unlockSize();
		endAllProgressBars();
		throw;
	}
}

