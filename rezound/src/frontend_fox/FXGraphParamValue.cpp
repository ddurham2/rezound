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


#define GET_SCALAR_VALUE(o) ( o->scalarSpinner==NULL ? 0 : o->scalarSpinner->getValue() )
	

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

				//printf("y: %d\n",y);

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





// --- FXGraphParamNode ---------------------------------------------------------



#define FX_NODE(n) ((FXGraphParamNode *)((n).userData))

#define NODE_WIDTH 9
#define NODE_HEIGHT 9

class FXGraphParamNode : public FXFrame
{
	FXDECLARE(FXGraphParamNode);
public:
	FXGraphParamNode(FXGraphParamValue *p,int x,int y,bool _isEdgeNode=false) :
		FXFrame(p->graphPanel,FRAME_RAISED | LAYOUT_EXPLICIT, x,y,NODE_WIDTH,NODE_HEIGHT),
		isEdgeNode(_isEdgeNode), // if this node is on the edge (can't remove it and can't move it left and right)
		owner(p),
		dragging(false),

		dragOffsetX(0),
		dragOffsetY(0)
	{
		disable(); // I have to disable so that if I press the right button while dragging after create it messes things up
		if(isEdgeNode)
			enable();

		setBackColor(FXRGB(0,200,64));
	}

	virtual void move(FXint x,FXint y)
	{
		// limit the position to inside the parent window, but let it go outside the bounds by half of the NODE_SIZE
		if(x<0-NODE_WIDTH/2)
			x=0-NODE_WIDTH/2;
		else if(x>(owner->graphPanel->getWidth())-NODE_WIDTH/2)
			x=(owner->graphPanel->getWidth())-NODE_WIDTH/2;

		if(y<0-NODE_HEIGHT/2)
			y=0-NODE_HEIGHT/2;
		else if(y>(owner->getGraphPanelHeight())-NODE_HEIGHT/2)
			y=(owner->getGraphPanelHeight())-NODE_HEIGHT/2;

		FXFrame::move(x,y);
	}

	long onDestroyNode(FXObject *sender,FXSelector sel,void *ptr)
	{
		//printf("node right release\n");
		 if(!isEdgeNode && underCursor())
		{
			owner->removeNode=this;
			hide();
		}
		ungrab();
		return(0);
	}

	long onDragStart(FXObject *sender,FXSelector sel,void *ptr)
	{
		//printf("node left press\n");
		dragging=true;

		FXEvent *ev=(FXEvent *)ptr;
		//dragOffsetX=ev->win_x;
		//dragOffsetY=ev->win_y;

		onDrag(sender,sel,ptr);
		return(0);
	}

	long onDragStop(FXObject *sender,FXSelector sel,void *ptr)
	{
		//printf("node left release\n");
		dragging=false;

		// erase status ??? make sure that changing these doesn't cause everything to re-layout
		clearStatus();
		return(0);
	}

	long onDrag(FXObject *sender,FXSelector sel,void *ptr)
	{
		if(dragging)
		{
			FXEvent *ev=(FXEvent *)ptr;
			
			static int g=0;
			printf("%5d node move while dragging x:%d y:%d\n",g++,ev->win_x,ev->win_y);fflush(stdout);

			int x,y;
			translateCoordinatesTo(x,y,getParent(),ev->win_x-dragOffsetX,ev->win_y-dragOffsetY);

			// if shift or control is held, lock the x or y position
			if(ev->state&SHIFTMASK)
				x=getX();
			else if(ev->state&CONTROLMASK)
				y=getY();

			owner->moveAndRecalcNode(this,x,y);

			updateStatus();
		}
		return(0);
	}


	FXGraphParamNode() {}

	bool isEdgeNode;
	FXGraphParamValue *owner;
	bool dragging;

	// the place where the mouse was clicked within the node so its middle doesn't jump under the cursor when clicked
	int dragOffsetX;
	int dragOffsetY;

