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

/********************************************************************************
*                                                                               *
*          P a  c  k  e  r   C a n v a s   W i n d o w   O b j e c t            *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2001 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or                 *
* modify it under the terms of the GNU Lesser General Public                    *
* License as published by the Free Software Foundation; either                  *
* version 2.1 of the License, or (at your option) any later version.            *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU             *
* Lesser General Public License for more details.                               *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public              *
* License along with this library; if not, write to the Free Software           *
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.    *
*********************************************************************************
* $Id$               *
********************************************************************************/
#include <fox/xincs.h>
#include <fox/fxver.h>
#include <fox/fxdefs.h>
#include <fox/FXStream.h>
#include <fox/FXString.h>
#include <fox/FXSize.h>
#include <fox/FXPoint.h>
#include <fox/FXRectangle.h>
#include <fox/FXSettings.h>
#include <fox/FXRegistry.h>
#include <fox/FXApp.h>
#include "FXPackerCanvas.h"



/*******************************************************************************/

// Map
FXDEFMAP(FXPackerCanvas) FXPackerCanvasMap[]={
  FXMAPFUNC(SEL_PAINT,0,FXPackerCanvas::onPaint),
  FXMAPFUNC(SEL_MOTION,0,FXPackerCanvas::onMotion),
  FXMAPFUNC(SEL_KEYPRESS,0,FXPackerCanvas::onKeyPress),
  FXMAPFUNC(SEL_KEYRELEASE,0,FXPackerCanvas::onKeyRelease),
  };


// Object implementation
FXIMPLEMENT(FXPackerCanvas,FXPacker,FXPackerCanvasMap,ARRAYNUMBER(FXPackerCanvasMap))


// For serialization
FXPackerCanvas::FXPackerCanvas(){
  flags|=FLAG_ENABLED|FLAG_SHOWN;
  }


// Make a canvas
FXPackerCanvas::FXPackerCanvas(FXComposite* p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs) :
  FXPacker(p,opts,x,y,w,h,pl,pr,pt,pb,hs,vs){
  flags|=FLAG_ENABLED|FLAG_SHOWN;
  backColor=getApp()->getBackColor();
  target=tgt;
  message=sel;
  }



// It can be focused on
FXbool FXPackerCanvas::canFocus() const { return TRUE; }


// Canvas is an object drawn by another
long FXPackerCanvas::onPaint(FXObject*,FXSelector,void* ptr){
  return target && target->handle(this,MKUINT(message,SEL_PAINT),ptr);
  }


// Mouse moved
long FXPackerCanvas::onMotion(FXObject*,FXSelector,void* ptr){
  return isEnabled() && target && target->handle(this,MKUINT(message,SEL_MOTION),ptr);
  }


// Handle keyboard press/release
long FXPackerCanvas::onKeyPress(FXObject*,FXSelector,void* ptr){
  flags&=~FLAG_TIP;
  return isEnabled() && target && target->handle(this,MKUINT(message,SEL_KEYPRESS),ptr);
  }


long FXPackerCanvas::onKeyRelease(FXObject*,FXSelector,void* ptr){
  return isEnabled() && target && target->handle(this,MKUINT(message,SEL_KEYRELEASE),ptr);
  }


