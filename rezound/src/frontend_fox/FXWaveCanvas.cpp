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



/* TODO:
 *
 * perhaps this widget should derive from a base class so that other derivations could be used to have other views of the audio data
 * 	this is rather than implementing this class to have multiple views... although the only thing that might 
 * 	be different is the draw function I call.. and perhaps what part of the screen to update since the waveform 
 * 	view always draws the whole vertical part and cannot blit when scrolling vertically, but an FFT vew might be 
 * 	able to use blit on vertical scrolling
 *
 * - I need to load a sound that is a square wave with complete 32767 to -32767 range to verify that it 
 *   renders properly AND that the scroll bars can fully extend to the length and not further
 *
 * - I thought there was a problem with the way select stop is drawn, but now I don't think there is
 *   	- example: load a sound of say length 5.  Well, when start = 0 and stop = 4 (fully selected)
 *   	 	   it doesn't draw as if the 5th sample is selected, but that's because, when start is 0
 *   	 	   it draw from X=0 and so when stop is 0 it too draws from X=0... hence when position 4
 *   	 	   or the 5th sample is selected, it does fill past the beginning of the selected sample
 *   	 	   - perhaps a floor on the start calc and a ceil on the stop calc would fix this.. but
 *   	 	     right now, I'm not going to worry about it
 *
 * - test that all samples are actually visible when zoomed all the way in
 * 	- this does have a little bit of a problem, but I noted it in the docs/devel/TODO and I 
 * 	  have more important problems to fix.
 */


#include "FXWaveCanvas.h"

#include <algorithm>

#include "settings.h"

#include "../backend/CSound.h"
#include "../backend/CLoadedSound.h"
#include "../backend/CSoundPlayerChannel.h"
#include "drawPortion.h"

static FXColor playStatusColor=FXRGB(255,0,0);

#define RIGHT_MARGIN 10

FXDEFMAP(FXWaveCanvas) FXWaveCanvasMap[]=
{
	//Message_Type		ID	Message_Handler
	FXMAPFUNC(SEL_PAINT,	0,	FXWaveCanvas::onPaint),
};

FXIMPLEMENT(FXWaveCanvas,FXCanvas,FXWaveCanvasMap,ARRAYNUMBER(FXWaveCanvasMap))

FXWaveCanvas::FXWaveCanvas(CLoadedSound *_loadedSound,FXComposite *p,FXObject *tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h) :
	FXCanvas(p,tgt,sel,opts,x,y,w,h),
	first(true),
	loadedSound(_loadedSound),

	horzZoomFactor(1.0),
	vertZoomFactor(1.0),

	horzOffset(0),prevHorzZoomFactor_horzOffset(-1.0),
	vertOffset(0),

	prevDrawPlayStatusX(0xffffffff),
	renderedStartPosition(0),renderedStopPosition(0),

	lastHorzZoom(-1.0),lastVertZoom(-1.0),

	lastChangedPosition(lcpStart),
	lastDrawWasUnsuccessful(false)
{
}

FXWaveCanvas::~FXWaveCanvas()
{
}

void FXWaveCanvas::updateFromEdit()
{
	const double hZoom=getHorzZoom();
	lastHorzZoom=-1.0;
	setHorzZoom(hZoom,hrtLeftEdge);

	const double origVertZoomFactor=vertZoomFactor;
	const double vZoom=getVertZoom();
	lastVertZoom=-1.0;
	setVertZoom(vZoom);
	if(origVertZoomFactor!=vertZoomFactor) // if the vertical zoom factor had to change, then re-center the vertical offset
		setVertOffset(getVertSize()/2-getHeight()/2);

	update();
}

