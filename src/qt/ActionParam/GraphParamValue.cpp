/* 
 * Copyright (C) 2007 - David W. Durham
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

#include "GraphParamValue.h"

#include <exception>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <math.h>

#include <istring>

#include <CNestedDataFile/CNestedDataFile.h>

#include "../../backend/CSound.h"

#include "../settings.h"

#include <QApplication>
#include <QPainter>
#include <QImage>
#include <QResizeEvent>
#include <QMouseEvent>

#define NODE_RADIUS 4

/* TODO:
 *
 *	- I probably need to limit the minimum size as it could cause divide by zero errors, and really be unusable anyway
 *
 * 	- I would have liked to reuse code (without copy/pasting) from CSoundWindow, but it's currently
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


GraphParamValue::GraphParamValue(const char *_name) :
	name(_name),

	draggingNode(-1),
	dragOffsetX(0),dragOffsetY(0),

	sound(NULL),
	start(0),
	stop(0),

	horzAxisLabel(""),
	horzUnits(""),
	horzValueMapper(NULL),

	vertAxisLabel(""),
	vertUnits(""),
	vertValueMapper(NULL),

	background(new QImage),
	hScalar(1.0),
	hOffset(0)
{
	setupUi(this);

	horzDeformSlider->installEventFilter(this);
	vertDeformSlider->installEventFilter(this);
	graphCanvas->installEventFilter(this);
	horzRuler->installEventFilter(this);
	vertRuler->installEventFilter(this);

	connect(horzDeformSlider,SIGNAL(valueChanged(int)),graphCanvas,SLOT(update()));
	connect(vertDeformSlider,SIGNAL(valueChanged(int)),graphCanvas,SLOT(update()));


	rulerFont=horzRuler->font();
	rulerFont.setPixelSize(10);

	horzRuler->setFixedHeight(QFontMetrics(rulerFont).height()+CUE_RADIUS*2+2);

	
	const int deformSliderRange=4000;

	horzDeformSlider->setRange(-deformSliderRange,deformSliderRange); // symmetry is assumed when the slider is used (and horz and very should be the same
	horzDeformSlider->setTickInterval((deformSliderRange*2+1)/(11-1)); // 11 tick marks
	horzDeformSlider->setValue(0);

	vertDeformSlider->setRange(-deformSliderRange,deformSliderRange); // symmetry is assumed when the slider is used (and horz and very should be the same
	vertDeformSlider->setTickInterval((deformSliderRange*2+1)/(11-1)); // 11 tick marks
	vertDeformSlider->setValue(0);



	clearButton->setIcon(loadIcon("graph_clear.png"));
	horzFlipButton->setIcon(loadIcon("graph_horz_flip.png"));
	vertFlipButton->setIcon(loadIcon("graph_vert_flip.png"));
	smoothButton->setIcon(loadIcon("graph_smooth.png"));

	nodes.reserve(100);

	clearNodes();
	clearUndo();
}

GraphParamValue::~GraphParamValue()
{
}

void GraphParamValue::setSound(CSound *_sound,sample_pos_t _start,sample_pos_t _stop)
{
	sound=_sound;
	start=_start;
	stop=_stop;

	horzAxisLabel=_("Time");
	horzUnits=_("s"); // seconds

	// make sure that the back buffer re-renders
	QResizeEvent e(graphCanvas->size(),graphCanvas->size());
	onGraphCanvasResize(&e);

	clearStatus();
}

void GraphParamValue::setHorzParameters(const string horzAxisLabel,const string horzUnits,AActionParamMapper *_horzValueMapper)
{
	this->horzAxisLabel=horzAxisLabel;
	this->horzUnits=horzUnits;
	horzValueMapper=_horzValueMapper;

	clearStatus();
	updateNumbers();
}

void GraphParamValue::setVertParameters(const string vertAxisLabel,const string vertUnits,AActionParamMapper *_vertValueMapper)
{
	this->vertAxisLabel=vertAxisLabel;
	this->vertUnits=vertUnits;
	vertValueMapper=_vertValueMapper;

printf("setting range: %d %d\nsetting defaut: %d\n",vertValueMapper->getMinScalar(),vertValueMapper->getMaxScalar(),vertValueMapper->getDefaultScalar());
	scalarSpinner->setRange(vertValueMapper->getMinScalar(),vertValueMapper->getMaxScalar());
	setScalar(vertValueMapper->getDefaultScalar());

	scalarLabel->setVisible(vertValueMapper->getMinScalar()!=vertValueMapper->getMaxScalar());
	scalarSpinner->setVisible(vertValueMapper->getMinScalar()!=vertValueMapper->getMaxScalar());

	clearStatus();
	updateNumbers();
}

void GraphParamValue::clearNodes()
{
	nodes.clear();

	// add endpoints
	CGraphParamValueNode first(0.0,vertValueMapper==NULL ? 0.5 : vertValueMapper->uninterpretValue(vertValueMapper->getDefaultValue()));
	nodes.push_back(first);

	CGraphParamValueNode last(1.0,vertValueMapper==NULL ? 0.5 : vertValueMapper->uninterpretValue(vertValueMapper->getDefaultValue()));
	nodes.push_back(last);

	graphCanvas->update();
}


void GraphParamValue::on_scalarSpinner_valueChanged(int v)
{
	setScalar(v);
	pushUndo();
}


	// ??? need a button that can change bandpass to notch
void GraphParamValue::on_clearButton_clicked()
{
	horzDeformSlider->setValue(0);
	vertDeformSlider->setValue(0);
	clearNodes();
	graphCanvas->update();
	updateNumbers();

	pushUndo();
}

void GraphParamValue::on_horzFlipButton_clicked()
{
	horzDeformSlider->setValue(-horzDeformSlider->value());
	nodes=flipGraphNodesHorizontally(nodes);
	graphCanvas->update();
	updateNumbers();

	pushUndo();
}

void GraphParamValue::on_vertFlipButton_clicked()
{
	vertDeformSlider->setValue(-vertDeformSlider->value());
	nodes=flipGraphNodesVertically(nodes);
	graphCanvas->update();
	updateNumbers();

	pushUndo();
}

void GraphParamValue::on_smoothButton_clicked()
{
	nodes=smoothGraphNodes(nodes);
	graphCanvas->update();
	updateNumbers();

	pushUndo();
}

void GraphParamValue::on_undoButton_clicked()
{
	popUndo();
}

void GraphParamValue::on_redoButton_clicked()
{
	popRedo();
}

// these are used to save an undo state on deform only when the user has stopped moving the slider
// however, it does not handle any keyboard modifications to the slider since they happen with the key repeat.. I would want to pushUndo() only when the user released a key (physically, not auto repeat)
void GraphParamValue::on_vertDeformSlider_sliderReleased()
{
	pushUndo();
}

void GraphParamValue::on_horzDeformSlider_sliderReleased()
{
	pushUndo();
}

const int GraphParamValue::getCueScreenX(size_t cueIndex) const
{
	sample_fpos_t X=((sample_fpos_t)sound->getCueTime(cueIndex)/hScalar-hOffset);
	if(X>=(-CUE_RADIUS-1) && X<(background->width()+CUE_RADIUS+1)) // ??? not sure about this call to width() it was just "width" in the old code
		return (int)sample_fpos_round(X);
	else
		return CUE_OFF_SCREEN;
}

const sample_pos_t GraphParamValue::getSamplePosForScreenX(int X) const
{
	sample_fpos_t p=sample_fpos_floor((X+(sample_fpos_t)hOffset)*hScalar);
	if(p<0)
		p=0.0;
	else if(p>=sound->getLength())
		p=sound->getLength()-1;

	return (sample_pos_t)p;
}

bool GraphParamValue::eventFilter(QObject *w,QEvent *e)
{
	if(w==horzDeformSlider && e->type()==QEvent::MouseButtonPress && ((QMouseEvent*)e)->button()==Qt::RightButton)
	{
		horzDeformSlider->setValue(0);
		graphCanvas->update();
		updateNumbers();

		pushUndo();
		return true;
	}
	else if(w==vertDeformSlider && e->type()==QEvent::MouseButtonPress && ((QMouseEvent*)e)->button()==Qt::RightButton)
	{
		vertDeformSlider->setValue(0);
		graphCanvas->update();
		updateNumbers();

		pushUndo();
		return true;
	}
	else if(w==graphCanvas)
	{
		if(e->type()==QEvent::Paint)
		{
			onGraphCanvasPaint((QPaintEvent*)e);
			return true;
		}
		else if(e->type()==QEvent::Resize)
		{
			onGraphCanvasResize((QResizeEvent *)e);
			// no return
		}
		else if(e->type()==QEvent::MouseButtonPress && ((QMouseEvent*)e)->button()==Qt::LeftButton)
		{
			onCreateOrStartDragNode((QMouseEvent*)e);
			return true;
		}
		else if(e->type()==QEvent::MouseMove)
		{
			onDragNode((QMouseEvent*)e);
			return true;
		}
		else if(e->type()==QEvent::MouseButtonRelease && ((QMouseEvent*)e)->button()==Qt::LeftButton)
		{
			onStopDragNode((QMouseEvent*)e);
			return true;
		}
		else if(e->type()==QEvent::MouseButtonRelease && ((QMouseEvent*)e)->button()==Qt::RightButton)
		{
			onDestroyNode((QMouseEvent*)e);
			return true;
		}
	}
	else if(w==horzRuler && e->type()==QEvent::Paint)
	{
		if(sound)
			drawWaveRuler(this,horzRuler,(QPaintEvent *)e,vertRuler->width(),horzRuler->width(),rulerFont,horzRuler->palette(),sound,false,0,0);
		else
			drawHorzRuler((QPaintEvent *)e);
	}
	else if(w==vertRuler && e->type()==QEvent::Paint)
	{
		drawVertRuler((QPaintEvent *)e);
	}

	return QWidget::eventFilter(w,e);
}

void GraphParamValue::drawHorzRuler(QPaintEvent *ev)
{
	QPainter p(horzRuler);

	p.setFont(rulerFont);
	const QFontMetrics rulerFontMetrics(rulerFont);

	p.setPen(horzRuler->palette().color(QPalette::Text));

	#define H_TICK_FREQ 50

	// N is the number of ticks to label
	int N=(horzRuler->width()/H_TICK_FREQ); // we label always an odd number of ticks so there is always a middle tick
	if(N%2==0)
		N++;

	// make sure there's at least 3
	if(N<3)
		N=3;

	const int s=0;
	const int e=horzRuler->width()-1;

	// x actually goes from the event's min-10 to max+10 in case part of some text needs be be redrawn (and we limit it to 0..height)
	const int maxX=min(horzRuler->width(),(ev->rect().width()+ev->rect().x())+10);
	for(int x=max(0,(ev->rect().x())-10);x<=maxX;x++)
	{
		// this is, y=s+(((e-s)*t)/(N-1)) [0..N interpolated across s..e] solved for t, and then we get the remainder of the resulting division instead of the actual quotient)
		const double t=fmod((double)((x-s)*(N-1)),(double)(e-s));
		const int renderX=x+vertRuler->width();

		if(t<(N-1))
		{ // draw and label this tick
			p.drawLine(renderX,horzRuler->height()-2,renderX,horzRuler->height()-10);
			//p.drawLine(renderX+H_TICK_FREQ/2,horzRuler->height()-2,renderX+H_TICK_FREQ/2,horzRuler->height()-4);

			const string sValue=getHorzValueString(screenToNodeHorzValue(x,false));

			const QRect size(rulerFontMetrics.boundingRect(sValue.c_str()));
			int yoffset=size.height();
			int xoffset=size.width(); // put text left of the tick mark

			p.drawText(renderX-xoffset-1,yoffset-4/*have to subtract 4 to make the text hug the tick marks*/+2,sValue.c_str());
		}
		// else, every 2 ticks (i.e. in between labeled ticks)
			//p.drawLine(getWidth()-2,renderY,getWidth()-5,renderY); // half way tick between labeled ticks
	}
}

