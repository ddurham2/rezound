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

#include "rememberShow.h"

#include <stdlib.h>

#include <string>
#include <algorithm>

#include <istring>

#include <fox/fx.h>

#include <CNestedDataFile/CNestedDataFile.h>

#include "settings.h"

static FXint decorWidth=-1;
static FXint decorHeight=-1;

void determineDecorSize(FXWindow *window)
{
	decorWidth=0;
	decorHeight=0;
	return;

	if(decorWidth!=-1)
		return;

	/*
		We have to determine the decor size because window managers don't necessary 
		put the window's position and get the window's position relative to the same
		point.  All that I have seen get the windows position as if the decorations 
		were not there, and set the window's position as if the decorations were there.

		Here, I create a window, move it to a location and then read its position back 
		to hopefully get a measure of this difference, and I'll use it as an offset
		when remembering and restoring window's positions.   And if the window manager
		does everything correctly, according to what you think should happen, then this
		offset should just work out to zero and this code will not have done anything 
		unfortunate.

		However, if the window manager places windows according to some pattern like, 
		cascade, or user-placement, then we have a problem, and window-position-remembering
		should probably be disabled.  Somehow this needs to be an initial question to
		the user, and then a setting in the preferences.

		Results:
			KDE 		-- works most of the time but inconsistantly
			Gnome 		-- was remembering window positions/sizes by window title already (didn't know how to turn it off)
			Windowmaker 	-- was crazy in its behavior, shouldn't attempt to remember positions
				- env var "$WMAKER_BIN_NAME" was set while window make was running, I don't know how reliable that is
			Enlightenment	-- was inconsitant in being able to tell me the window positions after a move, and sometimes the decorations would go away completely
				- E! has a function to remember window locations and sizes, so a user can just use that
			Blackbox	-- unsuccessful in ever getting the decor size, and still did seem to behave as tho get/set positions were inconsistant with eachother
			XFce		-- behaved like Windowmaker
			iceWM		-- behaved like Windowmaker
	 */
	
		// need to use a main window or the main window
	FXDialogBox *testWindow=new FXDialogBox(window->getApp(),"Testing Window Placement Descrepancy",DECOR_ALL,0,0,100,100);
	new FXLabel(testWindow,"Testing 1 2 3...");
	testWindow->create();
	testWindow->show();
	testWindow->getApp()->runWhileEvents(testWindow); // wait for it to show

	testWindow->move(100,100);
	testWindow->hide();
	testWindow->show();
	testWindow->getApp()->runWhileEvents(testWindow); // wait for it to show again

	decorWidth=testWindow->getX()-100;
	decorHeight=testWindow->getY()-100;

	printf("decorSize: (%d,%d)\n",decorWidth,decorHeight);

	testWindow->hide();
	delete testWindow;
}

bool rememberShow(FXTopWindow *window)
{
	// ,- isn't reliable with all window managers behaving differently 
	// |
	// V
	//determineDecorSize(window);

	// using this mechanism because FXToolBarShell's position method calls show which causes infinite recursion
	static bool inThis=false;
	if(inThis)
		return(false);
	inThis=true;

	const string title=("WindowDimensions."+window->getTitle()).text();
	if(!gSettingsRegistry->keyExists((title+"_X").c_str()))
	{
		inThis=false;
		return(false);
	}
	else
	{
		FXint x=atoi(gSettingsRegistry->getValue((title+"_X").c_str()).c_str());
		FXint y=atoi(gSettingsRegistry->getValue((title+"_Y").c_str()).c_str());
		FXint width=atoi(gSettingsRegistry->getValue((title+"_W").c_str()).c_str());
		FXint height=atoi(gSettingsRegistry->getValue((title+"_H").c_str()).c_str());

		//x+=decorWidth;
		//y+=decorHeight;

		//printf("window: %s X:%d Y:%d W:%d H:%d\n", title.c_str(),x,y,width,height);

#ifdef FOX_RESTORE_WINDOW_POSITIONS
		window->position(
			// make sure x,y are in visible range
			min(max(0,x),window->getRoot()->getDefaultWidth()),
			min(max(0,y),window->getRoot()->getDefaultHeight()),

			// no less than an window with 50 height or width
			max(50,width),
			max(50,height)
		);
#else
		// just restore the window's size
		if(window->getWidth()!=width || window->getHeight()!=height)
			window->resize( max(50,width), max(50,height) );
		else 
		{
			inThis=false;
			return(false);
		}
#endif

	}
	inThis=false;
	return(true);
}

void rememberHide(FXTopWindow *window)
{
	const string title=("WindowDimensions."+window->getTitle()).text();
	//printf("closing window: %s %d %d\n",window->getTitle().text(),window->getX(),window->getY());
	gSettingsRegistry->createKey((title+"_X").c_str(),istring(window->getX()/*-decorWidth*/));
	gSettingsRegistry->createKey((title+"_Y").c_str(),istring(window->getY()/*-decorHeight*/));
	gSettingsRegistry->createKey((title+"_W").c_str(),istring(window->getWidth()));
	gSettingsRegistry->createKey((title+"_H").c_str(),istring(window->getHeight()));
}

