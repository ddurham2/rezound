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

#include "FXRezWaveView.h"

#include <math.h>

#include <string>
#include <algorithm>

#include "settings.h"

#include "drawPortion.h"

#include "../backend/CLoadedSound.h"
#include "../backend/CSound.h"
#include "../backend/CSoundPlayerChannel.h"

// these could be static class members
static FXColor playStatusColor=FXRGB(255,0,0);

/* TODO:
 *
 * - I could fairly easily have a mode where if the sound is playing, then every .25 sec or so, scroll the window to that position so the play pos is always on screen
 *
 * - To make selection changes undoable, I could make an event which indicated the new and old values of the start and stop positions so that an AAction could be pushed onto the undo stack
 *     - but I should do this only on mouse up event so that all mouse movements don't create AActions
 *         - the old value is from just before the mouse down event and the new value is from the mouse up event
 *
 * - perhaps horzZoomFactor should be sample_fpos_t     I don't know how big it would get if 18tb where loaded... much bigger than a double I would imagine
 *
 * - ??? I dunno if I want to respond to mouse wheel events while dragging a select position 
 *
 * - Make ctrl-left-click center the positioned clicked
 *
 * - I need to load a sound that is a square wave with complete 32767 to -32767 range to verify that it renders properly AND that the scroll bars can fully extend to the length and not further
 *
 * - need to finish rendering and jumping to cues... always have two cues, start and stop
 *
 * - draw horz dB lines that scale with the vert zoom factor
 *
 * - I thought there was a problem with the way select stop is drawn, but now I don't think there is
 *   	- example: load a sound of say length 5.  Well, when start = 0 and stop = 4 (fully selected)
 *   	 	   it doesn't draw as if the 5th sample is selected, but that's because, when start is 0
 *   	 	   it draw from X=0 and so when stop is 0 it too draws from X=0... hence when position 4
 *   	 	   or the 5th sample is selected, it does fill past the beginning of the selected sample
 *   	 	   - perhaps a floor on the start calc and a ceil on the stop calc would fix this.. but
 *   	 	     right now, I'm not going to worry about it
 *
 * - I need to somewhere print the time that the cursor is over, and clear it onExit of the canvas
 *
 * - I have gone with fox's method of sending messages to the CSoundWindow that owns us to let it know when the start or stop position has changed and it should be sufficient for now, but
 *   	- I could use a TTrigger (but change it to support multiple registrations) and have the CSoundPlayerChannel tell us when to update the start or stop position which would for sure always be up to date
 *   	- Play position could work the same way
 *   	- Do I want to go with this way of doing it an perhaps simplify the job of making sure the status panel is up to date?
 *
 *   	- Minor: when I unshade the CSoundWindow I get at least 3 full redraws of the wave form
 *
 */



// --- declaration of FXWaveScrollArea -----------------------------------------------

class FXWaveScrollArea : public FXScrollArea
{
	FXDECLARE(FXWaveScrollArea)
public:
	FXWaveScrollArea(FXComposite *p,FXRezWaveView *parent,CLoadedSound *_loadedSound);
	virtual ~FXWaveScrollArea();

	sample_pos_t getSamplePosForScreenX(FXint X);


	void setHorzZoomFactor(double v,FXint keyboardState);
	double getHorzZoomFactor();
	double getMaxHorzZoomFactor();

	void setVertZoomFactor(double v);
	double getMaxVertZoomFactor();

	void drawPlayPosition(sample_pos_t dataPosition,bool justErasing,bool scrollToMakeVisible);

	void redraw();

	FXuint getSupposedCanvasWidth();
	FXuint getSupposedCanvasHeight();

	void updateWaveViewFromSelectChange(FXRezWaveView::LastChangedPosition _lastChangedPosition=FXRezWaveView::lcpNone);

	void updateFromEdit();

	// events I overload 

	virtual FXint getContentWidth();

	virtual FXint getContentHeight();

	virtual void moveContents(FXint x,FXint y);

	virtual void layout();

	// events I get by message

	long onPaint(FXObject *object,FXSelector sel,void *ptr);
	bool first;

	long onMouseDown(FXObject *object,FXSelector sel,void *ptr);

	long onMouseUp(FXObject *object,FXSelector sel,void *ptr);

	void handleMouseMoveSelectChange(FXint X);

	long onMouseMove(FXObject *object,FXSelector sel,void *ptr);

	long onAutoScroll(FXObject *object,FXSelector sel,void *ptr);

	enum
	{
		ID_CANVAS=FXScrollArea::ID_LAST,
		ID_LAST
	};


protected:
	FXWaveScrollArea() {}

private:

	friend class FXWaveRuler;  // so it can read pos_x and horzZoomFactor
	friend class FXRezWaveView;

	void drawPortion(int left,int width,FXDCWindow *dc);

	sample_pos_t snapPositionToCue(sample_pos_t p);
	void setSelectStartFromScreen(FXint X);
	void setSelectStopFromScreen(FXint X);

	FXRezWaveView *parent;

	FXCanvas *canvas;
	int skipDraw;

	CLoadedSound *loadedSound;
	CSound *sound;

	bool draggingSelectStart,draggingSelectStop;
	FXint prevDrawPlayStatusX;


	// these screen positions are sample_pos_t because horzZoomFactor could be 1
	const sample_fpos_t getRenderedDrawSelectStart() const	{ return sample_fpos_round((sample_fpos_t)renderedStartPosition/horzZoomFactor+(sample_fpos_t)pos_x); }
	const sample_fpos_t getRenderedDrawSelectStop() const	{ return sample_fpos_round((sample_fpos_t)renderedStopPosition/horzZoomFactor+(sample_fpos_t)pos_x); }
	const sample_fpos_t getDrawSelectStart() const		{ return sample_fpos_round((sample_fpos_t)loadedSound->channel->getStartPosition()/horzZoomFactor+(sample_fpos_t)pos_x); }
	const sample_fpos_t getDrawSelectStop() const		{ return sample_fpos_round((sample_fpos_t)loadedSound->channel->getStopPosition()/horzZoomFactor+(sample_fpos_t)pos_x); }
								
