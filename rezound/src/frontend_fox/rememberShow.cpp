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

bool rememberShow(FXTopWindow *window)
{
return(false);

	// using this mechanism because FXToolbarShell's position method calls show which causes infinite recursion
	static bool inThis=false;
	if(inThis)
		return(false);
	inThis=true;


	const string title=window->getTitle().text();
	if(!gSettingsRegistry->keyExists((title+"_X").c_str()))
	{
		inThis=false;
		return(false);
	}
	else
	{
		// ??? right now there is a bug... fox seems to read the screen pos according to the very top left corner including the decorations, but sets the screen pos according to the top left not including the decorations

		printf("window: %s %d %d\n",title.c_str(),atoi(gSettingsRegistry->getValue((title+"_X").c_str()).c_str()),atoi(gSettingsRegistry->getValue((title+"_Y").c_str()).c_str()));
		// ??? X and Y need to be guarenteed on screen, but fox or the window manager may handle that
		window->position(
			atoi(gSettingsRegistry->getValue((title+"_X").c_str()).c_str()),atoi(gSettingsRegistry->getValue((title+"_Y").c_str()).c_str()),
			max(50,atoi(gSettingsRegistry->getValue((title+"_W").c_str()).c_str())),max(50,atoi(gSettingsRegistry->getValue((title+"_H").c_str()).c_str()))
		);

/* could do it this way, but the offset is not what really happend.. fox needs to read it back from X after placing the window
		// getX/Y is not returning what it should !!!
		window->move(0,0);
		const FXint offsetY=window->getY();
		printf("offsetY: %d\n",offsetY);

		window->move(atoi(gSettingsRegistry->getValue((title+"_X").c_str()).c_str()),atoi(gSettingsRegistry->getValue((title+"_Y").c_str()).c_str())-offsetY);
		window->resize(max(50,atoi(gSettingsRegistry->getValue((title+"_W").c_str()).c_str())),max(50,atoi(gSettingsRegistry->getValue((title+"_H").c_str()).c_str())));
*/
	}
	inThis=false;
	return(true);
}

void rememberHide(FXTopWindow *window)
{
return;
	const string title=window->getTitle().text();
	printf("closing window: %d %d\n",window->getX(),window->getY());
	gSettingsRegistry->createKey((title+"_X").c_str(),istring(window->getX()));
	gSettingsRegistry->createKey((title+"_Y").c_str(),istring(window->getY()));
	gSettingsRegistry->createKey((title+"_W").c_str(),istring(window->getWidth()));
	gSettingsRegistry->createKey((title+"_H").c_str(),istring(window->getHeight()));
}