void GraphParamValue::onGraphCanvasPaint(QPaintEvent *e)
{
	QPainter p(graphCanvas);

	// draw the whole background
	p.drawImage(0,0,*background);

	// draw the connection between the nodes
	p.setPen(QColor(255,64,64));
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
			
			p.drawLine(x1,y1,x2,y2);
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
	{ // draw a cubic curve (http://astronomy.swin.edu.au/~pbourke/analysis/interpolation/index.html)
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

				p.drawLine(prevX,prevY,x,(int)y);
				prevX=x;
				prevY=(int)y;
			}
			
		}
	}
	else
	{ // draw a cosine interpolated curve (http://astronomy.swin.edu.au/~pbourke/analysis/interpolation/index.html)
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

				p.drawLine(prevX,prevY,x,(int)y);
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
		{
			// end point node
			p.setPen(QColor(0,191,255)); 
			p.setBrush(QColor(0,191,255)); 
		}
		else if(t==1/*only need to set after first iteration*/)
		{
			p.setPen(QColor(0,255,0));
			p.setBrush(QColor(0,255,0));
		}

		p.drawEllipse(x-NODE_RADIUS,y-NODE_RADIUS,NODE_RADIUS*2,NODE_RADIUS*2);
	}
}

void GraphParamValue::drawVertRuler(QPaintEvent *ev)
{
	QPainter p(vertRuler);
	p.setFont(rulerFont);

	p.setPen(vertRuler->palette().color(QPalette::Text));

	const QFontMetrics rulerFontMetrics(rulerFont);
	const int height=graphCanvas->height();

	#define V_TICK_FREQ 20

	// N is the number of ticks to label
	int N=(height/V_TICK_FREQ); // we label always an odd number of ticks so there is always a middle tick
	if(N%2==0)
		N++;

	// make sure there's at least 3
	if(N<3)
		N=3;

	const int s=0;
	const int e=height-1;

	// y actually goes from the event's min-10 to max+10 incase part of some text needs be be redrawn (and we limit it to 0..height)
	const int maxY=min(height,(ev->rect().height()+ev->rect().y())+10);
	for(int y=max(0,(ev->rect().y())-10);y<=maxY;y++)
	{
		// this is, y=s+(((e-s)*t)/(N-1)) [0..N interpolated across s..e] solved for t, and then we get the remainder of the resulting division instead of the actual quotient)
		const double t=fmod((double)((y-s)*(N-1)),(double)(e-s));
		const int renderY=y;

		if(t<(N-1))
		{ // draw and label this tick
			p.drawLine(vertRuler->width()-2,renderY,vertRuler->width()-10,renderY);

			const string s=getVertValueString(screenToNodeVertValue(y,false));
	
			const QRect size(rulerFontMetrics.boundingRect(s.c_str()));
			int yoffset=size.height(); // put text below the tick mark
			int xoffset=size.width();
			p.drawText(vertRuler->width()-xoffset-4,renderY+yoffset-4/*have to subtract 4 to make the text hug the tick marks*/,s.c_str());

			// make sure the ruler is wide enough
			if(vertRuler->width()<(xoffset+4))
				vertRuler->setFixedWidth(xoffset+4);
		}
		// else, every 2 ticks (i.e. in between labeled ticks)
			//p.drawLine(vertRuler->width()-2,renderY,vertRuler->width()-5,renderY); // half way tick between labeled ticks
	}
}

