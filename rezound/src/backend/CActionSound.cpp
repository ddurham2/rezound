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

#include "CActionSound.h"

#include <stdexcept>

#include "CSoundPlayerChannel.h"
#include "CSound.h"


// ----------------------------------------------------------------------
// -- CActionSound ------------------------------------------------------
// ----------------------------------------------------------------------

CActionSound::CActionSound(CSoundPlayerChannel *_channel,bool _doCrossfadeEdges) :
	sound(_channel->sound),
	doCrossfadeEdges(_doCrossfadeEdges)
{
	if(_channel==NULL)
		throw(runtime_error(string(__func__)+" -- channel parameter is NULL"));

	start=_channel->getStartPosition();
	stop=_channel->getStopPosition();
	allChannels();
}

CActionSound::CActionSound(const CActionSound &src)
{
	operator=(src);
}

unsigned CActionSound::countChannels() const
{
	unsigned i=0;
	for(unsigned t=0;t<MAX_CHANNELS;t++)
	{
		if(doChannel[t])
			i++;
	}
	return(i);
}

void CActionSound::allChannels()
{
	for(unsigned t=0;t<sound->getChannelCount();t++)
		doChannel[t]=true;
	for(unsigned t=sound->getChannelCount();t<MAX_CHANNELS;t++)
		doChannel[t]=false;
}

void CActionSound::noChannels()
{
	for(unsigned t=0;t<MAX_CHANNELS;t++)
		doChannel[t]=false;
}

sample_pos_t CActionSound::selectionLength() const
{
	return((stop-start)+1);
}

void CActionSound::selectAll() const
{
	start=0;
	stop=sound->getLength()-1;
}

void CActionSound::selectNone() const
{
	start=1;
	stop=0;
}

CActionSound &CActionSound::operator=(const CActionSound &rhs)
{
	sound=rhs.sound;
	doCrossfadeEdges=rhs.doCrossfadeEdges;

	for(unsigned t=0;t<MAX_CHANNELS;t++)
		doChannel[t]=rhs.doChannel[t];

	start=rhs.start;
	stop=rhs.stop;

	return(*this);
}



