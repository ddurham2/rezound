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

#ifndef __frontend_setting_H__
#define __frontend_setting_H__

#include "../../config/common.h"
#include "fox_compat.h"

#include "../backend/settings.h"


#define CUE_RADIUS 3
#define CUE_OFF_SCREEN (-(CUE_RADIUS+1000))


/*
 * True if vertical cue positions should be drawn across the waveform
 */
extern bool gDrawVerticalCuePositions;

/*
 * These are the parameters of whether or not to snap the selection positions
 * to the cue position, and if so, how far from a cue to does the position have
 * to be before it snaps to it.
 */
extern bool gSnapToCues;
extern unsigned gSnapToCueDistance;


/*
 * True if the sound windows should follow the play position
 */
extern bool gFollowPlayPosition;


/*
 * True if the wave view rendering should color max-valued samples brightly
 */
extern bool gRenderClippingWarning;


/*
 * The initial about of audio to show on screen in a newly created sound window
 */
extern double gInitialLengthToShow;


/*
 * Functions to call to read/write these vars to stable storage
 */
void readFrontendSettings();
void writeFrontendSettings();

#endif
