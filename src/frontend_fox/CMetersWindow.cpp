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

/* 
 * I would put this in fox_compat.h, but then I get 'multiple definitions' linker errors, this also means 
 * that I will need to do something different if I start to use it in more than just CMetersWindow.cpp 
 */
#ifndef FXBACKBUFFEREDCANVAS_H
	// if Jeroen hasn't added it yet to libfox, include my implementation of it
	#include "FXBackBufferedCanvas.h"
#endif


#include <math.h>

#include <stdexcept>
#include <algorithm>

#include "../backend/CSound_defs.h"
#include "../backend/unit_conv.h"
#include "../backend/ASoundPlayer.h"

#include "settings.h"


/*
??? I need to make this more general so I can use it for recording or playback
I need to make the analyzer a separate widget
and for both the meter and the analyzer I need to push the information to it rather than have it pull from ASoundPlayer
so that the meter and analyzer widgets are not tied to using ASoundPlayer
*/


// color definitions
#define M_BACKGROUND (FXRGB(0,0,0))
#define M_TEXT_COLOR (FXRGB(220,220,220))
#define M_METER_OFF (FXRGB(48,48,48))

#define M_GREEN (FXRGB(80,255,32))
#define M_YELLOW (FXRGB(255,212,48))
#define M_RED (FXRGB(255,38,0))

#define M_BRT_GREEN (FXRGB(144,255,96))
#define M_BRT_YELLOW (FXRGB(255,255,112))
#define M_BRT_RED (FXRGB(255,38,0))

#define M_PHASE_METER_AXIS (FXRGB(100,100,100))

#define MIN_METER_HEIGHT 15
#define MIN_METERS_WINDOW_HEIGHT 75

#define M_RMS_BALANCE_COLOR M_GREEN
#define M_PEAK_BALANCE_COLOR M_GREEN

#define ANALYZER_BAR_WIDTH 3

#define NUM_LEVEL_METER_TICKS 17
#define NUM_BALANCE_METER_TICKS 9

// --- CLevelMeter -----------------------------------------------------------

class CLevelMeter : public FXHorizontalFrame
{
	FXDECLARE(CLevelMeter);
public:
	CLevelMeter(FXComposite *parent) :
		FXHorizontalFrame(parent,LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT|LAYOUT_TOP | FRAME_NONE,0,0,0,0, 0,0,0,0, 0,0),
		statusFont(getApp()->getNormalFont()),
		canvas(new FXCanvas(this,this,ID_CANVAS,FRAME_NONE | LAYOUT_FILL_X|LAYOUT_FILL_Y)),
		grandMaxPeakLevelLabel(new FXLabel(this,"",NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_FIX_WIDTH|LAYOUT_FILL_Y,0,0,0,0, 1,1,0,0)),
		RMSLevel(0),
		peakLevel(0),
		maxPeakLevel(0),
		grandMaxPeakLevel(0),
		maxPeakFallTimer(0),
		stipplePattern(NULL)
	{
		setBackColor(M_BACKGROUND);

		// create the font to use for numbers
		FXFontDesc d;
		statusFont->getFontDesc(d);
		d.size=60;
		d.weight=FONTWEIGHT_NORMAL;
		statusFont=new FXFont(getApp(),d);


		grandMaxPeakLevelLabel->setTarget(this);
		grandMaxPeakLevelLabel->setSelector(ID_GRAND_MAX_PEAK_LEVEL_LABEL);
		grandMaxPeakLevelLabel->setFont(statusFont);
		grandMaxPeakLevelLabel->setTextColor(M_TEXT_COLOR);
		grandMaxPeakLevelLabel->setBackColor(M_BACKGROUND);


		//static char pix[]={0x55,0x2a};
		//stipplePattern=new FXBitmap(getApp(),pix,0,8,2);
		
		static unsigned char pix[]={0xaa,0x55};
		stipplePattern=new FXBitmap(getApp(),pix,0,8,2);

		stipplePattern->create();

	}

	~CLevelMeter()
	{
		delete statusFont;
	}

	void create()
	{
		FXHorizontalFrame::create();
		setGrandMaxPeakLevel(0);
		setHeight(max(statusFont->getFontHeight(),MIN_METER_HEIGHT)); // make meter only as tall as necessary (also with a defined minimum)
	}

	long onCanvasPaint(FXObject *sender,FXSelector sel,void *ptr)
	{
		FXDCWindow dc(canvas);

		const FXint width=canvas->getWidth();
		const FXint height=canvas->getHeight();

		// draw NUM_LEVEL_METER_TICKS tick marks above level indication
		dc.setForeground(M_BACKGROUND);
		dc.fillRectangle(0,0,width,2);
		dc.setForeground(M_TEXT_COLOR);
		for(int t=0;t<NUM_LEVEL_METER_TICKS;t++)
		{
			const int x=(width-1)*t/(NUM_LEVEL_METER_TICKS-1);
			dc.drawLine(x,0,x,1);
		}

		// draw horz line below level indication
		dc.setForeground(M_TEXT_COLOR);
		dc.drawLine(0,height-1,width,height-1);

		// draw gray background underneath the stippled level indication 
		dc.setForeground(M_METER_OFF);
		dc.fillRectangle(0,2,width,height-3);


		// if the global setting is disabled, stop drawing right here
		if(!gLevelMetersEnabled)
			return 1;


		// draw RMS level indication
		FXint x=(FXint)(RMSLevel*width/MAX_SAMPLE);
		dc.setFillStyle(FILL_STIPPLED);
		dc.setStipple(stipplePattern);
		if(x>(width*3/4))
		{
			dc.setForeground(M_GREEN); // green
			dc.fillRectangle(0,2,width/2,height-3);
			dc.setForeground(M_YELLOW); // yellow
			dc.fillRectangle(width/2,2,width/4,height-3);
			dc.setForeground(M_RED); // red
			dc.fillRectangle(width*3/4,2,x-(width*3/4),height-3);
		}
		else if(x>(width/2))
		{
			dc.setForeground(M_GREEN); // green
			dc.fillRectangle(0,2,width/2,height-3);
			dc.setForeground(M_YELLOW); // yellow
			dc.fillRectangle(width/2,2,x-width/2,height-3);
		}
		else
		{
			dc.setForeground(M_GREEN); // green
			dc.fillRectangle(0,2,x,height-3);
		}

		// draw peak indication
		FXint y=height/2;
		x=(FXint)(peakLevel*width/MAX_SAMPLE);
		dc.setFillStyle(FILL_SOLID);
		if(x>(width*3/4))
		{
			dc.setForeground(M_GREEN);
			dc.fillRectangle(0,y,width/2,2);

			dc.setForeground(M_YELLOW);
			dc.fillRectangle(width/2,y,width/4,2);

			dc.setForeground(M_RED);
			dc.fillRectangle(width*3/4,y,x-(width*3/4),2);
		}
		else if(x>(width/2))
		{
			dc.setForeground(M_GREEN);
			dc.fillRectangle(0,y,width/2,2);

			dc.setForeground(M_YELLOW);
			dc.fillRectangle(width/2,y,x-width/2,2);
		}
		else
		{
			dc.setForeground(M_GREEN);
			dc.fillRectangle(0,y,x,2);
		}

		// draw the max peak indicator
		x=(FXint)(maxPeakLevel*width/MAX_SAMPLE);
		dc.setFillStyle(FILL_SOLID);
		if(x>(width*3/4))
			dc.setForeground(M_BRT_RED);
		else if(x>(width/2))
			dc.setForeground(M_BRT_YELLOW);
		else
			dc.setForeground(M_BRT_GREEN);
		dc.fillRectangle(x-1,2,2,height-3);

		return 1;
	}

