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

#include "FXGraphParamValue.h"

#include <exception>
#include <stdexcept>
#include <string>
#include <algorithm>

#include <istring>

#include <CNestedDataFile/CNestedDataFile.h>
#define DOT (CNestedDataFile::delimChar)

#include "utils.h"

#include "../backend/CSound.h"

#define NODE_RADIUS 4

/* TODO:
 *
 *	- I probably need to limit the minimum size as it could cause divide by zero errors, and really be unusable anyway
 *
 * 	- I would have liked to reuse code (without copy/pasting) from FXRezWaveView, but it's currently
 * 	  too much tied to CLoadedSound for start and stop positions and such...
 * 	  - At least I pulled draw portion out not to have to rewrite that
 * 	  - but it would be nice if all the scrolling and zooming capabilities where also there....
 *
 * 	- I should also onMouseMove print the time that the cursor is over
 * 		- clear it onExit
 *
 * 	- draw a vertical to the time and a horizontal line to the ruler while dragging the nodes 
 *
 * 	- still a glitch with the drawing of the bottom tick in the ruler.. I might draw more values in the ticks
 * 		I think I've fixed this now
 *
 *   	- I might ought to draw the play position in there if I can get the event
 *
 *	- I might want to avoid redrawing everything.  Alternatively I could:
 *         - blit only the sections between the nodes involving the change
 *         	I think I do now ???
 *
 */


// --- declaration of CHorzRuler -----------------------------------------------

class CHorzRuler : public FXHorizontalFrame
{
	FXDECLARE(CHorzRuler)
public:
	CHorzRuler(FXComposite *p,FXGraphParamValue *_parent) :
		FXHorizontalFrame(p,FRAME_NONE | LAYOUT_FILL_X | LAYOUT_FIX_HEIGHT | LAYOUT_SIDE_TOP, 0,0,0,17),

		parent(_parent),

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

	virtual ~CHorzRuler()
	{
		delete font;
	}


	virtual void create()
	{
		FXHorizontalFrame::create();
		font->create();
	}

	// events I get by message

	long onPaint(FXObject *object,FXSelector sel,void *ptr)
	{
		FXHorizontalFrame::onPaint(object,sel,ptr);

		FXEvent *ev=(FXEvent*)ptr;

		FXDCWindow dc(this,ev);
		dc.setForeground(FXRGB(20,20,20));
#if REZ_FOX_VERSION<10117
		dc.setTextFont(font);
#else
		dc.setFont(font);
#endif

		#define H_TICK_FREQ 50

		// N is the number of ticks to label
		int N=(parent->getGraphPanelWidth()/H_TICK_FREQ); // we lable always an odd number of ticks so there is always a middle tick
		if(N%2==0)
			N++;

		// make sure there's at least 3
		if(N<3)
			N=3;

		const int s=0;
		const int e=parent->getGraphPanelWidth()-1;

		// x actually goes from the event's min-10 to max+10 incase part of some text needs be be redrawn (and we limit it to 0..height)
		const int maxX=min(parent->getGraphPanelWidth(),(ev->rect.w+ev->rect.x)+10);
		for(int x=max(0,(ev->rect.x)-10);x<=maxX;x++)
		{
			// this is, y=s+(((e-s)*t)/(N-1)) [0..N interpolated across s..e] solved for t, and then we get the remainder of the resulting division instead of the actual quotient)
			const double t=fmod((double)((x-s)*(N-1)),(double)(e-s));
			const int renderX=x+parent->vertRuler->getWidth();

			if(t<(N-1))
			{ // draw and label this tick
				dc.drawLine(renderX,getHeight()-2,renderX,getHeight()-10);
				//dc.drawLine(renderX+H_TICK_FREQ/2,getHeight()-2,renderX+H_TICK_FREQ/2,getHeight()-4);

				const string sValue=parent->getHorzValueString(parent->screenToNodeHorzValue(x,false));

				int offset=font->getTextWidth(sValue.c_str(),sValue.length()); // put text left of the tick mark

				dc.drawText(renderX-offset-1,font->getFontHeight()+2,sValue.c_str(),sValue.length());
			}
			// else, every 2 ticks (i.e. in between labled ticks)
				//dc.drawLine(getWidth()-2,renderY,getWidth()-5,renderY); // half way tick between labled ticks
		}

		return 0;
	}

protected:
	CHorzRuler() {}

private:
	FXGraphParamValue *parent;

	FXFont *font;

};

FXDEFMAP(CHorzRuler) CHorzRulerMap[]=
{
	//Message_Type				ID				Message_Handler
	FXMAPFUNC(SEL_PAINT,			0,				CHorzRuler::onPaint),
};

