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

/*
 * Some of the code for handling selection changes via the mouse is in here and 
 * some is in FXWaveCanvas because I would like it all to be in FXWaveCanvas if
 * possible, but handling the scrolling when on edges involves the * FXScrollArea.
 * It may be possible, but I didn't want to attempt it at the time of the rewrite
 * of this wave viewing widget set.
 */

#include "FXWaveScrollArea.h"

#include "FXRezWaveView.h"

#include "../backend/CLoadedSound.h"
#include "../backend/CSoundPlayerChannel.h"

/* TODO:
 *
 * - To make selection changes undoable, I could make an event which indicated the new and old values of the start and stop positions so that an AAction could be pushed onto the undo stack
 *     - but I should do this only on mouse up event so that all mouse movements don't create AActions
 *         - the old value is from just before the mouse down event and the new value is from the mouse up event
 *
 * - Make ctrl-left-click center the positioned clicked? for what purpose?
 *
 * - I need to somewhere print the time that the cursor is over, and clear it onExit of the canvas
 *
 * - I have gone with fox's method of sending messages to the CSoundWindow that owns us to let it know when the start or stop position has changed and it should be sufficient for now, but
 *   	- I could use a TTrigger (but change it to support multiple registrations) and have the CSoundPlayerChannel tell us when to update the start or stop position which would for sure always be up to date
 *   	- Play position could work the same way
 *   	- Do I want to go with this way of doing it an perhaps simplify the job of making sure the status panel is up to date?
 */



FXDEFMAP(FXWaveScrollArea) FXWaveViewScrollAreaMap[]=
{
	//Message_Type				ID				Message_Handler
	FXMAPFUNC(SEL_CONFIGURE,		0,				FXWaveScrollArea::onResize),
	FXMAPFUNC(SEL_LEFTBUTTONPRESS,		FXWaveScrollArea::ID_CANVAS,	FXWaveScrollArea::onMouseDown),
	FXMAPFUNC(SEL_RIGHTBUTTONPRESS,		FXWaveScrollArea::ID_CANVAS,	FXWaveScrollArea::onMouseDown),
	FXMAPFUNC(SEL_LEFTBUTTONRELEASE,	FXWaveScrollArea::ID_CANVAS,	FXWaveScrollArea::onMouseUp),
	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,	FXWaveScrollArea::ID_CANVAS,	FXWaveScrollArea::onMouseUp),
	FXMAPFUNC(SEL_MOTION,			FXWaveScrollArea::ID_CANVAS,	FXWaveScrollArea::onMouseMove),
	FXMAPFUNC(SEL_TIMEOUT,			FXWindow::ID_AUTOSCROLL,	FXWaveScrollArea::onAutoScroll),

#if FOX_MAJOR >= 1
	// re-route the mouse wheel event to scroll horizontally instead of its default, vertically
	FXMAPFUNC(SEL_MOUSEWHEEL,0,FXScrollArea::onHMouseWheel),
#endif
};

FXIMPLEMENT(FXWaveScrollArea,FXScrollArea,FXWaveViewScrollAreaMap,ARRAYNUMBER(FXWaveViewScrollAreaMap))

FXWaveScrollArea::FXWaveScrollArea(FXRezWaveView *_parent,CLoadedSound *_loadedSound) :
	FXScrollArea(_parent,LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0),

	parent(_parent),

	canvas(new FXWaveCanvas(_loadedSound,this,this,ID_CANVAS,FRAME_NONE)),

	loadedSound(_loadedSound),

	draggingSelectStart(false),draggingSelectStop(false)
{
	enable();

	setScrollStyle(HSCROLLER_ALWAYS|VSCROLLER_ALWAYS);

	horizontalScrollBar()->setLine(25);
	verticalScrollBar()->setLine(25);
}

FXWaveScrollArea::~FXWaveScrollArea()
{
}

void FXWaveScrollArea::updateFromEdit()
{
	canvas->updateFromEdit();
	setPosition(-canvas->getHorzOffset(),-canvas->getVertOffset());
	layout();
}

