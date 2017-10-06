/********************************************************************************
*                                                                               *
*             B a c k  B u f f e r e d  C a n v a s  W i d g e t                *
*                                                                               *
*********************************************************************************
* Copyright (C) 2002 by Davy Durham.  With coding tips from Jeroen van der Zijp *
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
* $Id$                       *
********************************************************************************/
#ifndef FXBACKBUFFEREDCANVAS_H
#define FXBACKBUFFEREDCANVAS_H


// NOTE: Jeroen (author of FOX) said that he would at some point put this widget into FOX
// so for now, I'm adding it to the missing directory in case someone doesn't have a version
// of FOX with this widget.  Eventually I should be able to remove it.  The original source
// had a separate .h and .cpp file.. I combined it for a quick and dirty way not to need a
// lib

//#include "FXCanvas.h" // uncomment when putting into lib

// namespace FX {


// Back Buffered Canvas, an area drawn by another object, except a little more expensive but to reduce flicker
class FXAPI FXBackBufferedCanvas : public FXCanvas 
{
	FXDECLARE(FXBackBufferedCanvas)
public:
	// Construct new back buffered drawing canvas widget
	FXBackBufferedCanvas(FXComposite* p,FXObject* tgt=NULL,FXSelector sel=0,FXuint opts=FRAME_NORMAL,FXint x=0,FXint y=0,FXint w=0,FXint h=0) :
		FXCanvas(p,tgt,sel,opts,x,y,w,h),
		backBuffer(new FXImage(getApp(),NULL,0,w,h)),
		inOnPaintEvent(false),
		currentDC(NULL)
	{
	}

	virtual ~FXBackBufferedCanvas()
	{
		delete backBuffer;
	}

	/*
	 * - This can be used to set the options of the back buffer to include
	 * IMAGE_OWNED so that the drawing code can call getBackBufferData()
	 * and render/modify the back buffer.
	 * - When IMAGE_OWNED is included in the options, then backBuffer->render()
	 * will be called just before the backbuffer is blitted to the screen.
	 */
	void setBackBufferOptions(FXint options)
	{
		delete backBuffer;
		if(options&IMAGE_OWNED) options|=IMAGE_KEEP;
		backBuffer=new FXImage(getApp(),NULL,options,getWidth(),getHeight());
	}
	// NOTE: ??? if IMAGE_OWNED is in the flags, then there is really no need to allocate an FXDCWindow and return it via the ptr parameter in the onPaint even or via the return value of beginPaint()

	void *getBackBufferData()
	{
		return backBuffer->getData();
	}

	void create()
	{
		FXCanvas::create();
		backBuffer->create();
	}

	long onPaint(FXObject *sender,FXSelector sel,void *ptr)
	{
		// ??? I hope no one triggers this event again while we're handling this event (could check the inOnPaintEvent flag and ignore it if necesary)

		endPaint(); // just in case someone forgot to call endPaint after calling beginPaint or an except wasn't handled or something

		if(((FXEvent *)ptr)->synthetic && target)
		{ // triggered by a call to update()
			inOnPaintEvent=true; // used to avoid problems if the user calls beginPaint and endPaint within the onPaint handler
			FXDCWindow dc(backBuffer);
			currentDC=&dc;

			target->handle(this,FXSEL(SEL_PAINT,message),currentDC);

			inOnPaintEvent=false;
			currentDC=NULL;
		}

		// now blit the back buffer to the screen
		FXDCWindow screenDC(this);
		FXEvent *ev=(FXEvent *)ptr;
		backBuffer->render();
		screenDC.drawArea(backBuffer,ev->rect.x,ev->rect.y,ev->rect.w,ev->rect.h,ev->rect.x,ev->rect.y);

		return 1;
	}

	// Resized .. so resize the back buffer and repaint it
	long onConfigure(FXObject *sender,FXSelector sel,void *ptr)
	{
		backBuffer->resize(getWidth(),getHeight());
		update();
		if(target)
			target->handle(this,FXSEL(SEL_CONFIGURE,message),ptr);
		return 1;
	}

	// Normally, the user should wait for the SEL_PAINT event to draw the the canvas, in which case
	// the ptr parameter would contain the DC to draw to. 
	//
	// However, these two methods need to be used if the user is not waiting for the SEL_PAINT event 
	// to draw to the canvas since the user should actually be drawing to the back buffer and not the
	// actual canvas on screen.  (I suppose much more work could be done to make the use of back buffering
	// completly transparent and make it have the same interface as FXCanvas)
	FXDC *beginPaint()
	{
		if(inOnPaintEvent)
			return currentDC;
		else
			return currentDC=new FXDCWindow(backBuffer);
	}

	void endPaint()
	{
		if(!inOnPaintEvent && currentDC!=NULL)
		{
			delete currentDC;
			currentDC=NULL;

			// I suppose we should just blit the whole thing when they're done painting
			FXDCWindow screenDC(this);
			backBuffer->render();
			screenDC.drawArea(backBuffer,0,0,getWidth(),getHeight(),0,0);
			// tell the event loop that no painting is necessary now for this window???
			// 	is there a way to do this Jeroen?
		}
	}

protected:
	FXBackBufferedCanvas() {}
private:
	FXImage *backBuffer;
	bool inOnPaintEvent;
	FXDC *currentDC;
};

// Map
FXDEFMAP(FXBackBufferedCanvas) FXBackBufferedCanvasMap[]=
{
	FXMAPFUNC(SEL_PAINT,0,FXBackBufferedCanvas::onPaint),
	FXMAPFUNC(SEL_CONFIGURE,0,FXBackBufferedCanvas::onConfigure),
};


// Object implementation
FXIMPLEMENT(FXBackBufferedCanvas,FXWindow,FXBackBufferedCanvasMap,ARRAYNUMBER(FXBackBufferedCanvasMap))

// }

#endif