	void setLevel(sample_t _RMSLevel,sample_t _peakLevel)
	{
		RMSLevel=_RMSLevel;
		peakLevel=_peakLevel;

		// start decreasing the max peak level after maxPeakFallTimer falls below zero
		if((--maxPeakFallTimer)<0)
		{
			maxPeakLevel=maxPeakLevel-(sample_t)(MAX_SAMPLE*gMaxPeakFallRate);
			maxPeakLevel=maxPeakLevel<0 ? 0 : maxPeakLevel;
			maxPeakFallTimer=0;
		}

		// if the peak level is >= the maxPeakLevel then set a new maxPeakLevel and reset the peak file timer
		if(peakLevel>=maxPeakLevel)
		{
			maxPeakLevel=peakLevel;
			maxPeakFallTimer=gMaxPeakFallDelayTime/gMeterUpdateTime;
		}

		if(peakLevel>grandMaxPeakLevel)
			setGrandMaxPeakLevel(peakLevel); // sets the label and everything

		canvas->update(); // flag for repainting
	}

	sample_t getRMSLevel() const
	{
		return RMSLevel;
	}

	sample_t getPeakLevel() const
	{
		return peakLevel;
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
	CLevelMeter() { }

private:
	friend class CMetersWindow;

	FXFont *statusFont;
	FXCanvas *canvas;
	FXLabel *grandMaxPeakLevelLabel;
	mix_sample_t RMSLevel,peakLevel,maxPeakLevel,grandMaxPeakLevel;
	int maxPeakFallTimer;
	FXBitmap *stipplePattern;
	
};

FXDEFMAP(CLevelMeter) CLevelMeterMap[]=
{
	//	  Message_Type			ID						Message_Handler
	FXMAPFUNC(SEL_PAINT,			CLevelMeter::ID_CANVAS,				CLevelMeter::onCanvasPaint),
	FXMAPFUNC(SEL_LEFTBUTTONRELEASE,	CLevelMeter::ID_GRAND_MAX_PEAK_LEVEL_LABEL,	CLevelMeter::onResetGrandMaxPeakLevel),
};

FXIMPLEMENT(CLevelMeter,FXHorizontalFrame,CLevelMeterMap,ARRAYNUMBER(CLevelMeterMap))





// --- CBalanceMeter -----------------------------------------------------------

class CBalanceMeter : public FXHorizontalFrame
{
	FXDECLARE(CBalanceMeter);
public:
	CBalanceMeter(FXComposite *parent) :
		FXHorizontalFrame(parent,LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT | FRAME_NONE, 0,0,0,0, 0,0,0,0, 0,0),
		statusFont(getApp()->getNormalFont()),
		leftLabel(new FXLabel(this,"-1.0")),
		canvas(new FXCanvas(this,this,ID_CANVAS,FRAME_NONE | LAYOUT_FILL_X|LAYOUT_FILL_Y)),
		rightLabel(new FXLabel(this,"+1.0")),
		RMSBalance(0),
		peakBalance(0),
		stipplePattern(NULL)
	{
		setBackColor(M_BACKGROUND);

		// create the font to use for numbers
		FXFontDesc d;
		statusFont->getFontDesc(d);
		d.size=60;
		d.weight=FONTWEIGHT_NORMAL;
		statusFont=new FXFont(getApp(),d);

		leftLabel->setFont(statusFont);
		leftLabel->setTextColor(M_TEXT_COLOR);
		leftLabel->setBackColor(M_BACKGROUND);

		rightLabel->setFont(statusFont);
		rightLabel->setTextColor(M_TEXT_COLOR);
		rightLabel->setBackColor(M_BACKGROUND);

		static unsigned char pix[]={0xaa,0x55};
		stipplePattern=new FXBitmap(getApp(),pix,0,8,2);

		stipplePattern->create();

	}

	~CBalanceMeter()
	{
		delete statusFont;
	}

	void create()
	{
		FXHorizontalFrame::create();
		setHeight(max(statusFont->getFontHeight(),MIN_METER_HEIGHT)); // make meter only as tall as necessary (also with a defined minimum)
	}