void FXWaveScrollArea::setHorzZoom(double v,FXWaveCanvas::HorzRecenterTypes horzRecenterType)
{
	const double oldZoom=canvas->getHorzZoom();

	canvas->setHorzZoom(v,horzRecenterType);
	const sample_pos_t horzOffset=canvas->getHorzOffset();

	// re-read the possibly adjusted horizontal offset since FXWaveCanvas::setHorzZoom() might try to keep something on screen that was previously visible

	if(v<oldZoom) 
		setPosition(-(FXint)horzOffset,pos_y);

	layout();

	if(v>oldZoom)
		setPosition(-(FXint)horzOffset,pos_y);

	horizontal->repaint(); // cause it's a little slow to update on its own
}

double FXWaveScrollArea::getHorzZoom() const
{
	return canvas->getHorzZoom();
}

void FXWaveScrollArea::setVertZoom(double v)
{
	const double oldZoom=canvas->getVertZoom();

	canvas->setVertZoom(v);
	const int vertOffset=canvas->getVertOffset();

	// re-read the possibly adjusted vertical offset since FXWaveCanvas::setVertZoom() tries to keep what was the middle still the middle of what's on screen

	if(v<oldZoom) 
		setPosition(pos_x,-vertOffset);

	layout();

	if(v>oldZoom)
		setPosition(pos_x,-vertOffset);

	vertical->repaint(); // cause it's a little slow to update on its own
}

void FXWaveScrollArea::drawPlayPosition(sample_pos_t dataPosition,bool justErasing,bool scrollToMakeVisible)
{
	canvas->drawPlayPosition(dataPosition,justErasing,scrollToMakeVisible,this);
}

void FXWaveScrollArea::redraw()
{
	update();
	canvas->update();
}

//??? here is one problematic area if I support >31-bit length files
	// FXint is only 31bit (at least it's typedef-ed from int)
	// when zoomed in, the content length is more than 2^31
	// also, in moveContents

FXint FXWaveScrollArea::getContentWidth()
{
	return(canvas->getHorzSize());
}

FXint FXWaveScrollArea::getContentHeight()
{
	return(canvas->getVertSize());
}

void FXWaveScrollArea::moveContents(FXint x,FXint y)
{
	FXint old_pos_x=pos_x;
	FXScrollArea::moveContents(x,y);

	canvas->setHorzOffset((sample_pos_t)-pos_x);
	canvas->setVertOffset((int)-pos_y);

	if(old_pos_x!=pos_x)
		parent->updateRuler();
}

void FXWaveScrollArea::layout()
{
	FXScrollArea::layout();
	// ??? if I allow the scrollbars to disappear I don't need to subtract their sizes
	canvas->resize(getWidth()-verticalScrollBar()->getWidth(),getHeight()-(horizontalScrollBar()->getHeight()));
}

long FXWaveScrollArea::onResize(FXObject *object,FXSelector sel,void *ptr)
{
	layout();
	updateFromEdit();
	return 1;
}

long FXWaveScrollArea::onMouseDown(FXObject*,FXSelector,void *ptr)
{
	FXEvent *ev=(FXEvent*) ptr;
	const FXint X=ev->click_x;

	if(!draggingSelectStop && !draggingSelectStart)
	{

		if(ev->click_button==LEFTBUTTON)
		{
			if(X<=(FXint)canvas->getDrawSelectStop())
			{
				draggingSelectStart=true;
				canvas->setSelectStartFromScreen(X);
			}
			else
			{
				draggingSelectStop=true;
				const sample_pos_t origSelectStop=loadedSound->channel->getStopPosition();
				canvas->setSelectStopFromScreen(X);
				loadedSound->channel->setStartPosition(origSelectStop);
			}
		}
		else if(ev->click_button==RIGHTBUTTON)
		{
			if(X>=(FXint)canvas->getDrawSelectStart())
			{
				draggingSelectStop=true;
				canvas->setSelectStopFromScreen(X);
			}
			else
			{
				draggingSelectStart=true;
				const sample_pos_t origSelectStart=loadedSound->channel->getStartPosition();
				canvas->setSelectStartFromScreen(X);
				loadedSound->channel->setStopPosition(origSelectStart);
			}
		}
	}
	updateFromSelectionChange();

	return 1;
}