	void updateStatus()
	{
//return;
		const int index=owner->findNode(this);
		if(index==-1)
			return;

		const CGraphParamValueNode &n=owner->nodes[index];

		// draw status
		const string time=owner->sound->getTimePosition((sample_pos_t)((sample_fpos_t)(owner->stop-owner->start+1)*n.position+owner->start));
		//owner->positionLabel->setText(("Time: "+time).c_str());
		owner->valueLabel->setText(("Value: "+istring(owner->interpretValue(n.value,GET_SCALAR_VALUE(owner)),6,3)).c_str());
	}

	void clearStatus()
	{
return;
		owner->positionLabel->setText("Time: ");
		owner->valueLabel->setText("Value: ");
	}
	
};

FXDEFMAP(FXGraphParamNode) FXGraphParamNodeMap[]=
{
	//Message_Type				ID			Message_Handler
	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,	0,			FXGraphParamNode::onDestroyNode),

	FXMAPFUNC(SEL_LEFTBUTTONPRESS,		0,			FXGraphParamNode::onDragStart),
	FXMAPFUNC(SEL_LEFTBUTTONRELEASE,	0,			FXGraphParamNode::onDragStop),
	FXMAPFUNC(SEL_MOTION,			0,			FXGraphParamNode::onDrag),
};

FXIMPLEMENT(FXGraphParamNode,FXFrame,FXGraphParamNodeMap,ARRAYNUMBER(FXGraphParamNodeMap));






// -----------------------------------------------


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
 *   	- I don't know why it's happening, but sometimes when you click on an existing node, it jumps a pixel either leftward, upward or both
 *   		- I assume it's moving because we moveAndRecalc the node even when it's just clicked and released, so the value isn't coming out the same each click
 *
 *	- There is a bug which I should probably ask the author of FOX to look at... 
 *		- if you left-press to create a node, then while holding the left button, you press and 
 *		  and release the right button, it never seems to get the left-release.  I guess it's some
 *		  grab/ungrab problem
 *
 *	- There is also some screwy stuff going on with this case: (and I can watch the print statments I put in to see it)
 *		- create a node, right press to delete it, but move outside the node, then release and it never ungrabs it seems EXCEPT, I put a call to ungrab in there... take that out and you'll see the problem
 *
 *	- There is still some goofy stuff that can go on if you have a large selection and move the mouse around fast
 *	- I've seen orphaned nodes sometimes... Perhaps after clearing?
 *
 *	- Create some predefined patterns
 *
 *	   
 */


FXDEFMAP(FXGraphParamValue) FXGraphParamValueMap[]=
{
	//Message_Type				ID					Message_Handler

	FXMAPFUNC(SEL_PAINT,			FXGraphParamValue::ID_GRAPH_PANEL,	FXGraphParamValue::onGraphPanelPaint),

	FXMAPFUNC(SEL_LEFTBUTTONPRESS,		FXGraphParamValue::ID_GRAPH_PANEL,	FXGraphParamValue::onCreateNode),
	FXMAPFUNC(SEL_MOTION,			FXGraphParamValue::ID_GRAPH_PANEL,	FXGraphParamValue::onDragNodeAfterCreate),
	FXMAPFUNC(SEL_LEFTBUTTONRELEASE,	FXGraphParamValue::ID_GRAPH_PANEL,	FXGraphParamValue::onStopDraggingNodeAfterCreate),

	FXMAPFUNC(SEL_COMMAND,			FXGraphParamValue::ID_SCALAR_SPINNER,	FXGraphParamValue::onScalarSpinnerChange),
	FXMAPFUNC(SEL_COMMAND,			FXGraphParamValue::ID_CLEAR_BUTTON,	FXGraphParamValue::onPatternButton),
};

FXIMPLEMENT(FXGraphParamValue,FXPacker,FXGraphParamValueMap,ARRAYNUMBER(FXGraphParamValueMap))

