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

#include "ASoundPlayer.h"

#include <stdexcept>

ASoundPlayer::ASoundPlayer()
{
}

ASoundPlayer::~ASoundPlayer()
{
}

void ASoundPlayer::initialize()
{
}

void ASoundPlayer::deinitialize()
{
	// wait for out standing sound player channels to go away???
	killAll();
}

CSoundPlayerChannel *ASoundPlayer::newSoundPlayerChannel(ASound *sound)
{
	return(new CSoundPlayerChannel(this,sound));
}

void ASoundPlayer::addSoundPlayerChannel(CSoundPlayerChannel *soundPlayerChannel)
{
	//if(!soundPlayerChannels.add(soundPlayerChannel))
	if(!soundPlayerChannels.insert(soundPlayerChannel).second)
		throw(runtime_error(string(__func__)+" -- sound player channel already in list"));
}

void ASoundPlayer::removeSoundPlayerChannel(CSoundPlayerChannel *soundPlayerChannel)
{
	soundPlayerChannel->kill();
	//soundPlayerChannels.removeValue(soundPlayerChannel);
	
	set<CSoundPlayerChannel *>::const_iterator i=soundPlayerChannels.find(soundPlayerChannel);
	if(i!=soundPlayerChannels.end())
		soundPlayerChannels.erase(i);
}

void ASoundPlayer::mixSoundPlayerChannels(const unsigned nChannels,sample_t * const buffer,const size_t bufferSize)
{
	memset(buffer,0,bufferSize*sizeof(*buffer)*nChannels);

	/*
	for(unsigned i=0;i<soundPlayerChannels.getSize();i++)
		soundPlayerChannels[i]->mixOntoBuffer(nChannels,buffer,bufferSize);
	*/
	for(set<CSoundPlayerChannel *>::iterator i=soundPlayerChannels.begin();i!=soundPlayerChannels.end();i++)
		(*i)->mixOntoBuffer(nChannels,buffer,bufferSize);
}

void ASoundPlayer::killAll()
{
	/*
	for(unsigned t=0;t<soundPlayerChannels.getSize();t++)
		soundPlayerChannels[t]->kill();
	*/
	for(set<CSoundPlayerChannel *>::iterator i=soundPlayerChannels.begin();i!=soundPlayerChannels.end();i++)
		(*i)->kill();
}


