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
#define M_TEXT_COLOR (FXRGB(164,164,164))
#define M_METER_OFF (FXRGB(48,48,48))

#define M_GREEN (FXRGB(80,255,32))
#define M_YELLOW (FXRGB(255,212,48))
#define M_RED (FXRGB(255,38,0))

#define M_BRT_GREEN (FXRGB(144,255,96))
#define M_BRT_YELLOW (FXRGB(255,255,112))
#define M_BRT_RED (FXRGB(255,38,0))

#define MIN_METER_HEIGHT 15
#define MIN_METERS_WINDOW_HEIGHT 75

#define ANALYZER_BAR_WIDTH 6


// --- CMeter ----------------------------------------------------------------

class CMeter : public FXHorizontalFrame
{
	FXDECLARE(CMeter);
public:
	CMeter::CMeter(FXComposite *parent) :
		FXHorizontalFrame(parent,LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT|LAYOUT_TOP | FRAME_NONE,0,0,0,0, 0,0,0,0, 0,0),
		statusFont(getApp()->getNormalFont()),
		canvas(new FXCanvas(this,this,ID_CANVAS,FRAME_NONE | LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,400,0)),
		grandMaxPeakLevelLabel(new FXLabel(this,"",NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_FIX_WIDTH|LAYOUT_FILL_Y,0,0,0,0, 1,1,0,0)),
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
		grandMaxPeakLevelLabel->setTextColor(M_TEXT_COLOR);
		grandMaxPeakLevelLabel->setBackColor(M_BACKGROUND);


		//static char pix[]={0x55,0x2a};
		//stipplePattern=new FXBitmap(getApp(),pix,0,8,2);
		
		static char pix[]={0xaa,0x55};
		stipplePattern=new FXBitmap(getApp(),pix,0,8,2);

		stipplePattern->create();

	}

	CMeter::~CMeter()
	{
		delete statusFont;
	}

	void create()
	{
		FXHorizontalFrame::create();
		setGrandMaxPeakLevel(0);
		setHeight(max(statusFont->getFontHeight(),MIN_METER_HEIGHT)); // make meter only as tall as necessary (also with a defined minimum)
	}

	long CMeter::onCanvasPaint(FXObject *sender,FXSelector sel,void *ptr)
	{
		FXDCWindow dc(canvas);

		const FXint width=canvas->getWidth();
		const FXint height=canvas->getHeight();

		// draw 11 tick marks above level indication
		dc.setForeground(M_BACKGROUND);
		dc.fillRectangle(0,0,width,2);
		dc.setForeground(M_TEXT_COLOR);
		#define NUM 11
		for(int t=0;t<NUM;t++)
		{
			const int x=(width-1)*t/(NUM-1);
			dc.drawLine(x,0,x,1);
		}

		// draw horz line below level indication
		dc.setForeground(M_TEXT_COLOR);
		dc.drawLine(0,height-1,width,height-1);

		// draw gray background underneath the stippled level indication 
		dc.setForeground(M_METER_OFF);
		dc.fillRectangle(0,2,width,height-3);

		// draw RMS level indication
		FXint x=(RMSLevel*width/MAX_SAMPLE);
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
		x=(peakLevel*width/MAX_SAMPLE);
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
		x=(maxPeakLevel*width/MAX_SAMPLE);
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






// --- CAnalyzer -------------------------------------------------------------

#include <fox/FXBackBufferedCanvas.h>
class CAnalyzer : public FXHorizontalFrame
{
	FXDECLARE(CAnalyzer);
public:
	CAnalyzer::CAnalyzer(FXComposite *parent) :
		FXHorizontalFrame(parent,LAYOUT_RIGHT|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 0,0),
		canvasFrame(new FXVerticalFrame(this,LAYOUT_FIX_WIDTH|LAYOUT_FILL_Y|FRAME_SUNKEN|FRAME_THICK,0,0,0,0, 2,2,2,2, 0,1)),
			canvas(new FXBackBufferedCanvas(canvasFrame,this,ID_CANVAS,LAYOUT_FILL_X|LAYOUT_FILL_Y)),
			labelFrame(new FXHorizontalFrame(canvasFrame,LAYOUT_FILL_X|FRAME_NONE,0,0,0,0, 0,0,0,0, 0,0)),

		zoomDial(new FXDial(this,this,ID_ZOOM_DIAL,DIAL_VERTICAL|DIAL_HAS_NOTCH|LAYOUT_FILL_Y|LAYOUT_FIX_WIDTH,0,0,18,0)),

		statusFont(getApp()->getNormalFont()),

		drawBarFreq(false)
	{
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
		zoomDial->setTipText("Adjust Zoom Factor for Analyzer\nRight-click for Default");
		zoom=zoomDial->getValue();
	}

	CAnalyzer::~CAnalyzer()
	{
		delete statusFont;
	}

	long CAnalyzer::onZoomDial(FXObject *sender,FXSelector sel,void *ptr)
	{
		zoom=zoomDial->getValue();
		canvas->update(); // not really necessary since we're doing it several times a second anyway
		return 1;
	}

	long CAnalyzer::onZoomDialDefault(FXObject *sender,FXSelector sel,void *ptr)
	{
		zoomDial->setValue(25);
		return onZoomDial(sender,sel,ptr);
	}

	long CAnalyzer::onCanvasPaint(FXObject *sender,FXSelector sel,void *ptr)
	{
		FXDC &dc=*((FXDC*)ptr); // back buffered canvases send the DC to draw onto in ptr

		dc.setForeground(M_BACKGROUND);
		dc.fillRectangle(0,0,canvas->getWidth(),canvas->getHeight());

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
			for(size_t t=0;t<analysis.size();t++)
			{
				const size_t barHeight=(size_t)floor(analysis[t]*canvasHeight);
				dc.fillRectangle(x+1,canvasHeight-barHeight,barWidth-1,barHeight);
				x+=barWidth;
			}
		}
		else
		{
			dc.drawText(3,3+12,"Configure with FFTW",19);
			dc.drawText(3,20+12,"for Freq. Analysis",18);
		}


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
#if REZ_FOX_VERSION<10117
			dc.setTextFont(statusFont);
#else
			dc.setFont(statusFont);
#endif
			dc.drawText(1,statusFont->getFontHeight(),f.c_str(),f.length());
		}

		return 1;
	}

	long CAnalyzer::onCanvasEnter(FXObject *sender,FXSelector sel,void *ptr)
	{
		drawBarFreq=true;
		return 1;
	}

	long CAnalyzer::onCanvasLeave(FXObject *sender,FXSelector sel,void *ptr)
	{
		drawBarFreq=false;
		return 1;
	}

	long CAnalyzer::onCanvasMotion(FXObject *sender,FXSelector sel,void *ptr)
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
				peaks.push_back(0.0);
				peakFallDelayTimers.push_back(0);
			}
			rebuildLabels=true;

		}

		// make peaks fall
		for(size_t t=0;t<peaks.size();t++)
		{
			peakFallDelayTimers[t]=max(0,peakFallDelayTimers[t]-1);
			if(peakFallDelayTimers[t]==0) // only decrease the peak when the fall timer has reached zero
				peaks[t]=max((float)0.0,(float)(peaks[t]-gAnalyzerPeakFallRate));
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
		for(FXint t=0;t<labelFrame->numChildren();t++)
			delete labelFrame->childAtIndex(t);
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

	enum
	{
		ID_CANVAS=FXHorizontalFrame::ID_LAST,
		ID_ZOOM_DIAL,
	};

protected:
	CAnalyzer() { }

private:
	friend class CMetersWindow;

	FXPacker *canvasFrame;
		FXCanvas *canvas;
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
 * To update the meters often I add a timeout to be fired every x-th of a second.
 */

CMetersWindow::CMetersWindow(FXComposite *parent) :
	FXHorizontalFrame(parent,LAYOUT_BOTTOM|LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 4,4,2,2, 4,1),
	statusFont(getApp()->getNormalFont()),
	levelMetersFrame(new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_SUNKEN|FRAME_THICK,0,0,0,0, 3,3,0,2, 0,1)),
		headerFrame(new FXHorizontalFrame(levelMetersFrame,LAYOUT_FILL_X|FRAME_NONE,0,0,0,0, 0,0,0,0, 0,0)),
			labelFrame(new FXPacker(headerFrame,LAYOUT_FILL_X|LAYOUT_BOTTOM|FRAME_NONE,0,0,0,0, 0,0,0,0, 0,0)),
			grandMaxPeakLevelLabel(new FXLabel(headerFrame,"max",NULL,LABEL_NORMAL|LAYOUT_FIX_WIDTH|LAYOUT_RIGHT,0,0,0,0, 1,1,0,0)),
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

			labelFrame->setTarget(this);
			labelFrame->setSelector(ID_LABEL_FRAME);
			labelFrame->setBackColor(M_BACKGROUND);
			#define MAKE_DB_LABEL(text) { FXLabel *l=new FXLabel(labelFrame,text,NULL,LAYOUT_FIX_X|LAYOUT_FIX_Y,0,0,0,0, 0,0,0,0); l->setBackColor(M_BACKGROUND); l->setTextColor(M_TEXT_COLOR); l->setFont(statusFont); }
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
			grandMaxPeakLevelLabel->setTextColor(M_TEXT_COLOR);
			grandMaxPeakLevelLabel->setBackColor(M_BACKGROUND);


	// schedule the first update meters event
	timeout=getApp()->addTimeout(gMeterUpdateTime,this,ID_UPDATE_TIMEOUT);
}