	long onCanvasPaint(FXObject *sender,FXSelector sel,void *ptr)
	{
		FXDCWindow dc(canvas);

		const FXint width=canvas->getWidth();
		const FXint height=canvas->getHeight();

		// draw NUM_LEVEL_METER_TICKS tick marks above level indication
		dc.setForeground(M_BACKGROUND);
		dc.fillRectangle(0,0,width,2);
		dc.setForeground(M_TEXT_COLOR);
		for(int t=0;t<NUM_BALANCE_METER_TICKS;t++)
		{
			const int x=(width-1)*t/(NUM_BALANCE_METER_TICKS-1);
			dc.drawLine(x,0,x,1);
		}

		// draw horz line below level indication
		dc.setForeground(M_TEXT_COLOR);
		dc.drawLine(0,height-1,width,height-1);

		// draw gray background underneath the stippled level indication 
		dc.setForeground(M_METER_OFF);
		dc.fillRectangle(0,2,width,height-3);


		// if the global setting is disabled, stop drawing right here
		if(!gLevelMetersEnabled)
			return 1;


		// draw RMS balance indication
		FXint x=(FXint)(RMSBalance*width)/2;
		dc.setFillStyle(FILL_STIPPLED);
		dc.setStipple(stipplePattern);

		dc.setForeground(M_RMS_BALANCE_COLOR);
		if(x>0)
			dc.fillRectangle(width/2,2,x,height-3);
		else // drawing has to be done a little differently when x is negative
			dc.fillRectangle(width/2+x,2,-x,height-3);


		// draw the peak balance indicator
		FXint y=height/2;
		x=(FXint)(peakBalance*width)/2;
		dc.setFillStyle(FILL_SOLID);
		dc.setForeground(M_PEAK_BALANCE_COLOR);
		if(x>0)
			dc.fillRectangle(width/2,y,x,2);
		else // drawing has to be done a little differently when x is negative
			dc.fillRectangle(width/2+x,y,-x,2);

		dc.setForeground(M_TEXT_COLOR);
		dc.drawLine(width/2,2,width/2,height-1);

		return 1;
	}

	void setBalance(sample_t leftRMSLevel,sample_t rightRMSLevel,sample_t leftPeakLevel,sample_t rightPeakLevel)
	{
		RMSBalance=((float)rightRMSLevel-(float)leftRMSLevel)/MAX_SAMPLE;
		peakBalance=((float)rightPeakLevel-(float)leftPeakLevel)/MAX_SAMPLE;
		canvas->update(); // flag for repainting
	}

	enum
	{
		ID_CANVAS=FXHorizontalFrame::ID_LAST
	};

protected:
	CBalanceMeter() { }

private:
	FXFont *statusFont;

	FXLabel *leftLabel;
	FXCanvas *canvas;
	FXLabel *rightLabel;

	float RMSBalance;
	float peakBalance;

	FXBitmap *stipplePattern;
	
};

FXDEFMAP(CBalanceMeter) CBalanceMeterMap[]=
{
	//	  Message_Type			ID						Message_Handler
	FXMAPFUNC(SEL_PAINT,			CBalanceMeter::ID_CANVAS,			CBalanceMeter::onCanvasPaint),
};

FXIMPLEMENT(CBalanceMeter,FXHorizontalFrame,CBalanceMeterMap,ARRAYNUMBER(CBalanceMeterMap))





// --- CStereoPhaseMeter -------------------------------------------------------------

class CStereoPhaseMeter : public FXHorizontalFrame
{
	FXDECLARE(CStereoPhaseMeter);
public:
	CStereoPhaseMeter(FXComposite *parent,sample_t *_samplingBuffer,size_t _samplingNFrames,unsigned _samplingNChannels,unsigned _samplingLeftChannel,unsigned _samplingRightChannel) :
		FXHorizontalFrame(parent,LAYOUT_RIGHT|LAYOUT_FIX_WIDTH|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 0,0),
		canvasFrame(new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_SUNKEN|FRAME_THICK,0,0,0,0, 2,2,2,2, 0,1)),
			canvas(new FXBackBufferedCanvas(canvasFrame,this,ID_CANVAS,LAYOUT_FILL_X|LAYOUT_FILL_Y)),
		zoomDial(new FXDial(this,this,ID_ZOOM_DIAL,DIAL_VERTICAL|DIAL_HAS_NOTCH|LAYOUT_FILL_Y|LAYOUT_FIX_WIDTH,0,0,16,0)),

		statusFont(getApp()->getNormalFont()),

		samplingBuffer(_samplingBuffer),
		samplingNFrames(_samplingNFrames),
		samplingNChannels(_samplingNChannels),
		samplingLeftChannel(_samplingLeftChannel),
		samplingRightChannel(_samplingRightChannel),

		clear(true)
	{
		hide();
		setBackColor(M_BACKGROUND);
			canvasFrame->setBackColor(M_BACKGROUND);
			
#if REZ_FOX_VERSION<10129
		canvas->setBackBufferOptions(IMAGE_OWNED|IMAGE_ALPHA); // IMAGE_ALPHA: the alpha channel is not used, but it's a place holder so I can work with 32bit values
#else
		canvas->setBackBufferOptions(IMAGE_OWNED); /* 1.1.29 and later always has 32bit image data */
#endif

		zoomDial->setRange(100,400);
		zoomDial->setValue(100);
		zoomDial->setRevolutionIncrement((400-100+10)*2);
		zoomDial->setTipText(_("Adjust Zoom Factor for Stereo Phase Meter\nAll the way down means no zooming"));
		zoom=((float)zoomDial->getValue())/100.0f;


		// create the font to use for numbers
		FXFontDesc d;
		statusFont->getFontDesc(d);
		d.size=60;
		d.weight=FONTWEIGHT_NORMAL;
		statusFont=new FXFont(getApp(),d);
	}

	~CStereoPhaseMeter()
	{
		delete statusFont;
	}

	long onZoomDial(FXObject *sender,FXSelector sel,void *ptr)
	{
		zoom=((float)zoomDial->getValue())/100.0f;
		canvas->update(); // not really necessary since we're doing it several times a second anyway
		return 1;
	}

	long onResize(FXObject *sender,FXSelector sel,void *ptr)
	{
		// make square
		resize(getHeight()+getHSpacing()+zoomDial->getWidth(),getHeight());
		recalcRotateLookup();
		clearCanvas();
		return 1;
	}