	FXRezWaveView::LastChangedPosition lastChangedPosition;

	sample_pos_t renderedStartPosition,renderedStopPosition;

	double horzZoomFactor;
	double vertZoomFactor;

	UnitTypes unitType;
	//void setUnitType(UnitTypes v);

	ViewTypes viewType;
	//void setViewType(ViewTypes v);
};

// --- declaration of FXWaveRuler -----------------------------------------------

class FXWaveRuler : public FXComposite
{
	FXDECLARE(FXWaveRuler)
public:
	FXWaveRuler(FXComposite *p,FXRezWaveView *parent,CLoadedSound *_loadedSound);
	virtual ~FXWaveRuler();

	virtual void create();

	// events I get by message

	long onPaint(FXObject *object,FXSelector sel,void *ptr);

	long onPopupMenu(FXObject *object,FXSelector sel,void *ptr);

	long onFindStartPosition(FXObject *object,FXSelector sel,void *ptr);
	long onFindStopPosition(FXObject *object,FXSelector sel,void *ptr);

	long onSetPositionToCue(FXObject *object,FXSelector sel,void *ptr);
	long onAddCue(FXObject *object,FXSelector sel,void *ptr);
	long onRemoveCue(FXObject *object,FXSelector sel,void *ptr);
	long onEditCue(FXObject *object,FXSelector sel,void *ptr);
	long onShowCueList(FXObject *object,FXSelector sel,void *ptr);

	enum
	{
		ID_FIND_START_POSITION=FXComposite::ID_LAST,
		ID_FIND_STOP_POSITION,

		ID_SET_START_POSITION,
		ID_SET_STOP_POSITION,

		ID_ADD_CUE,
		ID_ADD_CUE_AT_START_POSITION,
		ID_ADD_CUE_AT_STOP_POSITION,
		ID_REMOVE_CUE,
		ID_EDIT_CUE,

		ID_SHOW_CUE_LIST,

		ID_POPUP_MENU,

		ID_LAST
	};

protected:
	FXWaveRuler() {}

private:
	FXRezWaveView *parent;

	CLoadedSound *loadedSound;
	CSound *sound;

	FXFont *font;

	size_t cueClicked; // the index of the cue clicked on holding the value between the click event and the menu item event
	sample_pos_t addCueTime; // the time in the audio where the mouse was clicked to add a cue if that's what they choose

	FXint getCueScreenX(size_t cueIndex) const;
	sample_pos_t getCueTimeFromX(FXint screenX) const;

};





// --- FXRezWaveView ---------------------------------------------------------

FXDEFMAP(FXRezWaveView) FXRezWaveViewMap[]=
{
	//Message_Type					ID				Message_Handler
	//FXMAPFUNC(SEL_TIMEOUT,			FXWindow::ID_AUTOSCROLL,	FXRezWaveView::onAutoScroll),
};

FXIMPLEMENT(FXRezWaveView,FXPacker,FXRezWaveViewMap,ARRAYNUMBER(FXRezWaveViewMap))

FXRezWaveView::FXRezWaveView(FXComposite* p,CLoadedSound *_loadedSound) :
	FXPacker(p,FRAME_NONE|LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 0,0),

	rulerPanel(new FXWaveRuler(this,this,_loadedSound/*,FRAME_NONE | LAYOUT_FILL_X | LAYOUT_FIX_HEIGHT, 0,0,0,15*/)),
	waveScrollArea(new FXWaveScrollArea(this,this,_loadedSound))
{
	enable();
}

FXRezWaveView::~FXRezWaveView()
{
}

void FXRezWaveView::setHorzZoomFactor(double v,FXint keyboardState)
{
	waveScrollArea->setHorzZoomFactor(v,keyboardState);
}

FXint FXRezWaveView::getCanvasWidth()
{
	return(waveScrollArea->getSupposedCanvasWidth());
}

double FXRezWaveView::getHorzZoomFactor()
{
	return(waveScrollArea->getHorzZoomFactor());
}

double FXRezWaveView::getMaxHorzZoomFactor()
{
	return(waveScrollArea->getMaxHorzZoomFactor());
}

void FXRezWaveView::setVertZoomFactor(double v)
{
	waveScrollArea->setVertZoomFactor(v);
}

double FXRezWaveView::getMaxVertZoomFactor()
{
	return(waveScrollArea->getMaxVertZoomFactor());
}

void FXRezWaveView::horzScroll(FXint x)
{
	waveScrollArea->setPosition(0,waveScrollArea->pos_y);
}

void FXRezWaveView::drawPlayPosition(sample_pos_t dataPosition,bool justErasing,bool scrollToMakeVisible)
{
	waveScrollArea->drawPlayPosition(dataPosition,justErasing,scrollToMakeVisible);
}

void FXRezWaveView::redraw()
{
	waveScrollArea->redraw();
}

void FXRezWaveView::centerStartPos()
{
	waveScrollArea->setPosition((FXint)-sample_fpos_round((sample_fpos_t)waveScrollArea->loadedSound->channel->getStartPosition()/(sample_fpos_t)waveScrollArea->horzZoomFactor) + waveScrollArea->getSupposedCanvasWidth()/2 ,waveScrollArea->pos_y);
	waveScrollArea->canvas->update();
}

void FXRezWaveView::centerStopPos()
{
	waveScrollArea->setPosition((FXint)-sample_fpos_round((sample_fpos_t)waveScrollArea->loadedSound->channel->getStopPosition()/(sample_fpos_t)waveScrollArea->horzZoomFactor) + waveScrollArea->getSupposedCanvasWidth()/2 ,waveScrollArea->pos_y);
	waveScrollArea->canvas->update();
}

void FXRezWaveView::updateFromSelectionChange(LastChangedPosition _lastChangedPosition)
{
	waveScrollArea->updateWaveViewFromSelectChange(_lastChangedPosition);
}

void FXRezWaveView::updateFromEdit()
{
	rulerPanel->update();
	waveScrollArea->updateFromEdit();
}


