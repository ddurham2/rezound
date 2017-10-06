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

#include "CLoadedSound.h"

#include "AAction.h"
#include "CSoundPlayerChannel.h"
#include "CSound.h"

CLoadedSound::CLoadedSound(const CLoadedSound &src) :
	channel(src.channel),
	sound(src.channel->getSound()),
	translator(src.translator),
	filename(src.filename),
	_isReadOnly(src._isReadOnly)
{
}

CLoadedSound::CLoadedSound(const string _filename,CSoundPlayerChannel *_channel,bool __isReadOnly,const ASoundTranslator *_translator) :
	channel(_channel),
	sound(_channel->getSound()),
	translator(_translator),
	filename(_filename),
	_isReadOnly(__isReadOnly)
{
}

CLoadedSound::~CLoadedSound()
{
	clearUndoHistory();
	delete channel;
}

void CLoadedSound::clearUndoHistory()
{
	while(!actions.empty())
	{
		delete actions.top();
		actions.pop();
	}
}

const string CLoadedSound::getFilename() const
{
	return(filename);
}

void CLoadedSound::changeFilename(const string newFilename)
{
	sound->changeWorkingFilename(newFilename);
	filename=newFilename;
}

bool CLoadedSound::isReadOnly() const
{
	return(_isReadOnly);
}