FXGraphParamValue::FXGraphParamValue(f_at_xs _interpretValue,f_at_xs _uninterpretValue,const int minScalar,const int maxScalar,const int initScalar,FXComposite *p,int opts,int x,int y,int w,int h) :
	FXPacker(p,opts|FRAME_RIDGE,x,y,w,h, 2,2,2,2, 0,0),

	sound(NULL),
	start(0),
	stop(0),
	firstTime(true),

	buttonPanel(new FXHorizontalFrame(this,FRAME_NONE | LAYOUT_SIDE_BOTTOM | LAYOUT_FILL_X)),
		scalarLabel(NULL),
		scalarSpinner(NULL),
	vRuler(new FXValueRuler(this,this)),
	statusPanel(new FXHorizontalFrame(this,FRAME_RAISED | LAYOUT_SIDE_BOTTOM | LAYOUT_FILL_X, 0,0,0,0, 0,0,0,0, 4,0)),
		positionLabel(new FXLabel(statusPanel,"Time: ",NULL)),
		valueLabel(new FXLabel(statusPanel,"Value: ",NULL)),
		unitsLabel(new FXLabel(statusPanel,"x",NULL)),
	graphPanelParent(new FXPacker(this,LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,2,2)),
	graphPanel(new FXPackerCanvas(graphPanelParent,this,ID_GRAPH_PANEL,LAYOUT_FILL_X|LAYOUT_FILL_Y)),

	removeNode(NULL),
	draggingNodeAfterCreate(NULL),

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
}

void FXGraphParamValue::layout()
{
	FXPacker::layout();

//printf("layout 456\n");

/* add back but try to get cleanStatus and updateStatus not to cause a whole layout event
	if(nodes.size()>=2)
	{
		// relocate all the nodes based on their distances and values
		for(size_t t=0;t<nodes.size();t++)
		{
			FX_NODE(nodes[t])->isEdgeNode=false;
			FX_NODE(nodes[t])->move((int)(nodes[t].position*graphPanel->getWidth()-NODE_WIDTH/2),(int)((1.0-nodes[t].value)*getGraphPanelHeight()-NODE_HEIGHT/2));
		}
		FX_NODE(nodes[0])->isEdgeNode=true;
		FX_NODE(nodes[nodes.size()-1])->isEdgeNode=true;
	}
*/

	// render waveform to the backbuffer
	backBuffer->resize(graphPanel->getWidth(),graphPanel->getHeight());
	if(sound!=NULL)
	{
		FXDCWindow dc(backBuffer);
		drawPortion(0,graphPanel->getWidth(),&dc);
	}
}

void FXGraphParamValue::setSound(ASound *_sound,sample_pos_t _start,sample_pos_t _stop)
{
	sound=_sound;
	start=_start;
	stop=_stop;

	// make sure that the back buffer re-renders
	layout();
}

void FXGraphParamValue::clearNodes()
{
	for(size_t t=0;t<nodes.size();t++)
		delete (FXGraphParamNode *)(nodes[t].userData);
	nodes.clear();

	// remove any nodes that accidently got removed from nodes (via bad logic in insertIntoNodes on my part)
	while(graphPanel->numChildren()>0)
		delete graphPanel->childAtIndex(0);

	// add endpoints
	CGraphParamValueNode first(0.0,0.5,new FXGraphParamNode(this,0-NODE_WIDTH/2,getGraphPanelHeight()/2-NODE_HEIGHT/2,true));
	FX_NODE(first)->create();
	FX_NODE(first)->recalc();
	nodes.push_back(first);

	CGraphParamValueNode last(1.0,0.5,new FXGraphParamNode(this,graphPanel->getWidth()-NODE_WIDTH/2,getGraphPanelHeight()/2-NODE_HEIGHT/2,true));
	FX_NODE(last)->create();
	FX_NODE(last)->recalc();
	nodes.push_back(last);

	firstTime=false;

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
	if(firstTime)
		clearNodes();

	static int g=0;
	//printf("on paint: %d\n",g++);

	if(removeNode!=NULL)
	{
		for(size_t t=0;t<nodes.size();t++)
		{
			if(nodes[t].userData==removeNode)
			{
				nodes.erase(nodes.begin()+t);
				break;
			}
		}
	}

	FXDCWindow dc(graphPanel);

	// draw the whole background
	dc.drawImage(backBuffer,0,0);


	// draw the lines connecting the nodes
	dc.setForeground(FXRGB(255,64,64));
	for(size_t t=1;t<nodes.size();t++)
	{
		CGraphParamValueNode &n1=nodes[t-1];
		CGraphParamValueNode &n2=nodes[t];
		
		const int x1=FX_NODE(n1)->getX()+NODE_WIDTH/2;
		const int y1=FX_NODE(n1)->getY()+NODE_HEIGHT/2;
		const int x2=FX_NODE(n2)->getX()+NODE_WIDTH/2;
		const int y2=FX_NODE(n2)->getY()+NODE_HEIGHT/2;
		
		dc.drawLine(x1,y1,x2,y2);
	}

	return(1);
}

