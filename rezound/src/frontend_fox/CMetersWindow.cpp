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

#include "CMetersWindow.h"

#include "../backend/CSound_defs.h"
#include "../backend/ASoundPlayer.h"

class CMeter : public FXPacker
{
	FXDECLARE(CMeter);
public:
	CMeter::CMeter(FXComposite *parent) :
		FXPacker(parent,LAYOUT_FILL_X|LAYOUT_FILL_Y | FRAME_NONE,0,0,0,0, 0,0,0,0, 0,0),
		canvas(new FXCanvas(this,this,ID_CANVAS,FRAME_NONE | LAYOUT_FILL_X|LAYOUT_FILL_Y))
	{
	}

	CMeter::~CMeter()
	{
	}

	long CMeter::onPaint(FXObject *sender,FXSelector sel,void *ptr)
	{
		FXint x=1+(level*getWidth()/MAX_SAMPLE);

		FXDCWindow dc(canvas);
		//dc.begin(canvas);  as J if it's better to do this or if it's not necessary
		dc.setForeground(FXRGB(0,255,0));
		dc.fillRectangle(0,0,x,getHeight());
		dc.setForeground(FXRGB(0,0,0));
		dc.fillRectangle(x,0,getWidth()-x,getHeight());
		//dc.end();
		return 1;
	}

	void setLevel(sample_t _level)
	{
		level=_level;
		canvas->update(); // flag for repainting
	}

	enum
	{
		ID_CANVAS=FXPacker::ID_LAST
	};

protected:
	CMeter() { }

private:
	FXCanvas *canvas;
	mix_sample_t level;
	
};

FXDEFMAP(CMeter) CMeterMap[]=
{
	//	  Message_Type			ID					Message_Handler
	FXMAPFUNC(SEL_PAINT,			CMeter::ID_CANVAS,			CMeter::onPaint),
};

FXIMPLEMENT(CMeter,FXPacker,CMeterMap,ARRAYNUMBER(CMeterMap))


// ----------------------------------------------------------

FXDEFMAP(CMetersWindow) CMetersWindowMap[]=
{
	//	  Message_Type			ID						Message_Handler
	FXMAPFUNC(SEL_CHORE,			CMetersWindow::ID_UPDATE_CHORE,			CMetersWindow::onUpdateMeters),
	FXMAPFUNC(SEL_TIMEOUT,			CMetersWindow::ID_UPDATE_TIMEOUT,		CMetersWindow::onUpdateMetersSetChore),
};

FXIMPLEMENT(CMetersWindow,FXPacker,CMetersWindowMap,ARRAYNUMBER(CMetersWindowMap))


// ----------------------------------------------------------

/*
 * To update the meters often I add a timeout to be fired every x-th of a second.
 * When I receive that message I add a chore which gets processed when all other
 * messages have been processed and at the end of the chore I set up the timeout 
 * again.
 */

#define METER_UPDATE_RATE 50	 // update every <this value> milliseconds

CMetersWindow::CMetersWindow(FXComposite *parent) :
	FXPacker(parent,LAYOUT_BOTTOM|LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT|FRAME_RIDGE,0,0,0,45, 3,3,3,3, 0,0),
	statusFont(getApp()->getNormalFont()),
	metersFrame(new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_SUNKEN|FRAME_THICK,0,0,0,0, 2,2,2,2, 2,2)),
	soundPlayer(NULL)
{
	// create the font to use for meters
        FXFontDesc d;
        statusFont->getFontDesc(d);
        d.size-=10;
        statusFont=new FXFont(getApp(),d);

	metersFrame->setBackColor(FXRGB(0,0,0));

	chore=NULL;
	timeout=getApp()->addTimeout(METER_UPDATE_RATE,this,ID_UPDATE_TIMEOUT);
}

CMetersWindow::~CMetersWindow()
{
	getApp()->removeChore(chore);
	getApp()->removeTimeout(timeout);
}

long CMetersWindow::onUpdateMeters(FXObject *sender,FXSelector sel,void *ptr)
{
	if(soundPlayer!=NULL)
	{
		// ??? getPeakLevel needs to keep a max since the last time I called it and if possible do mid chunk analysis so I can even do faster meter updates than the buffers might be being processed
		for(size_t t=0;t<meters.size();t++)
			meters[t]->setLevel(soundPlayer->getPeakLevel(t));
	}

	timeout=getApp()->addTimeout(METER_UPDATE_RATE,this,ID_UPDATE_TIMEOUT);
	return 1;
}

long CMetersWindow::onUpdateMetersSetChore(FXObject *sender,FXSelector sel,void *ptr)
{
	chore=getApp()->addChore(this,ID_UPDATE_CHORE);
	return 1;
}

void CMetersWindow::setSoundPlayer(ASoundPlayer *_soundPlayer)
{
	soundPlayer=_soundPlayer;

	for(size_t t=0;t<soundPlayer->devices[0].channelCount;t++)
		meters.push_back(new CMeter(metersFrame));
}