	long onCanvasPaint(FXObject *sender,FXSelector sel,void *ptr)
	{
		FXColor *data=(FXColor *)canvas->getBackBufferData();

		// the w and h that we're going to render to (minus some borders and tick marks)
		const FXint canvasSize=canvas->getWidth();  // w==h is guarenteed in onResize()

		if(clear)
		{
			memset(data,0,canvasSize*canvasSize*sizeof(FXColor));
			clear=false;
		}

		// if the global setting is disabled, stop drawing right here
		if(!gStereoPhaseMetersEnabled)
			return 1;

		// fade previous frame (so we get a ghosting history)
		for(FXint t=0;t<canvasSize*canvasSize;t++)
		{
			const FXColor c=data[t];
			data[t]= c==0 ? 0 : FXRGB( FXREDVAL(c)*7/8, FXGREENVAL(c)*7/8, FXBLUEVAL(c)*7/8 );
		}

		// draw the axies
		for(FXint t=0;t<canvasSize;t++)
		{
				// I have no idea with '-canvasSize/2' is necessary but it doesn't look right if I omit it
			data[t+(canvasSize*canvasSize/2)-canvasSize/2]=M_PHASE_METER_AXIS; // horz
			data[(t*canvasSize)+canvasSize/2]=M_PHASE_METER_AXIS; // vert
		}

		// draw the points
		for(size_t t=0;t<samplingNFrames;t++)
		{
			// let x and y be the normalized (1.0) sample values (x:right y:left) then scaled up to the canvas width/height and centered in the square
			const FXint x=(FXint)( (zoom*samplingBuffer[t*samplingNChannels+samplingRightChannel])*canvasSize/2/MAX_SAMPLE) + canvasSize/2;
			const FXint y=(FXint)(-(zoom*samplingBuffer[t*samplingNChannels+samplingLeftChannel ])*canvasSize/2/MAX_SAMPLE) + canvasSize/2; // negation because increasing values go down on the screen which is up-side-down from the Cartesian plane

			if(x>=0 && x<canvasSize && y>=0 && y<canvasSize)
			{
				if(gStereoPhaseMeterUnrotate)
					data[unrotateMapping[y*canvasSize+x]]=M_BRT_GREEN;
				else
					data[y*canvasSize+x]=M_BRT_GREEN;

			}
		}

		return 0;
	}

	long onPopupMenu(FXObject *object,FXSelector sel,void *ptr)
	{
		FXEvent *event=(FXEvent*)ptr;

		FXMenuPane popupMenu(this);
			// ??? make sure that these get deleted when gotoMenu is deleted
#if REZ_FOX_VERSION>=10119
			(new FXMenuCheck(&popupMenu,_("Unrotate from Natural 45 Degree Line"),this, ID_UNROTATE))->setCheck(gStereoPhaseMeterUnrotate);
#else
			FXMenuCommand *m=new FXMenuCommand(&popupMenu,_("Unrotate from Natural 45 Degree Line"),NULL,this, ID_UNROTATE);
			if(gStereoPhaseMeterUnrotate)
				m->check();
			else
				m->uncheck();
#endif

		popupMenu.create();
		popupMenu.popup(NULL,event->root_x,event->root_y);
		getApp()->runModalWhileShown(&popupMenu);

		return 0;
	}

	long onUnrotateMenuItem(FXObject *object,FXSelector sel,void *ptr)
	{
#if REZ_FOX_VERSION>=10119
		gStereoPhaseMeterUnrotate= ((FXMenuCheck *)object)->getCheck() ? true : false;
#else
		if(dynamic_cast<FXMenuCommand *>(object)->isChecked())
			dynamic_cast<FXMenuCommand *>(object)->uncheck();
		else
			dynamic_cast<FXMenuCommand *>(object)->check();
		gStereoPhaseMeterUnrotate= ((FXMenuCommand *)object)->isChecked() ? true : false;
#endif
		return 0;
	}

	void updateCanvas()
	{
		canvas->update();
	}

	void clearCanvas()
	{
		clear=true;
	}

	enum
	{
		ID_CANVAS=FXHorizontalFrame::ID_LAST,
		ID_ZOOM_DIAL,
		ID_UNROTATE,
	};

protected:
	CStereoPhaseMeter() : samplingNFrames(0), samplingNChannels(0),samplingLeftChannel(0), samplingRightChannel(0) { }

private:
	FXPacker *canvasFrame;
		FXBackBufferedCanvas *canvas;
	FXDial *zoomDial;
	FXFont *statusFont;

	const sample_t *samplingBuffer;
	const size_t samplingNFrames;
	const unsigned samplingNChannels;
	const unsigned samplingLeftChannel;
	const unsigned samplingRightChannel;

	bool clear;

	float zoom;

	std::vector<FXint> unrotateMapping; // width*height number of pixels mapping

	void recalcRotateLookup()
	{
		const FXint width=canvas->getWidth();
		const FXint height=canvas->getHeight();

		unrotateMapping.resize(width*height);

		const double ang=-M_PI_4; // -45 degrees

		for(FXint sx=0;sx<width;sx++)
		for(FXint sy=0;sy<height;sy++)
		{
			double wx=sx-width/2;
			double wy=sy-height/2;

			// shrink square so that the corners aren't cut off when rotated 
			// (this also fixes the gaps between pixels if I don't shrink it)
			wx*=(sqrt(2.0)/2.0); 
			wy*=(sqrt(2.0)/2.0);

			double rot_wx=wx*cos(ang)-wy*sin(ang);
			double rot_wy=wx*sin(ang)+wy*cos(ang);

			rot_wx+=width/2;
			rot_wy+=height/2;

			rot_wx=round(rot_wx);
			rot_wy=round(rot_wy);
			FXint offset=(FXint)(rot_wy*width+rot_wx);
			if((rot_wx>=0 && rot_wx<width && rot_wy>=0 && rot_wy<height) && (offset>=0 && offset<(width*height)))
				unrotateMapping[sy*width+sx]=offset;
			else
				unrotateMapping[sy*width+sx]=0;
		}
		
	}
};

FXDEFMAP(CStereoPhaseMeter) CStereoPhaseMeterMap[]=
{
	//	  Message_Type			ID				Message_Handler
	FXMAPFUNC(SEL_PAINT,			CStereoPhaseMeter::ID_CANVAS,	CStereoPhaseMeter::onCanvasPaint),
	FXMAPFUNC(SEL_CONFIGURE,		0,				CStereoPhaseMeter::onResize),
	FXMAPFUNC(SEL_RIGHTBUTTONPRESS,		CStereoPhaseMeter::ID_CANVAS,	CStereoPhaseMeter::onPopupMenu),
	FXMAPFUNC(SEL_COMMAND,			CStereoPhaseMeter::ID_UNROTATE,	CStereoPhaseMeter::onUnrotateMenuItem),
	FXMAPFUNC(SEL_CHANGED,			CStereoPhaseMeter::ID_ZOOM_DIAL,CStereoPhaseMeter::onZoomDial),
};