#include "drawPortion.h"
void GraphParamValue::onGraphCanvasResize(QResizeEvent *e)
{
	// render waveform to the backbuffer
	delete background;
	background=new QImage(graphCanvas->size(),QImage::Format_RGB32);
	QPainter p(background);
	if(sound!=NULL)
	{
		const int canvasWidth=e->size().width();
		const int canvasHeight=e->size().height();
		const sample_pos_t length=stop-start+1;
		hScalar=(double)((sample_fpos_t)length/canvasWidth);
		hOffset=(sample_pos_t)(start/hScalar);
		const float vScalar=MAX_WAVE_HEIGHT/(float)canvasHeight;

		drawPortion(0,canvasWidth,&p,sound,canvasWidth,canvasHeight,-1,-1,hScalar,hOffset,vScalar,0,true);

		if(gDrawVerticalCuePositions)
		{	// draw cue positions as inverted colors

			// calculate the min and max times based on the left and right boundries of the drawing
			const sample_pos_t minTime=getSamplePosForScreenX(0);
			const sample_pos_t maxTime=getSamplePosForScreenX(canvasWidth+1);

			/* ??? if I iterated over the cues by increasing time, then I could be more efficient by finding the neared cue to minTime and stop when a cue is greater than maxTime */
			const size_t cueCount=sound->getCueCount();
			for(size_t t=0;t<cueCount;t++)
			{
				const sample_pos_t cueTime=sound->getCueTime(t);
				if(cueTime>=minTime && cueTime<=maxTime)
				{
					const int x=getCueScreenX(t);
					drawPortion(x,1,&p,sound,canvasWidth,canvasHeight,-1,-1,hScalar,hOffset,vScalar,0,true,true);
				}
			}
		}
	}
	else
		p.fillRect(0,0,e->size().width(),e->size().height(),QColor(0,0,0));

	/*
	// make the vertical deform slider's top and bottom appear at the top and bottom of the graph canvas
	int totalPad=graphCanvas->parentWidget()->height()-graphCanvas->height();
	vertDeformPanel->setPadTop(graphCanvas->getY());
	vertDeformPanel->setPadBottom(totalPad-graphCanvas->getY());
	*/
}