CMetersWindow::~CMetersWindow()
{
	getApp()->removeTimeout(timeout);
}

long CMetersWindow::onUpdateMeters(FXObject *sender,FXSelector sel,void *ptr)
{
	if(soundPlayer!=NULL && meters.size()>0 && soundPlayer->isInitialized())
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

		if(analyzer->setAnalysis(soundPlayer->getFrequencyAnalysis(),soundPlayer->getFrequencyAnalysisOctaveStride()))
		{
			const vector<float> a=soundPlayer->getFrequencyAnalysis();
			vector<size_t> frequencies;
			for(size_t t=0;t<a.size();t++)
				frequencies.push_back(soundPlayer->getFrequency(t));
			analyzer->rebuildLabels(frequencies,soundPlayer->getFrequencyAnalysisOctaveStride());
		}
	}

	// schedule another update again in METER_UPDATE_RATE milliseconds
	timeout=getApp()->addTimeout(gMeterUpdateTime,this,ID_UPDATE_TIMEOUT);
	return 0;
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
		meters.push_back(new CMeter(levelMetersFrame));

	setHeight(
		max(
			headerFrame->getHeight() + (soundPlayer->devices[0].channelCount * max(statusFont->getFontHeight(),MIN_METER_HEIGHT+levelMetersFrame->getVSpacing())) + (getVSpacing()*(numChildren()-1)) + (getPadTop()+getPadBottom()+levelMetersFrame->getPadTop()+levelMetersFrame->getPadBottom() + 2+2+2+2/*frame rendering*/),
			(unsigned)MIN_METERS_WINDOW_HEIGHT
		)
	);
}

void CMetersWindow::resetGrandMaxPeakLevels()
{
	for(size_t t=0;t<meters.size();t++)
		meters[t]->setGrandMaxPeakLevel(0);
}

