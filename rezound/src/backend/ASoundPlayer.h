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

#ifndef __ASoundPlayer_H__
#define __ASoundPlayer_H__

#include "../../config/common.h"

class ASoundPlayer;

#include "CEnvelope.h"

#include <set>

/*
 - This class should be derived from to create the actual player for a given
   operating system platform.

 - This class is not thread-safe, methods from two independent threads should 
   not call common methods for an ASoundPlayer object without a locking mechanism

 - if deinitialize is overridden, then it should not raise an exception if
   the player is not initialized.

 - overriding initialize and deinitialize methods should invoke the overridden method

 - the derived class's destructor needs to call deinitialize because if this base
   class did it, some stuff could be freed in the derived class before the base
   class's destructor ran

 - To play sounds, call createChannel to get a CSoundPlayerChannel object that
   you can use to play, stop, seek ...
*/

#include "CSound_defs.h"
class CSoundPlayerChannel;

#define MAX_OUTPUT_DEVICES 16
class ASoundPlayer
{
public:
	struct RDeviceParams
	{
		unsigned channelCount;
		unsigned sampleRate;

		RDeviceParams() : channelCount(1),sampleRate(44100) { }
	} devices[MAX_OUTPUT_DEVICES];

	virtual ~ASoundPlayer();

	virtual void initialize();
	virtual void deinitialize();

	void killAll();

	CSoundPlayerChannel *newSoundPlayerChannel(CSound *sound);

protected:

	ASoundPlayer();

	// bufferSize is in sample frames (NOT BYTES)
	void mixSoundPlayerChannels(const unsigned nChannels,sample_t * const buffer,const size_t bufferSize);

private:

	friend class CSoundPlayerChannel;

	set<CSoundPlayerChannel *> soundPlayerChannels; // ??? might as well be a vector
	void addSoundPlayerChannel(CSoundPlayerChannel *soundPlayerChannel);
	void removeSoundPlayerChannel(CSoundPlayerChannel *soundPlayerChannel);

};



#endif