// always returns an odd value >=1
int GraphParamValue::getGraphPanelWidth() const
{
	int w=graphCanvas->width();
	if(w%2==0)
		w--;
	return max(w,1);
}

// always returns an odd value >=1
int GraphParamValue::getGraphPanelHeight() const
{
	int h=graphCanvas->height();
	if(h%2==0)
		h--;
	return max(h,1);
}

int GraphParamValue::insertIntoNodes(const CGraphParamValueNode &node)
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

void GraphParamValue::onCreateOrStartDragNode(QMouseEvent *ev)
{
	int nodeIndex=findNodeAt(ev->x(),ev->y());
	if(nodeIndex==-1)
	{ // create a new node
		CGraphParamValueNode node(screenToNodeHorzValue(ev->x()),screenToNodeVertValue(ev->y()));
		draggingNode=insertIntoNodes(node);

		dragOffsetX=0;
		dragOffsetY=0;
	}
	else
	{
		dragOffsetX=ev->x()-nodeToScreenX(nodes[nodeIndex]);
		dragOffsetY=ev->y()-nodeToScreenY(nodes[nodeIndex]);
		draggingNode=nodeIndex;
	}
	
	updateStatus();
	graphCanvas->update();

	pushUndo();
}

void GraphParamValue::onDragNode(QMouseEvent *ev)
{
	if(draggingNode!=-1)
	{
		bool lockX=(draggingNode==0 || draggingNode==(int)nodes.size()-1) ? true : false; // lock X for endpoints
		bool lockY=false;

		// if shift or control is held, lock the x or y position
		if(QApplication::keyboardModifiers() & Qt::ShiftModifier)
			lockX=true;
		if(QApplication::keyboardModifiers() & Qt::ControlModifier)
			lockY=true;


		CGraphParamValueNode node=nodes[draggingNode];

		// calculate the new positions
		node.x=	lockX ? node.x : screenToNodeHorzValue(ev->x()-dragOffsetX);
		node.y=	lockY ? node.y : screenToNodeVertValue(ev->y()-dragOffsetY);

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
}

void GraphParamValue::onStopDragNode(QMouseEvent *e)
{
	draggingNode=-1;
	clearStatus();
}

void GraphParamValue::onDestroyNode(QMouseEvent *ev)
{
	int nodeIndex=findNodeAt(ev->x(),ev->y());
	if(nodeIndex!=-1 && nodeIndex!=0 && nodeIndex!=(int)(nodes.size()-1)) // not the first or the last node
	{
		nodes.erase(nodes.begin()+nodeIndex);
		draggingNode=-1;
		graphCanvas->update();
		updateNumbers();

		pushUndo();
	}
}

int GraphParamValue::findNodeAt(int x,int y)
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

double GraphParamValue::deformNodeX(double x,int sliderValue) const
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

	// assuming symmetry on low and hi and vert and horz are the same
	int lo=horzDeformSlider->minimum();
	int hi=horzDeformSlider->maximum(); 

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

double GraphParamValue::undeformNodeX(double x,int sliderValue) const
{
	/* this computes the inverse of the previous deformNodeX method above */
	/* which simplifies to just calling deformNodeX with the negated sliderValue */
	return deformNodeX(x,-sliderValue);
}

int GraphParamValue::nodeToScreenX(const CGraphParamValueNode &node)
{
	return (int)(deformNodeX(node.x,horzDeformSlider->value())*(graphCanvas->width()-1));
}

int GraphParamValue::nodeToScreenY(const CGraphParamValueNode &node)
{
	return (int)((1.0-deformNodeX(node.y,vertDeformSlider->value()))*(getGraphPanelHeight()));
}

double GraphParamValue::screenToNodeHorzValue(int x,bool undeform)
{
	double v=((double)x/(double)(graphCanvas->width()-1));
	if(v<0.0)
		v=0.0;
	else if(v>1.0)
		v=1.0;

	if(undeform)
		return undeformNodeX(v,horzDeformSlider->value());
	else
		return v;
}

double GraphParamValue::screenToNodeVertValue(int y,bool undeform)
{
	double v=(1.0-((double)y/(double)(getGraphPanelHeight()-1)));
	if(v<0.0)
		v=0.0;
	else if(v>1.0)
		v=1.0;

	if(undeform)
		return undeformNodeX(v,vertDeformSlider->value());
	else
		return v;
}

void GraphParamValue::updateNumbers()
{
	horzRuler->update();
	vertRuler->update();
	updateStatus();
}

const CGraphParamValueNodeList &GraphParamValue::getNodes() const
{
	retNodes=nodes;
	for(size_t t=0;t<retNodes.size();t++)
	{
		retNodes[t].x=deformNodeX(retNodes[t].x,horzDeformSlider->value());
		retNodes[t].y=deformNodeX(retNodes[t].y,vertDeformSlider->value());

		if(horzValueMapper!=NULL)
			retNodes[t].x=horzValueMapper->interpretValue(retNodes[t].x);
		if(vertValueMapper!=NULL)
			retNodes[t].y=vertValueMapper->interpretValue(retNodes[t].y);
	}

	return retNodes;
}

/*
 * only copies nodes and deformation parameters and scalars, but not interpret functions and such
 */
void GraphParamValue::copyFrom(const GraphParamValue *src)
{
	// make undo stack copy when implemented ???
	
	// copy deformation values
	horzDeformSlider->setValue(src->horzDeformSlider->value());
	vertDeformSlider->setValue(src->vertDeformSlider->value());

	// copy scalar values
	setScalar(src->getScalar());

	// copy nodes
	nodes.clear();
	for(size_t t=0;t<src->nodes.size();t++)
		nodes.push_back(src->nodes[t]);

	// cause everything to refresh
	updateNumbers();
	update();
	graphCanvas->update();
}

void GraphParamValue::swapWith(GraphParamValue *src)
{
	int t;

	// swap deformation values
	t=horzDeformSlider->value();
	horzDeformSlider->setValue(src->horzDeformSlider->value());
	src->horzDeformSlider->setValue(t);

	t=vertDeformSlider->value();
	vertDeformSlider->setValue(src->vertDeformSlider->value());
	src->vertDeformSlider->setValue(t);


	// copy scalar values
	t=getScalar();
	setScalar(src->getScalar());
	src->setScalar(t);

	// copy nodes
	swap(nodes,src->nodes);

	// cause everything to refresh
	updateNumbers();
	update();
	graphCanvas->update();

	src->updateNumbers();
	src->update();
	src->graphCanvas->update();

	pushUndo();
	src->pushUndo();
}

const string GraphParamValue::getVertValueString(double vertValue) const
{
	return istring(vertValueMapper->interpretValue(vertValue),5,3);
}

const string GraphParamValue::getHorzValueString(double horzValue) const
{
	if(sound==NULL)
		return istring(horzValueMapper->interpretValue(horzValue),5,3);
	else
		return sound->getTimePosition((sample_pos_t)((sample_fpos_t)(stop-start+1)*horzValue+start),3,false);
}


void GraphParamValue::updateStatus()
{
	pointCountLabel->setText((istring(nodes.size())+" "+_("points")).c_str());

	if(draggingNode==-1)
		return;

	const CGraphParamValueNode &n=nodes[draggingNode];

	// draw status
	horzValueLabel->setText((horzAxisLabel+": "+getHorzValueString(deformNodeX(n.x,horzDeformSlider->value()))+horzUnits).c_str());
	vertValueLabel->setText((vertAxisLabel+": "+getVertValueString(deformNodeX(n.y,vertDeformSlider->value()))+vertUnits).c_str());
}

void GraphParamValue::clearStatus()
{
	horzValueLabel->setText((horzAxisLabel+": #"+horzUnits).c_str());
	vertValueLabel->setText((vertAxisLabel+": #"+vertUnits).c_str());
	pointCountLabel->setText((istring(nodes.size())+" "+_("points")).c_str());
}
	

const int GraphParamValue::getScalar() const
{
	return scalarSpinner->isHidden() ? vertValueMapper->getDefaultScalar() : vertValueMapper->getScalar();
}

void GraphParamValue::setScalar(const int scalar)
{
	prevScalar=scalar;
	vertValueMapper->setScalar(scalar);
	scalarSpinner->blockSignals(true); // no need to fire signal, and it would pushUndo if we did
	/*if(!scalarSpinner->isHidden())*/ scalarSpinner->setValue(vertValueMapper->getScalar());
	scalarSpinner->blockSignals(false);
	updateNumbers();
}

const int GraphParamValue::getMinScalar() const
{
	/*
	int lo=0,hi=0;
	if(scalarSpinner!=NULL)
		scalarSpinner->getRange(lo,hi);
	return lo;
	*/
	return vertValueMapper->getMinScalar();
}

const int GraphParamValue::getMaxScalar() const
{
	/*
	int lo=0,hi=0;
	if(scalarSpinner!=NULL)
		scalarSpinner->getRange(lo,hi);
	return hi;
	*/
	return vertValueMapper->getMaxScalar();
}

const string GraphParamValue::getName() const
{
	return name;
}

GraphParamValue::RState::RState(GraphParamValue *v)
{
	nodes=v->nodes;
	scalar=v->prevScalar;
	horzDeform=v->horzDeformSlider->value();
	vertDeform=v->vertDeformSlider->value();
}

void GraphParamValue::pushUndo()
{
	// once they do something else, that clears the redo stack
	clearRedo();

	// keep stack to a bounded size
	while(undoStack.size()>100) {
		delete undoStack.front();
		undoStack.pop_front();
	}

	undoStack.push_back(new RState(this));

	setUndoButtonsState();
}

void GraphParamValue::popUndo()
{
	if(undoStack.size()<2)
		return;

	// the undoStack always has the current state at the top
	// so, move the current state to the redo stack 
	RState *s=undoStack.back();
	undoStack.pop_back();
	redoStack.push_back(s);

	// now set the current state to what is on the top of the undo stack
	applyState(undoStack.back());

	setUndoButtonsState();
}

void GraphParamValue::popRedo()
{
	if(redoStack.size()<1)
		return;

	// pop from the redo stack and push onto the undo stack
	RState *s=redoStack.back();
	redoStack.pop_back();
	undoStack.push_back(s);

	// now make the current state to what is on the top of the undo stack
	applyState(undoStack.back());

	setUndoButtonsState();
}

void GraphParamValue::applyState(RState *s)
{
	nodes=s->nodes;
	setScalar(s->scalar);
	horzDeformSlider->setValue(s->horzDeform);
	vertDeformSlider->setValue(s->vertDeform);

	updateNumbers();
	update();
	graphCanvas->update();
}

void GraphParamValue::clearUndo()
{
	while(undoStack.size()>0) {
		delete undoStack.front();
		undoStack.pop_front();
	}
	// undoStack always contains the current state at top
	undoStack.push_back(new RState(this));

	setUndoButtonsState();
}

void GraphParamValue::clearRedo()
{
	while(redoStack.size()>0) {
		delete redoStack.front();
		redoStack.pop_front();
	}

	setUndoButtonsState();
}

void GraphParamValue::setUndoButtonsState()
{
	undoButton->setEnabled(undoStack.size()>1);
	redoButton->setEnabled(!redoStack.empty());
}


void GraphParamValue::readFromFile(const string &prefix,CNestedDataFile *f)
{
	pushUndo();

	const string key=prefix DOT getName();

	if(f->keyExists(key DOT "scalar"))
		setScalar(f->getValue<int>(key DOT "scalar"));
	else	
		setScalar(vertValueMapper->getDefaultScalar());

	nodes.clear();

	const string k1=key DOT "node_positions";
	const string k2=key DOT "node_values";
	if(f->keyExists(k1)==CNestedDataFile::ktValue && f->keyExists(k2)==CNestedDataFile::ktValue)
	{
		const vector<double> positions=f->getValue<vector<double> >(k1);
		const vector<double> values=f->getValue<vector<double> >(k2);

		// ??? I could either save the node x and values as [0,1], or I could use save the actual values ( <-- currently)
		for(size_t t=0;t<positions.size();t++)
			nodes.push_back(CGraphParamValueNode(positions[t],values[t]));

	}
	else
		clearNodes();

	if(f->keyExists(key DOT "horzDeformSlider")==CNestedDataFile::ktValue)
		horzDeformSlider->setValue(f->getValue<int>(key DOT "horzDeformSlider"));
	if(f->keyExists(key DOT "vertDeformSlider")==CNestedDataFile::ktValue)
		vertDeformSlider->setValue(f->getValue<int>(key DOT "vertDeformSlider"));

	updateNumbers();
	update();
	graphCanvas->update();
}

void GraphParamValue::writeToFile(const string &prefix,CNestedDataFile *f) const
{
	const string key=prefix DOT getName();

	if(getMinScalar()!=getMaxScalar())
		f->setValue<int>(key DOT "scalar",getScalar());

	vector<double> positions,values;
	for(size_t t=0;t<nodes.size();t++)
	{
		positions.push_back(nodes[t].x);
		values.push_back(nodes[t].y);
	}

	const string k1=key DOT "node_positions";
	f->setValue<vector<double> >(k1,positions);

	const string k2=key DOT "node_values";
	f->setValue<vector<double> >(k2,values);

	if(getMinScalar()!=getMaxScalar())
		f->setValue<int>(key DOT "scalar",getScalar());

	f->setValue<int>(key DOT "horzDeformSlider",horzDeformSlider->value());
	f->setValue<int>(key DOT "vertDeformSlider",vertDeformSlider->value());
}

