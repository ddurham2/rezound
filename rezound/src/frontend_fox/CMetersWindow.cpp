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

#define METER_UPDATE_RATE 50	 // update every <this value> milliseconds

#include "CMetersWindow.h"

#include <math.h>

#include <stdexcept>

#include "../backend/CSound_defs.h"
#include "../backend/unit_conv.h"
#include "../backend/ASoundPlayer.h"

class CMeter : public FXHorizontalFrame
{
	FXDECLARE(CMeter);
public:
	CMeter::CMeter(FXComposite *parent) :
		FXHorizontalFrame(parent,LAYOUT_FILL_X|LAYOUT_FILL_Y | FRAME_NONE,0,0,0,0, 0,0,0,0, 0,0),
		statusFont(getApp()->getNormalFont()),
		canvas(new FXCanvas(this,this,ID_CANVAS,FRAME_NONE | LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,400,0)),
		grandMaxPeakLevelLabel(new FXLabel(this,"",NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_FIX_WIDTH|LAYOUT_FILL_Y,0,0,0,0, 1,1,2,2)),
		RMSLevel(0),
		peakLevel(0),
		maxPeakLevel(0),
		grandMaxPeakLevel(0),
		maxPeakFallTimer(0),
		stipplePattern(NULL)
	{
		// create the font to use for numbers
		FXFontDesc d;
		statusFont->getFontDesc(d);
		d.size=60;
		d.weight=FONTWEIGHT_NORMAL;
		statusFont=new FXFont(getApp(),d);


		grandMaxPeakLevelLabel->setTarget(this);
		grandMaxPeakLevelLabel->setSelector(ID_GRAND_MAX_PEAK_LEVEL_LABEL);
		grandMaxPeakLevelLabel->setFont(statusFont);
		grandMaxPeakLevelLabel->setTextColor(FXRGB(128,128,128));
		grandMaxPeakLevelLabel->setBackColor(FXRGB(0,0,0));


		//static char pix[]={0x55,0x2a};
		//stipplePattern=new FXBitmap(getApp(),pix,0,8,2);
		
		static char pix[]={0xaa,0x55};
		stipplePattern=new FXBitmap(getApp(),pix,0,8,2);

		stipplePattern->create();

	}

	CMeter::~CMeter()
	{
	}

	void create()
	{
		FXHorizontalFrame::create();
		setGrandMaxPeakLevel(0);
	}