FXIMPLEMENT(CHorzRuler,FXHorizontalFrame,CHorzRulerMap,ARRAYNUMBER(CHorzRulerMap))



// --- declaration of CVertRuler -----------------------------------------------

class CVertRuler : public FXHorizontalFrame
{
	FXDECLARE(CVertRuler)
public:
	CVertRuler(FXComposite *p,FXGraphParamValue *_parent) :
		FXHorizontalFrame(p,FRAME_NONE | LAYOUT_FILL_Y | LAYOUT_FIX_WIDTH | LAYOUT_SIDE_LEFT, 0,0,42),

		parent(_parent),

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

	virtual ~CVertRuler()
	{
		delete font;
	}

	virtual void create()
	{
		FXHorizontalFrame::create();
		font->create();
	}

	// events I get by message

	long onPaint(FXObject *object,FXSelector sel,void *ptr)
	{
		FXHorizontalFrame::onPaint(object,sel,ptr);

		FXEvent *ev=(FXEvent*)ptr;

		FXDCWindow dc(this,ev);
		dc.setForeground(FXRGB(20,20,20));
#if REZ_FOX_VERSION<10117
		dc.setTextFont(font);
#else
		dc.setFont(font);
#endif

		#define V_TICK_FREQ 20

		// N is the number of ticks to label
		int N=(parent->getGraphPanelHeight()/V_TICK_FREQ); // we lable always an odd number of ticks so there is always a middle tick
		if(N%2==0)
			N++;

		// make sure there's at least 3
		if(N<3)
			N=3;

		const int s=0;
		const int e=parent->getGraphPanelHeight()-1;

		// y actually goes from the event's min-10 to max+10 incase part of some text needs be be redrawn (and we limit it to 0..height)
		const int maxY=min(parent->getGraphPanelHeight(),(ev->rect.h+ev->rect.y)+10);
		for(int y=max(0,(ev->rect.y)-10);y<=maxY;y++)
		{
			// this is, y=s+(((e-s)*t)/(N-1)) [0..N interpolated across s..e] solved for t, and then we get the remainder of the resulting division instead of the actual quotient)
			const double t=fmod((double)((y-s)*(N-1)),(double)(e-s));
			const int renderY=y+(((FXPacker *)(parent->graphCanvas->getParent()))->getPadTop());

			if(t<(N-1))
			{ // draw and label this tick
				dc.drawLine(getWidth()-2,renderY,getWidth()-10,renderY);

				const string s=parent->getVertValueString(parent->screenToNodeVertValue(y));
		
				int yoffset=font->getFontHeight(); // put text below the tick mark
				int xoffset=font->getTextWidth(s.c_str(),s.length());
				dc.drawText(getWidth()-xoffset-7,renderY+yoffset,s.c_str(),s.length());
			}
			// else, every 2 ticks (i.e. in between labled ticks)
				//dc.drawLine(getWidth()-2,renderY,getWidth()-5,renderY); // half way tick between labled ticks
		}

		return 0;
	}

protected:
	CVertRuler() {}

private:
	FXGraphParamValue *parent;

	FXFont *font;

};

FXDEFMAP(CVertRuler) CVertRulerMap[]=
{
	//Message_Type				ID				Message_Handler
	FXMAPFUNC(SEL_PAINT,			0,				CVertRuler::onPaint),
};

FXIMPLEMENT(CVertRuler,FXHorizontalFrame,CVertRulerMap,ARRAYNUMBER(CVertRulerMap))



// -----------------------------------------------

FXDEFMAP(FXGraphParamValue) FXGraphParamValueMap[]=
{
	//Message_Type				ID					Message_Handler

	FXMAPFUNC(SEL_PAINT,			FXGraphParamValue::ID_GRAPH_CANVAS,	FXGraphParamValue::onGraphPanelPaint),

	FXMAPFUNC(SEL_CONFIGURE,		FXGraphParamValue::ID_GRAPH_CANVAS,	FXGraphParamValue::onGraphPanelResize),

	FXMAPFUNC(SEL_LEFTBUTTONPRESS,		FXGraphParamValue::ID_GRAPH_CANVAS,	FXGraphParamValue::onCreateOrStartDragNode),
	FXMAPFUNC(SEL_MOTION,			FXGraphParamValue::ID_GRAPH_CANVAS,	FXGraphParamValue::onDragNode),
	FXMAPFUNC(SEL_LEFTBUTTONRELEASE,	FXGraphParamValue::ID_GRAPH_CANVAS,	FXGraphParamValue::onStopDragNode),
	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,	FXGraphParamValue::ID_GRAPH_CANVAS,	FXGraphParamValue::onDestroyNode),

	FXMAPFUNC(SEL_COMMAND,			FXGraphParamValue::ID_SCALAR_SPINNER,	FXGraphParamValue::onScalarSpinnerChange),
	FXMAPFUNC(SEL_COMMAND,			FXGraphParamValue::ID_CLEAR_BUTTON,	FXGraphParamValue::onPatternButton),

	FXMAPFUNC(SEL_CHANGED,			FXGraphParamValue::ID_HORZ_DEFORM_SLIDER,FXGraphParamValue::onHorzDeformSliderChange),
	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,	FXGraphParamValue::ID_HORZ_DEFORM_SLIDER,FXGraphParamValue::onHorzDeformSliderReset),
	FXMAPFUNC(SEL_CHANGED,			FXGraphParamValue::ID_VERT_DEFORM_SLIDER,FXGraphParamValue::onVertDeformSliderChange),
	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,	FXGraphParamValue::ID_VERT_DEFORM_SLIDER,FXGraphParamValue::onVertDeformSliderReset),
};

