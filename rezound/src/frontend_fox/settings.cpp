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

#include "settings.h"

#include <CNestedDataFile/CNestedDataFile.h>


bool gDrawVerticalCuePositions=true;
bool gSnapToCues=true;
unsigned gSnapToCueDistance=5;

bool gFollowPlayPosition=true;

bool gRenderClippingWarning=true;

double gInitialLengthToShow=120.0; // default 120 seconds


void readFrontendSettings()
{
	GET_SETTING("FOX" DOT "drawVerticalCuePositions",gDrawVerticalCuePositions,bool)
	GET_SETTING("FOX" DOT "snapToCues",gSnapToCues,bool)
	GET_SETTING("FOX" DOT "snapToCueDistance",gSnapToCueDistance,unsigned)

	GET_SETTING("FOX" DOT "followPlayPosition",gFollowPlayPosition,bool)

	GET_SETTING("FOX" DOT "renderClippingWarning",gRenderClippingWarning,bool)

	GET_SETTING("FOX" DOT "initialLengthToShow",gInitialLengthToShow,double)
}

void writeFrontendSettings()
{
	gSettingsRegistry->createValue<bool>("FOX" DOT "drawVerticalCuePositions",gDrawVerticalCuePositions);
	gSettingsRegistry->createValue<bool>("FOX" DOT "snapToCues",gSnapToCues);
	gSettingsRegistry->createValue<unsigned>("FOX" DOT "snapToCueDistance",gSnapToCueDistance);

	gSettingsRegistry->createValue<bool>("FOX" DOT "followPlayPosition",gFollowPlayPosition);

	gSettingsRegistry->createValue<bool>("FOX" DOT "renderClippingWarning",gRenderClippingWarning);

	gSettingsRegistry->createValue<double>("FOX" DOT "initialLengthToShow",gInitialLengthToShow);
}

