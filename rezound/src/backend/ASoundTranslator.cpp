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

#include <stdexcept>

vector<const ASoundTranslator *> ASoundTranslator::registeredTranslators;

ASoundTranslator::ASoundTranslator()
{
}

ASoundTranslator::~ASoundTranslator()
{
}

void ASoundTranslator::loadSound(const string filename,CSound *sound) const
{
	sound->lockForResize();
	try
	{
		// use working file if it exists
		if(!sound->createFromWorkingPoolFileIfExists(filename))
			onLoadSound(filename,sound);

		sound->unlockForResize();
	}
	catch(...)
	{
		sound->unlockForResize();
		throw;
	}
}

void ASoundTranslator::saveSound(const string filename,CSound *sound) const
{
	// lockSize
	onSaveSound(filename,sound);
}