FXIMPLEMENT(FXGraphParamValue,FXVerticalFrame,FXGraphParamValueMap,ARRAYNUMBER(FXGraphParamValueMap))

FXGraphParamValue::FXGraphParamValue(const string _title,const int minScalar,const int maxScalar,const int _initScalar,FXComposite *p,int opts,int x,int y,int w,int h) :
	FXVerticalFrame(p,opts|FRAME_RAISED,x,y,w,h, 0,0,0,0, 0,0),

	title(_title),

	initScalar(_initScalar),

	graphPanel(new FXPacker(this,FRAME_NONE | LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 0,0)),
		vertDeformPanel(new FXVerticalFrame(graphPanel,FRAME_NONE | LAYOUT_SIDE_RIGHT|LAYOUT_FILL_Y, 0,0,0,0, 2,2,0,0, 0,0)),
			vertDeformSlider(new FXSlider(vertDeformPanel,this,ID_VERT_DEFORM_SLIDER, FRAME_NONE | LAYOUT_FILL_Y | SLIDER_VERTICAL|SLIDER_TICKS_LEFT|SLIDER_ARROW_LEFT, 0,0,0,0, 0,0,0,0)),
		horzRuler(new CHorzRuler(graphPanel,this)),
		vertRuler(new CVertRuler(graphPanel,this)),
		horzDeformPanel(new FXHorizontalFrame(graphPanel,FRAME_NONE | LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X, 0,0,0,0, 0,0,2,0, 0,0)),
			horzDeformSlider(new FXSlider(horzDeformPanel,this,ID_HORZ_DEFORM_SLIDER, FRAME_NONE | LAYOUT_FILL_X | SLIDER_HORIZONTAL|SLIDER_TICKS_TOP|SLIDER_ARROW_UP,0,0,0,0, 0,0,0,0)),
		graphCanvas(new FXCanvas(graphPanel,this,ID_GRAPH_CANVAS,LAYOUT_FILL_X|LAYOUT_FILL_Y)),
	
	statusPanel(new FXHorizontalFrame(this,FRAME_NONE | LAYOUT_FILL_X, 0,0,0,0, 0,0,0,0, 4,0)),
		horzValueLabel(new FXLabel(statusPanel,": ",NULL,LAYOUT_LEFT)),
		vertValueLabel(new FXLabel(statusPanel,": ",NULL,LAYOUT_LEFT)),

	buttonPanel(new FXHorizontalFrame(this,FRAME_NONE | LAYOUT_FILL_X, 0,0,0,0, 0,0,2,2)),
		scalarLabel(NULL),
		scalarSpinner(NULL),

	draggingNode(-1),
	dragOffsetX(0),dragOffsetY(0),

	sound(NULL),
	start(0),
	stop(0),

	horzAxisLabel(""),
	horzUnits(""),
	horzInterpretValue(NULL),
	horzUninterpretValue(NULL),

	vertAxisLabel(""),
	vertUnits(""),
	vertInterpretValue(NULL),
	vertUninterpretValue(NULL),

	backBuffer(new FXImage(getApp())),

	textFont(getApp()->getNormalFont())
{
	// create a smaller font to use 
        FXFontDesc d;
        textFont->getFontDesc(d);
        d.size-=10;
        textFont=new FXFont(getApp(),d);

	
	const int deformSliderRange=4000;

	horzDeformSlider->setRange(-deformSliderRange,deformSliderRange); // symmetry is assumed when the slider is used (and horz and very should be the same
	horzDeformSlider->setTickDelta((deformSliderRange*2+1)/(11-1)); // 11 tick marks
	horzDeformSlider->setValue(0);
	horzDeformSlider->setTipText("Horizontally Deform the Nodes Toward One Side or the Other\nRight-Click to Reset to the Middle");

	vertDeformSlider->setRange(-deformSliderRange,deformSliderRange); // symmetry is assumed when the slider is used (and horz and very should be the same
	vertDeformSlider->setTickDelta((deformSliderRange*2+1)/(11-1)); // 11 tick marks
	vertDeformSlider->setValue(0);
	vertDeformSlider->setTipText("Vertically Deform the Nodes Toward the Top or Bottom\nRight-Click to Reset to the Middle");



	if(minScalar!=maxScalar)
	{
		scalarLabel=new FXLabel(buttonPanel,"Scalar",NULL,LABEL_NORMAL|LAYOUT_CENTER_Y);
		scalarSpinner=new FXSpinner(buttonPanel,5,this,ID_SCALAR_SPINNER,SPIN_NORMAL|FRAME_SUNKEN|FRAME_THICK);
		scalarSpinner->setRange(minScalar,maxScalar);
		scalarSpinner->setValue(initScalar);
	}

	new FXButton(buttonPanel,"Clear",NULL,this,ID_CLEAR_BUTTON);

	nodes.reserve(100);

	backBuffer->create();

	clearNodes();

	setFontOfAllChildren(buttonPanel,textFont);
	setFontOfAllChildren(statusPanel,textFont);
}