	long CMeter::onCanvasPaint(FXObject *sender,FXSelector sel,void *ptr)
	{
		FXDCWindow dc(canvas);
		dc.begin(canvas);  // ??? ask J if it's better to do this or if it's not necessary

		const FXint width=canvas->getWidth();
		const FXint height=canvas->getHeight();

		// draw 11 tick marks above level indication
		dc.setForeground(FXRGB(0,0,0));
		dc.fillRectangle(0,0,width,2);
		dc.setForeground(FXRGB(128,128,128));
		#define NUM 11
		for(int t=0;t<NUM;t++)
		{
			const int x=(width-1)*t/(NUM-1);
			dc.drawLine(x,0,x,1);
		}

		// draw horz line below level indication
		dc.setForeground(FXRGB(128,128,128));
		dc.drawLine(0,height-1,width,height-1);

		// draw gray background underneath the stippled level indication 
		dc.setForeground(FXRGB(48,48,48));
		dc.fillRectangle(0,2,width,height-3);

		// draw RMS level indication
		FXint x=(RMSLevel*width/MAX_SAMPLE);
		dc.setFillStyle(FILL_STIPPLED);
		dc.setStipple(stipplePattern);
		if(x>(width*3/4))
		{
			dc.setForeground(FXRGB(80,255,32)); // green
			dc.fillRectangle(0,2,width/2,height-3);
			dc.setForeground(FXRGB(255,212,48)); // yellow
			dc.fillRectangle(width/2,2,width/4,height-3);
			dc.setForeground(FXRGB(255,38,0)); // red
			dc.fillRectangle(width*3/4,2,x-(width*3/4),height-3);
		}
		else if(x>(width/2))
		{
			dc.setForeground(FXRGB(80,255,32)); // green
			dc.fillRectangle(0,2,width/2,height-3);
			dc.setForeground(FXRGB(255,212,48)); // yellow
			dc.fillRectangle(width/2,2,x-width/2,height-3);
		}
		else
		{
			dc.setForeground(FXRGB(80,255,32)); // green
			dc.fillRectangle(0,2,x,height-3);
		}

		// draw peak indication
		FXint y=height/2;
		x=(peakLevel*width/MAX_SAMPLE);
		dc.setFillStyle(FILL_SOLID);
		if(x>(width*3/4))
		{
			dc.setForeground(FXRGB(80,255,32)); // green
			dc.fillRectangle(0,y,width/2,2);

			dc.setForeground(FXRGB(255,212,48)); // yellow
			dc.fillRectangle(width/2,y,width/4,2);

			dc.setForeground(FXRGB(255,38,0)); // red
			dc.fillRectangle(width*3/4,y,x-(width*3/4),2);
		}
		else if(x>(width/2))
		{
			dc.setForeground(FXRGB(80,255,32)); // green
			dc.fillRectangle(0,y,width/2,2);

			dc.setForeground(FXRGB(255,212,48)); // yellow
			dc.fillRectangle(width/2,y,x-width/2,2);
		}
		else
		{
			dc.setForeground(FXRGB(80,255,32)); // green
			dc.fillRectangle(0,y,x,2);
		}

		// draw the max peak indicator
		x=(maxPeakLevel*width/MAX_SAMPLE);
		dc.setFillStyle(FILL_SOLID);
		if(x>(width*3/4))
			dc.setForeground(FXRGB(255,38,0)); // brighter red
		else if(x>(width/2))
			dc.setForeground(FXRGB(255,255,112)); // brighter yellow
		else
			dc.setForeground(FXRGB(144,255,96)); // brighter green
		dc.fillRectangle(x-1,2,2,height-3);
			

		dc.end();
		return 1;
	}

	void setLevel(sample_t _RMSLevel,sample_t _peakLevel)
	{
		RMSLevel=_RMSLevel;
		peakLevel=_peakLevel;

		// start decreasing the max peak level after maxPeakFallTimer falls below zero
		if((--maxPeakFallTimer)<0)
		{
			//maxPeakLevel=0;
			maxPeakLevel=maxPeakLevel-MAX_SAMPLE/50; // fall 2% of the max sample
			maxPeakLevel=maxPeakLevel<0 ? 0 : maxPeakLevel;
			maxPeakFallTimer=0;
		}

		// if the peak level is >= the maxPeakLevel then set a new maxPeakLevel and reset the peak file timer
		if(peakLevel>=maxPeakLevel)
		{
			maxPeakLevel=peakLevel;
			maxPeakFallTimer=10;
		}

		if(peakLevel>grandMaxPeakLevel)
			setGrandMaxPeakLevel(peakLevel); // sets the label and everything

		canvas->update(); // flag for repainting
	}

	void setGrandMaxPeakLevel(const sample_t maxPeakLevel)
	{
		grandMaxPeakLevel=maxPeakLevel;
		grandMaxPeakLevelLabel->setText((istring(amp_to_dBFS(grandMaxPeakLevel),3,1,false)+" dBFS").c_str());

		// and make sure grandMaxPeakLevelLabel is wide enough for the text
		const FXint w=statusFont->getTextWidth(grandMaxPeakLevelLabel->getText().text(),grandMaxPeakLevelLabel->getText().length());
		if(grandMaxPeakLevelLabel->getWidth()<w+2)
			grandMaxPeakLevelLabel->setWidth(w+2);
	}

	long onResetGrandMaxPeakLevel(FXObject *sender,FXSelector,void*)
	{
		setGrandMaxPeakLevel(0);
		return 1;
	}