void FXWaveCanvas::setHorzZoom(double v,HorzRecenterTypes horzRecenterType)
{
	v=max(0.0,min(1.0,v)); // make v in range: [0,1]

	if(v!=lastHorzZoom)
	{
		const sample_fpos_t old_horzZoomFactor=horzZoomFactor;
		const bool startPosIsOnScreen=isStartPosOnScreen();
		const bool stopPosIsOnScreen=isStopPosOnScreen();
		const sample_fpos_t startPositionScreenX=getDrawSelectStart();
		const sample_fpos_t stopPositionScreenX=getDrawSelectStop();

		// how many samples are represented by one pixel when fully zoomed out (accounting for a small right side margin of unused space)
		const sample_fpos_t maxZoomFactor=(sample_fpos_t)loadedSound->sound->getLength()/(sample_fpos_t)max(1,(int)(getWidth()-RIGHT_MARGIN));

		// map v:0..1 --> horzZoomFactor:maxZoomFactor..1
		horzZoomFactor=maxZoomFactor+((1.0-maxZoomFactor)*v);

		// adjust the horz offset to try to keep something centered on screen while zooming
		sample_fpos_t tHorzOffset=horzOffset;
		switch(horzRecenterType)
		{
		case hrtNone:
			break;

		case hrtAuto:
			if(lastChangedPosition==lcpStart && startPosIsOnScreen)
				// start postion where it is
				tHorzOffset=loadedSound->channel->getStartPosition()/horzZoomFactor-startPositionScreenX;
			else if(lastChangedPosition==lcpStop && stopPosIsOnScreen)
				// stop postion where it is
				tHorzOffset=loadedSound->channel->getStopPosition()/horzZoomFactor-stopPositionScreenX;
			else if(startPosIsOnScreen)
				// start postion where it is
				tHorzOffset=loadedSound->channel->getStartPosition()/horzZoomFactor-startPositionScreenX;
			else if(stopPosIsOnScreen)
				// stop postion where it is
				tHorzOffset=loadedSound->channel->getStopPosition()/horzZoomFactor-stopPositionScreenX;
			else
				// leftEdge
				tHorzOffset=horzOffset/horzZoomFactor*old_horzZoomFactor;
			break;

		case hrtStart:
			tHorzOffset=getHorzOffsetToCenterStartPos();
			break;

		case hrtStop:
			tHorzOffset=getHorzOffsetToCenterStopPos();
			break;

		case hrtLeftEdge:
			tHorzOffset=(sample_pos_t)(sample_fpos_round(horzOffset/horzZoomFactor*old_horzZoomFactor));
			break;
		};
		// make sure it's in range and round
		horzOffset=(sample_pos_t)sample_fpos_round(max((sample_fpos_t)0.0,min((sample_fpos_t)getHorzSize(),tHorzOffset)));

		// if there are so few samples that less than one sample is represented by a pixel, then don't allow the offset because it causes problems
		if(horzZoomFactor<1.0)
			horzOffset=0;

		update();
		lastHorzZoom=v;
	}
}

const sample_pos_t FXWaveCanvas::getHorzSize() const
{
	return((sample_pos_t)((sample_fpos_t)loadedSound->sound->getLength()/horzZoomFactor)+RIGHT_MARGIN); // accounting for the right margin of unused space
}

const double FXWaveCanvas::getHorzZoom() const
{
	return(lastHorzZoom);
}

const double FXWaveCanvas::getVertZoom() const
{
	return(lastVertZoom);
}

void FXWaveCanvas::setVertZoom(double v)
{
	v=max(0.0,min(1.0,v)); // make v in range: [0,1]

	if(v!=lastVertZoom)
	{
		// how many sample values are represented by one pixel when fully zoomed out
		const double maxZoomFactor=(MAX_SAMPLE*2.0)/(double)getHeight() * loadedSound->sound->getChannelCount();

		const int oldSize=getVertSize()/2;

		// map v:0..1 --> vertZoomFactor:maxZoomFactor..1
		vertZoomFactor=maxZoomFactor+((1.0-maxZoomFactor)*v);

		// adjust the vertical offset to try to keep the same thing centered when changing the zoom factor
		const int newSize=getVertSize()/2;
		vertOffset+=(newSize-oldSize);
		vertOffset=max(0,vertOffset);

		update();
		lastVertZoom=v;
	}
}

const int FXWaveCanvas::getVertSize() const
{
	return((int)ceil(((MAX_SAMPLE*2)/vertZoomFactor)*loadedSound->sound->getChannelCount()));
}