FXGraphParamValue::~FXGraphParamValue()
{
	delete textFont;
}

// set the default w & h of this widget
FXint FXGraphParamValue::getDefaultWidth() { return 500; }
FXint FXGraphParamValue::getDefaultHeight() { return 300; }

void FXGraphParamValue::setSound(CSound *_sound,sample_pos_t _start,sample_pos_t _stop)
{
	sound=_sound;
	start=_start;
	stop=_stop;

	horzAxisLabel="Time";
	horzUnits="s";
	horzInterpretValue=NULL;
	horzUninterpretValue=NULL;

	// make sure that the back buffer re-renders
	onGraphPanelResize(NULL,0,NULL);

	clearStatus();
}

void FXGraphParamValue::setHorzParameters(const string horzAxisLabel,const string horzUnits,f_at_xs interpretValue,f_at_xs uninterpretValue)
{
	this->horzAxisLabel=horzAxisLabel;
	this->horzUnits=horzUnits;
	horzInterpretValue=interpretValue;
	horzUninterpretValue=uninterpretValue;

	clearStatus();
	updateNumbers();
}

void FXGraphParamValue::setVertParameters(const string vertAxisLabel,const string vertUnits,f_at_xs interpretValue,f_at_xs uninterpretValue)
{
	this->vertAxisLabel=vertAxisLabel;
	this->vertUnits=vertUnits;
	vertInterpretValue=interpretValue;
	vertUninterpretValue=uninterpretValue;

	clearStatus();
	updateNumbers();
}

void FXGraphParamValue::clearNodes()
{
	nodes.clear();

	// add endpoints
	CGraphParamValueNode first(0.0,0.5);
	nodes.push_back(first);

	CGraphParamValueNode last(1.0,0.5);
	nodes.push_back(last);

	graphCanvas->update();
}


long FXGraphParamValue::onScalarSpinnerChange(FXObject *sender,FXSelector sel,void *ptr)
{
	updateNumbers();
	return 1;
}

long FXGraphParamValue::onPatternButton(FXObject *sender,FXSelector sel,void *ptr)
{
	switch(FXSELID(sel))
	{
	case ID_CLEAR_BUTTON:
		horzDeformSlider->setValue(0);
		vertDeformSlider->setValue(0);
		clearNodes();
		break;

	}

	graphCanvas->update();
	return 1;
}

long FXGraphParamValue::onHorzDeformSliderChange(FXObject *sender,FXSelector sel,void *ptr)
{
	graphCanvas->update();
	return 1;
}

long FXGraphParamValue::onVertDeformSliderChange(FXObject *sender,FXSelector sel,void *ptr)
{
	graphCanvas->update();
	return 1;
}

long FXGraphParamValue::onHorzDeformSliderReset(FXObject *sender,FXSelector sel,void *ptr)
{
	horzDeformSlider->setValue(0);
	graphCanvas->update();
	return 1;
}

long FXGraphParamValue::onVertDeformSliderReset(FXObject *sender,FXSelector sel,void *ptr)
{
	vertDeformSlider->setValue(0);
	graphCanvas->update();
	return 1;
}