FXIMPLEMENT(CStereoPhaseMeter,FXHorizontalFrame,CStereoPhaseMeterMap,ARRAYNUMBER(CStereoPhaseMeterMap))








// --- CAnalyzer -------------------------------------------------------------

class CAnalyzer : public FXHorizontalFrame
{
	FXDECLARE(CAnalyzer);
public:
	CAnalyzer(FXComposite *parent) :
		FXHorizontalFrame(parent,LAYOUT_RIGHT|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 0,0),
		canvasFrame(new FXVerticalFrame(this,LAYOUT_FIX_WIDTH|LAYOUT_FILL_Y|FRAME_SUNKEN|FRAME_THICK,0,0,0,0, 2,2,2,2, 0,1)),
			canvas(new FXBackBufferedCanvas(canvasFrame,this,ID_CANVAS,LAYOUT_FILL_X|LAYOUT_FILL_Y)),
			labelFrame(new FXHorizontalFrame(canvasFrame,LAYOUT_FILL_X|FRAME_NONE,0,0,0,0, 0,0,0,0, 0,0)),

		zoomDial(new FXDial(this,this,ID_ZOOM_DIAL,DIAL_VERTICAL|DIAL_HAS_NOTCH|LAYOUT_FILL_Y|LAYOUT_FIX_WIDTH,0,0,16,0)),

		statusFont(getApp()->getNormalFont()),

		drawBarFreq(false)
	{
		hide();
		setBackColor(M_BACKGROUND);
			canvasFrame->setBackColor(M_BACKGROUND);
			labelFrame->setBackColor(M_BACKGROUND);

		// create the font to use for numbers
		FXFontDesc d;
		statusFont->getFontDesc(d);
		d.size=60;
		d.weight=FONTWEIGHT_NORMAL;
		statusFont=new FXFont(getApp(),d);

		zoomDial->setRange(1,200);
		zoomDial->setValue(25);
		zoomDial->setRevolutionIncrement(200*2);
		zoomDial->setTipText(_("Adjust Zoom Factor for Analyzer\nRight-click for Default"));
		zoom=zoomDial->getValue();

		canvasFrame->setWidth(150);
	}

	~CAnalyzer()
	{
		delete statusFont;
	}

	long onZoomDial(FXObject *sender,FXSelector sel,void *ptr)
	{
		zoom=zoomDial->getValue();
		canvas->update(); // not really necessary since we're doing it several times a second anyway
		return 1;
	}

	long onZoomDialDefault(FXObject *sender,FXSelector sel,void *ptr)
	{
		zoomDial->setValue(25);
		return onZoomDial(sender,sel,ptr);
	}

	long onCanvasPaint(FXObject *sender,FXSelector sel,void *ptr)
	{
		FXDC &dc=*((FXDC*)ptr); // back buffered canvases send the DC to draw onto in ptr

		dc.setForeground(M_BACKGROUND);
		dc.fillRectangle(0,0,canvas->getWidth(),canvas->getHeight());

		// if the global setting is disabled, stop drawing right here
		if(!gFrequencyAnalyzerEnabled)
			return 1;

		// the w and h that we're going to render to (minus some borders and tick marks)
		const size_t canvasWidth=canvas->getWidth(); 
		const size_t canvasHeight=canvas->getHeight();
		
		const size_t barWidth=analysis.size()>0 ? canvasWidth/analysis.size() : 1;

		// draw vertical octave separator lines
		dc.setForeground(M_METER_OFF);
		if(octaveStride>0)
		{
			for(size_t x=0;x<canvasWidth;x+=(barWidth*octaveStride))
				dc.drawLine(x,0,x,canvasHeight);
		}

		// ??? also probably want some dB labels 
		// draw 5 horz lines up the panel
		dc.setForeground(M_TEXT_COLOR);
		for(size_t t=0;t<4;t++)
		{
			size_t y=((canvasHeight-1)*t/(4-1));
			dc.drawLine(0,y,canvasWidth,y);
		}

			
		// draw frequency analysis bars  (or render text if fftw wasn't installed)
		dc.setForeground(M_GREEN);
		if(analysis.size()>0)
		{
			size_t x=0;
			const size_t drawBarWidth=max(1,(int)barWidth-1);
			for(size_t t=0;t<analysis.size();t++)
			{
				const size_t barHeight=(size_t)floor(analysis[t]*canvasHeight);
				dc.fillRectangle(x+1,canvasHeight-barHeight,drawBarWidth,barHeight);
				x+=barWidth;
			}
		}
#ifndef HAVE_FFTW
		else
		{
			dc.compat_setFont(getApp()->getNormalFont());
			dc.drawText(3,3+12,_("Configure with FFTW"),19);
			dc.drawText(3,20+12,_("for Freq. Analysis"),18);
		}
#endif


		// draw the peaks
		dc.setForeground(M_BRT_YELLOW);
		size_t x=0;
		for(size_t t=0;t<peaks.size();t++)
		{
			const size_t peakHeight=(size_t)floor(peaks[t]*canvasHeight);
			const FXint y=canvasHeight-peakHeight-1;
			dc.drawLine(x+1,y,x+barWidth-1,y);
			x+=barWidth;
		}

		// if mouse is over the canvas, then report what frequency is to be displaye
		if(analysis.size()>0 && drawBarFreq && barFreqIndex<frequencies.size())
		{
			const string f=istring(frequencies[barFreqIndex])+"Hz";
			const int w=statusFont->getTextWidth(f.c_str(),f.length());
			dc.setForeground(M_METER_OFF);
			dc.fillRectangle(0,0,w+3,statusFont->getFontHeight()+2);
			dc.setForeground(M_TEXT_COLOR);
			dc.compat_setFont(statusFont);
			dc.drawText(1,statusFont->getFontHeight(),f.c_str(),f.length());
		}

		return 1;
	}

	long onCanvasEnter(FXObject *sender,FXSelector sel,void *ptr)
	{
		drawBarFreq=true;
		return onCanvasMotion(sender,sel,ptr);
	}