bool isBetween(double v,double p1,double p2)
{
	if(p1<p2)
		return(v>=p1 && v<=p2);
	else // if(p1>=p2)
		return(v>=p2 && v<=p1);

}

// always returns an even value >=2
int FXGraphParamValue::getGraphPanelHeight() const
{
	int h=graphPanel->getHeight();
	if(h%2==1)
		h--;
	return(max(h,2));
}

int FXGraphParamValue::findNode(FXGraphParamNode *node) const
{
	for(size_t t=0;t<nodes.size();t++)
	{
		if(nodes[t].userData==node)
			return(t);
	}
	return(-1);
}

/*
 * - This method recalculates the position and value for a node given a screen x and y on graphPanel
 * - It then removes and adds it to the node list to be moved to the correct spot
 */
void FXGraphParamValue::moveAndRecalcNode(FXGraphParamNode *fx_node,int x,int y)
{

	// if it's an edge node, don't let the X change
	fx_node->move(fx_node->isEdgeNode ? fx_node->getX() : x,y);

	const int index=findNode(fx_node);
	if(index==-1)
	{
		printf("warning: node not found in node list\n");
		return;
	}

	CGraphParamValueNode &node=nodes[index];

	// recalculate the position and distance value
	node.position=screenToNodePosition(fx_node->getX()+NODE_WIDTH/2);
	node.value=screenToNodeValue(fx_node->getY()+NODE_HEIGHT/2);

	//printf("recalcutating to: %f %f\n",node.position,node.value);

	if(!fx_node->isEdgeNode)
	{
		CGraphParamValueNode temp=node; // making a copy since it's about to go away after the remove
		// now remove and re-insert the node into the nodes list
		nodes.erase(nodes.begin()+index);
		insertIntoNodes(temp);
	}
}

/*
 * - This method does the logic to figure out where to insert a given node, 'n' into the nodes list
 * - This method is called for a newly created node
 * - And this method is called everytime a node is moved, it is removed from the list, then re-inserted
 */
void FXGraphParamValue::insertIntoNodes(CGraphParamValueNode &n)
{
	try
	{
		FX_NODE(n)->isEdgeNode=false;
		FX_NODE(nodes[0])->isEdgeNode=false;
		FX_NODE(nodes[nodes.size()-1])->isEdgeNode=false;

		// insert into the list appropriately
		bool handled=false;
		for(size_t x=0;x<nodes.size();x++)
		{
			CGraphParamValueNode &temp=nodes[x];
			
			if(n.position<temp.position)
			{ 
				nodes.insert(nodes.begin()+x,n);
				handled=true;
				break;
			}
			else if(n.position==temp.position)
			{ // if it's on the same X, see where it fits in vertically

				// find i1..i2 where they all have the same positions (that is, they are all in a vertical line
				int i1=x;
				int i2=x;
				for(size_t y=i1+1;y<nodes.size();y++)
				{
					if(nodes[y].position!=n.position)
						break;
					i2=y;
				}

				if(i1==i2)
				{ // we should get inserted either before or after the node at i1 (or i2.. no diff)
					if(nodes[i1].value>n.value)
						nodes.insert(nodes.begin()+i1,n);
					else
						nodes.insert(nodes.begin()+i1+1,n);

				}
				else if(i1>i2)
				{
					printf("************************************************** warning -- i1 > i2 %d>%d\n",i1,i2);
				}
				else
				{
					// now find where, from i1..i2 that n belongs
					bool done=false;
					for(int t=i1;t<i2;t++)
					{
						if(isBetween(n.value,nodes[t].value,nodes[t+1].value))
						{
							done=true;
							nodes.insert(nodes.begin()+t+1,n);
							break;
						}
					}
					if(!done)
						nodes.insert(nodes.begin()+i2+1,n);
				}

				handled=true;
				break;
			}
		}
		
		if(!handled)
		{
			printf("node not inserted into list... remove it???\n");
			// ??? deleteing it caused a segfault... ???
		}
	}
	catch(exception &e)
	{
		printf("exception -- %s\n",e.what());
	}

	FX_NODE(nodes[0])->isEdgeNode=true;
	FX_NODE(nodes[nodes.size()-1])->isEdgeNode=true;
}