long FXGraphParamValue::onGraphPanelPaint(FXObject *sender,FXSelector sel,void *ptr)
{
	FXDCWindow dc(graphCanvas);

	// draw the whole background
	dc.drawImage(backBuffer,0,0);

	// draw the connection between the nodes
	dc.setForeground(FXRGB(255,64,64));
	//if(0)
	{ // draw lines
		for(size_t t=1;t<nodes.size();t++)
		{
			const CGraphParamValueNode &n1=nodes[t-1];
			const CGraphParamValueNode &n2=nodes[t];
			
			const int x1=nodeToScreenX(n1);
			const int y1=nodeToScreenY(n1);
			const int x2=nodeToScreenX(n2);
			const int y2=nodeToScreenY(n2);
			
			dc.drawLine(x1,y1,x2,y2);
		}
	}
/*
Okay, here i have coded cubic and cosine based curves to connect the dots instead of or in addition to
the straight line segments.  However, after playing around with with them a few issues arise.
1) Cosine always has a horizontal tangent at each node
2) Cubic curves have a couple of problems.
	- The top or bottom of a local maximum can extend above or below the screen
	- It's hard to control the concavity of each direction change by adding nodes without affecting the shape
A better overall solution may be to have a 'smooth' button for the line segments or to use bezier curves (except they
don't pass thru the control points

	else
	{ // draw a cubic curve (http://astronomy.swin.edu.au/~pbourke/curves/interpolation/)
		int prevX=nodeToScreenX(nodes[0]);
		int prevY=nodeToScreenY(nodes[0]);
		for(int t=1;t<(int)nodes.size();t++)
		{
			const CGraphParamValueNode &n0=nodes[max(0,t-2)];
			const CGraphParamValueNode &n1=nodes[max(0,t-1)];
			const CGraphParamValueNode &n2=nodes[t];
			const CGraphParamValueNode &n3=nodes[min((int)nodes.size()-1,t+1)];
			
			//const int x0=nodeToScreenX(n0);
			const int y0=nodeToScreenY(n0);
			const int x1=nodeToScreenX(n1);
			const int y1=nodeToScreenY(n1);
			const int x2=nodeToScreenX(n2);
			const int y2=nodeToScreenY(n2);
			//const int x3=nodeToScreenX(n3);
			const int y3=nodeToScreenY(n3);

			for(int x=x1;x<x2;x++)
			{
				const float mu=(float)(x-x1)/(float)(x2-x1);
				const float mu2=mu*mu;
				const float a0=y3-y2-y0+y1;
				const float a1=y0-y1-a0;
				const float a2=y2-y0;
				const float a3=y1;

				const float y=a0*mu*mu2+a1*mu2+a2*mu+a3;

				dc.drawLine(prevX,prevY,x,(int)y);
				prevX=x;
				prevY=(int)y;
			}
			
		}
	}
	else
	{ // draw a cosine interpolated curve (http://astronomy.swin.edu.au/~pbourke/curves/interpolation/)
		int prevX=nodeToScreenX(nodes[0]);
		int prevY=nodeToScreenY(nodes[0]);
		for(size_t t=1;t<nodes.size();t++)
		{
			const CGraphParamValueNode &n1=nodes[t-1];
			const CGraphParamValueNode &n2=nodes[t];
			
			const int x1=nodeToScreenX(n1);
			const int y1=nodeToScreenY(n1);
			const int x2=nodeToScreenX(n2);
			const int y2=nodeToScreenY(n2);
			
			for(int x=x1;x<x2;x++)
			{
				const float mu=(float)(x-x1)/(float)(x2-x1);
				const float mu2=(1.0-cos(mu*M_PI))/2.0;

				const float y=y1*(1.0-mu2)+y2*mu2;

				dc.drawLine(prevX,prevY,x,(int)y);
				prevX=x;
				prevY=(int)y;
			}
		}
	}
*/

	// draw the nodes
	for(size_t t=0;t<nodes.size();t++)
	{
		CGraphParamValueNode &n=nodes[t];

		const int x=nodeToScreenX(n);
		const int y=nodeToScreenY(n);
		
		if(t==0 || t==nodes.size()-1)
			dc.setForeground(FXRGB(0,191,255)); // end point node
		else
			dc.setForeground(FXRGB(0,255,0));

		dc.fillArc(x-NODE_RADIUS,y-NODE_RADIUS,NODE_RADIUS*2,NODE_RADIUS*2,0*64,360*64);
	}
		
	return 1;
}