long FXWaveScrollArea::onMouseUp(FXObject*,FXSelector,void *ptr)
{
	FXEvent *ev=(FXEvent*) ptr;

	if((ev->click_button==LEFTBUTTON || ev->click_button==RIGHTBUTTON) && (draggingSelectStart || draggingSelectStop))
	{
		stopAutoScroll();
		draggingSelectStart=draggingSelectStop=false;
	}
	return 1;
}

void FXWaveScrollArea::handleMouseMoveSelectChange(FXint X)
{
	// I put it here so that when shift is held to stop the autoscrolling that it doesn't select anything off screen even if the mouse is outside the wave view
	X=max((int)0,min((int)X,(int)(canvas->getWidth()-1)));

	if(draggingSelectStart)
	{
		if(X>(FXint)canvas->getDrawSelectStop())
		{
			draggingSelectStart=false;
			draggingSelectStop=true;
			const sample_pos_t origSelectStop=loadedSound->channel->getStopPosition();
			canvas->setSelectStopFromScreen(X);
			loadedSound->channel->setStartPosition(origSelectStop);
		}
		else
			canvas->setSelectStartFromScreen(X);

		updateFromSelectionChange();
	}
	else if(draggingSelectStop)
	{
		if(X<=(FXint)canvas->getDrawSelectStart())
		{
			draggingSelectStop=false;
			draggingSelectStart=true;
			const sample_pos_t origSelectStart=loadedSound->channel->getStartPosition();
			canvas->setSelectStartFromScreen(X);
			loadedSound->channel->setStopPosition(origSelectStart);
		}
		else
			canvas->setSelectStopFromScreen(X);

		updateFromSelectionChange();
	}
}

long FXWaveScrollArea::onMouseMove(FXObject*,FXSelector,void *ptr)
{
	FXEvent *ev=(FXEvent*) ptr;

	// Here we detect if the mouse is against the side walls and scroll that way if the mouse is down
	if(draggingSelectStart || draggingSelectStop)
	{
		if(!(ev->state&SHIFTMASK))
		{
			if(startAutoScroll(ev->win_x,ev->win_y))
				return 1;
		}
	}

	handleMouseMoveSelectChange(ev->win_x);

	return 1;
}

long FXWaveScrollArea::onAutoScroll(FXObject *object,FXSelector sel,void *ptr)
{
	long ret=FXScrollArea::onAutoScroll(object,sel,ptr);
	handleMouseMoveSelectChange(((FXEvent *)ptr)->win_x);
	return(ret);
}


void FXWaveScrollArea::centerStartPos()
{
	setPosition(-canvas->getHorzOffsetToCenterStartPos(),pos_y);
}

void FXWaveScrollArea::centerStopPos()
{
	setPosition(-canvas->getHorzOffsetToCenterStopPos(),pos_y);
}

void FXWaveScrollArea::showAmount(double seconds,sample_pos_t pos)
{
	canvas->showAmount(seconds,pos);
	updateFromEdit();
}

const sample_pos_t FXWaveScrollArea::getSamplePosForScreenX(FXint x) const
{
	return canvas->getSamplePosForScreenX(x);
}

const FXint FXWaveScrollArea::getCanvasWidth() const
{
	return canvas->getWidth();
}

void FXWaveScrollArea::updateFromSelectionChange(FXWaveCanvas::LastChangedPositions lastChangedPosition)
{
	canvas->updateFromSelectionChange(lastChangedPosition);
	if(parent->target)
		parent->target->handle(parent,MKUINT(parent->message,FXRezWaveView::SEL_SELECTION_CHANGED),NULL);
}

const FXint FXWaveScrollArea::getCueScreenX(size_t cueIndex) const
{
	return canvas->getCueScreenX(cueIndex);
}

const sample_pos_t FXWaveScrollArea::getCueTimeFromX(FXint screenX) const
{
	return canvas->getCueTimeFromX(screenX);
}
