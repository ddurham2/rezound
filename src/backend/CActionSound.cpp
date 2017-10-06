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

#include "settings.h"
#include "unit_conv.h"


// ----------------------------------------------------------------------
// -- CActionSound ------------------------------------------------------
// ----------------------------------------------------------------------

CActionSound::CActionSound(CSoundPlayerChannel *_channel,CrossfadeEdgesTypes _doCrossfadeEdges) :
	sound(_channel->getSound()),
	doCrossfadeEdges(_doCrossfadeEdges)
{
	if(_channel==NULL)
		throw(runtime_error(string(__func__)+" -- channel parameter is NULL"));

	start=_channel->getStartPosition();
	stop=_channel->getStopPosition();
	
	playPosition=_channel->getPosition();

	// now shift playPosition by a setting
	const int playPositionShift=s_to_samples_offset(gPlayPositionShift,sound->getSampleRate());
	if(playPositionShift!=0)
	{
		if(playPositionShift>0 && (playPosition+playPositionShift)<sound->getLength()) // don't overrun the length
			playPosition+=playPositionShift;
		else if(playPositionShift<0 && (sample_fpos_t)playPosition>playPositionShift) // don't underrun zero
			playPosition+=playPositionShift;
		else
			playPosition=0;
	}

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
	return i;
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
	return (stop-start)+1;
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
	playPosition=rhs.playPosition;

	return *this;
}