#include "drawPortion.h"
long FXGraphParamValue::onGraphPanelResize(FXObject *sender,FXSelector sel,void *ptr)
{
	// render waveform to the backbuffer
	backBuffer->resize(graphCanvas->getWidth(),graphCanvas->getHeight());
	FXDCWindow dc(backBuffer);
	if(sound!=NULL)
	{
		const int canvasWidth=graphCanvas->getWidth();
		const int canvasHeight=graphCanvas->getHeight();
		const sample_pos_t length=stop-start+1;
		const double hScalar=(double)((sample_fpos_t)length/canvasWidth);
		const int hOffset=(int)(start/hScalar);
		const double vScalar=(65536.0/(double)canvasHeight)*(double)sound->getChannelCount();

		drawPortion(0,canvasWidth,&dc,sound,canvasWidth,canvasHeight,-1,-1,hScalar,hOffset,vScalar,0,true);
	}
	else
	{
		dc.setForeground(FXRGB(0,0,0));
		dc.setFillStyle(FILL_SOLID);
		dc.fillRectangle(0,0,graphCanvas->getWidth(),graphCanvas->getHeight());
	}

	// make the vertical deform slider's top and bottom appear at the top and bottom of the graph canvas
	int totalPad=graphCanvas->getParent()->getHeight()-graphCanvas->getHeight();
	vertDeformPanel->setPadTop(graphCanvas->getY());
	vertDeformPanel->setPadBottom(totalPad-graphCanvas->getY());

	return 1;
}

// always returns an odd value >=1
int FXGraphParamValue::getGraphPanelWidth() const
{
	int w=graphCanvas->getWidth();
	if(w%2==0)
		w--;
	return max(w,1);
}

// always returns an odd value >=1
int FXGraphParamValue::getGraphPanelHeight() const
{
	int h=graphCanvas->getHeight();
	if(h%2==0)
		h--;
	return max(h,1);
}

int FXGraphParamValue::insertIntoNodes(const CGraphParamValueNode &node)
{
	// sanity checks
	if(nodes.size()<2)
		throw runtime_error(string(__func__)+" -- nodes doesn't already contain at least 2 items");
	if(node.x<0.0 || node.x>1.0)
		throw runtime_error(string(__func__)+" -- node's x is out of range: "+istring(node.x));

	int insertedIndex=-1;

	// - basically, this function inserts the node into sorted order first by the nodes' postions

	// - insert into the first position to keep the node's position in non-desreasing order
	for(size_t t1=1;t1<nodes.size();t1++)
	{
		CGraphParamValueNode &temp=nodes[t1];
		if(temp.x>=node.x)
		{
			nodes.insert(nodes.begin()+t1,node);
			insertedIndex=t1;
			break;
		}
	}

	return insertedIndex;
}

long FXGraphParamValue::onCreateOrStartDragNode(FXObject *sender,FXSelector sel,void *ptr)
{
	FXEvent *ev=(FXEvent *)ptr;

	int nodeIndex=findNodeAt(ev->win_x,ev->win_y);
	if(nodeIndex==-1)
	{ // create a new node
		CGraphParamValueNode node(screenToNodeHorzValue(ev->win_x),screenToNodeVertValue(ev->win_y));
		draggingNode=insertIntoNodes(node);

		dragOffsetX=0;
		dragOffsetY=0;
	}
	else
	{
		dragOffsetX=ev->win_x-nodeToScreenX(nodes[nodeIndex]);
		dragOffsetY=ev->win_y-nodeToScreenY(nodes[nodeIndex]);
		draggingNode=nodeIndex;
	}
	
	updateStatus();
	graphCanvas->update();
	return 1;
}

long FXGraphParamValue::onDragNode(FXObject *sender,FXSelector sel,void *ptr)
{
	if(draggingNode!=-1)
	{
		FXEvent *ev=(FXEvent *)ptr;

		bool lockX=(draggingNode==0 || draggingNode==(int)nodes.size()-1) ? true : false; // lock X for endpoints
		bool lockY=false;

		// if shift or control is held, lock the x or y position
		if(ev->state&SHIFTMASK)
			lockX=true;
		else if(ev->state&CONTROLMASK)
			lockY=true;


		CGraphParamValueNode node=nodes[draggingNode];

		// calculate the new positions
		node.x=	lockX ? node.x : screenToNodeHorzValue(ev->win_x-dragOffsetX);
		node.y=	lockY ? node.y : screenToNodeVertValue(ev->win_y-dragOffsetY);

		// update the node within the vector
		if(draggingNode!=0 && draggingNode!=(int)nodes.size()-1)
		{ // not an end point
			// re-insert the node so that the nodes vector will be in the proper order
			nodes.erase(nodes.begin()+draggingNode);
			draggingNode=insertIntoNodes(node);
		}
		else
			nodes[draggingNode]=node;

		// redraw
		updateStatus();
		graphCanvas->update();
	}
	return 1;
}

long FXGraphParamValue::onStopDragNode(FXObject *sender,FXSelector sel,void *ptr)
{
	draggingNode=-1;
	clearStatus();
	return 1;
}

