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

#include "FXWaveScrollArea.h"

#include <string>

#include "settings.h"

// --- declaration of FXWaveRuler -----------------------------------------------

class FXWaveRuler : public FXComposite
{
	FXDECLARE(FXWaveRuler)
public:
	FXWaveRuler(FXComposite *p,FXRezWaveView *parent,CLoadedSound *_loadedSound);
	virtual ~FXWaveRuler();

	virtual void create();

	size_t getClickedCue(FXint x,FXint y);

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

	long onLeftBtnRelease(FXObject *object,FXSelector sel,void *ptr);

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

	rulerPanel(new FXWaveRuler(this,this,_loadedSound)),
	waveScrollArea(new FXWaveScrollArea(this,_loadedSound))
{
	enable();
}

FXRezWaveView::~FXRezWaveView()
{
}

void FXRezWaveView::setHorzZoom(double v,FXWaveCanvas::HorzRecenterTypes horzRecenterType)
{
	waveScrollArea->setHorzZoom(v,horzRecenterType);
	updateRuler();
}

double FXRezWaveView::getHorzZoom() const
{
	return waveScrollArea->getHorzZoom();
}

void FXRezWaveView::setVertZoom(double v)
{
	waveScrollArea->setVertZoom(v);
}

void FXRezWaveView::drawPlayPosition(sample_pos_t dataPosition,bool justErasing,bool scrollToMakeVisible)
{
	waveScrollArea->drawPlayPosition(dataPosition,justErasing,scrollToMakeVisible);
}

void FXRezWaveView::redraw()
{
	update();
	waveScrollArea->redraw();
	rulerPanel->update();
}

void FXRezWaveView::centerStartPos()
{
	waveScrollArea->centerStartPos();
	rulerPanel->update();
}

void FXRezWaveView::centerStopPos()
{
	waveScrollArea->centerStopPos();
	rulerPanel->update();
}

void FXRezWaveView::showAmount(double seconds,sample_pos_t pos)
{
	waveScrollArea->showAmount(seconds,pos);
}

void FXRezWaveView::updateFromSelectionChange(FXWaveCanvas::LastChangedPositions lastChangedPosition)
{
	waveScrollArea->updateFromSelectionChange(lastChangedPosition);
}

void FXRezWaveView::updateFromEdit()
{
	waveScrollArea->updateFromEdit();
	update();
	rulerPanel->update();
}

void FXRezWaveView::updateRuler()
{
	rulerPanel->update();
}

void FXRezWaveView::getWaveSize(int &top,int &height)
{
	top=waveScrollArea->getY();
	height=waveScrollArea->getHeight()-waveScrollArea->horizontalScrollBar()->getHeight();
}






// --- FXWaveRuler -------------------------------------------------------

#include "../backend/CLoadedSound.h"
#include "../backend/CSound.h"
#include "../backend/CSoundPlayerChannel.h"

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

		// double click handler
	FXMAPFUNC(SEL_LEFTBUTTONRELEASE,	0,						FXWaveRuler::onLeftBtnRelease),

};

FXIMPLEMENT(FXWaveRuler,FXComposite,FXWaveRulerMap,ARRAYNUMBER(FXWaveRulerMap))


FXWaveRuler::FXWaveRuler(FXComposite *p,FXRezWaveView *_parent,CLoadedSound *_loadedSound) :
	FXComposite(p,FRAME_NONE | LAYOUT_FILL_X | LAYOUT_FIX_HEIGHT, 0,0,0,13+8),

	parent(_parent),

	loadedSound(_loadedSound),
	sound(_loadedSound->sound),

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

#define CUE_Y ((height-1)-(1+CUE_RADIUS))

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

	FXint lastX=parent->waveScrollArea->getCanvasWidth();

	const FXint left=0;
	const FXint right=left+lastX;

	FXint s=ev->rect.x;
	s-=LABEL_TICK_FREQ-1;

	FXint e=s+ev->rect.w;
	e+=LABEL_TICK_FREQ-1;

	if(e>lastX)
		e=lastX;

	dc.setForeground(FXRGB(20,20,20));
#if REZ_FOX_VERSION<10117
	dc.setTextFont(font);
#else
	dc.setFont(font);