	long onCanvasLeave(FXObject *sender,FXSelector sel,void *ptr)
	{
		drawBarFreq=false;
		return onCanvasMotion(sender,sel,ptr);
	}

	long onCanvasMotion(FXObject *sender,FXSelector sel,void *ptr)
	{
		barFreqIndex=((FXEvent *)ptr)->win_x/ANALYZER_BAR_WIDTH;
		return 1;
	}

	bool setAnalysis(const vector<float> &_analysis,size_t _octaveStride)
	{
		analysis=_analysis;
		for(size_t t=0;t<analysis.size();t++)
			analysis[t]*=zoom;

		octaveStride=_octaveStride;

		// resize the canvas frame if needed
		FXint desiredWidth;
		if(analysis.size()>0)
			desiredWidth=(analysis.size()*ANALYZER_BAR_WIDTH)+canvasFrame->getPadLeft()+canvasFrame->getPadRight()+4/*for sunken frame*/;
		else
			desiredWidth=150;  // big enough to render a message about installing fftw

		bool rebuildLabels=false;

		if(canvasFrame->getWidth()!=desiredWidth)
		{
			canvasFrame->setWidth(desiredWidth);
			rebuildLabels=true;
		}

		
		// rebuild peaks if the number of elements in analysis changed from the last time this was called  (should really only do anything the first time this is called)
		if(peaks.size()!=analysis.size())
		{
			peaks.clear();
			for(size_t t=0;t<analysis.size();t++)
			{
				peaks.push_back(0.0f);
				peakFallDelayTimers.push_back(0);
			}
			rebuildLabels=true;

		}

		// make peaks fall
		for(size_t t=0;t<peaks.size();t++)
		{
			peakFallDelayTimers[t]=max(0,peakFallDelayTimers[t]-1);
			if(peakFallDelayTimers[t]==0) // only decrease the peak when the fall timer has reached zero
				peaks[t]=max(0.0f,(float)(peaks[t]-gAnalyzerPeakFallRate));
		}


		// update peaks if there is a new max
		for(size_t t=0;t<analysis.size();t++)
		{
			if(peaks[t]<analysis[t])
			{
				peaks[t]=analysis[t];
				peakFallDelayTimers[t]=gAnalyzerPeakFallDelayTime/gMeterUpdateTime;
			}
		}
		
		canvas->update(); // flag for repainting

		return rebuildLabels;
	}

	void rebuildLabels(const vector<size_t> _frequencies,size_t octaveStride)
	{
		frequencies=_frequencies;

		while(labelFrame->numChildren()>0)
			delete labelFrame->childAtIndex(0);

		for(size_t t=0;t<analysis.size()/octaveStride;t++)
		{
			string text;
			if(frequencies[t*octaveStride]>=1000)
				text=istring(frequencies[t*octaveStride]/1000.0,1,1)+"k";
			else
				text=istring(frequencies[t*octaveStride]);

			FXLabel *l=new FXLabel(labelFrame,text.c_str(),NULL,JUSTIFY_LEFT|LAYOUT_FIX_WIDTH,0,0,0,0, 0,0,0,0);
			l->setBackColor(M_BACKGROUND);
			l->setTextColor(M_TEXT_COLOR);
			l->setFont(statusFont);
			if(t!=((analysis.size()/octaveStride)-1))
				l->setWidth(ANALYZER_BAR_WIDTH*octaveStride);
			else
				l->setWidth((ANALYZER_BAR_WIDTH*octaveStride)+(ANALYZER_BAR_WIDTH*(analysis.size()%octaveStride))); // make the last one the remaining width
			l->create();
		}
	}

	void clearCanvas()
	{
		vector<float> d2;
		setAnalysis(d2,1);

		vector<size_t> d1;
		rebuildLabels(d1,1);
	}

	enum
	{
		ID_CANVAS=FXHorizontalFrame::ID_LAST,
		ID_ZOOM_DIAL,
	};

protected:
	CAnalyzer() { }

private:
	FXPacker *canvasFrame;
		FXBackBufferedCanvas *canvas;
		FXPacker *labelFrame;
	FXDial *zoomDial;
	FXFont *statusFont;

	vector<float> analysis;
	vector<float> peaks;
	vector<int> peakFallDelayTimers;
	size_t octaveStride;
	float zoom;

	// used when the mouse is over the canvas to draw what the frequency for the pointed-to bar is
	vector<size_t> frequencies;
	bool drawBarFreq;
	size_t barFreqIndex;
};

FXDEFMAP(CAnalyzer) CAnalyzerMap[]=
{
	//	  Message_Type			ID					Message_Handler
	FXMAPFUNC(SEL_PAINT,			CAnalyzer::ID_CANVAS,			CAnalyzer::onCanvasPaint),
	FXMAPFUNC(SEL_ENTER,			CAnalyzer::ID_CANVAS,			CAnalyzer::onCanvasEnter),
	FXMAPFUNC(SEL_LEAVE,			CAnalyzer::ID_CANVAS,			CAnalyzer::onCanvasLeave),
	FXMAPFUNC(SEL_MOTION,			CAnalyzer::ID_CANVAS,			CAnalyzer::onCanvasMotion),

	FXMAPFUNC(SEL_CHANGED,			CAnalyzer::ID_ZOOM_DIAL,		CAnalyzer::onZoomDial),
	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,	CAnalyzer::ID_ZOOM_DIAL,		CAnalyzer::onZoomDialDefault),
};

FXIMPLEMENT(CAnalyzer,FXHorizontalFrame,CAnalyzerMap,ARRAYNUMBER(CAnalyzerMap))








// --- CMetersWindow ---------------------------------------------------------

FXDEFMAP(CMetersWindow) CMetersWindowMap[]=
{
	//	  Message_Type			ID						Message_Handler
	FXMAPFUNC(SEL_TIMEOUT,			CMetersWindow::ID_UPDATE_TIMEOUT,		CMetersWindow::onUpdateMeters),
	FXMAPFUNC(SEL_CONFIGURE,		CMetersWindow::ID_LABEL_FRAME,			CMetersWindow::onLabelFrameConfigure),
	FXMAPFUNC(SEL_LEFTBUTTONRELEASE,	CMetersWindow::ID_GRAND_MAX_PEAK_LEVEL_LABEL,	CMetersWindow::onResetGrandMaxPeakLevels),
};

