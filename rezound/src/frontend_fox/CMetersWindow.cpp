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
		static char pix[]={0x55,0x2a};
		stipplePattern=new FXBitmap(getApp(),pix,0,8,2);
		stipplePattern->create();
	}

	CMeter::~CMeter()
	{
	}

	long CMeter::onPaint(FXObject *sender,FXSelector sel,void *ptr)
	{
		FXint x=1+(level*getWidth()/MAX_SAMPLE);

		FXDCWindow dc(canvas);
		dc.begin(canvas);  // ??? ask J if it's better to do this or if it's not necessary

		// draw 11 tick marks above level indication
		dc.setForeground(FXRGB(0,0,0));
		dc.fillRectangle(0,0,getWidth(),2);
		dc.setForeground(FXRGB(128,128,128));
		#define NUM 11
		for(int t=0;t<NUM;t++)
		{
			const int x=(getWidth()-1)*t/(NUM-1);
			dc.drawLine(x,0,x,1);
		}

		// draw horz line below level indication
		dc.setForeground(FXRGB(128,128,128));
		dc.drawLine(0,getHeight()-1,getWidth(),getHeight()-1);

		// draw gray background underneath the stippled level indication 
		dc.setForeground(FXRGB(48,48,48));
		dc.fillRectangle(0,2,getWidth(),getHeight()-3);

		// draw level indication
		dc.setFillStyle(FILL_STIPPLED);
		dc.setStipple(stipplePattern);
		dc.setForeground(FXRGB(0,255,0));
		dc.fillRectangle(0,2,x,getHeight()-3);

			

		dc.end();
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
	FXBitmap *stipplePattern;
	
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
	FXMAPFUNC(SEL_CONFIGURE,		CMetersWindow::ID_LABEL_FRAME,			CMetersWindow::onLabelFrameConfigure),
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
	FXPacker(parent,LAYOUT_BOTTOM|LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT|FRAME_RIDGE,0,0,0,55, 3,3,3,3, 0,0),
	statusFont(getApp()->getNormalFont()),
	metersFrame(new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_SUNKEN|FRAME_THICK,0,0,0,0, 2,2,0,2, 0,1)),
		labelFrame(new FXPacker(metersFrame,LAYOUT_FILL_X|FRAME_NONE,0,0,0,0, 0,0,0,0, 0,0)),
	soundPlayer(NULL)
{
	// create the font to use for meters
        FXFontDesc d;
        statusFont->getFontDesc(d);
        d.size=60;
	d.weight=FONTWEIGHT_NORMAL;
        statusFont=new FXFont(getApp(),d);

		labelFrame->setTarget(this);
		labelFrame->setSelector(ID_LABEL_FRAME);
		labelFrame->setBackColor(FXRGB(0,0,0));
		#define MAKE_DB_LABEL(text) { FXLabel *l=new FXLabel(labelFrame,text,NULL,LAYOUT_FIX_X|LAYOUT_FIX_Y,0,0,0,0, 0,0,0,0); l->setBackColor(FXRGB(0,0,0)); l->setTextColor(FXRGB(128,128,128)); l->setFont(statusFont); }
		MAKE_DB_LABEL("dBFS")
		MAKE_DB_LABEL("-20")
		MAKE_DB_LABEL("-14")
		MAKE_DB_LABEL("-10.5")
		MAKE_DB_LABEL("-8")
		MAKE_DB_LABEL("-6")
		MAKE_DB_LABEL("-4.4")
		MAKE_DB_LABEL("-3.1")
		MAKE_DB_LABEL("-2")
		MAKE_DB_LABEL("-1")
		MAKE_DB_LABEL("0")

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

long CMetersWindow::onLabelFrameConfigure(FXObject *sender,FXSelector,void*)
{
	for(FXint t=0;t<labelFrame->numChildren();t++)
	{
		const int x=(getWidth()/*-1 should be needed, except for the comment below*/)*t/(labelFrame->numChildren()-1);

		int w= t==0 ? 0 : labelFrame->childAtIndex(t)->getWidth();
		if(t!=labelFrame->numChildren()-1)
			w/=2;

		labelFrame->childAtIndex(t)->setX((x-w)-(t*2)); // ??? I have NO earthly idea why '-(t*2)' is required except that the labels are not being placed where I'm placing them
	}
	return 1;
}

void CMetersWindow::setSoundPlayer(ASoundPlayer *_soundPlayer)
{
	soundPlayer=_soundPlayer;

	for(size_t t=0;t<soundPlayer->devices[0].channelCount;t++)
		meters.push_back(new CMeter(metersFrame));
}