long FXGraphParamValue::onDestroyNode(FXObject *sender,FXSelector sel,void *ptr)
{
	FXEvent *ev=(FXEvent *)ptr;
	int nodeIndex=findNodeAt(ev->win_x,ev->win_y);
	if(nodeIndex!=-1 && nodeIndex!=0 && nodeIndex!=(int)(nodes.size()-1)) // not the first or the last node
	{
		nodes.erase(nodes.begin()+nodeIndex);
		draggingNode=-1;
		graphCanvas->update();
	}
	return 1;
}

int FXGraphParamValue::findNodeAt(int x,int y)
{
	for(size_t t=0;t<nodes.size();t++)
	{
		CGraphParamValueNode &n=nodes[t];
		
		const int nx=nodeToScreenX(n);
		const int ny=nodeToScreenY(n);

		// see if the node's position is <= the NODE_RADIUS from the give x,y position
		if( ((nx-x)*(nx-x)+(ny-y)*(ny-y)) <= (NODE_RADIUS*NODE_RADIUS) )
			return t;
	}

	return -1;
}

double FXGraphParamValue::deformNodeX(double x,FXint sliderValue) const
{
/*
	This deformation takes x:[0,1] and translates it to a deformation of x yet still with the range of [0,1]
	The deformation is to plug x into one quadrant of the unit circle given by the graph of f(x)=cos(asin(x)) where x:[0,1]
	Then I use the rule that cos(asin(x)) == sqrt(1-x^2)
	So, f(x) = sqrt(1-x^2)
	
	Then I let the slider define a variable, p, that adjusts how much from a straight line to a more and more taught curve
	to deform x by.  This comes from generalizing the f(x) above to:
		f(x) = ( 1-x^p ) ^ (1/p)  (that is, p takes the place of '2' in the previous f(x))

	So, now p can go from 1 (where f(x) produces the line, y=x) and upwards where elbow of the curve is more and more and more bent toward (1,0)
	
	So, the x coord of the node plugged into this f(x) where s==1 does no deforming and where s is greater, the x is more bent in one direction

	So the slider going from [-N,N] is scaled to s: [-1,1] and when s>=0 I let s define p as p=(K*s)+1
	But when s<0 I let be defined as p=(K*(-s))+1, but I flip the f(x) equation horizontally and vertically so that the bend tends toward (0,1)
	This way, the slider deforms the nodes' x values leftwards or rightwards depending on the slider's value being - or +

	And as a side note: I square s (but maintain the sign) so that more values in the middle will be available but s is will [-1,1] 
	
*/

	FXint lo,hi; // assuming symmetry on low and hi and vert and horz are the same
	horzDeformSlider->getRange(lo,hi);

		// s=[-1,1]
	double s=(double)sliderValue/(double)hi;
	s=fabs(s)*s;

	if(s>=0)
	{
		double p=(10.0*s)+1.0;
		return pow( 1.0-pow(1.0-x,p) , 1.0/p );
	}
	else
	{
		double p=(10.0*(-s))+1.0;
		return 1.0-pow( 1.0-pow(x,p) , 1.0/p );
	}

}

double FXGraphParamValue::undeformNodeX(double x,FXint sliderValue) const
{
	/* this computes the inverse of the previous deformNodeX method above */
	/* which simplifies to just calling deformNodeX with the negated sliderValue */
	return deformNodeX(x,-sliderValue);
}

int FXGraphParamValue::nodeToScreenX(const CGraphParamValueNode &node)
{
	return (int)(deformNodeX(node.x,horzDeformSlider->getValue())*(graphCanvas->getWidth()-1));
}

int FXGraphParamValue::nodeToScreenY(const CGraphParamValueNode &node)
{
	return (int)((1.0-deformNodeX(node.y,vertDeformSlider->getValue()))*(getGraphPanelHeight()));
}

double FXGraphParamValue::screenToNodeHorzValue(int x,bool undeform)
{
	double v=((double)x/(double)(graphCanvas->getWidth()-1));
	if(v<0.0)
		v=0.0;
	else if(v>1.0)
		v=1.0;

	if(undeform)
		return undeformNodeX(v,horzDeformSlider->getValue());
	else
		return v;
}

double FXGraphParamValue::screenToNodeVertValue(int y,bool undeform)
{
	double v=(1.0-((double)y/(double)(getGraphPanelHeight()-1)));
	if(v<0.0)
		v=0.0;
	else if(v>1.0)
		v=1.0;

	if(undeform)
		return undeformNodeX(v,vertDeformSlider->getValue());
	else
		return v;
}

void FXGraphParamValue::updateNumbers()
{
	horzRuler->update();
	vertRuler->update();
	updateStatus();
}

