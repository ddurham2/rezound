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

#include "../backend/CSound.h"

#define NODE_RADIUS 4

#define GET_SCALAR_VALUE(o) ( o->scalarSpinner==NULL ? 0 : o->scalarSpinner->getValue() )
	


/* TODO:
 *
 *	- I probably need to limit the minimum size as it could cause divide by zero errors, and really be unusable anyway
 *
 *	- be able to save or load a particular curve to disk
 *
 * 	- I would have liked to reuse code (without copy/pasting) from FXRezWaveView, but it's currently
 * 	  too much tied to CLoadedSound for start and stop positions and such...
 * 	  - At least I pulled draw portion out not to have to rewrite that
 * 	  - but it would be nice if all the scrolling and zooming capabilities where also there....
 *
 * 	- I would like to put a horz ruler at the top of the rendered wave
 * 		- I should also onMouseMove print the time that the cursor is over
 * 			- clear it onExit
 *
 * 	- draw a vertical to the time and a horizontal line to the ruler while dragging the nodes 
 *
 * 	- still a glitch with the drawing of the bottom tick in the ruler.. I might draw more values in the ticks
 *
 *   	- I might ought to draw the play position in there if I can get the event
 *
 *	- I might want to avoid redrawing everything.  Alternatively I could:
 *         - blit only the sections between the nodes involving the change
 *
 *	- Create some predefined patterns
 *	   
 */


// --- declaration of FXValueRuler -----------------------------------------------


class FXValueRuler : public FXHorizontalFrame
{
	FXDECLARE(FXValueRuler)
public:
	FXValueRuler(FXComposite *p,FXGraphParamValue *_parent) :
		FXHorizontalFrame(p,FRAME_RAISED | LAYOUT_FILL_Y | LAYOUT_FIX_WIDTH | LAYOUT_SIDE_LEFT, 0,0,30),

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

	virtual ~FXValueRuler()
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
		dc.setTextFont(font);

		#define TICK_FREQ 20

		// N is the number of ticks to label
		int N=(parent->getGraphPanelHeight()/TICK_FREQ); // we lable always an odd number of ticks so there is always a middle tick
		if(N%2==0)
			N++;

		// make sure there's at least 3
		if(N<3)
			N=3;

		const int s=0;
		const int e=parent->getGraphPanelHeight()-1;

		// y actually goes from the event's min-10 to max+10 incase part of some text needs be be redrawn (and we limit it to 0..height-1)
		const int maxY=max(parent->getGraphPanelHeight()-1,(ev->rect.h+ev->rect.y)+10);
		for(int y=max(0,(ev->rect.y)-10);y<=maxY;y++)
		{
			// this is, y=s+(((e-s)*t)/(N-1)) [0..N interpolated across s..e] solved for t, and then we get the remainder of the resulting division instead of the acutal quotient)
			const double t=fmod((double)((y-s)*(N-1)),(double)(e-s));
			const int renderY=y+(((FXPacker *)(parent->graphPanel->getParent()))->getPadTop());

			if(t<(N-1))
			{ // draw and label this tick
				dc.drawLine(getWidth()-2,renderY,getWidth()-10,renderY);

				double value=parent->interpretValue(parent->screenToNodeValue(y),GET_SCALAR_VALUE(parent));

				const string sValue=istring(value,5,3);

				int offset;
				if(y==parent->getGraphPanelHeight()-1)
					offset=-2; // put text above the tick mark
				else
					offset=font->getFontHeight(); // put text below the tick mark

				dc.drawText(3,renderY+offset,sValue.c_str(),sValue.length());
			}
			// else, ever 2 ticks (i.e. in between labled ticks)
				//dc.drawLine(getWidth()-2,renderY,getWidth()-5,renderY); // half way tick between labled ticks
		}

		return(0);
	}

protected:
	FXValueRuler() {}

private:
	FXGraphParamValue *parent;

	FXFont *font;

};

FXDEFMAP(FXValueRuler) FXValueRulerMap[]=
{
	//Message_Type				ID				Message_Handler
	FXMAPFUNC(SEL_PAINT,			0,				FXValueRuler::onPaint),
};

FXIMPLEMENT(FXValueRuler,FXHorizontalFrame,FXValueRulerMap,ARRAYNUMBER(FXValueRulerMap))



// -----------------------------------------------