	enum
	{
		ID_CANVAS=FXHorizontalFrame::ID_LAST,
		ID_GRAND_MAX_PEAK_LEVEL_LABEL
	};

protected:
	CMeter() { }

private:
	friend class CMetersWindow;

	FXFont *statusFont;
	FXCanvas *canvas;
	FXLabel *grandMaxPeakLevelLabel;
	mix_sample_t RMSLevel,peakLevel,maxPeakLevel,grandMaxPeakLevel;
	int maxPeakFallTimer;
	FXBitmap *stipplePattern;
	
};

FXDEFMAP(CMeter) CMeterMap[]=
{
	//	  Message_Type			ID					Message_Handler
	FXMAPFUNC(SEL_PAINT,			CMeter::ID_CANVAS,			CMeter::onCanvasPaint),
	FXMAPFUNC(SEL_LEFTBUTTONRELEASE,	CMeter::ID_GRAND_MAX_PEAK_LEVEL_LABEL,	CMeter::onResetGrandMaxPeakLevel),
};

FXIMPLEMENT(CMeter,FXHorizontalFrame,CMeterMap,ARRAYNUMBER(CMeterMap))






// ----------------------------------------------------------

FXDEFMAP(CMetersWindow) CMetersWindowMap[]=
{
	//	  Message_Type			ID						Message_Handler
	FXMAPFUNC(SEL_CHORE,			CMetersWindow::ID_UPDATE_CHORE,			CMetersWindow::onUpdateMeters),
	FXMAPFUNC(SEL_TIMEOUT,			CMetersWindow::ID_UPDATE_TIMEOUT,		CMetersWindow::onUpdateMetersSetChore),
	FXMAPFUNC(SEL_CONFIGURE,		CMetersWindow::ID_LABEL_FRAME,			CMetersWindow::onLabelFrameConfigure),
	FXMAPFUNC(SEL_LEFTBUTTONRELEASE,	CMetersWindow::ID_GRAND_MAX_PEAK_LEVEL_LABEL,	CMetersWindow::onResetGrandMaxPeakLevels),
};

FXIMPLEMENT(CMetersWindow,FXPacker,CMetersWindowMap,ARRAYNUMBER(CMetersWindowMap))


/*
 * To update the meters often I add a timeout to be fired every x-th of a second.
 * When I receive that message I add a chore which gets processed when all other
 * messages have been processed and at the end of the chore I set up the timeout 
 * again.
 * the cycle is
 * -construction-> AAA -N ms-> BBB -chore-> CCC -N ms-> BBB -chore-> CCC -N ms-> BBB -chore-> CCC -> ...
 */

CMetersWindow::CMetersWindow(FXComposite *parent) :
	FXPacker(parent,LAYOUT_BOTTOM|LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT|FRAME_RIDGE,0,0,0,55, 3,3,3,3, 0,0),
	statusFont(getApp()->getNormalFont()),
	metersFrame(new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_SUNKEN|FRAME_THICK,0,0,0,0, 2,2,0,2, 0,1)),
		headerFrame(new FXHorizontalFrame(metersFrame,LAYOUT_FILL_X|FRAME_NONE,0,0,0,0, 0,0,0,0, 0,0)),
			labelFrame(new FXPacker(headerFrame,LAYOUT_FILL_X|LAYOUT_BOTTOM|FRAME_NONE,0,0,0,0, 0,0,0,0, 0,0)),
			grandMaxPeakLevelLabel(new FXLabel(headerFrame,"max",NULL,LABEL_NORMAL|LAYOUT_FIX_WIDTH|LAYOUT_RIGHT,0,0,0,0, 1,1,1,1)),
	soundPlayer(NULL)
{
	// create the font to use for meters
        FXFontDesc d;
        statusFont->getFontDesc(d);
        d.size=60;
	d.weight=FONTWEIGHT_NORMAL;
        statusFont=new FXFont(getApp(),d);

	metersFrame->setBackColor(FXRGB(0,0,0));
		headerFrame->setBackColor(FXRGB(0,0,0));

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

			grandMaxPeakLevelLabel->setTarget(this);
			grandMaxPeakLevelLabel->setSelector(ID_GRAND_MAX_PEAK_LEVEL_LABEL);
			grandMaxPeakLevelLabel->setFont(statusFont);
			grandMaxPeakLevelLabel->setTextColor(FXRGB(128,128,128));
			grandMaxPeakLevelLabel->setBackColor(FXRGB(0,0,0));


	// AAA
	chore=NULL;
	// schedule the first update meters event
	timeout=getApp()->addTimeout(METER_UPDATE_RATE,this,ID_UPDATE_TIMEOUT);
}

