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

// the subscript into this vector is the enum TimeUnits
vector<bool> gShowTimeUnits;

void readFrontendSettings()
{
	GET_SETTING("FOX" DOT "drawVerticalCuePositions",gDrawVerticalCuePositions,bool)
	GET_SETTING("FOX" DOT "snapToCues",gSnapToCues,bool)
	GET_SETTING("FOX" DOT "snapToCueDistance",gSnapToCueDistance,unsigned)

	GET_SETTING("FOX" DOT "followPlayPosition",gFollowPlayPosition,bool)

	GET_SETTING("FOX" DOT "renderClippingWarning",gRenderClippingWarning,bool)

	GET_SETTING("FOX" DOT "initialLengthToShow",gInitialLengthToShow,double)

	GET_SETTING("FOX" DOT "showTimeUnits",gShowTimeUnits,vector<bool>)
	if(gShowTimeUnits.size()!=TIME_UNITS_COUNT)
	{	// default it
		gShowTimeUnits.clear();
		gShowTimeUnits.push_back(true);
		gShowTimeUnits.push_back(false);
		gShowTimeUnits.push_back(false);
	}
}

void writeFrontendSettings()
{
	gSettingsRegistry->setValue<bool>("FOX" DOT "drawVerticalCuePositions",gDrawVerticalCuePositions);
	gSettingsRegistry->setValue<bool>("FOX" DOT "snapToCues",gSnapToCues);
	gSettingsRegistry->setValue<unsigned>("FOX" DOT "snapToCueDistance",gSnapToCueDistance);

	gSettingsRegistry->setValue<bool>("FOX" DOT "followPlayPosition",gFollowPlayPosition);

	gSettingsRegistry->setValue<bool>("FOX" DOT "renderClippingWarning",gRenderClippingWarning);

	gSettingsRegistry->setValue<double>("FOX" DOT "initialLengthToShow",gInitialLengthToShow);

	gSettingsRegistry->setValue<vector<bool> >("FOX" DOT "showTimeUnits",gShowTimeUnits);
}

void popupTimeUnitsSelectionMenu(FXWindow *owner,FXEvent *ev)
{
#if REZ_FOX_VERSION>=10119 /* nah.. don't support prior .. is pain (with setting up event handlers) */
	FXMenuPane timeUnitsMenu(owner);
		// ??? make sure these get deleted when timeUnitsMenu gets deleted
		new FXMenuCaption(&timeUnitsMenu,_("Show Time Units in..."),NULL,JUSTIFY_LEFT);
		new FXMenuSeparator(&timeUnitsMenu);
			FXMenuCheck *secondsItem=new FXMenuCheck(&timeUnitsMenu,_("Seconds"));
				secondsItem->setCheck(gShowTimeUnits[0]);
			FXMenuCheck *framesItem=new FXMenuCheck(&timeUnitsMenu,_("Frames"));
				framesItem->setCheck(gShowTimeUnits[1]);
			FXMenuCheck *bytesItem=new FXMenuCheck(&timeUnitsMenu,_("Bytes"));
				bytesItem->setCheck(gShowTimeUnits[2]);

		timeUnitsMenu.create();
		timeUnitsMenu.popup(NULL,ev->root_x,ev->root_y);
		owner->getApp()->runModalWhileShown(&timeUnitsMenu);

	gShowTimeUnits[0]=secondsItem->getCheck();
	gShowTimeUnits[1]=framesItem->getCheck();
	gShowTimeUnits[2]=bytesItem->getCheck();
#endif
}