FXDEFMAP(FXGraphParamValue) FXGraphParamValueMap[]=
{
	//Message_Type				ID					Message_Handler

	FXMAPFUNC(SEL_PAINT,			FXGraphParamValue::ID_GRAPH_PANEL,	FXGraphParamValue::onGraphPanelPaint),

	FXMAPFUNC(SEL_CONFIGURE,		FXGraphParamValue::ID_GRAPH_PANEL,	FXGraphParamValue::onGraphPanelResize),

	FXMAPFUNC(SEL_LEFTBUTTONPRESS,		FXGraphParamValue::ID_GRAPH_PANEL,	FXGraphParamValue::onCreateOrStartDragNode),
	FXMAPFUNC(SEL_MOTION,			FXGraphParamValue::ID_GRAPH_PANEL,	FXGraphParamValue::onDragNode),
	FXMAPFUNC(SEL_LEFTBUTTONRELEASE,	FXGraphParamValue::ID_GRAPH_PANEL,	FXGraphParamValue::onStopDragNode),
	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,	FXGraphParamValue::ID_GRAPH_PANEL,	FXGraphParamValue::onDestroyNode),

	FXMAPFUNC(SEL_COMMAND,			FXGraphParamValue::ID_SCALAR_SPINNER,	FXGraphParamValue::onScalarSpinnerChange),
	FXMAPFUNC(SEL_COMMAND,			FXGraphParamValue::ID_CLEAR_BUTTON,	FXGraphParamValue::onPatternButton),
};

FXIMPLEMENT(FXGraphParamValue,FXPacker,FXGraphParamValueMap,ARRAYNUMBER(FXGraphParamValueMap))

FXGraphParamValue::FXGraphParamValue(const string _title,f_at_xs _interpretValue,f_at_xs _uninterpretValue,const int minScalar,const int maxScalar,const int initScalar,FXComposite *p,int opts,int x,int y,int w,int h) :
	FXPacker(p,opts|FRAME_RIDGE,x,y,w,h, 2,2,2,2, 0,0),

	title(_title),

	sound(NULL),
	start(0),
	stop(0),

	buttonPanel(new FXHorizontalFrame(this,FRAME_NONE | LAYOUT_SIDE_BOTTOM | LAYOUT_FILL_X)),
		scalarLabel(NULL),
		scalarSpinner(NULL),
	vRuler(new FXValueRuler(this,this)),
	statusPanel(new FXHorizontalFrame(this,FRAME_RAISED | LAYOUT_SIDE_BOTTOM | LAYOUT_FILL_X, 0,0,0,0, 0,0,0,0, 4,0)),
		positionLabel(new FXLabel(statusPanel,"Time: ",NULL,LAYOUT_LEFT)),
		valueLabel(new FXLabel(statusPanel,"Value: ",NULL)),
		unitsLabel(new FXLabel(statusPanel,"x",NULL)),
	graphPanelParent(new FXPacker(this,LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,2,2)),
	graphPanel(new FXCanvas(graphPanelParent,this,ID_GRAPH_PANEL,LAYOUT_FILL_X|LAYOUT_FILL_Y)),

	draggingNode(-1),
	dragOffsetX(0),dragOffsetY(0),

	interpretValue(_interpretValue),
	uninterpretValue(_uninterpretValue),

	backBuffer(new FXImage(getApp()))
{
	// just to give a minimum width and height to the panel
	new FXFrame(this,LAYOUT_FIX_WIDTH | FRAME_NONE | LAYOUT_SIDE_TOP, 0,0,500,0, 0,0,0,0);
	new FXFrame(this,LAYOUT_FIX_HEIGHT | FRAME_NONE | LAYOUT_SIDE_RIGHT, 0,0,0,250, 0,0,0,0);

	if(minScalar!=maxScalar)
	{
		scalarLabel=new FXLabel(buttonPanel,"Scalar",NULL);
		scalarSpinner=new FXSpinner(buttonPanel,5,this,ID_SCALAR_SPINNER);
		scalarSpinner->setRange(minScalar,maxScalar);
		scalarSpinner->setValue(initScalar);
	}

	new FXButton(buttonPanel,"Clear",NULL,this,ID_CLEAR_BUTTON);

	nodes.reserve(100);

	backBuffer->create();

	clearNodes();
}

void FXGraphParamValue::setSound(CSound *_sound,sample_pos_t _start,sample_pos_t _stop)
{
	sound=_sound;
	start=_start;
	stop=_stop;

	// make sure that the back buffer re-renders
	onGraphPanelResize(NULL,0,NULL);
}