FXIMPLEMENT(CMetersWindow,FXHorizontalFrame,CMetersWindowMap,ARRAYNUMBER(CMetersWindowMap))


/*
 * To update the meters I add a timeout to be fired every x-th of a second.
 */

CMetersWindow::CMetersWindow(FXComposite *parent) :
	FXHorizontalFrame(parent,LAYOUT_BOTTOM|LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 4,4,2,2, 4,1),
	statusFont(getApp()->getNormalFont()),
	levelMetersFrame(new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_SUNKEN|FRAME_THICK,0,0,0,0, 3,3,0,2, 0,1)),
		headerFrame(new FXHorizontalFrame(levelMetersFrame,LAYOUT_FILL_X|FRAME_NONE,0,0,0,0, 0,0,0,0, 0,0)),
			labelFrame(new FXPacker(headerFrame,LAYOUT_FILL_X|LAYOUT_BOTTOM|FRAME_NONE,0,0,0,0, 0,0,0,0, 0,0)),
			grandMaxPeakLevelLabel(new FXLabel(headerFrame,"max",NULL,LABEL_NORMAL|LAYOUT_FIX_WIDTH|LAYOUT_RIGHT,0,0,0,0, 1,1,0,0)),
		balanceMetersFrame(new FXHorizontalFrame(levelMetersFrame,FRAME_NONE | LAYOUT_BOTTOM | LAYOUT_FILL_X, 0,0,0,0, 0,0,0,0, 0,0)),
			balanceMetersRightMargin(new FXLabel(balanceMetersFrame," ",NULL,LAYOUT_RIGHT | FRAME_NONE | LAYOUT_FIX_WIDTH|LAYOUT_FILL_Y)),
	analyzer(new CAnalyzer(this)),
	soundPlayer(NULL)
{
	// create the font to use for meters
	FXFontDesc d;
	statusFont->getFontDesc(d);
	d.size=60;
	d.weight=FONTWEIGHT_NORMAL;
	statusFont=new FXFont(getApp(),d);

	levelMetersFrame->setBackColor(M_BACKGROUND);
		headerFrame->setBackColor(M_BACKGROUND);

			// create the labels above the tick marks
			labelFrame->setTarget(this);
			labelFrame->setSelector(ID_LABEL_FRAME);
			labelFrame->setBackColor(M_BACKGROUND);
			#define MAKE_DB_LABEL(text) { FXLabel *l=new FXLabel(labelFrame,(text),NULL,LAYOUT_FIX_X|LAYOUT_FIX_Y,0,0,0,0, 0,0,0,0); l->setBackColor(M_BACKGROUND); l->setTextColor(M_TEXT_COLOR); l->setFont(statusFont); }
			MAKE_DB_LABEL("dBFS") // create the -infinity label as just the units label
			for(int t=1;t<NUM_LEVEL_METER_TICKS;t++)
			{
				istring s;
				if(t>NUM_LEVEL_METER_TICKS/2)
					s=istring(round(scalar_to_dB((double)t/(NUM_LEVEL_METER_TICKS-1))*10)/10,3,1); // round to nearest tenth
				else 
					s=istring(round(scalar_to_dB((double)t/(NUM_LEVEL_METER_TICKS-1))),3,1); // round to nearest int

				if(s.rfind(".0")!=istring::npos) // remove .0's from the right side
					s.erase(s.length()-2,2);
				MAKE_DB_LABEL(s.c_str())
			}

			grandMaxPeakLevelLabel->setTarget(this);
			grandMaxPeakLevelLabel->setSelector(ID_GRAND_MAX_PEAK_LEVEL_LABEL);
			grandMaxPeakLevelLabel->setFont(statusFont);
			grandMaxPeakLevelLabel->setTextColor(M_TEXT_COLOR);
			grandMaxPeakLevelLabel->setBackColor(M_BACKGROUND);

		balanceMetersFrame->setBackColor(M_BACKGROUND);
			balanceMetersRightMargin->setFont(statusFont);
			balanceMetersRightMargin->setTextColor(M_TEXT_COLOR);
			balanceMetersRightMargin->setBackColor(M_BACKGROUND);


	// schedule the first update meters event
#if REZ_FOX_VERSION<10322
	timeout=getApp()->addTimeout(this,ID_UPDATE_TIMEOUT,gMeterUpdateTime);
#else
	getApp()->addTimeout(this,ID_UPDATE_TIMEOUT,gMeterUpdateTime);
#endif
}

CMetersWindow::~CMetersWindow()
{
#if REZ_FOX_VERSION<10322
	getApp()->removeTimeout(timeout);
#else
	getApp()->removeTimeout(this,ID_UPDATE_TIMEOUT);
#endif
}