CMetersWindow::~CMetersWindow()
{
	getApp()->removeChore(chore);
	getApp()->removeTimeout(timeout);
}

// BBB
long CMetersWindow::onUpdateMetersSetChore(FXObject *sender,FXSelector sel,void *ptr)
{
	// now schedule an event to update the meters when everything else is finished
	chore=getApp()->addChore(this,ID_UPDATE_CHORE);
	return 1;
}

// CCC
long CMetersWindow::onUpdateMeters(FXObject *sender,FXSelector sel,void *ptr)
{
	if(soundPlayer!=NULL && meters.size()>0)
	{
		for(size_t t=0;t<meters.size();t++)
			meters[t]->setLevel(soundPlayer->getRMSLevel(t),soundPlayer->getPeakLevel(t));

		// make sure all the meters' grandMaxPeakLabels are the same width
		int maxGrandMaxPeakLabelWidth=grandMaxPeakLevelLabel->getWidth();
		bool resize=false;
		for(size_t t=0;t<meters.size();t++)
		{
			if(maxGrandMaxPeakLabelWidth<meters[t]->grandMaxPeakLevelLabel->getWidth())
			{
				maxGrandMaxPeakLabelWidth=meters[t]->grandMaxPeakLevelLabel->getWidth();
				resize=true;
			}
		}

		if(resize)
		{
			grandMaxPeakLevelLabel->setWidth(maxGrandMaxPeakLabelWidth);
			for(size_t t=0;t<meters.size();t++)
				meters[t]->grandMaxPeakLevelLabel->setWidth(maxGrandMaxPeakLabelWidth);
		}

	}

	// schedule another update again in METER_UPDATE_RATE milliseconds
	timeout=getApp()->addTimeout(METER_UPDATE_RATE,this,ID_UPDATE_TIMEOUT);
	return 1;
}

long CMetersWindow::onLabelFrameConfigure(FXObject *sender,FXSelector,void*)
{
	for(FXint t=0;t<labelFrame->numChildren();t++)
	{
		const int x=(labelFrame->getWidth()-1)*t/(labelFrame->numChildren()-1);

		int w= t==0 ? 0 : labelFrame->childAtIndex(t)->getWidth();
		if(t!=labelFrame->numChildren()-1)
			w/=2;

		labelFrame->childAtIndex(t)->setX((x-w));
	}
	return 1;
}

long CMetersWindow::onResetGrandMaxPeakLevels(FXObject *sender,FXSelector sel,void *ptr)
{
	resetGrandMaxPeakLevels();
	return 1;
}

void CMetersWindow::setSoundPlayer(ASoundPlayer *_soundPlayer)
{
	if(soundPlayer!=NULL)
		throw runtime_error(string(__func__)+" -- internal error -- sound player already set -- perhaps I need to allow this");
	soundPlayer=_soundPlayer;

	for(size_t t=0;t<soundPlayer->devices[0].channelCount;t++)
		meters.push_back(new CMeter(metersFrame));
}

void CMetersWindow::resetGrandMaxPeakLevels()
{
	for(size_t t=0;t<meters.size();t++)
		meters[t]->setGrandMaxPeakLevel(0);
}