/*
There is actaully a bug with using drawArea, when using Xinerama for dual monitors 
If you scroll while the window is stretched across two screens FXCanvas::drawArea
fails to copy from the other screen.  I suppose it would be possibly to fix in X or
it could be fixed in fox's drawArea() method.  But as of now, I have no way of 
knowing of this condition of the window.  
*/
void FXWaveCanvas::setHorzOffset(sample_pos_t v)
{
	if(v!=horzOffset)
	{
		const sample_pos_t oldHorzOffset=horzOffset;
		horzOffset=v;

		if(horzZoomFactor==prevHorzZoomFactor_horzOffset)
		{
			FXDCWindow dc(this);

			// copy part of the screen if possible
			if(horzOffset>oldHorzOffset)
			{ // scrolling leftwards
				// means we need to copy some of the canvas to the left then draw new stuff on the right side
				FXint sx=horzOffset-oldHorzOffset;
				prevDrawPlayStatusX-=sx; // make sure the draw play status erases the from the scrolled position
				if(getWidth()>sx)
				{
					// copy the waveform over that already exists on the canvas
					dc.drawArea(this,sx,0,getWidth()-sx,getHeight(),0,0);

					// handle if some of the canvas that we attempted to copy was off the right of the screen (which doesn't exist to copy from)
					FXint sub=0;
					FXint screenXOfCanvasRight,dummy;
					translateCoordinatesTo(screenXOfCanvasRight,dummy,getApp()->getRootWindow(),getX()+getWidth(),0);
					const FXint screenWidth=getApp()->getRootWindow()->getDefaultWidth();
					if(screenXOfCanvasRight>screenWidth)
					{
						sx+=(screenXOfCanvasRight-screenWidth); // make the following update, update a more leftward and wider region
						sub=(screenXOfCanvasRight-screenWidth); // this makes the update avoid telling it to update the off-screen area
					}
	
					// update the new part to draw
					update(getWidth()-sx,0,sx-sub,getHeight());
				}
				else
					update(); // just update everything since they scrolled more than the canvas's width
			}
			else
			{ // scrolling rightwards
				// means we need to copy some of the canvas to the right then draw new stuff on the left side
				FXint sx=oldHorzOffset-horzOffset;
				prevDrawPlayStatusX+=sx; // make sure the draw play status erases the from the scrolled position
				if(getWidth()>sx)
				{
					// copy the waveform over that already exists on the canvas
					dc.drawArea(this,0,0,getWidth()-sx,getHeight(),sx,0);

					// handle if some of the canvas that we attempted to copy was off the right of the screen (which doesn't exist to copy from)
					FXint add=0;
					FXint screenXOfCanvasLeft,dummy;
					translateCoordinatesTo(screenXOfCanvasLeft,dummy,getApp()->getRootWindow(),getX(),0);
					if(screenXOfCanvasLeft<0)
						add=-screenXOfCanvasLeft; // this makes the update shift the update that would be off-screen to on-screen
	
					// update the new part to draw
					update(0+add,0,sx,getHeight());
				}
				else
					update(); // just update everything since they scrolled more than the canvas's width
			}

			// I have to force the updates to go ahead and finish now otherwise it has drawing 
			// problems, I'm guessing that the drawArea()'s and update()'s are getting out of 
			// sync when using the scroll wheel causing so many scroll events so fast
			repaint();
		}
		else 
			update(); // update everything since the horz zoom position is new
		
		prevHorzZoomFactor_horzOffset=horzZoomFactor;
	}
}

const sample_pos_t FXWaveCanvas::getHorzOffset() const
{
	return(horzOffset);
}

void FXWaveCanvas::setVertOffset(int v)
{
	if(v!=vertOffset)
	{
		vertOffset=v;

		// ???  should update only what is necessary from the previous position if we were in a view mode where that would help.. when drawing just the waveform, drawing anything horizontal means drawing the whole vertical part too
		
		// assuming that the vertZoomFactor is the same as the last time, but if that changed, then setVertZoomFactor() would have update the whole area
		update();
	}
}