void FXGraphParamValue::clearNodes()
{
	nodes.clear();

	// add endpoints
	CGraphParamValueNode first(0.0,0.5);
	nodes.push_back(first);

	CGraphParamValueNode last(1.0,0.5);
	nodes.push_back(last);

	graphPanel->update();
}


long FXGraphParamValue::onScalarSpinnerChange(FXObject *sender,FXSelector sel,void *ptr)
{
	updateNumbers();
	return(1);
}

long FXGraphParamValue::onPatternButton(FXObject *sender,FXSelector sel,void *ptr)
{
	clearNodes();

	graphPanel->update();
	return(1);
}

long FXGraphParamValue::onGraphPanelPaint(FXObject *sender,FXSelector sel,void *ptr)
{
	FXDCWindow dc(graphPanel);

	// draw the whole background
	dc.drawImage(backBuffer,0,0);

	// draw the lines connecting the nodes
	dc.setForeground(FXRGB(255,64,64));
	for(size_t t=1;t<nodes.size();t++)
	{
		CGraphParamValueNode &n1=nodes[t-1];
		CGraphParamValueNode &n2=nodes[t];
		
		const int x1=nodeToScreenX(n1);
		const int y1=nodeToScreenY(n1);
		const int x2=nodeToScreenX(n2);
		const int y2=nodeToScreenY(n2);
		
		dc.drawLine(x1,y1,x2,y2);
	}

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
		
	return(1);
}

#include "drawPortion.h"
long FXGraphParamValue::onGraphPanelResize(FXObject *sender,FXSelector sel,void *ptr)
{
	// render waveform to the backbuffer
	backBuffer->resize(graphPanel->getWidth(),graphPanel->getHeight());
	if(sound!=NULL)
	{
		FXDCWindow dc(backBuffer);

		const int canvasWidth=graphPanel->getWidth();
		const int canvasHeight=graphPanel->getHeight();
		const sample_pos_t length=stop-start+1;
		const double hScalar=(double)((sample_fpos_t)length/canvasWidth);
		const int hOffset=(int)(start/hScalar);
		const double vScalar=(65536.0/(double)canvasHeight)*(double)sound->getChannelCount();

		drawPortion(0,canvasWidth,&dc,sound,canvasWidth,canvasHeight,-1,-1,hScalar,hOffset,vScalar,0,true);
	}
	return(1);
}

// always returns an even value >=2
int FXGraphParamValue::getGraphPanelHeight() const
{
	int h=graphPanel->getHeight();
	if(h%2==1)
		h--;
	return(max(h,2));
}

int FXGraphParamValue::insertIntoNodes(const CGraphParamValueNode &node)
{
	// sanity checks
	if(nodes.size()<2)
		throw(runtime_error(string(__func__)+" -- nodes doesn't already contain at least 2 items"));
	if(node.position<0.0 || node.position>1.0)
		throw(runtime_error(string(__func__)+" -- node's position is out of range: "+istring(node.position)));

	int insertedIndex=-1;

	// - basically, this function inserts the node into sorted order first by the nodes' postions

	// - insert into the first position to keep the node's position in non-desreasing order
	for(size_t t1=1;t1<nodes.size();t1++)
	{
		CGraphParamValueNode &temp=nodes[t1];
		if(temp.position>=node.position)
		{
			nodes.insert(nodes.begin()+t1,node);
			insertedIndex=t1;
			break;
		}
	}

	return(insertedIndex);
}

long FXGraphParamValue::onCreateOrStartDragNode(FXObject *sender,FXSelector sel,void *ptr)
{
	FXEvent *ev=(FXEvent *)ptr;

	int nodeIndex=findNodeAt(ev->win_x,ev->win_y);
	if(nodeIndex==-1)
	{ // create a new node
		CGraphParamValueNode node(screenToNodePosition(ev->win_x),screenToNodeValue(ev->win_y));
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
	graphPanel->update();
	return(1);
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
		node.position=	lockX ? node.position : screenToNodePosition(ev->win_x-dragOffsetX);
		node.value=	lockY ? node.value : screenToNodeValue(ev->win_y-dragOffsetY);

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
		graphPanel->update();
	}
	return(1);
}

long FXGraphParamValue::onStopDragNode(FXObject *sender,FXSelector sel,void *ptr)
{
	draggingNode=-1;
	clearStatus();
	return(1);
}

long FXGraphParamValue::onDestroyNode(FXObject *sender,FXSelector sel,void *ptr)
{
	FXEvent *ev=(FXEvent *)ptr;
	int nodeIndex=findNodeAt(ev->win_x,ev->win_y);
	if(nodeIndex!=-1)
	{
		nodes.erase(nodes.begin()+nodeIndex);
		draggingNode=-1;
		graphPanel->update();
	}
	return(1);
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
			return(t);
	}

	return(-1);
}