void FXRezWaveView::getWaveSize(int &top,int &height)
{
	top=waveScrollArea->getY();
	height=waveScrollArea->getHeight()-waveScrollArea->horizontalScrollBar()->getHeight();
}

// --- FXWaveScrollArea -------------------------------------------------------

FXDEFMAP(FXWaveScrollArea) FXWaveViewScrollAreaMap[]=
{
	//Message_Type				ID				Message_Handler
	FXMAPFUNC(SEL_PAINT,			FXWaveScrollArea::ID_CANVAS,	FXWaveScrollArea::onPaint),
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

FXWaveScrollArea::FXWaveScrollArea(FXComposite *p,FXRezWaveView *_parent,CLoadedSound *_loadedSound) :
	FXScrollArea(p,FRAME_RIDGE|LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0),

	first(true),

	parent(_parent),

	canvas(new FXCanvas(this,this,ID_CANVAS,FRAME_NONE, 0,0)),
	skipDraw(0),

	loadedSound(_loadedSound),
	sound(_loadedSound->getSound()), // I don't expect CLoadedSound's sound object pointer to change.. but if it did, this would be bad.. and I'd need to just always all loadedSound->getSound()

	draggingSelectStart(false),draggingSelectStop(false),
	prevDrawPlayStatusX(0xffffffff),

	lastChangedPosition(FXRezWaveView::lcpNone),
	
	renderedStartPosition(0),renderedStopPosition(0),

	horzZoomFactor(1.0),vertZoomFactor(1.0),

	unitType(utSamples),
	viewType(vtNormal)
{
	enable();

	setScrollStyle(HSCROLLER_ALWAYS|VSCROLLER_ALWAYS);

	horizontalScrollBar()->setLine(25);
	verticalScrollBar()->setLine(25);

	// start out with something selected
	loadedSound->channel->setStartPosition(loadedSound->getSound()->getLength()/2-loadedSound->getSound()->getLength()/4);
	loadedSound->channel->setStopPosition(loadedSound->getSound()->getLength()/2+loadedSound->getSound()->getLength()/4);
	updateFromEdit();
	lastChangedPosition=FXRezWaveView::lcpStart;
}

FXWaveScrollArea::~FXWaveScrollArea()
{
}

sample_pos_t FXWaveScrollArea::getSamplePosForScreenX(FXint X)
{
	sample_fpos_t p=sample_fpos_floor(((sample_fpos_t)(X-pos_x))*horzZoomFactor);
	if(p<0)
		p=0.0;
	else if(p>=sound->getLength())
		p=sound->getLength()-1;

	return((sample_pos_t)p);
}


void FXWaveScrollArea::setHorzZoomFactor(double v,FXint keyboardState)
{
	if(v<0)
		v=1.0;
	else if(v>getMaxHorzZoomFactor())
		v=getMaxHorzZoomFactor();

	skipDraw++; // setPosition and/or layout causes a draw, so lets skip it

	sample_fpos_t startPositionScreenX=getDrawSelectStart();
	sample_fpos_t stopPositionScreenX=getDrawSelectStop();

	double old_horzZoomFactor=horzZoomFactor;

	horzZoomFactor=v;
	layout();

	const FXuint canvasWidth=getSupposedCanvasWidth();
	const bool startPositionIsOnScreen=(startPositionScreenX>=0 && startPositionScreenX<canvasWidth);
	const bool stopPositionIsOnScreen=(stopPositionScreenX>=0 && stopPositionScreenX<canvasWidth);

	#define ZOOM_ON_START_POS\
	{ \
		lastChangedPosition=FXRezWaveView::lcpStart; \
		setPosition(-(sample_pos_t)sample_fpos_round(((sample_fpos_t)loadedSound->channel->getStartPosition())/v-(sample_fpos_t)startPositionScreenX),pos_y); \
	}

	#define ZOOM_ON_STOP_POS \
	{ \
		lastChangedPosition=FXRezWaveView::lcpStop; \
		setPosition(-(sample_pos_t)sample_fpos_round(((sample_fpos_t)loadedSound->channel->getStopPosition())/v-(sample_fpos_t)stopPositionScreenX),pos_y); \
	}

	// if we're zooming because of a keyboard state, then make sure we zoom on a position that is on screen
	if(keyboardState&SHIFTMASK)
		startPositionScreenX=canvasWidth/2;
	if(keyboardState&CONTROLMASK)
		stopPositionScreenX=canvasWidth/2;

	if(keyboardState&SHIFTMASK)
		ZOOM_ON_START_POS
	else if(keyboardState&CONTROLMASK)
		ZOOM_ON_STOP_POS
	else if(lastChangedPosition==FXRezWaveView::lcpStart && startPositionIsOnScreen)
		ZOOM_ON_START_POS
	else if(lastChangedPosition==FXRezWaveView::lcpStop && stopPositionIsOnScreen)
		ZOOM_ON_STOP_POS
	else // zoom on left edge of screen
		setPosition(-(sample_pos_t)(sample_fpos_round(-((sample_fpos_t)pos_x)*old_horzZoomFactor/v)),pos_y); \

	skipDraw--;
}

double FXWaveScrollArea::getHorzZoomFactor()
{
	return(horzZoomFactor);
}

double FXWaveScrollArea::getMaxHorzZoomFactor()
{
	if(sound!=NULL)
		return((double)((sample_fpos_t)sound->getLength()/(sample_fpos_t)(getSupposedCanvasWidth()))); // divide by zero ???
	else
		return(1);
}

void FXWaveScrollArea::setVertZoomFactor(double v)
{
	int old_vOffset=(-pos_y)-((verticalScrollBar()->getRange()-getSupposedCanvasHeight())/2);
	vertZoomFactor=v;

	layout();

	// adjust the vertical scroll bar so it keeps the same thing on screen (just solved the vOffset calc equation for the position variable)
	skipDraw++; // set position causes a draw, so lets skip it
	setPosition(pos_x, -(old_vOffset+((verticalScrollBar()->getRange()-getSupposedCanvasHeight())/2))  );
	skipDraw--;
}

double FXWaveScrollArea::getMaxVertZoomFactor()
{
	/*
	if(sound!=NULL)
		return((((double)65536*(double)sound->getChannelCount())/(double)(getSupposedCanvasHeight()))-(2*sound->getChannelCount()));
	else
		return(1);
	*/

	return( (MAX_SAMPLE/((double)getSupposedCanvasHeight()/2.0)) * sound->getChannelCount());
}

void FXWaveScrollArea::drawPlayPosition(sample_pos_t dataPosition,bool justErasing,bool scrollToMakeVisible)
{
	FXint drawPlayStatusX;
	FXDCWindow dc(canvas);

	if(!justErasing)
	{
		// recalc new draw position
		drawPlayStatusX=(FXint)((sample_fpos_t)dataPosition/horzZoomFactor+pos_x);


						// don't follow play position if paused unless the user is shuttling
		if(scrollToMakeVisible && (!loadedSound->channel->isPaused() || loadedSound->channel->getPlaySpeed()!=1.0))
		{ // scroll the wave view to make the play position visible on the left most side of the screen

			// if the play position is off the screen
			if(getSupposedCanvasWidth()>25 && (drawPlayStatusX<0 || drawPlayStatusX>=(sample_fpos_t)getSupposedCanvasWidth()))
			{
				setPosition(-(sample_pos_t)sample_fpos_round((sample_fpos_t)dataPosition/horzZoomFactor-25),pos_y); \
				drawPlayStatusX=25;
			}
		}

	}

	// erase the old position (unless it's in the same place
	if(prevDrawPlayStatusX!=(signed)0xffffffff && (justErasing || prevDrawPlayStatusX!=drawPlayStatusX))
	{
		drawPortion(prevDrawPlayStatusX,1,&dc);
		prevDrawPlayStatusX=0xffffffff;
	}

	if(!justErasing)
	{ // draw the new position
		if(drawPlayStatusX>=0 && drawPlayStatusX<(FXint)getSupposedCanvasWidth())
		{
			//dc.setFunction(BLT_SRC_XOR_DST); // xor the play position on so it doesn't cover up waveform
			dc.setForeground(playStatusColor);
			dc.drawLine(drawPlayStatusX,0,drawPlayStatusX,getSupposedCanvasHeight()-1);
			prevDrawPlayStatusX=drawPlayStatusX;
		}
	}
}

void FXWaveScrollArea::redraw()
{
	canvas->update();
	parent->rulerPanel->update();
}

FXint FXWaveScrollArea::getContentWidth()
{
	return(sound==NULL ? 1 : (FXint)((sample_fpos_t)sound->getLength()/horzZoomFactor));
}

FXint FXWaveScrollArea::getContentHeight()
{
	/*
						// ??? is this due to 16bit values... or just a suffenciently large number to be useful
	return(sound==NULL ? 1 : (FXint)((double)65536*(double)sound->getChannelCount()/vertZoomFactor));
	*/

	// ??? MAX with getSupposedCanvasHeight()? 
	return((FXint)(((MAX_SAMPLE*2)/vertZoomFactor) * sound->getChannelCount()));
}

void FXWaveScrollArea::moveContents(FXint x,FXint y)
{
	const FXint oldX=getXPosition();
	const FXint oldY=getYPosition();
	const FXint newX=x;
	const FXint newY=y;
	FXScrollArea::moveContents(x,y);

	// I put this to avoid the jerky drawing when horz zooming calls setPosition and this gets called even though we're about to repain the whole screen
	if(skipDraw>0)
		return;

	if(oldY!=newY || abs(oldX-newX)>=(FXint)getSupposedCanvasWidth())
	{ // it scrolled verically or scrolled more than the canvas width
		redraw();
	}
	else
	{
		FXDCWindow dc(canvas);
		if(oldX>newX)
		{ // scrolling to the right
			// means we need to copy some of the canvas to the left then draw new stuff on the right side
			const FXint sx=oldX-newX;
			prevDrawPlayStatusX-=sx; // make sure the draw play status erases the from the scrolled position
			dc.drawArea(canvas,oldX-newX,0,getSupposedCanvasWidth()-sx,getSupposedCanvasHeight(),0,0);
			drawPortion(getSupposedCanvasWidth()-sx,sx,&dc);
			parent->rulerPanel->update();
		}
		else if(newX>oldX)
		{ // scrolling to the left
			// means we need to copy some of the canvas to the right then draw new stuff on the left side

			const FXint sx=newX-oldX;
			prevDrawPlayStatusX+=sx; // make sure the draw play status erases the from the scrolled position
			dc.drawArea(canvas,0,0,getSupposedCanvasWidth()-sx,getSupposedCanvasHeight(),sx,0);
			drawPortion(0,sx,&dc);
			parent->rulerPanel->update();
		}
	}
}

void FXWaveScrollArea::layout()
{
	FXScrollArea::layout();
	canvas->resize(getSupposedCanvasWidth(),getSupposedCanvasHeight());
}

long FXWaveScrollArea::onPaint(FXObject*,FXSelector,void *ptr)
{
	FXEvent *ev=(FXEvent*)ptr;
	FXDCWindow dc(canvas,ev);

	if(first)
	{ // draw black while data is drawing the first time
		dc.setForeground(backGroundColor);
		dc.setFillStyle(FILL_SOLID);
		dc.fillRectangle(ev->rect.x,ev->rect.y,ev->rect.w,ev->rect.h);
		first=false;
	}

	drawPortion(ev->rect.x,ev->rect.w,&dc);

	return 1;
}

long FXWaveScrollArea::onMouseDown(FXObject*,FXSelector,void *ptr)
{
	if(sound!=NULL && !sound->isEmpty())
	{
		FXEvent *ev=(FXEvent*) ptr;
		const FXint X=ev->click_x;

		if(!draggingSelectStop && !draggingSelectStart)
		{

			if(ev->click_button==LEFTBUTTON)
			{
				if(X<=(FXint)getDrawSelectStop())
				{
					draggingSelectStart=true;
					setSelectStartFromScreen(X);
				}
				else
				{
					draggingSelectStop=true;
					const sample_pos_t origSelectStop=loadedSound->channel->getStopPosition();
					setSelectStopFromScreen(X);
					loadedSound->channel->setStartPosition(origSelectStop);
				}
			}
			else if(ev->click_button==RIGHTBUTTON)
			{
				if(X>=(FXint)getDrawSelectStart())
				{
					draggingSelectStop=true;
					setSelectStopFromScreen(X);
				}
				else
				{
					draggingSelectStart=true;
					const sample_pos_t origSelectStart=loadedSound->channel->getStartPosition();
					setSelectStartFromScreen(X);
					loadedSound->channel->setStopPosition(origSelectStart);
				}
			}
		}
		updateWaveViewFromSelectChange();

	}

	return(1); // QQQ
	//return(0);
}

long FXWaveScrollArea::onMouseUp(FXObject*,FXSelector,void *ptr)
{
	if(sound!=NULL && !sound->isEmpty())
	{
		FXEvent *ev=(FXEvent*) ptr;

		if((ev->click_button==LEFTBUTTON || ev->click_button==RIGHTBUTTON) && (draggingSelectStart || draggingSelectStop))
		{
			stopAutoScroll();
			draggingSelectStart=draggingSelectStop=false;
		}
	}
	return(1); // QQQ
	//return(0);
}

void FXWaveScrollArea::handleMouseMoveSelectChange(FXint X)
{
	// ??? I don't think should should cause any problems... 
	// I put it here so that when shift is held to stop the autoscrolling that it doesn't select anything off screen even if the mouse is outside the wave view
	X=max((int)0,min((int)X,(int)(getSupposedCanvasWidth()-1)));

	if(draggingSelectStart)
	{
		if(X>(FXint)getDrawSelectStop())
		{
			draggingSelectStart=false;
			draggingSelectStop=true;
			const sample_pos_t origSelectStop=loadedSound->channel->getStopPosition();
			setSelectStopFromScreen(X);
			loadedSound->channel->setStartPosition(origSelectStop);
		}
		else
			setSelectStartFromScreen(X);

		updateWaveViewFromSelectChange();
	}
	else if(draggingSelectStop)
	{
		if(X<=(FXint)getDrawSelectStart())
		{
			draggingSelectStop=false;
			draggingSelectStart=true;
			const sample_pos_t origSelectStart=loadedSound->channel->getStartPosition();
			setSelectStartFromScreen(X);
			loadedSound->channel->setStopPosition(origSelectStart);
		}
		else
			setSelectStopFromScreen(X);

		updateWaveViewFromSelectChange();
	}
}

long FXWaveScrollArea::onMouseMove(FXObject*,FXSelector,void *ptr)
{
	if(sound!=NULL && !sound->isEmpty())
	{
		FXEvent *ev=(FXEvent*) ptr;

		// Here we detect if the mouse is against the side walls and scroll that way if the mouse is down
		if(draggingSelectStart || draggingSelectStop)
		{
			if(!(ev->state&SHIFTMASK))
			{
				if(startAutoScroll(ev->win_x,ev->win_y))
					return(1); // QQQ
					//return(0);
			}
		}

		handleMouseMoveSelectChange(ev->win_x);
	}
	return(1); // QQQ
	//return(0);
}

long FXWaveScrollArea::onAutoScroll(FXObject *object,FXSelector sel,void *ptr)
{
	long ret=FXScrollArea::onAutoScroll(object,sel,ptr);
	handleMouseMoveSelectChange(((FXEvent *)ptr)->win_x);
	return(ret);
}

FXuint FXWaveScrollArea::getSupposedCanvasWidth()
{
	// ??? if I allow the scrollbars to disappear I don't need to subtract it's size
	return getWidth()-verticalScrollBar()->getWidth();
}

FXuint FXWaveScrollArea::getSupposedCanvasHeight()
{
	// ??? if I allow the scrollbars to disappear I don't need to subtract it's size
	return getHeight()-(horizontalScrollBar()->getHeight());
}

void FXWaveScrollArea::drawPortion(int left,int width,FXDCWindow *dc)
{
	// certain code sets skipDraw incase it wants to avoid a redraw when it knows another is about to happen
	if(skipDraw>0)
		return;

	if(!shown())
		return;

	sound->lockSize();
	try
	{
		const int vOffset=pos_y+((verticalScrollBar()->getRange()-getSupposedCanvasHeight())/2);
		::drawPortion(left,width,dc,sound,getSupposedCanvasWidth(),getSupposedCanvasHeight(),(int)getDrawSelectStart(),(int)getDrawSelectStop(),horzZoomFactor,-pos_x,vertZoomFactor,vOffset);
		sound->unlockSize();
	}
	catch(...)
	{
		sound->unlockSize();
		throw;
	}
}

sample_pos_t FXWaveScrollArea::snapPositionToCue(sample_pos_t p)
{
	const sample_pos_t snapToCueDistance=(sample_pos_t)(gSnapToCueDistance*horzZoomFactor+0.5); // ??? make 5 a global setting
	if(gSnapToCues)
	{
		FXint dummy;
		FXuint keyboardModifierState;
		getCursorPosition(dummy,dummy,keyboardModifierState);

		if(! (keyboardModifierState&SHIFTMASK))
		{ // and shift isn't being held down
			size_t cueIndex;
			sample_pos_t distance;

			const bool found=loadedSound->getSound()->findNearestCue(p,cueIndex,distance);
			if(found && distance<=snapToCueDistance)
				p=loadedSound->getSound()->getCueTime(cueIndex);
		}
	}

	return(p);
}

void FXWaveScrollArea::setSelectStartFromScreen(FXint X)
{
	if(sound==NULL || sound->isEmpty())
		return;

	sample_pos_t newSelectStart=snapPositionToCue(getSamplePosForScreenX(X));

	// if we selected the right-most pixel on the screen
	// and--if there were a next pixel--and selecting that
	// would select >= sound's length, we make newSelectStart
	// be the sound's length - 1
	// 							??? why X+1?			??? may beed to check >=len-1
	if(X>=((FXint)getSupposedCanvasWidth()-1) && (sample_pos_t)((sample_fpos_t)(X-pos_x+1)*horzZoomFactor)>=sound->getLength())
		newSelectStart=sound->getLength()-1;


	if(newSelectStart<0)
		newSelectStart=0;
	else if(newSelectStart>=sound->getLength())
		newSelectStart=sound->getLength()-1;

	//if(FOnSelectStartChange!=NULL)
		//FOnSelectStartChange(this);
	
	lastChangedPosition=FXRezWaveView::lcpStart;
	loadedSound->channel->setStartPosition(newSelectStart);
}

void FXWaveScrollArea::setSelectStopFromScreen(FXint X)
{
	if(sound==NULL || sound->isEmpty())
		return;

	sample_pos_t newSelectStop=snapPositionToCue(getSamplePosForScreenX(X));

	// if we selected the right-most pixel on the screen
	// and--if there were a next pixel--and selecting that
	// would select >= sound's length, we make newSelectStop
	// be the sound's length - 1
	// 											??? may beed to check >=len-1
	if(X>=((FXint)getSupposedCanvasWidth()-1) && (sample_pos_t)((sample_fpos_t)(X-pos_x+1)*horzZoomFactor)>=sound->getLength())
		newSelectStop=sound->getLength()-1;

	if(newSelectStop<0)
		newSelectStop=0;
	else if(newSelectStop>=sound->getLength())
		newSelectStop=sound->getLength()-1;

	//if(FOnSelectStopChange!=NULL)
		//FOnSelectStopChange(this);
		
	lastChangedPosition=FXRezWaveView::lcpStop;
	loadedSound->channel->setStopPosition(newSelectStop);
}

void FXWaveScrollArea::updateWaveViewFromSelectChange(FXRezWaveView::LastChangedPosition _lastChangedPosition)
{
	if(_lastChangedPosition!=FXRezWaveView::lcpNone)
		lastChangedPosition=_lastChangedPosition;

	if(sound!=NULL)
	{
		//sound->lockSize(); ??? I need to, but can't lock here because drawPortion is going to lock
		try
		{
			// I changed it to left-1 and width+1 because the on autoscroll event on around 40-80% horz zoom levels it was skipping a pixel when drawing the selection background sometimes

			const sample_fpos_t oldDrawSelectStart=getRenderedDrawSelectStart();
			const sample_fpos_t drawSelectStart=getDrawSelectStart();
			const sample_fpos_t oldDrawSelectStop=getRenderedDrawSelectStop();
			const sample_fpos_t drawSelectStop=getDrawSelectStop();

			renderedStartPosition=loadedSound->channel->getStartPosition();
			renderedStopPosition=loadedSound->channel->getStopPosition();

			if(oldDrawSelectStart!=drawSelectStart)
			{
				FXDCWindow dc(canvas);
				if(drawSelectStart>oldDrawSelectStart)
					//drawPortion(oldDrawSelectStart-1,(drawSelectStart-oldDrawSelectStart)+1,&dc); // shortening
					drawPortion((int)oldDrawSelectStart,(int)(drawSelectStart-oldDrawSelectStart),&dc); // shortening
				else
					//drawPortion(drawSelectStart-1,(oldDrawSelectStart-drawSelectStart)+1,&dc); // lengthening
					drawPortion((int)drawSelectStart,(int)(oldDrawSelectStart-drawSelectStart),&dc); // lengthening
			}
			if(oldDrawSelectStop!=drawSelectStop)
			{
				FXDCWindow dc(canvas);
				if(drawSelectStop>oldDrawSelectStop)
					//drawPortion(oldDrawSelectStop+1-1,(drawSelectStop-oldDrawSelectStop)+1,&dc); // lengthening
					drawPortion((int)oldDrawSelectStop+1,(int)(drawSelectStop-oldDrawSelectStop),&dc); // lengthening
				else
					//drawPortion(drawSelectStop+1-1,(oldDrawSelectStop-drawSelectStop)+1,&dc); // shortening
					drawPortion((int)drawSelectStop+1,(int)(oldDrawSelectStop-drawSelectStop),&dc); // shortening
			}
			//sound->unlockSize();
		}
		catch(...)
		{
			//sound->unlockSize();
			throw;
		}
	}

	if(parent->target)
		parent->target->handle(parent,MKUINT(parent->message,FXRezWaveView::SEL_SELECTION_CHANGED),NULL);
}

void FXWaveScrollArea::updateFromEdit()
{
	// reinitialize these since I redraw the whole thing anyway
	renderedStartPosition=loadedSound->channel->getStartPosition();
	renderedStopPosition=loadedSound->channel->getStopPosition();

	update();
	canvas->update();
}







// --- FXWaveRuler -------------------------------------------------------

FXDEFMAP(FXWaveRuler) FXWaveRulerMap[]=
{
	//Message_Type				ID						Message_Handler
	FXMAPFUNC(SEL_RIGHTBUTTONPRESS,		0,						FXWaveRuler::onPopupMenu),
	
	FXMAPFUNC(SEL_PAINT,			0,						FXWaveRuler::onPaint),

	FXMAPFUNC(SEL_COMMAND,			FXWaveRuler::ID_FIND_START_POSITION,		FXWaveRuler::onFindStartPosition),
	FXMAPFUNC(SEL_COMMAND,			FXWaveRuler::ID_FIND_STOP_POSITION,		FXWaveRuler::onFindStopPosition),

	FXMAPFUNC(SEL_COMMAND,			FXWaveRuler::ID_SET_START_POSITION,		FXWaveRuler::onSetPositionToCue),
	FXMAPFUNC(SEL_COMMAND,			FXWaveRuler::ID_SET_STOP_POSITION,		FXWaveRuler::onSetPositionToCue),

	FXMAPFUNC(SEL_COMMAND,			FXWaveRuler::ID_ADD_CUE,			FXWaveRuler::onAddCue),
	FXMAPFUNC(SEL_COMMAND,			FXWaveRuler::ID_ADD_CUE_AT_START_POSITION,	FXWaveRuler::onAddCue),
	FXMAPFUNC(SEL_COMMAND,			FXWaveRuler::ID_ADD_CUE_AT_STOP_POSITION,	FXWaveRuler::onAddCue),
	FXMAPFUNC(SEL_COMMAND,			FXWaveRuler::ID_REMOVE_CUE,			FXWaveRuler::onRemoveCue),
	FXMAPFUNC(SEL_COMMAND,			FXWaveRuler::ID_EDIT_CUE,			FXWaveRuler::onEditCue),

	FXMAPFUNC(SEL_COMMAND,			FXWaveRuler::ID_SHOW_CUE_LIST,			FXWaveRuler::onShowCueList),
	// I wish this worked... change fox FXMAPFUNC(SEL_DOUBLECLICKED,		FXWaveRuler::ID_SHOW_CUE_LIST,			FXWaveRuler::onShowCueList),
};

FXIMPLEMENT(FXWaveRuler,FXComposite,FXWaveRulerMap,ARRAYNUMBER(FXWaveRulerMap))


FXWaveRuler::FXWaveRuler(FXComposite *p,FXRezWaveView *_parent,CLoadedSound *_loadedSound) :
	FXComposite(p,FRAME_NONE | LAYOUT_FILL_X | LAYOUT_FIX_HEIGHT, 0,0,0,13+8),

	parent(_parent),

	loadedSound(_loadedSound),
	sound(_loadedSound->getSound()),

	font(getApp()->getNormalFont())
{
	enable();
	flags|=FLAG_SHOWN; // I have to do this, or it will not show up.. like height is 0 or something

	FXFontDesc d;
	font->getFontDesc(d);
	d.weight=FONTWEIGHT_LIGHT;
	d.size=65;
	font=new FXFont(getApp(),d);
}

FXWaveRuler::~FXWaveRuler()
{
	delete font;
}

void FXWaveRuler::create()
{
	FXComposite::create();
	font->create();
}

#define CUE_RADIUS 3
#define CUE_Y ((height-1)-(1+CUE_RADIUS))
#define CUE_OFF_SCREEN (-(CUE_RADIUS+1000))

long FXWaveRuler::onPaint(FXObject *object,FXSelector sel,void *ptr)
{
	FXComposite::onPaint(object,sel,ptr);

	FXEvent *ev=(FXEvent*)ptr;
	FXDCWindow dc(this,ev);

	#define LABEL_TICK_FREQ 80
	/*
	 * Here, we draw the ruler
	 * 	We want to always put 1 labled position every LABELED_TICK_FREQ pixels 
	 * 	And draw ruler ticks also
	 */

	FXint lastX=parent->waveScrollArea->getSupposedCanvasWidth();

	const FXint left=0;
	const FXint right=left+lastX;

	FXint s=ev->rect.x;
	s-=LABEL_TICK_FREQ-1;

	FXint e=s+ev->rect.w;
	e+=LABEL_TICK_FREQ-1;

	if(e>lastX)
		e=lastX;

	dc.setForeground(FXRGB(20,20,20));
	dc.setTextFont(font);
	for(FXint x=s;x<=e;x++)
	{
		if((x%LABEL_TICK_FREQ)==0)
		{
			dc.drawLine(x,0,x,10);

			const string time=sound->getTimePosition(parent->waveScrollArea->getSamplePosForScreenX(x));

			dc.drawText(x+2,12,time.c_str(),time.length());
		}
		else if(((x+(LABEL_TICK_FREQ/2))%(LABEL_TICK_FREQ))==0)
			dc.drawLine(x,0,x,4); // half way tick between labled ticks
		else if((x%(LABEL_TICK_FREQ/10))==0)
			dc.drawLine(x,0,x,2); // small tenth ticks
	}

	// render cues
	//dc.setFillStyle(FILL_STIPPLED);
	//dc.setStipple(STIPPLE_GRAY);
	const size_t cueCount=sound->getCueCount();
	for(size_t t=0;t<cueCount;t++)
	{
		// ??? I could figure out the min and max screen viable time and make sure that is in range before testing every cue
		FXint cueXPosition=getCueScreenX(t);

		if(cueXPosition!=CUE_OFF_SCREEN)
		{
			if(sound->isCueAnchored(t))
				dc.setForeground(FXRGB(0,0,255));
			else
				dc.setForeground(FXRGB(255,0,0));
		
			dc.drawLine(cueXPosition,height-1,cueXPosition,CUE_Y);
			dc.setForeground(FXRGB(0,0,0));
			dc.drawArc(cueXPosition-CUE_RADIUS,CUE_Y-CUE_RADIUS,CUE_RADIUS*2,CUE_RADIUS*2,0*64,360*64);
		
			const string cueName=sound->getCueName(t);
			dc.drawText(cueXPosition+CUE_RADIUS+1,height-1,cueName.data(),cueName.size());
		}
	}

	return(1); // QQQ
	//return(0);
}

long FXWaveRuler::onPopupMenu(FXObject *object,FXSelector sel,void *ptr)
{
	if(!underCursor())
		return(1);

	// ??? if the parent->target is NULL, I should pop up the windows

	FXEvent *event=(FXEvent*)ptr;

	// see if any cue was clicked on

	const size_t cueCount=sound->getCueCount();
	cueClicked=cueCount;
	if(cueCount>0)
	{
		// go in reverse order so that the more recent one created can be removed first
		// I mean, if you accidently create one, then want to remove it, and it's on top
		// of another one then it needs to find that more recent one first
		for(size_t t=cueCount;t>=1;t--)
		{
			FXint X=getCueScreenX(t-1);
			if(X!=CUE_OFF_SCREEN)
			{
				FXint Y=CUE_Y;

				// check distance from clicked point to the cue's position
				if( ((event->win_x-X)*(event->win_x-X))+((event->win_y-Y)*(event->win_y-Y)) <= (CUE_RADIUS*CUE_RADIUS) )
				{
					cueClicked=t-1;
					break;
				}
			}
		}
	}

	if(cueClicked<cueCount)
	{
		FXMenuPane cueMenu(this);
			// ??? make sure that these get deleted when cueMenu is deleted

			const sample_fpos_t cueTime=sound->getCueTime(cueClicked);

			if(cueTime<loadedSound->channel->getStartPosition())
				new FXMenuCommand(&cueMenu,"Set Start Position to Cue",NULL,this,ID_SET_START_POSITION);
			else if(cueTime>loadedSound->channel->getStopPosition())
				new FXMenuCommand(&cueMenu,"Set Stop Position to Cue",NULL,this,ID_SET_STOP_POSITION);
			else
			{
				new FXMenuCommand(&cueMenu,"Set Start Position to Cue",NULL,this,ID_SET_START_POSITION);
				new FXMenuCommand(&cueMenu,"Set Stop Position to Cue",NULL,this,ID_SET_STOP_POSITION);
			}
		
			new FXMenuSeparator(&cueMenu);
			new FXMenuCommand(&cueMenu,"&Edit Cue",NULL,this,ID_EDIT_CUE);
			new FXMenuCommand(&cueMenu,"&Remove Cue",NULL,this,ID_REMOVE_CUE);
			new FXMenuSeparator(&cueMenu);
			new FXMenuCommand(&cueMenu,"Show Cues &List",NULL,this,ID_SHOW_CUE_LIST);

		cueMenu.create();
		cueMenu.popup(NULL,event->root_x,event->root_y);
		getApp()->runModalWhileShown(&cueMenu);
	}
	else
	{
		addCueTime=  getCueTimeFromX(event->win_x);

		FXMenuPane gotoMenu(this);
			// ??? make sure that these get deleted when gotoMenu is deleted
			new FXMenuCommand(&gotoMenu,"Center Start Position",NULL,this,ID_FIND_START_POSITION);
			new FXMenuCommand(&gotoMenu,"Center Stop Position",NULL,this,ID_FIND_STOP_POSITION);
			new FXMenuSeparator(&gotoMenu);
			new FXMenuCommand(&gotoMenu,"Add Cue at This Position",NULL,this,ID_ADD_CUE);
			new FXMenuCommand(&gotoMenu,"Add Cue at Start Position",NULL,this,ID_ADD_CUE_AT_START_POSITION);
			new FXMenuCommand(&gotoMenu,"Add Cue at Stop Position",NULL,this,ID_ADD_CUE_AT_STOP_POSITION);
			new FXMenuSeparator(&gotoMenu);
			new FXMenuCommand(&gotoMenu,"Show Cues &List",NULL,this,ID_SHOW_CUE_LIST);

		gotoMenu.create();
		gotoMenu.popup(NULL,event->root_x,event->root_y);
		getApp()->runModalWhileShown(&gotoMenu);
	}


	return(0);
}

long FXWaveRuler::onFindStartPosition(FXObject *object,FXSelector sel,void *ptr)
{ 
	FXWaveScrollArea *wsa=parent->waveScrollArea;
	wsa->parent->centerStartPos();
	return 1;
}

long FXWaveRuler::onFindStopPosition(FXObject *object,FXSelector sel,void *ptr)
{
	FXWaveScrollArea *wsa=parent->waveScrollArea;
	wsa->parent->centerStopPos();
	return 1;
}

long FXWaveRuler::onSetPositionToCue(FXObject *object,FXSelector sel,void *ptr)
{
	switch(SELID(sel))
	{
	case ID_SET_START_POSITION:
		loadedSound->channel->setStartPosition((sample_pos_t)sound->getCueTime(cueClicked));
		break;

	case ID_SET_STOP_POSITION:
		loadedSound->channel->setStopPosition((sample_pos_t)sound->getCueTime(cueClicked));
		break;
	}

	parent->updateFromSelectionChange();

	return 1;
}

long FXWaveRuler::onAddCue(FXObject *object,FXSelector sel,void *ptr)
{
	switch(SELID(sel))
	{
	case ID_ADD_CUE:
		if(parent->target)
			parent->target->handle(parent,MKUINT(parent->message,FXRezWaveView::SEL_ADD_CUE),&addCueTime);
		break;
	case ID_ADD_CUE_AT_START_POSITION:
		if(parent->target)
			parent->target->handle(parent,MKUINT(parent->message,FXRezWaveView::SEL_ADD_CUE_AT_START_POSITION),NULL);
		break;
	case ID_ADD_CUE_AT_STOP_POSITION:
		if(parent->target)
			parent->target->handle(parent,MKUINT(parent->message,FXRezWaveView::SEL_ADD_CUE_AT_STOP_POSITION),NULL);
		break;
	}
	update();

	return 1;
}

long FXWaveRuler::onRemoveCue(FXObject *object,FXSelector sel,void *ptr)
{
	if(parent->target)
		parent->target->handle(parent,MKUINT(parent->message,FXRezWaveView::SEL_REMOVE_CUE),&cueClicked);
	update();
	return 1;
}

long FXWaveRuler::onEditCue(FXObject *object,FXSelector sel,void *ptr)
{
	if(parent->target)
		parent->target->handle(parent,MKUINT(parent->message,FXRezWaveView::SEL_EDIT_CUE),&cueClicked);
	update();
	return 1;
}

long FXWaveRuler::onShowCueList(FXObject *object,FXSelector sel,void *ptr)
{
	if(parent->target)
		parent->target->handle(parent,MKUINT(parent->message,FXRezWaveView::SEL_SHOW_CUE_LIST),NULL);
	update();
	return 1;
}

FXint FXWaveRuler::getCueScreenX(size_t cueIndex) const
{
	sample_fpos_t X=((sample_fpos_t)sound->getCueTime(cueIndex)/parent->waveScrollArea->horzZoomFactor+parent->waveScrollArea->pos_x);
	if(X>=(-CUE_RADIUS-1) && X<(width+CUE_RADIUS+1))
		return((FXint)sample_fpos_round(X));
	else
		return(CUE_OFF_SCREEN);

}

sample_pos_t FXWaveRuler::getCueTimeFromX(FXint screenX) const
{
	return((sample_pos_t)(((sample_fpos_t)screenX-(sample_fpos_t)parent->waveScrollArea->pos_x)*parent->waveScrollArea->horzZoomFactor));
}