const int FXWaveCanvas::getVertOffset() const
{
	return(vertOffset);
}


long FXWaveCanvas::onPaint(FXObject *object,FXSelector sel,void *ptr)
{
	FXEvent *ev=(FXEvent*)ptr;
	FXDCWindow dc(this,ev);

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

void FXWaveCanvas::drawPortion(int left,int width,FXDCWindow *dc)
{
	if(!shown())
		return;


	// Here is where while an action may be processing that it has the sound locked 
	// for resize so if a redraw occurs that this would deadlock if used a waiting 
	// lock instead of a try-lock.
	// so, I just draw an empty background on the whole thing until a real redraw 
	// can succeed.  
	//
	// ??? One better solution would be to arrange for a back buffer to be saved 
	// before the action started, and I could blit from that for updates
	if(!loadedSound->sound->trylockSize())
	{ // can't lock.. just paint with background.. whole thing first time the lock fails.. just do updated part on not-the-first time
		dc->setForeground(backGroundColor);
		if(lastDrawWasUnsuccessful)
			dc->fillRectangle(left,0,width,getHeight());
		else
			update();
		lastDrawWasUnsuccessful=true;
		return;
	}
	else
		lastDrawWasUnsuccessful=false;

	try
	{
		renderedStartPosition=loadedSound->channel->getStartPosition();
		renderedStopPosition=loadedSound->channel->getStopPosition();

		const int vOffset=((getVertSize()-getHeight())/2)-vertOffset;
		::drawPortion(left,width,dc,loadedSound->sound,getWidth(),getHeight(),(int)getDrawSelectStart(),(int)getDrawSelectStop(),horzZoomFactor,horzOffset,vertZoomFactor,vOffset);
		loadedSound->sound->unlockSize();
	}
	catch(...)
	{
		loadedSound->sound->unlockSize();
		throw;
	}
}


const sample_pos_t FXWaveCanvas::getHorzOffsetToCenterStartPos() const
{
	return (sample_pos_t)max((sample_fpos_t)0.0,sample_fpos_round(loadedSound->channel->getStartPosition()/horzZoomFactor)-getWidth()/2);
}

const sample_pos_t FXWaveCanvas::getHorzOffsetToCenterStopPos() const
{
	return (sample_pos_t)max((sample_fpos_t)0.0,sample_fpos_round(loadedSound->channel->getStopPosition()/horzZoomFactor)-getWidth()/2);
}

void FXWaveCanvas::showAmount(double seconds,sample_pos_t pos,int marginPixels)
{
	if(seconds<(loadedSound->sound->getLength()/loadedSound->sound->getSampleRate()))
	{
		horzZoomFactor=max((sample_fpos_t)1.0,((sample_fpos_t)seconds*loadedSound->sound->getSampleRate())/(getWidth()-(2*marginPixels)));
		horzOffset=(sample_pos_t)(max((sample_fpos_t)0,min((sample_fpos_t)loadedSound->sound->getLength()-getWidth()+RIGHT_MARGIN,(sample_fpos_t)pos-(marginPixels*horzZoomFactor)))/horzZoomFactor);
		prevHorzZoomFactor_horzOffset=horzZoomFactor;

		// recalc the percent value so that getHorzZoom() will return the correct value
		const sample_fpos_t maxZoomFactor=(sample_fpos_t)loadedSound->sound->getLength()/(sample_fpos_t)max(1,(int)(getWidth()-RIGHT_MARGIN));
		lastHorzZoom=(horzZoomFactor-maxZoomFactor)/(1.0-maxZoomFactor);
	}
	else
	{
		setHorzZoom(0.0);
		setHorzOffset(0);
	}

	update();
}        

const bool FXWaveCanvas::isStartPosOnScreen() const
{
	sample_fpos_t x=getDrawSelectStart();
	return (x>=0 && x<getWidth());
}

const bool FXWaveCanvas::isStopPosOnScreen() const
{
	sample_fpos_t x=getDrawSelectStop();
	return (x>=0 && x<getWidth());
}

const sample_fpos_t FXWaveCanvas::getRenderedDrawSelectStart() const
{
	return sample_fpos_round((sample_fpos_t)renderedStartPosition/horzZoomFactor-(sample_fpos_t)horzOffset); 
}

const sample_fpos_t FXWaveCanvas::getRenderedDrawSelectStop() const
{
	return sample_fpos_round((sample_fpos_t)renderedStopPosition/horzZoomFactor-(sample_fpos_t)horzOffset);
}

const sample_fpos_t FXWaveCanvas::getDrawSelectStart() const
{
	return sample_fpos_round((sample_fpos_t)loadedSound->channel->getStartPosition()/horzZoomFactor-(sample_fpos_t)horzOffset);
}

const sample_fpos_t FXWaveCanvas::getDrawSelectStop() const
{
	return sample_fpos_round((sample_fpos_t)loadedSound->channel->getStopPosition()/horzZoomFactor-(sample_fpos_t)horzOffset);
}


// --- cue methods -------------------------------------------------------

const sample_pos_t FXWaveCanvas::snapPositionToCue(sample_pos_t p) const
{
	const sample_pos_t snapToCueDistance=(sample_pos_t)(gSnapToCueDistance*horzZoomFactor+0.5);
	if(gSnapToCues)
	{
		FXint dummy;
		FXuint keyboardModifierState;
		getCursorPosition(dummy,dummy,keyboardModifierState);

		if(! (keyboardModifierState&SHIFTMASK))
		{ // and shift isn't being held down
			size_t cueIndex;
			sample_pos_t distance;

			const bool found=loadedSound->sound->findNearestCue(p,cueIndex,distance);
			if(found && distance<=snapToCueDistance)
				p=loadedSound->sound->getCueTime(cueIndex);
		}
	}
	return(p);
}

const FXint FXWaveCanvas::getCueScreenX(size_t cueIndex) const
{
	sample_fpos_t X=((sample_fpos_t)loadedSound->sound->getCueTime(cueIndex)/horzZoomFactor-horzOffset);
	if(X>=(-CUE_RADIUS-1) && X<(width+CUE_RADIUS+1))
		return((FXint)sample_fpos_round(X));
	else
		return(CUE_OFF_SCREEN);

}

const sample_pos_t FXWaveCanvas::getCueTimeFromX(FXint screenX) const
{
	// ??? uh this is VERY similar to getSamplePosForScreenX
	return((sample_pos_t)(((sample_fpos_t)screenX+(sample_fpos_t)horzOffset)*horzZoomFactor));

}


// --- setting of selection positions methods ------------------------------------------------

const sample_pos_t FXWaveCanvas::getSamplePosForScreenX(FXint X) const
{
	// ??? uh this is VERY similar to getCueTimeFromX
	sample_fpos_t p=sample_fpos_floor(((sample_fpos_t)(X+horzOffset))*horzZoomFactor);
	if(p<0)
		p=0.0;
	else if(p>=loadedSound->sound->getLength())
		p=loadedSound->sound->getLength()-1;

	return((sample_pos_t)p);
}

void FXWaveCanvas::setSelectStartFromScreen(FXint X)
{
	sample_pos_t newSelectStart=snapPositionToCue(getSamplePosForScreenX(X));

	// if we selected the right-most pixel on the screen
	// and--if there were a next pixel--and selecting that
	// would select >= sound's length, we make newSelectStart
	// be the sound's length - 1
	// 							??? why X+1?			??? may beed to check >=len-1
	if(X>=((FXint)getWidth()-1) && (sample_pos_t)((sample_fpos_t)(X+horzOffset+1)*horzZoomFactor)>=loadedSound->sound->getLength())
		newSelectStart=loadedSound->sound->getLength()-1;


	if(newSelectStart<0)
		newSelectStart=0;
	else if(newSelectStart>=loadedSound->sound->getLength())
		newSelectStart=loadedSound->sound->getLength()-1;

	lastChangedPosition=lcpStart;
	loadedSound->channel->setStartPosition(newSelectStart);
}

void FXWaveCanvas::setSelectStopFromScreen(FXint X)
{
	sample_pos_t newSelectStop=snapPositionToCue(getSamplePosForScreenX(X));

	// if we selected the right-most pixel on the screen
	// and--if there were a next pixel--and selecting that
	// would select >= sound's length, we make newSelectStop
	// be the sound's length - 1
	// 											??? may beed to check >=len-1
	if(X>=((FXint)getWidth()-1) && (sample_pos_t)((sample_fpos_t)(X+horzOffset+1)*horzZoomFactor)>=loadedSound->sound->getLength())
		newSelectStop=loadedSound->sound->getLength()-1;

	if(newSelectStop<0)
		newSelectStop=0;
	else if(newSelectStop>=loadedSound->sound->getLength())
		newSelectStop=loadedSound->sound->getLength()-1;

	lastChangedPosition=lcpStop;
	loadedSound->channel->setStopPosition(newSelectStop);
}

void FXWaveCanvas::updateFromSelectionChange(LastChangedPositions _lastChangedPosition)
{
	if(_lastChangedPosition!=lcpNone)
		lastChangedPosition=_lastChangedPosition;

	// ??? couldn't I get rid of the lastChangedPosition and the enum by detecting here which position moved?

	const sample_fpos_t oldDrawSelectStart=getRenderedDrawSelectStart();
	const sample_fpos_t drawSelectStart=getDrawSelectStart();
	const sample_fpos_t oldDrawSelectStop=getRenderedDrawSelectStop();
	const sample_fpos_t drawSelectStop=getDrawSelectStop();

	if(oldDrawSelectStart!=drawSelectStart)
	{
		if(drawSelectStart>oldDrawSelectStart)
			update((int)oldDrawSelectStart,0,(int)(drawSelectStart-oldDrawSelectStart),getHeight()); // shortening
		else
			update((int)drawSelectStart,0,(int)(oldDrawSelectStart-drawSelectStart),getHeight()); // lengthening
	}
	if(oldDrawSelectStop!=drawSelectStop)
	{
		if(drawSelectStop>oldDrawSelectStop)
			update((int)oldDrawSelectStop+1,0,(int)(drawSelectStop-oldDrawSelectStop),getHeight()); // lengthening
		else
			update((int)drawSelectStop+1,0,(int)(oldDrawSelectStop-drawSelectStop),getHeight()); // shortening
	}
}

void FXWaveCanvas::drawPlayPosition(sample_pos_t dataPosition,bool justErasing,bool scrollToMakeVisible,FXScrollArea *optScrollArea)
{
	FXint drawPlayStatusX=0;
	FXDCWindow dc(this);

	if(!justErasing)
	{
		// recalc new draw position
		drawPlayStatusX=(FXint)((sample_fpos_t)dataPosition/horzZoomFactor-horzOffset);

						// don't follow play position if paused unless the user is shuttling
		if(scrollToMakeVisible && (!loadedSound->channel->isPaused() || loadedSound->channel->getSeekSpeed()!=1.0))
		{ // scroll the wave view to make the play position visible on the left most side of the screen

			// if the play position is off the screen
			if(getWidth()>25 && (drawPlayStatusX<0 || drawPlayStatusX>=(sample_fpos_t)getWidth()))
			{
				if(optScrollArea!=NULL)
				{
					// ??? I may need to flush() so that the line drawing will be going onto the right stuff
					optScrollArea->setPosition(-(sample_pos_t)sample_fpos_round((sample_fpos_t)dataPosition/horzZoomFactor-25),-vertOffset); \
				}
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
		if(drawPlayStatusX>=0 && drawPlayStatusX<getWidth())
		{
			//dc.setFunction(BLT_SRC_XOR_DST); // xor the play position on so it doesn't cover up waveform
			dc.setForeground(playStatusColor);
			dc.drawLine(drawPlayStatusX,0,drawPlayStatusX,getHeight()-1);
			prevDrawPlayStatusX=drawPlayStatusX;
		}
	}
}