#endif
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
	const size_t cueCount=sound->getCueCount();
	for(size_t t=0;t<cueCount;t++)
	{
		// ??? I could figure out the min and max screen viable time and make sure that is in range before testing every cue
		FXint cueXPosition=parent->waveScrollArea->getCueScreenX(t);

		if(cueXPosition!=CUE_OFF_SCREEN)
		{
			dc.setForeground(FXRGB(0,0,0));
			const string cueName=sound->getCueName(t);
			dc.drawText(cueXPosition+CUE_RADIUS+1,height-1,cueName.data(),cueName.size());

			dc.drawArc(cueXPosition-CUE_RADIUS,CUE_Y-CUE_RADIUS,CUE_RADIUS*2,CUE_RADIUS*2,0*64,360*64);

			dc.setForeground(sound->isCueAnchored(t) ? FXRGB(0,0,255) : FXRGB(255,0,0));
			dc.drawLine(cueXPosition-1,height-4,cueXPosition-1,CUE_Y-1);
			dc.drawLine(cueXPosition,height-1,cueXPosition,CUE_Y);
			dc.drawLine(cueXPosition+1,height-4,cueXPosition+1,CUE_Y-1);
		}
	}

	return 1;
}

size_t FXWaveRuler::getClickedCue(FXint x,FXint y)
{
	const size_t cueCount=sound->getCueCount();
	size_t cueClicked=cueCount;
	if(cueCount>0)
	{
		// go in reverse order so that the more recent one created can be removed first
		// I mean, if you accidently create one, then want to remove it, and it's on top
		// of another one then it needs to find that more recent one first
		for(size_t t=cueCount;t>=1;t--)
		{
			FXint X=parent->waveScrollArea->getCueScreenX(t-1);
			if(X!=CUE_OFF_SCREEN)
			{
				FXint Y=CUE_Y;

				// check distance from clicked point to the cue's position
				if( ((x-X)*(x-X))+((y-Y)*(y-Y)) <= (CUE_RADIUS*CUE_RADIUS) )
				{
					cueClicked=t-1;
					break;
				}
			}
		}
	}
	return(cueClicked);
}

long FXWaveRuler::onPopupMenu(FXObject *object,FXSelector sel,void *ptr)
{
	if(!underCursor())
		return(1);

	// ??? if the parent->target is NULL, I should pop up the windows

	FXEvent *event=(FXEvent*)ptr;

	// see if any cue was clicked on
	cueClicked=getClickedCue(event->win_x,event->win_y);

	if(cueClicked<sound->getCueCount())
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
		addCueTime=  parent->waveScrollArea->getCueTimeFromX(event->win_x);

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
	wsa->centerStartPos();
	return 1;
}

long FXWaveRuler::onFindStopPosition(FXObject *object,FXSelector sel,void *ptr)
{
	FXWaveScrollArea *wsa=parent->waveScrollArea;
	wsa->centerStopPos();
	return 1;
}

long FXWaveRuler::onSetPositionToCue(FXObject *object,FXSelector sel,void *ptr)
{
	switch(FXSELID(sel))
	{
	case ID_SET_START_POSITION:
		loadedSound->channel->setStartPosition((sample_pos_t)sound->getCueTime(cueClicked));
		break;

	case ID_SET_STOP_POSITION:
		loadedSound->channel->setStopPosition((sample_pos_t)sound->getCueTime(cueClicked));
		break;
	}

	parent->waveScrollArea->updateFromSelectionChange();

	return 1;
}

long FXWaveRuler::onAddCue(FXObject *object,FXSelector sel,void *ptr)
{
	switch(FXSELID(sel))
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

long FXWaveRuler::onLeftBtnRelease(FXObject *object,FXSelector sel,void *ptr)
{
	FXComposite::handle(object,sel,ptr); // if FXComposite ever starts sending SEL_DOUBLECLICKED events, then the event will happen twice because I'm doing what's below.. detect the version and remove this handler

	FXEvent* event=(FXEvent*)ptr;
	if(event->click_count==2) // <-- double click
	{
			// setting data member
		cueClicked=getClickedCue(event->win_x,event->win_y);
		if(cueClicked<sound->getCueCount())
			return onEditCue(object,sel,(void *)cueClicked);
		else
			return onShowCueList(object,sel,ptr);
	}
	else
		return 0;
}