long FXGraphParamValue::onCreateNode(FXObject *sender,FXSelector sel,void *ptr)
{
	//printf("panel left press\n");
	FXEvent *ev=(FXEvent *)ptr;

	CGraphParamValueNode n(screenToNodePosition(ev->win_x),screenToNodeValue(ev->win_y),new FXGraphParamNode(this,ev->win_x-NODE_WIDTH/2,ev->win_y-NODE_HEIGHT/2,false));
	FX_NODE(n)->create();
	FX_NODE(n)->recalc();
	FX_NODE(n)->dragging=true;
	draggingNodeAfterCreate=(FXGraphParamNode *)n.userData;

	insertIntoNodes(n);

	if(draggingNodeAfterCreate!=NULL)
	{
		FXEvent ev2=*((FXEvent *)ptr); // making a copy of event since we're modifying it
		((FXWindow *)sender)->translateCoordinatesTo(ev2.win_x,ev2.win_y,draggingNodeAfterCreate,ev2.win_x-NODE_WIDTH/2,ev2.win_y-NODE_HEIGHT/2);
		draggingNodeAfterCreate->onDrag(sender,sel,&ev2);
	}

	graphPanel->update();
	return(0);
}

long FXGraphParamValue::onDragNodeAfterCreate(FXObject *sender,FXSelector sel,void *ptr)
{
	/*
	{
		FXEvent *ev=(FXEvent *)ptr;
		printf("X: %d\n",ev->win_x);
	}
	*/

	if(draggingNodeAfterCreate!=NULL)
	{
		//printf("panel move while dragging after create\n");

		FXEvent ev=*((FXEvent *)ptr); // making a copy of event since we're modifying it
		((FXWindow *)sender)->translateCoordinatesTo(ev.win_x,ev.win_y,draggingNodeAfterCreate,ev.win_x-NODE_WIDTH/2,ev.win_y-NODE_HEIGHT/2);
		draggingNodeAfterCreate->onDrag(sender,sel,&ev);
	}
	return(0);
}

long FXGraphParamValue::onStopDraggingNodeAfterCreate(FXObject *sender,FXSelector sel,void *ptr)
{
	//printf("panel left release\n");
	if(draggingNodeAfterCreate!=NULL)
	{
		draggingNodeAfterCreate->onDragStop(sender,sel,ptr);
		draggingNodeAfterCreate->enable();
		draggingNodeAfterCreate->dragging=false;
		draggingNodeAfterCreate=NULL;
	}
	return(0);
}

#include "drawPortion.h"
void FXGraphParamValue::drawPortion(int left,int width,FXDCWindow *dc)
{
	if(sound==NULL)
		throw(runtime_error(string(__func__)+" -- sound is NULL"));

	const int canvasWidth=graphPanel->getWidth();
	const int canvasHeight=graphPanel->getHeight();
	const sample_pos_t length=stop-start+1;
	const double hScalar=(double)((sample_fpos_t)length/canvasWidth);
	const int hOffset=(int)(start/hScalar);
	const double vScalar=(65536.0/(double)canvasHeight)*(double)sound->getChannelCount();

	::drawPortion(left,width,dc,sound,canvasWidth,canvasHeight,-1,-1,hScalar,hOffset,vScalar,0,true);
}

double FXGraphParamValue::screenToNodePosition(int x)
{
	return((double)x/(double)graphPanel->getWidth());
}

double FXGraphParamValue::screenToNodeValue(int y)
{
	return(1.0-((double)y/(double)getGraphPanelHeight()));
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
	for(size_t t=0;t<retNodes.size();t++)
		retNodes[t].value=interpretValue(nodes[t].value,GET_SCALAR_VALUE(this));

	return(retNodes);
}