const CGraphParamValueNodeList &FXGraphParamValue::getNodes() const
{
	retNodes=nodes;
	for(size_t t=0;t<retNodes.size();t++)
	{
		retNodes[t].x=deformNodeX(retNodes[t].x,horzDeformSlider->getValue());
		retNodes[t].y=deformNodeX(retNodes[t].y,vertDeformSlider->getValue());

		if(horzInterpretValue!=NULL)
			retNodes[t].x=horzInterpretValue(retNodes[t].x,0);
		if(vertInterpretValue!=NULL)
			retNodes[t].y=vertInterpretValue(retNodes[t].y,getScalar());
	}

	return retNodes;
}

const string FXGraphParamValue::getVertValueString(double vertValue) const
{
	return istring(vertInterpretValue(vertValue,getScalar()),5,3);
}

const string FXGraphParamValue::getHorzValueString(double horzValue) const
{
	if(sound==NULL)
		return istring(horzInterpretValue(horzValue,0),5,3);
	else
		return sound->getTimePosition((sample_pos_t)((sample_fpos_t)(stop-start+1)*horzValue+start),3,false);
}


void FXGraphParamValue::updateStatus()
{
	if(draggingNode==-1)
		return;

	const CGraphParamValueNode &n=nodes[draggingNode];

	// draw status
	horzValueLabel->setText((horzAxisLabel+": "+getHorzValueString(deformNodeX(n.x,horzDeformSlider->getValue()))+horzUnits).c_str());
	vertValueLabel->setText((vertAxisLabel+": "+getVertValueString(deformNodeX(n.y,vertDeformSlider->getValue()))+vertUnits).c_str());
}

void FXGraphParamValue::clearStatus()
{
	horzValueLabel->setText((horzAxisLabel+": #"+horzUnits).c_str());
	vertValueLabel->setText((vertAxisLabel+": #"+vertUnits).c_str());
}
	

const int FXGraphParamValue::getScalar() const
{
	return scalarSpinner==NULL ? 0 : scalarSpinner->getValue();
}

void FXGraphParamValue::setScalar(const int scalar)
{
	if(scalarSpinner!=NULL)
		scalarSpinner->setValue(scalar);
}

const int FXGraphParamValue::getMinScalar() const
{
	FXint lo=0,hi=0;
	if(scalarSpinner!=NULL)
		scalarSpinner->getRange(lo,hi);
	return lo;
}

const int FXGraphParamValue::getMaxScalar() const
{
	FXint lo=0,hi=0;
	if(scalarSpinner!=NULL)
		scalarSpinner->getRange(lo,hi);
	return hi;
}

const string FXGraphParamValue::getTitle() const
{
	return title;
}

void FXGraphParamValue::readFromFile(const string &prefix,CNestedDataFile *f)
{
	const string key=prefix+DOT+getTitle()+DOT;

	if(f->keyExists((key+"scalar").c_str()))
	{
		const string s=f->getValue((key+"scalar").c_str());
		setScalar(atoi(s.c_str()));
	}
	else	
		setScalar(initScalar);

	nodes.clear();

	const string k1=key+"node_positions";
	if(f->keyExists(k1.c_str()))
	{
		const string k2=key+"node_values";
		const size_t count=f->getArraySize(k1.c_str());
		// ??? I could either save the node x and values as [0,1], or I could use save the actual values ( <-- currently)
		for(size_t t=0;t<count;t++)
			nodes.push_back(CGraphParamValueNode(atof(f->getArrayValue(k1.c_str(),t).c_str()),atof(f->getArrayValue(k2.c_str(),t).c_str())));
	}
	else
		clearNodes();

	updateNumbers();
	update();
	graphCanvas->update();
}

void FXGraphParamValue::writeToFile(const string &prefix,CNestedDataFile *f) const
{
	const string key=prefix+DOT+getTitle()+DOT;

	if(getMinScalar()!=getMaxScalar())
		f->createKey((key+"scalar").c_str(),istring(getScalar()));

	const string k1=key+"node_positions";
		// ??? I could implement a createArrayKey which takes a double so I wouldn't have to convert to string here
	f->removeKey(k1.c_str());
	for(size_t t=0;t<nodes.size();t++)
		f->createArrayKey(k1.c_str(),t,istring(nodes[t].x).c_str());

	const string k2=key+"node_values";
	f->removeKey(k2.c_str());
	for(size_t t=0;t<nodes.size();t++)
		f->createArrayKey(k2.c_str(),t,istring(nodes[t].y).c_str());

	if(getMinScalar()!=getMaxScalar())
		f->createKey((key+"scalar").c_str(),istring(getScalar()));
}


