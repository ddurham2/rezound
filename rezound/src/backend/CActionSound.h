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

#ifndef __CActionSound_H__
#define __CActionSound_H__

#include "../../config/common.h"

class CActionSound;


#include "CSound_defs.h"

class CSoundPlayerChannel;


/*
    This class specifies which sound, which channels in that sound
    and from where and to where to do the action to that sound.
*/
class CActionSound
{
public:
	CSound *sound;
	bool doChannel[MAX_CHANNELS];
	bool doCrossfadeEdges;

	// the start and stop data-members are used to set the
	// selectStart and selectStop positions after doing the action
	mutable sample_pos_t start,stop;

	// default method of doing an action (all channels, apply just to selection)
	CActionSound(CSoundPlayerChannel *channel,bool doCrossfadeEdges);
	CActionSound(const CActionSound &src);

	unsigned countChannels() const;
	void allChannels();
	void noChannels();

	sample_pos_t selectionLength() const;
	void selectAll() const;
	void selectNone() const;

	CActionSound &operator=(const CActionSound &rhs);
};


#endif
