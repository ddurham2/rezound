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
#ifndef DRAW_PORTION_H
#define DRAW_PORTION_H

#include "../../config/common.h"
#include "fox_compat.h"

class CSound;

#include <fox/fxdefs.h>

#ifdef FOX_NO_NAMESPACE
	class FXDCWindow;

	extern FXColor backGroundColor;
	extern FXColor clippedWaveformColor;
#else
	namespace FX { class FXDCWindow; }
	using namespace FX;

	extern FX::FXColor backGroundColor;
	extern FX::FXColor clippedWaveformColor;
#endif

extern void drawPortion(int left,int width,FXDCWindow *dc,CSound *sound,int canvasWidth,int canvasHeight,int drawSelectStart,int drawSelectStop,double horzZoomFactor,int hOffset,double vertZoomFactor,int vOffset,bool darkened=false);



#endif