double FXGraphParamValue::screenToNodePosition(int x)
{
	double v=((double)x/(double)(graphPanel->getWidth()-1));
	if(v<0.0)
		v=0.0;
	else if(v>1.0)
		v=1.0;
	return(v);
}

double FXGraphParamValue::screenToNodeValue(int y)
{
	double v=(1.0-((double)y/(double)(getGraphPanelHeight())));
	if(v<0.0)
		v=0.0;
	else if(v>1.0)
		v=1.0;
	return(v);
}

int FXGraphParamValue::nodeToScreenX(const CGraphParamValueNode &node)
{
	return((int)(node.position*(graphPanel->getWidth()-1)));
}

int FXGraphParamValue::nodeToScreenY(const CGraphParamValueNode &node)
{
	return((int)((1.0-node.value)*(getGraphPanelHeight())));
}

void FXGraphParamValue::updateNumbers()
{
	vRuler->update();
}

void FXGraphParamValue::setUnits(FXString _units)
{
	units=_units;

	unitsLabel->setText(units);
	updateNumbers();
}

const CGraphParamValueNodeList &FXGraphParamValue::getNodes() const
{
	retNodes=nodes;
	// adjust the value by the given function
	for(size_t t=0;t<retNodes.size();t++)
		retNodes[t].value=interpretValue(nodes[t].value,GET_SCALAR_VALUE(this));

	return(retNodes);
}

void FXGraphParamValue::updateStatus()
{
	if(draggingNode==-1)
		return;

	const CGraphParamValueNode &n=nodes[draggingNode];

	// draw status
	const string time=sound->getTimePosition((sample_pos_t)((sample_fpos_t)(stop-start+1)*n.position+start));
	positionLabel->setText(("Time: "+time).c_str());
	valueLabel->setText(("Value: "+istring(interpretValue(n.value,GET_SCALAR_VALUE(this)),6,3)).c_str());
}

void FXGraphParamValue::clearStatus()
{
	positionLabel->setText("Time: ");
	valueLabel->setText("Value: ");
}
	

const int FXGraphParamValue::getScalar() const
{
	return(GET_SCALAR_VALUE(this));
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
	return(lo);
}

const int FXGraphParamValue::getMaxScalar() const
{
	FXint lo=0,hi=0;
	if(scalarSpinner!=NULL)
		scalarSpinner->getRange(lo,hi);
	return(hi);
}

const string FXGraphParamValue::getTitle() const
{
	return(title);
}

void FXGraphParamValue::readFromFile(const string &prefix,CNestedDataFile &f)
{
	const string key=prefix+"."+getTitle()+".";

	const string s=f.getValue((key+"scalar").c_str());
	setScalar(atoi(s.c_str()));


	nodes.clear();
	const string k1=key+"node_positions";
	const string k2=key+"node_values";
	const size_t count=f.getArraySize(k1.c_str());
	// ??? I could either save the node positions and values as [0,1], or I could use save the actual values ( <-- currently)
	for(size_t t=0;t<count;t++)
		nodes.push_back(CGraphParamValueNode(atof(f.getArrayValue(k1.c_str(),t).c_str()),atof(f.getArrayValue(k2.c_str(),t).c_str())));

	updateNumbers();
	update();
	graphPanel->update();
}

void FXGraphParamValue::writeToFile(const string &prefix,CNestedDataFile &f) const
{
	const string key=prefix+"."+getTitle()+".";

	if(getMinScalar()!=getMaxScalar())
		f.createKey((key+"scalar").c_str(),istring(getScalar()));

	//const CGraphParamValueNodeList nodes=getNodes();


	const string k1=key+"node_positions";
		// ??? I could implement a createArrayKey which takes a double so I wouldn't have to convert to string here
	f.removeKey(k1.c_str());
	for(size_t t=0;t<nodes.size();t++)
		f.createArrayKey(k1.c_str(),t,istring(nodes[t].position).c_str());

	const string k2=key+"node_values";
	f.removeKey(k2.c_str());
	for(size_t t=0;t<nodes.size();t++)
		f.createArrayKey(k2.c_str(),t,istring(nodes[t].value).c_str());

	if(getMinScalar()!=getMaxScalar())
		f.createKey((key+"scalar").c_str(),istring(getScalar()));
}