long CMetersWindow::onUpdateMeters(FXObject *sender,FXSelector sel,void *ptr)
{
	if(soundPlayer!=NULL && levelMeters.size()>0 && soundPlayer->isInitialized())
	{
		if(gLevelMetersEnabled)
		{
			// set the level for all the level meters
			for(size_t t=0;t<levelMeters.size();t++)
				levelMeters[t]->setLevel(soundPlayer->getRMSLevel(t),soundPlayer->getPeakLevel(t));

			// set the balance meter position
			for(size_t t=0;t<balanceMeters.size();t++)
				// there is a balance meter for every two level meters
				// 	NOTE: ??? using getPeakLevel doesn't given an accurate peaked balance because the two peaks are from two separate points of time.. I'm not sure if this is a really bad issue right now or not
				balanceMeters[t]->setBalance(levelMeters[t*2+0]->getRMSLevel(),levelMeters[t*2+1]->getRMSLevel(),levelMeters[t*2+0]->getPeakLevel(),levelMeters[t*2+1]->getPeakLevel());

			// make sure all the levelMeters' grandMaxPeakLabels are the same width
			int maxGrandMaxPeakLabelWidth=grandMaxPeakLevelLabel->getWidth();
			bool resize=false;
			for(size_t t=0;t<levelMeters.size();t++)
			{
				if(maxGrandMaxPeakLabelWidth<levelMeters[t]->grandMaxPeakLevelLabel->getWidth())
				{
					maxGrandMaxPeakLabelWidth=levelMeters[t]->grandMaxPeakLevelLabel->getWidth();
					resize=true;
				}
			}

			if(resize)
			{
				grandMaxPeakLevelLabel->setWidth(maxGrandMaxPeakLabelWidth);
				for(size_t t=0;t<levelMeters.size();t++)
					levelMeters[t]->grandMaxPeakLevelLabel->setWidth(maxGrandMaxPeakLabelWidth);
				balanceMetersRightMargin->setWidth(maxGrandMaxPeakLabelWidth);
			}
		}

		if(gStereoPhaseMetersEnabled && !stereoPhaseMeters.empty())
		{
			soundPlayer->getSamplingForStereoPhaseMeters(samplingForStereoPhaseMeters.data(),samplingForStereoPhaseMeters.size());
			for(size_t t=0;t<stereoPhaseMeters.size();t++)
				stereoPhaseMeters[t]->updateCanvas();
		}

		if(gFrequencyAnalyzerEnabled)
		{
			if(analyzer->setAnalysis(soundPlayer->getFrequencyAnalysis(),soundPlayer->getFrequencyAnalysisOctaveStride()))
			{
				const vector<float> a=soundPlayer->getFrequencyAnalysis();
				vector<size_t> frequencies;
				for(size_t t=0;t<a.size();t++)
					frequencies.push_back(soundPlayer->getFrequency(t));
				analyzer->rebuildLabels(frequencies,soundPlayer->getFrequencyAnalysisOctaveStride());
			}
		}
	}

	// schedule another update again in METER_UPDATE_RATE milliseconds
#if REZ_FOX_VERSION<10322
	timeout=getApp()->addTimeout(this,ID_UPDATE_TIMEOUT,gMeterUpdateTime);
#else
	getApp()->addTimeout(this,ID_UPDATE_TIMEOUT,gMeterUpdateTime);
#endif
	return 0; // returning 0 because 1 makes it use a ton of CPU (because returning 1 causes FXApp::refresh() to be called)
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
	if(_soundPlayer==NULL)
		return;

	soundPlayer=_soundPlayer;

	for(size_t t=0;t<soundPlayer->devices[0].channelCount;t++)
		levelMeters.push_back(new CLevelMeter(levelMetersFrame));

	for(size_t t=0;t<soundPlayer->devices[0].channelCount/2;t++)
		balanceMeters.push_back(new CBalanceMeter(balanceMetersFrame));

	samplingForStereoPhaseMeters.resize(gStereoPhaseMeterPointCount*soundPlayer->devices[0].channelCount);
	for(size_t t=0;t<soundPlayer->devices[0].channelCount/2;t++)
	{
		stereoPhaseMeters.insert(stereoPhaseMeters.begin(),
			new CStereoPhaseMeter(
				this,
				samplingForStereoPhaseMeters.data(),
				gStereoPhaseMeterPointCount,
				soundPlayer->devices[0].channelCount,
				t*2+0,
				t*2+1
			)
		);
	}

	setHeight(
		max(
								// +1 for the balance meter(s)
			headerFrame->getHeight() + ((soundPlayer->devices[0].channelCount+1) * max(statusFont->getFontHeight(),MIN_METER_HEIGHT+levelMetersFrame->getVSpacing())) + (getVSpacing()*(numChildren()-1)) + (getPadTop()+getPadBottom()+levelMetersFrame->getPadTop()+levelMetersFrame->getPadBottom() + 2+2+2+2/*frame rendering*/),
			(unsigned)MIN_METERS_WINDOW_HEIGHT
		)
	);

	// make sure the GUI initially reflects the global settings
	enableLevelMeters(isLevelMetersEnabled());
	enableStereoPhaseMeters(isStereoPhaseMetersEnabled());
	enableFrequencyAnalyzer(isFrequencyAnalyzerEnabled());
}

void CMetersWindow::resetGrandMaxPeakLevels()
{
	for(size_t t=0;t<levelMeters.size();t++)
		levelMeters[t]->setGrandMaxPeakLevel(0);
}

bool CMetersWindow::isLevelMetersEnabled() const
{
	return gLevelMetersEnabled;
}

void CMetersWindow::enableLevelMeters(bool enable)
{
	gLevelMetersEnabled=enable;
	if(!enable)
	{ 
		// just to cause a canvas update
		for(size_t t=0;t<levelMeters.size();t++)
			levelMeters[t]->setLevel(0,0);
		for(size_t t=0;t<balanceMeters.size();t++)
			balanceMeters[t]->setBalance(0,0,0,0);
	}

	/* looks stupid
	if(enable)
		levelMetersFrame->show();
	else
		levelMetersFrame->hide();
	*/
	showHideAll();
}

bool CMetersWindow::isStereoPhaseMetersEnabled() const
{
	return gStereoPhaseMetersEnabled;
}

void CMetersWindow::enableStereoPhaseMeters(bool enable)
{
	gStereoPhaseMetersEnabled=enable;
	if(!enable)
	{ 
		// just to cause a canvas update
		for(size_t t=0;t<stereoPhaseMeters.size();t++)
		{
			if(stereoPhaseMeters[t]->shown())
				stereoPhaseMeters[t]->clearCanvas();
		}
	}

	for(size_t t=0;t<stereoPhaseMeters.size();t++)
	{
		if(enable)
		{
			// have to call this so that onResize() will get called in order to calculate the unrotateMapping
			stereoPhaseMeters[t]->resize(1,1); 

			stereoPhaseMeters[t]->show();
		}
		else
			stereoPhaseMeters[t]->hide();
	}
	showHideAll();
}

bool CMetersWindow::isFrequencyAnalyzerEnabled() const
{
	return gFrequencyAnalyzerEnabled;
}

void CMetersWindow::enableFrequencyAnalyzer(bool enable)
{
	gFrequencyAnalyzerEnabled=enable;
	if(!enable)
	{
		if(analyzer->shown())
			analyzer->clearCanvas();
	}

	if(enable)
		analyzer->show();
	else
		analyzer->hide();
	showHideAll();
}

void CMetersWindow::showHideAll()
{
	recalc();
	if(isLevelMetersEnabled() || isStereoPhaseMetersEnabled() || isFrequencyAnalyzerEnabled())
		show();
	else
		hide();
}
