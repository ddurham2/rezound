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

#include "FXPluginRoutingParamValue.h"

#include "utils.h"

#include <map>

#include <CPath.h>
#include <istring>
#include <CNestedDataFile/CNestedDataFile.h>

#include "CSoundFileManager.h"
#include "../backend/CLoadedSound.h"
#include "../backend/CSound.h"
#include "../backend/AStatusComm.h"
#include "../backend/main_controls.h"
#include "../backend/AAction.h" // for EUserMessage

#include "../backend/LADSPA/ladspa.h" /* ??? eventually don't want LADSPA specific code in here */
#include "../backend/LADSPA/utils.h"

#include "CFOXIcons.h"

#define COL_BACKGROUND FXRGB(44,44,64)
#define COL_FILENAME FXRGB(164,164,184)


static int wire_color_index=0;
static FXColor wire_colors[]={FXRGB(255,255,0),FXRGB(255,0,0),FXRGB(128,128,255),FXRGB(0,255,0),FXRGB(255,0,255),FXRGB(0,255,255),FXRGB(255,128,0),FXRGB(128,255,0),FXRGB(0,255,128),FXRGB(0,128,255),FXRGB(255,0,128),FXRGB(128,0,255)};
#define N_WIRE_COLORS (sizeof(wire_colors)/sizeof(*wire_colors))
#define RESET_WIRE_COLOR {wire_color_index=0;}
#define CURRENT_WIRE_COLOR (wire_colors[wire_color_index%N_WIRE_COLORS])
#define NEXT_WIRE_COLOR (wire_colors[(wire_color_index++)%N_WIRE_COLORS])

#define COL_INPUT_NODE FXRGB(0,255,255)
#define COL_OUTPUT_NODE FXRGB(255,128,0)

#define NODE_RADIUS 4
#define WIRE_THICKNESS 3

/*
 * I would rather this use a more generic prebuilt FOX API for draggable/droppable 
 * connections with rules about what can connect to what, etc
 *
 * But the way it is...
 *
 * ??? I could store all sources, instances and outputs in a single vector or map
 * with a enum tag as to what family it belonged to.. then I wouldn't have to 
 * loop 3 times in most of the code stanzas
 *
 * ??? add gettext hooks wherever necessary
 *
 * ??? if this were more generic, the instance's could be created from a list of loaded plugins and input could be fed to any 
 *
 */

/*****************************/
class CCompositeCanvas : public FXComposite
{
	FXDECLARE(CCompositeCanvas)
public:
	CCompositeCanvas(FXComposite* p,FXObject* tgt=NULL,FXSelector sel=0,FXuint opts=FRAME_NORMAL,FXint x=0,FXint y=0,FXint w=0,FXint h=0) : 
		FXComposite(p,opts,x,y,w,h)
	{
		enable();
		flags|=FLAG_SHOWN; // I have to do this, or it will not show up.. like height is 0 or something

		setTarget(tgt);
		setSelector(sel);
	}
  	CCompositeCanvas() {}

	virtual ~CCompositeCanvas() {}

	long onPaint(FXObject*,FXSelector,void* ptr){
		return target && target->handle(this,FXSEL(SEL_PAINT,message),ptr);
	}
	long onMouseAny(FXObject*,FXSelector sel,void* ptr){
		return target && target->handle(this,FXSEL(FXSELTYPE(sel),message),ptr);
	}
};
FXDEFMAP(CCompositeCanvas) CCompositeCanvasMap[]={
	FXMAPFUNC(SEL_PAINT,0,CCompositeCanvas::onPaint),
	FXMAPFUNC(SEL_CONFIGURE,0,CCompositeCanvas::onConfigure),
	FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,CCompositeCanvas::onMouseAny),
	FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,CCompositeCanvas::onMouseAny),
	FXMAPFUNC(SEL_RIGHTBUTTONPRESS,0,CCompositeCanvas::onMouseAny),
	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,0,CCompositeCanvas::onMouseAny),
};
FXIMPLEMENT(CCompositeCanvas,FXComposite,CCompositeCanvasMap,ARRAYNUMBER(CCompositeCanvasMap))

/*****************************/

class FXMyScrollWindow : public FXScrollWindow
{
public:
	FXMyScrollWindow(FXComposite* p,FXuint opts) :
		FXScrollWindow(p,opts)
	{
	}

	virtual ~FXMyScrollWindow() {}

	FXScrollWindow::startAutoScroll;
	FXScrollWindow::stopAutoScroll;
};

/*****************************/

FXDEFMAP(FXPluginRoutingParamValue) FXPluginRoutingParamValueMap[]=
{
	//Message_Type				ID							Message_Handler
	FXMAPFUNC(SEL_PAINT,			FXPluginRoutingParamValue::ID_CANVAS,			FXPluginRoutingParamValue::onPaint),
	FXMAPFUNC(SEL_MOTION,			FXPluginRoutingParamValue::ID_CANVAS,			FXPluginRoutingParamValue::onMouseMove),
	FXMAPFUNC(SEL_LEFTBUTTONPRESS,		FXPluginRoutingParamValue::ID_CANVAS,			FXPluginRoutingParamValue::onMouseDown),
	FXMAPFUNC(SEL_LEFTBUTTONRELEASE,	FXPluginRoutingParamValue::ID_CANVAS,			FXPluginRoutingParamValue::onMouseUp),
	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,	FXPluginRoutingParamValue::ID_CANVAS,			FXPluginRoutingParamValue::onMouseUp),

	FXMAPFUNC(SEL_CONFIGURE,		0,							FXPluginRoutingParamValue::onConfigure),

	FXMAPFUNC(SEL_COMMAND,			FXPluginRoutingParamValue::ID_NEW_INSTANCE_BUTTON,	FXPluginRoutingParamValue::onNewInstanceButton),
	FXMAPFUNC(SEL_COMMAND,			FXPluginRoutingParamValue::ID_ADD_SOURCE_BUTTON,	FXPluginRoutingParamValue::onAddSourceButton),

	FXMAPFUNC(SEL_COMMAND,			FXPluginRoutingParamValue::ID_RESET_BUTTON,		FXPluginRoutingParamValue::onResetButton),
	FXMAPFUNC(SEL_COMMAND,			FXPluginRoutingParamValue::ID_DEFAULT_BUTTON,		FXPluginRoutingParamValue::onDefaultButton),

	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,	FXPluginRoutingParamValue::ID_INSTANCE,			FXPluginRoutingParamValue::onRemoveInstance),
	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,	FXPluginRoutingParamValue::ID_INSTANCE_TITLE,		FXPluginRoutingParamValue::onRemoveInstance),
	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,	FXPluginRoutingParamValue::ID_INSTANCE_PORT,		FXPluginRoutingParamValue::onRemoveInstance),

	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,	FXPluginRoutingParamValue::ID_SOURCE,			FXPluginRoutingParamValue::onRemoveSource),
	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,	FXPluginRoutingParamValue::ID_SOURCE_TITLE,		FXPluginRoutingParamValue::onRemoveSource),
	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,	FXPluginRoutingParamValue::ID_SOURCE_CHANNEL,		FXPluginRoutingParamValue::onRemoveSource),

	FXMAPFUNC(SEL_COMMAND,			FXPluginRoutingParamValue::ID_REMOVE_CONNECTION,	FXPluginRoutingParamValue::onRemoveConnection),
	FXMAPFUNC(SEL_CHANGED,			FXPluginRoutingParamValue::ID_SET_CONNECTION_GAIN_SLIDER,FXPluginRoutingParamValue::onSetConnectionGainSlider),
	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,	FXPluginRoutingParamValue::ID_SET_CONNECTION_GAIN_SLIDER,FXPluginRoutingParamValue::onSetConnectionGainSlider),

	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,	FXPluginRoutingParamValue::ID_OUTPUT,			FXPluginRoutingParamValue::onOutputPopup),
	FXMAPFUNC(SEL_COMMAND,			FXPluginRoutingParamValue::ID_APPEND_NEW_OUTPUT_CHANNEL,FXPluginRoutingParamValue::onAppendNewOutputChannel),
	FXMAPFUNC(SEL_COMMAND,			FXPluginRoutingParamValue::ID_REMOVE_LAST_OUTPUT_CHANNEL,FXPluginRoutingParamValue::onRemoveLastOutputChannel),
};

FXIMPLEMENT(FXPluginRoutingParamValue,FXPacker,FXPluginRoutingParamValueMap,ARRAYNUMBER(FXPluginRoutingParamValueMap))

#define ASSURE_HEIGHT(parent,height) new FXFrame(parent,FRAME_NONE|LAYOUT_SIDE_LEFT | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT,0,0,0,height);
#define ASSURE_WIDTH(parent,width) new FXFrame(parent,FRAME_NONE|LAYOUT_SIDE_BOTTOM | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT,0,0,width,0);

FXPluginRoutingParamValue::FXPluginRoutingParamValue(FXComposite *p,int opts,const char *_name,const LADSPA_Descriptor *_desc) :
	FXPacker(p,opts|FRAME_RAISED | LAYOUT_FILL_Y|LAYOUT_FILL_X,0,0,0,0, 2,2,2,2, 0,0),
	name(_name),
	pluginDesc(_desc),
	actionSound(NULL),

	firstTime(true),

	contents(new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, DEFAULT_PAD,DEFAULT_PAD,DEFAULT_PAD,DEFAULT_PAD, 0,0)),
		scrollWindow(new FXMyScrollWindow(contents,FRAME_RAISED | LAYOUT_FILL_X|LAYOUT_FILL_Y)),
			canvasParent(new FXPacker(scrollWindow,LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 10,10,10,10)),
				canvas(new CCompositeCanvas(canvasParent,this,ID_CANVAS,LAYOUT_FILL_X|LAYOUT_FILL_Y)),

	mouseIsOverNode(false),
	draggingWire(false),

	textFont(getApp()->getNormalFont())
{
	ASSURE_WIDTH(this,690);
	ASSURE_HEIGHT(this,230);

	contents->raise();

	// create controls at bottom
	{
		FXPacker *p1,*p2,*p3;
		p1=new FXHorizontalFrame(contents,FRAME_RAISED | LAYOUT_BOTTOM|LAYOUT_FILL_X, 0,0,0,0, 0,0,0,0);
			p2=new FXVerticalFrame(p1,LAYOUT_LEFT|LAYOUT_FILL_Y);
				new FXButton(p2,_("Use Sound ->\tAdd the Selected Sound as a Source Above"),NULL,this,ID_ADD_SOURCE_BUTTON,BUTTON_NORMAL|LAYOUT_FILL_X);
				new FXButton(p2,_("New Instance\tCreate a New Instance of the Plugin"),NULL,this,ID_NEW_INSTANCE_BUTTON,BUTTON_NORMAL|LAYOUT_FILL_X);
				new FXButton(p2,_("Default\tGuess at the Desired Routing"),NULL,this,ID_DEFAULT_BUTTON,BUTTON_NORMAL|LAYOUT_FILL_X);
				new FXButton(p2,_("Clear"),NULL,this,ID_RESET_BUTTON,BUTTON_NORMAL|LAYOUT_FILL_X);
			p2=new FXPacker(p1,FRAME_NONE | LAYOUT_FILL_X|LAYOUT_FILL_Y);
				p3=new FXPacker(p2,LAYOUT_FILL_X|LAYOUT_FILL_Y | FRAME_SUNKEN|FRAME_THICK, 0,0,0,0, 0,0,0,0, 0,0);
					soundList=new FXList(p3,5,NULL,0,LIST_NORMAL|LIST_SINGLESELECT | LAYOUT_FILL_Y|LAYOUT_FILL_X);
	}

	canvasParent->setBackColor(COL_BACKGROUND);

	// create a smaller font to use 
        FXFontDesc d;
        textFont->getFontDesc(d);
        d.size=80;
        textFont=new FXFont(getApp(),d);

	FOXIcons->plugin_wave->setTransparentColor(FXRGB(255,0,255));
}

FXPluginRoutingParamValue::~FXPluginRoutingParamValue()
{
	delete textFont;
}

void FXPluginRoutingParamValue::drawNodeConnector(FXDCWindow &dc,const REntity::RNode &node)
{
	FXWindow *win=node.window;

	// get x and y on canvas of the left (or right) edge of a node
	FXint x1,y;
	getNodePosition(&node,x1,y);

	FXint x2=x1+(node.type==REntity::RNode::tInput ? NODE_RADIUS*2 : -NODE_RADIUS*2);

	dc.setForeground(wire_colors[0]);
	dc.setLineWidth(WIRE_THICKNESS);
	dc.drawLine(x1,y,x2,y);

	dc.setLineWidth(1);
	dc.setForeground(node.type==REntity::RNode::tInput ? COL_INPUT_NODE : COL_OUTPUT_NODE);
	dc.drawArc(x1-NODE_RADIUS,y-NODE_RADIUS,NODE_RADIUS*2,NODE_RADIUS*2,0*64,360*64);
}

void FXPluginRoutingParamValue::drawSolidNode(FXDCWindow &dc,const RNodeDesc &desc)
{
	REntity *entity;
	REntity::RNode *node;
	findNode(desc,entity,node);

	FXint x,y;
	getNodePosition(node,x,y);

	dc.setForeground(node->type==REntity::RNode::tInput ? COL_INPUT_NODE : COL_OUTPUT_NODE);
	dc.fillArc(x-NODE_RADIUS,y-NODE_RADIUS,NODE_RADIUS*2,NODE_RADIUS*2,0*64,360*64);
}

/*
 * Gives you the screen position on the canvas given a node
 */
void FXPluginRoutingParamValue::getNodePosition(const REntity::RNode *node,FXint &x,FXint &y)
{
	node->window->translateCoordinatesTo(
		x,y,
		canvas,
		node->type==REntity::RNode::tInput ? -(2*NODE_RADIUS) : (node->window->getWidth()+(2*NODE_RADIUS)),
		node->window->getHeight()/2
	);
}

FXint FXPluginRoutingParamValue::findNearestHoleWithinInstancesColumn(FXint instanceX,FXint fromx,FXint fromy)
{
	FXint canvasHeight=canvas->getHeight();
	/*
	const FXint v_spacing=calc_spacing(canvasHeight,saved_minInstancesColumnHeight,N_instances.size());
	const FXint y=calc_initial_offset(canvasHeight,saved_minInstancesColumnHeight,N_instances.size());
	for(size_t t=0;t<N_instances.size();t++)
	{
		FXWindow *win=N_instances[t].window;

		win->position(x,y,instancesColumnWidth,win->getDefaultHeight());
		y+=win->getDefaultHeight();
		y+=v_spacing;
	}
	*/

	#define distance(x1,y1,x2,y2) (  ( ((x2)-(x1))*((x2)-(x1)) )+( ((y2)-(y1))*((y2)-(y1)) )  )
	if(N_instances.size()==1)
	{ // we have two big spaces above and below the single instance
		FXint midy1=N_instances[0].window->getY()/2;
		FXint midy2=(N_instances[0].window->getY()+N_instances[0].window->getHeight()) + ((canvasHeight-(N_instances[0].window->getY()+N_instances[0].window->getHeight()))/2);


		if( distance(instanceX,midy1,fromx,fromy)<distance(instanceX,midy2,fromx,fromy))
			return midy1;
		else
			return midy2;
	}
	else if(N_instances.size()==2)
	{ // we have two spaces above and below the two instances and one in between
		FXint midy1=N_instances[0].window->getY()/2;
		FXint midy2=(N_instances[0].window->getY()+N_instances[0].window->getHeight()) + ((N_instances[1].window->getY()-(N_instances[0].window->getY()+N_instances[0].window->getHeight()))/2);
		FXint midy3=(N_instances[1].window->getY()+N_instances[1].window->getHeight()) + ((canvasHeight-(N_instances[1].window->getY()+N_instances[1].window->getHeight()))/2);

		if( distance(instanceX,midy1,fromx,fromy)<distance(instanceX,midy2,fromx,fromy))
		{
			if(distance(instanceX,midy1,fromx,fromy)<distance(instanceX,midy3,fromx,fromy))
				return midy1;
			else
				return midy3;
		}
		else
		{
			if(distance(instanceX,midy2,fromx,fromy)<distance(instanceX,midy3,fromx,fromy))
				return midy2;
			else
				return midy3;
		}
	}
	else
	{ // we have spaces only between the instances
		FXint min_distance=0x7fffffff;
		FXint min_midy=0;
		for(size_t t=0;t<N_instances.size()-1;t++)
		{
			FXint midy=(N_instances[t].window->getY()+N_instances[t].window->getHeight()) + ((N_instances[t+1].window->getY()-(N_instances[t].window->getY()+N_instances[t].window->getHeight()))/2);
			FXint dist=distance(instanceX,midy,fromx,fromy);
			if(dist<min_distance)
			{
				min_distance=dist;
				min_midy=midy;
			}
		}
		return min_midy;
	}
}

/*
 * This function returns the distance from the point (px,py) to the 
 * line segment that falls between (x1,y1)-(x2,y2).
 */
static float point_to_line_segment_distance(float x1,float y1,float x2,float y2,float px,float py)
{
	if(x1==x2 && y1==y2)
		return sqrt((x1-px)*(x1-px)+(y1-py)*(y1-py));

	float u=((px-x1)*(x2-x1)+(py-y1)*(y2-y1))/((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));

	if(u<0.0f || u>1.0f)
		return 999999.0f; // really far away

	float x=x1+u*(x2-x1);
	float y=y1+u*(y2-y1);
	
	return sqrt((px-x)*(px-x)+(py-y)*(py-y));
}

long FXPluginRoutingParamValue::onPaint(FXObject *sender,FXSelector sel,void *ptr)
{
	FXEvent *ev=(FXEvent *)ptr;
	FXDCWindow dc(canvas);

	const FXint width=canvas->getWidth();
	const FXint height=canvas->getHeight();

	dc.setForeground(COL_BACKGROUND);
	dc.fillRectangle(ev->rect.x,ev->rect.y,ev->rect.w,ev->rect.h);

	// draw all the node 'connectors'
	for(size_t t=0;t<N_sources.size();t++)
		for(size_t i=0;i<N_sources[t].nodes.size();i++)
			drawNodeConnector(dc,N_sources[t].nodes[i]);
	for(size_t t=0;t<N_instances.size();t++)
		for(size_t i=0;i<N_instances[t].nodes.size();i++)
			drawNodeConnector(dc,N_instances[t].nodes[i]);
	for(size_t i=0;i<N_output.nodes.size();i++)
		drawNodeConnector(dc,N_output.nodes[i]);

	// draw all the connections
	map<FXint,int> y_store; // stores the number of times in the value how many times the key has been used
	for(size_t t=0;t<connections.size();t++)
	{
		RConnection &c=connections[t];

		// figure out output node screen location
		REntity *srcEntity;
		REntity::RNode *srcNode;
		findNode(c.src,srcEntity,srcNode);

		FXint x1,y1;
		getNodePosition(srcNode,x1,y1);


		// figure out input node screen location
		REntity *destEntity;
		REntity::RNode *destNode;
		findNode(c.dest,destEntity,destNode);

		FXint x2,y2;
		getNodePosition(destNode,x2,y2);

		dc.setLineWidth(WIRE_THICKNESS);
		dc.setLineStyle(LINE_SOLID);

		dc.setForeground(c.wireColor);

		// draw wire
		{
			const int g=(int)(min(255.0f,c.gain*255.0f));

			if(N_instances.size()>0 && c.src.family==RNodeDesc::fSources && c.dest.family==RNodeDesc::fOutputs)
			{ // if the wire goes from a source all the way to the output, I'll dodge instances 
				// the idea here is to aim from the source's node to the nearest 
				// hole between or outside of the instances then from that point 
				// to the output node
				

				FXint instance_x=saved_instanceColumnX+saved_instancesColumnWidth/2;
				FXint instance_y=findNearestHoleWithinInstancesColumn(instance_x,x1,y1);
	
				if(y_store.find(instance_y)==y_store.end())
					y_store[instance_y]=0;
				else
					y_store[instance_y]++;

				instance_y+=y_store[instance_y]*6; // offset the returned y by 6 pixels each times it's used again


				c.line1.x1=x1;
				c.line1.y1=y1;
				c.line1.x2=saved_instanceColumnX;
				c.line1.y2=instance_y;

				c.line2.x1=saved_instanceColumnX;
				c.line2.y1=instance_y;
				c.line2.x2=saved_instanceColumnX+saved_instancesColumnWidth;
				c.line2.y2=instance_y;

				c.line3.x1=saved_instanceColumnX+saved_instancesColumnWidth;
				c.line3.y1=instance_y;
				c.line3.x2=x2;
				c.line3.y2=y2;

				dc.drawLine(c.line1.x1,c.line1.y1,c.line1.x2,c.line1.y2);
				dc.drawLine(c.line2.x1,c.line2.y1,c.line2.x2,c.line2.y2);
				dc.drawLine(c.line3.x1,c.line3.y1,c.line3.x2,c.line3.y2);

				dc.setLineWidth(1);
				dc.setLineStyle(LINE_ONOFF_DASH);
				//dc.setDashes(0,"\xf0",8);

				dc.setForeground(FXRGB(g,g,g));

				dc.drawLine(c.line1.x1,c.line1.y1,c.line1.x2,c.line1.y2);
				dc.drawLine(c.line2.x1,c.line2.y1,c.line2.x2,c.line2.y2);
				dc.drawLine(c.line3.x1,c.line3.y1,c.line3.x2,c.line3.y2);

			}
			else
			{ // otherwise draw the line straight there
				c.line1.x1=x1;
				c.line1.y1=y1;
				c.line1.x2=x2;
				c.line1.y2=y2;
				c.line2.x1=0x7fffffff;
				c.line3.x1=0x7fffffff;

				dc.drawLine(c.line1.x1,c.line1.y1,c.line1.x2,c.line1.y2);

				dc.setLineWidth(1);
				dc.setLineStyle(LINE_ONOFF_DASH);
				//dc.setDashes(0,"\xf0",8);

				dc.setForeground(FXRGB(g,g,g));

				dc.drawLine(c.line1.x1,c.line1.y1,c.line1.x2,c.line1.y2);
			}
		}

		dc.setLineWidth(1);
		dc.setLineStyle(LINE_SOLID);

		// draw output node solid (??? could call drawSolidNode())
		dc.setForeground(COL_OUTPUT_NODE);
		dc.fillArc(x1-NODE_RADIUS,y1-NODE_RADIUS,NODE_RADIUS*2,NODE_RADIUS*2,0*64,360*64);

		// draw output node solid (??? could call drawSolidNode())
		dc.setForeground(COL_INPUT_NODE);
		dc.fillArc(x2-NODE_RADIUS,y2-NODE_RADIUS,NODE_RADIUS*2,NODE_RADIUS*2,0*64,360*64);
	}

	// draw what's being dragged
	if(draggingWire)
	{
		// draw wire
		{
			REntity *entity;
			REntity::RNode *node;
			findNode(draggingFrom,entity,node);


			FXint x1,y1;
			getNodePosition(node,x1,y1);

			FXint x2=ev->win_x;
			FXint y2=ev->win_y;

			dc.setLineStyle(LINE_SOLID);
			dc.setLineWidth(WIRE_THICKNESS);

			dc.setForeground(CURRENT_WIRE_COLOR);
			dc.drawLine(x1,y1,x2,y2);
		}

		// draw solid node
		drawSolidNode(dc,draggingFrom);
	}

	if(mouseIsOverNode)
		drawSolidNode(dc,mouseOverNode);

	return 0;
}

bool FXPluginRoutingParamValue::isValidConnection(const RNodeDesc &desc1,const RNodeDesc &desc2)
{
	if(desc1==desc2)
		return false;

	REntity *entity1;
	REntity::RNode *node1;
	if(!findNode(desc1,entity1,node1))
		return false;

	REntity *entity2;
	REntity::RNode *node2;
	if(!findNode(desc2,entity2,node2))
		return false;

	// don't allow an input to connect to an input or output to output
	if(node1->type==node2->type)
		return false;
	// don't allow both nodes to have the same family
	else if(desc1.family==desc2.family)
		return false; // no, don't allow this

	return true;
}

long FXPluginRoutingParamValue::onMouseMove(FXObject *sender,FXSelector sel,void *ptr)
{
	FXEvent *ev=(FXEvent *)ptr;
	FXint x=ev->win_x;
	FXint y=ev->win_y;

	if(draggingWire)
	{
		/* ??? I don't know what, but this autoscrolling stuff isn't working correctly, although I can't see any difference between this situation and the on in FXWaveScrollArea which works find */
#if REZ_FOX_VERSION>=10125
		if(scrollWindow->startAutoScroll(ev))
#else
		if(scrollWindow->startAutoScroll(x,y))
#endif
			return 0;
	}

	const bool oldMouseIsOverNode=mouseIsOverNode;
	mouseIsOverNode=(ev->state&LEFTBUTTONMASK) && whereNode(x,y,mouseOverNode);

	// don't consider mouse is over the node if it's not a valid connection
	if(mouseIsOverNode && draggingWire)
		mouseIsOverNode=isValidConnection(mouseOverNode,draggingFrom);

	if(draggingWire || mouseIsOverNode!=oldMouseIsOverNode)
		canvas->update();

	return 0;
}

long FXPluginRoutingParamValue::onMouseDown(FXObject *sender,FXSelector sel,void *ptr)
{
	FXEvent *ev=(FXEvent *)ptr;
	FXint x=ev->win_x;
	FXint y=ev->win_y;

	if(ev->state&LEFTBUTTONMASK)
	{ // left button pressed
		mouseIsOverNode=whereNode(x,y,mouseOverNode); // do here since we only do in move event iff left button is down
		
		// don't consider mouse is over the node if it's not a valid connection
		if(mouseIsOverNode && draggingWire)
			mouseIsOverNode=isValidConnection(mouseOverNode,draggingFrom);

		if(mouseIsOverNode)
		{
			draggingWire=true;
			draggingFrom=mouseOverNode;
			mouseIsOverNode=false;
			canvas->update();
		}
	}
	
	return 0;
}

long FXPluginRoutingParamValue::onMouseUp(FXObject *sender,FXSelector sel,void *ptr)
{
	FXEvent *ev=(FXEvent *)ptr;

	if(FXSELTYPE(sel)==SEL_LEFTBUTTONRELEASE)
	{ // left button up
		scrollWindow->stopAutoScroll();

		if(draggingWire && mouseIsOverNode)
		{ // make a connection
			REntity *entity;
			REntity::RNode *node;
			findNode(draggingFrom,entity,node);

			RConnection c;

			// add the connection so that an output is the src and input is the dest
			if(node->type==REntity::RNode::tOutput)
			{
				c.src=draggingFrom;
				c.dest=mouseOverNode;;
			}
			else
			{
				c.src=mouseOverNode;;
				c.dest=draggingFrom;
			}
			c.gain=1.0;

			// make sure this connection doesn't already exist
			bool found=false;
			for(size_t t=0;t<connections.size();t++)
			{
				if(connections[t]==c)
					found=true;
			}

			if(!found)
			{
				c.wireColor=NEXT_WIRE_COLOR;
				connections.push_back(c);
			}

		}

		if(draggingWire) // update if draggingWire is about to change
			canvas->update();

		draggingWire=false;
		mouseIsOverNode=false;
	}
	else if(FXSELTYPE(sel)==SEL_RIGHTBUTTONRELEASE)
	{ // right button up
		FXint x=ev->win_x;
		FXint y=ev->win_y;

		// detect a right-click on a wire
		for(size_t t=0;t<connections.size();t++)
		{
			const RConnection &c=connections[t];
			if(
				                           point_to_line_segment_distance(c.line1.x1,c.line1.y1,c.line1.x2,c.line1.y2,x,y)<=((float)WIRE_THICKNESS)/2  || 
				(c.line2.x1!=0x7fffffff && point_to_line_segment_distance(c.line2.x1,c.line2.y1,c.line3.x2,c.line2.y2,x,y)<=((float)WIRE_THICKNESS)/2) || 
				(c.line3.x1!=0x7fffffff && point_to_line_segment_distance(c.line3.x1,c.line3.y1,c.line2.x2,c.line3.y2,x,y)<=((float)WIRE_THICKNESS)/2)
			)
			{ // found one
				clickedConnection=t;

				FXMenuPane menu(this);
					// ??? make sure that these get deleted when menu is deleted
					new FXMenuCommand(&menu,_("Remove Connection"),NULL,this,ID_REMOVE_CONNECTION);
					new FXMenuSeparator(&menu);
					FXHorizontalFrame *f=new FXHorizontalFrame(&menu);
						new FXLabel(f,"Gain");
						FXSlider *s=new FXSlider(f,this,ID_SET_CONNECTION_GAIN_SLIDER,SLIDER_NORMAL | LAYOUT_CENTER_Y|LAYOUT_FIX_WIDTH,0,0,100,0);
						s->setRange(0,200);
						connectionGainLabel=new FXLabel(f,"1.00x",NULL,0,SLIDER_NORMAL | LAYOUT_FILL_X);

						s->setValue((int)(connections[clickedConnection].gain*100.0));
						onSetConnectionGainSlider(s,0,NULL);

				menu.create();
				menu.popup(NULL,ev->root_x,ev->root_y);
				getApp()->runModalWhileShown(&menu);

				break;
			}
		}
	}

	return 0;
}

long FXPluginRoutingParamValue::onConfigure(FXObject *sender,FXSelector sel,void *ptr)
{
	initialLayout();
	return 1;
}

const CPluginMapping FXPluginRoutingParamValue::getValue() const
{
	CPluginMapping ret;

	ret.outputAppendCount=0;
	if(N_output.nodes.size()>actionSound->getChannelCount())
		ret.outputAppendCount=N_output.nodes.size()-actionSound->getChannelCount();

	// build CPluginMapping::inputMappings
	for(size_t t=0;t<N_instances.size();t++)
	{
		vector<vector<CPluginMapping::RInputDesc> > inputMapping;

		size_t inputPortCount=0;
		for(size_t i=0;i<pluginDesc->PortCount;i++)
		{
			const LADSPA_PortDescriptor pd=pluginDesc->PortDescriptors[i];
			if(LADSPA_IS_PORT_INPUT(pd) && LADSPA_IS_PORT_AUDIO(pd))
			{
				// set mapping as which source sound and channel maps to port i on instance t of the plugin
				vector<CPluginMapping::RInputDesc> mapping;
				for(size_t j=0;j<connections.size();j++)
				{
					if(	connections[j].src.family==RNodeDesc::fSources && 
						connections[j].dest==RNodeDesc(RNodeDesc::fInstances,t,inputPortCount))
					{
						REntity *srcEntity;
						REntity::RNode *srcNode;
						findNode(connections[j].src,srcEntity,srcNode);

						mapping.push_back(CPluginMapping::RInputDesc(
							srcNode->u.source.soundFileManagerIndex,
							connections[j].src.nodeIndex,
							(CPluginMapping::RInputDesc::WhenDataRunsOut)(srcNode->u.source.wdro->getCurrentItem()),
							srcEntity->howMuch ? (CPluginMapping::RInputDesc::HowMuch)(srcEntity->howMuch->getCurrentItem()) : (CPluginMapping::RInputDesc::HowMuch)0,
							connections[j].gain
						));
					}
				}
	
				inputMapping.push_back(mapping);
				inputPortCount++;
			}
		}
	
		ret.inputMappings.push_back(inputMapping);
	}

	// build CPluginMapping::outputMappings
	for(size_t t=0;t<N_instances.size();t++)
	{
		vector<vector<CPluginMapping::ROutputDesc> > outputMapping;

			/* ??? :-/ hmm, since the sound is not locked at this point, the number of channels could in theory change between now and when the action executes.. it would be safest to check for this in the backend */
		for(size_t i=0;i<N_output.nodes.size();i++)
		{
			vector<CPluginMapping::ROutputDesc> mapping;

			for(size_t j=0;j<connections.size();j++)
			{
				// here, we are seeing if any connections map from instance t to channel i on the output
				// if so, the node index on the plugin instance entity - inputPortCount will be the output port
				// index for the mapping
				if(	connections[j].src.family==RNodeDesc::fInstances && connections[j].src.entityIndex==t && 
					connections[j].dest==RNodeDesc(RNodeDesc::fOutputs,0,i))
				{
					REntity *srcEntity;
					REntity::RNode *srcNode;
					findNode(connections[j].src,srcEntity,srcNode);

					mapping.push_back(CPluginMapping::ROutputDesc(
						srcNode->u.instance.pluginPortIndex,
						connections[j].gain
					));
				}
			}

			outputMapping.push_back(mapping);
		}

		ret.outputMappings.push_back(outputMapping);
	}

	// build CPluginMapping::passThrus
	for(size_t t=0;t<N_output.nodes.size();t++)
	{
		vector<CPluginMapping::RInputDesc> passThru;

		for(size_t j=0;j<connections.size();j++)
		{
			if(	connections[j].dest.family==RNodeDesc::fOutputs && connections[j].dest.nodeIndex==t && 
				connections[j].src.family==RNodeDesc::fSources)
			{
				REntity *srcEntity;
				REntity::RNode *srcNode;
				findNode(connections[j].src,srcEntity,srcNode);

				passThru.push_back(CPluginMapping::RInputDesc(
					srcNode->u.source.soundFileManagerIndex,
					connections[j].src.nodeIndex,
					(CPluginMapping::RInputDesc::WhenDataRunsOut)(srcNode->u.source.wdro->getCurrentItem()),
					srcEntity->howMuch ? (CPluginMapping::RInputDesc::HowMuch)(srcEntity->howMuch->getCurrentItem()) : (CPluginMapping::RInputDesc::HowMuch)0,
					connections[j].gain
				));
			}
		}

		ret.passThrus.push_back(passThru);
	}

	ret.outputRemoveCount=0;
	if(N_output.nodes.size()<actionSound->getChannelCount())
		ret.outputRemoveCount=actionSound->getChannelCount()-N_output.nodes.size();

	// now make sure at least *some*thing got mapped to an output
	if(!ret.somethingMappedToAnOutput())
		throw EUserMessage(_("Nothing mapped to an output; check the routing"));


	return ret;
}

/*
void FXPluginRoutingParamValue::setValue(const double value)
{
	defaultValue=value;
	prvSetValue(value);
}
*/

const string FXPluginRoutingParamValue::getName() const
{
	return name;
}

void FXPluginRoutingParamValue::setSound(CSound *_actionSound)
{
	/* 
	 * Since prvSetSound() resets all the connections and
	 * created widgets, I won't call that unless something
	 * has changed from the previous time the widget was
	 * used
	 */
	bool redo=false;
	if(
		actionSound!=_actionSound ||
		actionSound->getChannelCount()!=_actionSound->getChannelCount()
	)
		redo=true;
	if(!redo)
	{
		for(size_t t=0;t<N_sources.size();t++)
		for(size_t i=0;i<N_sources[t].nodes.size();i++)
		{
			// make sure the index is even still valid in the sound file manager
			const size_t index=N_sources[t].nodes[i].u.source.soundFileManagerIndex;
			if(index>=gSoundFileManager->getOpenedCount())
			{
				redo=true;
				goto done;
			}

			// make sure it's the same filename
			const CLoadedSound *loaded=gSoundFileManager->getSound(index);
			if(loaded->getFilename()!=dynamic_cast<FXLabel *>(N_sources[t].window->getFirst())->getTipText().text())
			{
				redo=true;
				goto done;
			}

			// make sure it has the same number of channels
			if(loaded->sound->getChannelCount()!=N_sources[t].nodes.size())
			{
				redo=true;
				goto done;
			}
		}
	}

	done:
	if(redo)
	{
		prvSetSound(_actionSound);

		try
		{
			guessAtRouting();
		}
		catch(exception &e)
		{
			// I guess I couldn't guess at the routing
		}

		firstTime=false;
	}
	else
		resetSoundList(); // in case new sounds have been loaded since then
}

void FXPluginRoutingParamValue::prvSetSound(CSound *_actionSound)
{
	actionSound=_actionSound;
	/* I really only want to do this if the sound is different from last time or the number of channels has changed
	 * or the other possible inputs has changed */

	// reset information
	clearAll();
	resetSoundList();

	// setup widgets to represent sources
	for(size_t t=0;t<gSoundFileManager->getOpenedCount();t++)
	{
		CLoadedSound *loaded=gSoundFileManager->getSound(t);
		if(loaded->sound==actionSound)
		{
			newSource(loaded,t);
			break;
		}
	}

	// setup plugin one first instance
	newInstance();
	

	// setup output parent
	createOutput();

	/* ZZZ
	 * This is a stupid way to do it, but I can't figure out how else to do it:
	 * but the second time the widget is shown on a dialog the children of the 
	 * scroll window aren't sized or positioned right, but this makes it work
	 * I've tried calling recalc() and layout() in several combinations but i 
	 * couldn't get it to work... this worked...
	 */
	resize(getWidth()+1,getHeight()+1);
	resize(getWidth()-1,getHeight()-1);
}

void FXPluginRoutingParamValue::createOutput()
{
	FXVerticalFrame *output=new FXVerticalFrame(canvas,FRAME_RAISED|LAYOUT_EXPLICIT, 0,0,0,0, 1,1,1,1, 0,0);
	output->setTarget(this);
	output->setSelector(ID_OUTPUT);

	N_output.window=output;
	{
		FXLabel *label=new FXLabel(output,_("Output"),NULL,FRAME_LINE|LABEL_NORMAL | LAYOUT_FILL_X);
		label->setTarget(this);
		label->setSelector(ID_OUTPUT);
		label->setBackColor(COL_FILENAME);
		for(unsigned t=0;t<actionSound->getChannelCount();t++)
			createOutputChannel();
	}

	if(!firstTime)
	{
		output->create();
		output->show();
	}
}

// creates a new output channel and appends it
void FXPluginRoutingParamValue::createOutputChannel()
{
	FXWindow *win=new FXLabel(N_output.window,istring(N_output.nodes.size()+1).c_str(),FOXIcons->plugin_wave,JUSTIFY_LEFT|ICON_AFTER_TEXT | LAYOUT_FILL_X);
	win->setTarget(this);
	win->setSelector(ID_OUTPUT);

	N_output.nodes.push_back(REntity::RNode(win,REntity::RNode::tInput));
}

// removes an output channel from the end.. this also handles removing any connections involving that channel
void FXPluginRoutingParamValue::removeOutputChannel()
{
	if(N_output.nodes.size()>0)
	{
		// remove connections involving this channel we're going to remove
		for(size_t t=0;t<connections.size();t++)
		{
			if(connections[t].dest.family==RNodeDesc::fOutputs && connections[t].dest.nodeIndex==N_output.nodes.size()-1)
			{
				connections.erase(connections.begin()+t);
				t--;
			}
		}

		delete N_output.nodes[N_output.nodes.size()-1].window;
		N_output.nodes.pop_back();
	}
}

void FXPluginRoutingParamValue::newInstance()
{
	FXVerticalFrame *instance=new FXVerticalFrame(canvas,FRAME_RAISED|LAYOUT_EXPLICIT, 0,0,0,0, 1,1,1,1, 0,0);
	instance->setTarget(this);
	instance->setSelector(ID_INSTANCE);

	REntity N_instance(instance);
	{
		/*??? eventually wouldn't want LADSPA specific code here.. LADSPA would be wrapped in my native plugin API */
		FXLabel *label=new FXLabel(instance,pluginDesc->Name,NULL,FRAME_LINE|LABEL_NORMAL | LAYOUT_FILL_X);
		label->setTarget(this);
		label->setSelector(ID_INSTANCE_TITLE);
		label->setBackColor(COL_FILENAME);

		// input audio ports
		int inputPortCount=0;
		for(unsigned t=0;t<pluginDesc->PortCount;t++)
		{
			const LADSPA_PortDescriptor pd=pluginDesc->PortDescriptors[t];
			if(LADSPA_IS_PORT_INPUT(pd) && LADSPA_IS_PORT_AUDIO(pd))
			{
				FXWindow *win=new FXLabel(instance,pluginDesc->PortNames[t],NULL,JUSTIFY_LEFT|ICON_AFTER_TEXT | LAYOUT_FILL_X);
				win->setTarget(this);
				win->setSelector(ID_INSTANCE_PORT);

				N_instance.nodes.push_back(REntity::RNode(win,REntity::RNode::tInput));
				N_instance.nodes[N_instance.nodes.size()-1].u.instance.pluginPortIndex=inputPortCount++;
			}
		}
		// output audio ports
		int outputPortCount=0;
		for(unsigned t=0;t<pluginDesc->PortCount;t++)
		{
			const LADSPA_PortDescriptor pd=pluginDesc->PortDescriptors[t];
			if(LADSPA_IS_PORT_OUTPUT(pd) && LADSPA_IS_PORT_AUDIO(pd))
			{
				FXWindow *win=new FXLabel(instance,pluginDesc->PortNames[t],NULL,JUSTIFY_RIGHT|ICON_BEFORE_TEXT | LAYOUT_FILL_X);
				win->setTarget(this);
				win->setSelector(ID_INSTANCE_PORT);

				N_instance.nodes.push_back(REntity::RNode(win,REntity::RNode::tOutput));
				N_instance.nodes[N_instance.nodes.size()-1].u.instance.pluginPortIndex=outputPortCount++;
			}
		}
	}
	N_instances.push_back(N_instance);

	if(!firstTime)
	{
		instance->create();
		instance->show();
	}
}

void FXPluginRoutingParamValue::newSource(CLoadedSound *loaded,size_t soundFileManagerIndex)
{
	const CSound &sound=*(loaded->sound);
	const string basename=CPath(loaded->getFilename()).baseName();

	FXVerticalFrame *source=new FXVerticalFrame(canvas,FRAME_RAISED|LAYOUT_EXPLICIT, 0,0,0,0, 1,1,1,1, 0,2);
	source->setTarget(this);
	source->setSelector(ID_SOURCE);

	REntity N_source(source);
	{
		FXLabel *label=new FXLabel(source,basename.c_str(),NULL,FRAME_LINE|LABEL_NORMAL | LAYOUT_FILL_X);
		label->setTarget(this);
		label->setSelector(ID_SOURCE_TITLE);
		label->setBackColor(COL_FILENAME);
		label->setTipText(loaded->getFilename().c_str());

		if(loaded->sound!=actionSound)
		{
			FXPacker *p=new FXHorizontalFrame(source,FRAME_NONE|LAYOUT_FILL_X, 0,0,0,0, 2,2,0,0, 0,0);
			(new FXLabel(p,_("Use ")))->setFont(textFont);

			FXComboBox *c=new FXComboBox(p,0,2,NULL,0,COMBOBOX_NORMAL|COMBOBOX_STATIC | FRAME_SUNKEN | LAYOUT_FILL_X);
				// these have to maintain the order in CPluginMapping::RInputDesc::WhenDataRunsOut because the index ends up being the value of that enum
			c->appendItem(_("Selection"));
			c->appendItem(_("All"));
			c->setFont(textFont);
			N_source.howMuch=c;
		}

		for(unsigned i=0;i<sound.getChannelCount();i++)
		{
			FXHorizontalFrame *nodeParent=new FXHorizontalFrame(source,FRAME_NONE|LAYOUT_FILL_X, 0,0,0,0, 2,2,0,0, 0,0);

			FXComboBox *wdroControl=new FXComboBox(nodeParent,8,2,NULL,0,COMBOBOX_NORMAL|COMBOBOX_STATIC | FRAME_SUNKEN);
			wdroControl->setFont(textFont);
			wdroControl->setTipText(_("When Data Runs Out ..."));
				// these have to maintain the order in CPluginMapping::RInputDesc::WhenDataRunsOut because the index ends up being the value of that enum
			wdroControl->appendItem(_("Silence"));
			wdroControl->appendItem(_("Loop"));


			FXWindow *win=new FXLabel(nodeParent,istring(i+1).c_str(),FOXIcons->plugin_wave,JUSTIFY_RIGHT|ICON_BEFORE_TEXT | LAYOUT_FILL_X);
			win->setTarget(this);
			win->setSelector(ID_SOURCE_CHANNEL);


			N_source.nodes.push_back(REntity::RNode(win,REntity::RNode::tOutput));
			N_source.nodes[N_source.nodes.size()-1].u.source.soundFileManagerIndex=soundFileManagerIndex;
			N_source.nodes[N_source.nodes.size()-1].u.source.wdro=wdroControl;
		}
	}
	N_sources.push_back(N_source);

	soundList->disableItem(soundFileManagerIndex);

	if(!firstTime)
	{
		source->create();
		source->show();
	}
}

/* 
 * given the actual size of the canvas and the minumal size needed for all items and the number items
 * this returns the amount of space between each item
 */
static FXint calc_spacing(FXint actual,FXint minimal,size_t num_items)
{
	if(num_items<=0)
		return 0;
	else if(num_items==1)
		return 0; 				// [  *  ]
	else if(num_items==2)
		return (actual-minimal)/2;		// [ *  * ]
	else
		return (actual-minimal)/(num_items-1);	// [*  *  *]
}

/*
 * given the actual size of the canvas and the minumal size needed for all items and the number items
 * this returns how far to offset the first item 
 */
static FXint calc_initial_offset(FXint actual,FXint minimal,size_t num_items)
{
	if(num_items<=0)
		return 0;
	else if(num_items==1)
		return (actual-minimal)/2;		// [  *  ]
	else if(num_items==2)
		return (actual-minimal)/4;		// [ *  * ]
	else 
		return 0;				// [*  *  *]
}

/*
 * given the spacing between items and the number of items
 * this returns the total amount of spacing between all items
 */
static FXint calc_extra_spacing(FXint spacing,size_t num_items)
{
	if(num_items<=0)
		return 0;
	else if(num_items==1)
		return spacing*2;			// spacing on both sides
	else if(num_items==2)
		return spacing*3;			// spacing before beginning, in middle and after end
	else
		return spacing*num_items-1;		// spacing only between items
}

void FXPluginRoutingParamValue::initialLayout()
{
	#define H_SPACING 60	// minimal space between columns
	#define V_SPACING 15	// minimal space between objects in columns vertically

	// layout is:  sources in a column, instances in a column, output(s) in a column
	// spread out all the items in each column across the height and width of the 
	// area, and let there be a minimum that the area will shink, otherwise defined
	// by the current size of the canvas

	// calculate width/height of sources column
	FXint sourcesColumnWidth=0;
	FXint minSourcesColumnHeight=0;
	{
		for(size_t t=0;t<N_sources.size();t++)
			sourcesColumnWidth=max(sourcesColumnWidth,N_sources[t].window->getDefaultWidth());

		for(size_t t=0;t<N_sources.size();t++)
			minSourcesColumnHeight+=N_sources[t].window->getDefaultHeight();
	}

	// calculate width/height of instances column
	FXint instancesColumnWidth=0;
	FXint minInstancesColumnHeight=0;
	{
		for(size_t t=0;t<N_instances.size();t++)
			instancesColumnWidth=max(instancesColumnWidth,N_instances[t].window->getDefaultWidth());

		for(size_t t=0;t<N_instances.size();t++)
			minInstancesColumnHeight+=N_instances[t].window->getDefaultHeight();
	}

	// calculate width/height of output(s) column
	FXint outputsColumnWidth=0;
	FXint minOutputsColumnHeight=0;
	if(N_output.window)
	{
		outputsColumnWidth=max(outputsColumnWidth,N_output.window->getDefaultWidth());

		minOutputsColumnHeight+=N_output.window->getDefaultHeight();
	}

	// ----------------------------------------

	const FXint total_H_SPACING=calc_extra_spacing(H_SPACING,3);
	const FXint total_V_SPACING=calc_extra_spacing(V_SPACING,max(N_sources.size(),max(N_instances.size(),(size_t)1)));

	// calc the size that the canvas would like to be with no scroll bars
	const FXint nowCanvasWidth=scrollWindow->getWidth()-(canvasParent->getPadLeft()+canvasParent->getPadRight());
	const FXint nowCanvasHeight=scrollWindow->getHeight()-(canvasParent->getPadTop()+canvasParent->getPadBottom());

	// calc the minmum we will allow the canvas to get
	const FXint minCanvasWidth=sourcesColumnWidth+instancesColumnWidth+outputsColumnWidth;
	const FXint minCanvasHeight=max(minSourcesColumnHeight,max(minInstancesColumnHeight,minOutputsColumnHeight));

	// now determine what the canvas size will be (adding spacing so that there will always be at least that much between items)
	const FXint canvasWidth=max(nowCanvasWidth,minCanvasWidth + total_H_SPACING);
	const FXint canvasHeight=max(nowCanvasHeight,minCanvasHeight + total_V_SPACING);
	canvas->resize(canvasWidth,canvasHeight);

	// ----------------------------------------

	const FXint h_spacing=calc_spacing(canvasWidth,minCanvasWidth,3);
	FXint x=calc_initial_offset(canvasWidth,minCanvasWidth,3);

	// resize and position sources
	{
		FXint v_spacing=calc_spacing(canvasHeight,minSourcesColumnHeight,N_sources.size());
		FXint y=calc_initial_offset(canvasHeight,minSourcesColumnHeight,N_sources.size());
		for(size_t t=0;t<N_sources.size();t++)
		{
			FXWindow *win=N_sources[t].window;

			win->position(x,y,sourcesColumnWidth,win->getDefaultHeight());
			y+=win->getDefaultHeight();
			y+=v_spacing;
		}
	}
	x+=sourcesColumnWidth+h_spacing;

	// resize and position instances
	{
		// save this info for when drawing wires from a source to an output
		saved_minInstancesColumnHeight=minInstancesColumnHeight;
		saved_instancesColumnWidth=instancesColumnWidth;
		saved_instanceColumnX=x;

		FXint v_spacing=calc_spacing(canvasHeight,minInstancesColumnHeight,N_instances.size());
		FXint y=calc_initial_offset(canvasHeight,minInstancesColumnHeight,N_instances.size());
		for(size_t t=0;t<N_instances.size();t++)
		{
			FXWindow *win=N_instances[t].window;

			win->position(x,y,instancesColumnWidth,win->getDefaultHeight());
			y+=win->getDefaultHeight();
			y+=v_spacing;
		}
	}
	x+=instancesColumnWidth+h_spacing;

	// resize and position output(s)
	{
		FXint v_spacing=calc_spacing(canvasHeight,minOutputsColumnHeight,1);
		FXint y=calc_initial_offset(canvasHeight,minOutputsColumnHeight,1);
		// loop for all outputs (just 1 right now)
		if(N_output.window)
		{
			FXWindow *win=N_output.window;

			win->position(x,y,outputsColumnWidth,win->getDefaultHeight());
			y+=win->getDefaultHeight();
			y+=v_spacing;
		}
	}

	canvas->update();
}

bool FXPluginRoutingParamValue::findNode(const RNodeDesc &desc,REntity *&entity,REntity::RNode *&node) const
{
	entity=findEntity(desc);
	if(entity)
	{
		if(desc.nodeIndex>=entity->nodes.size())
			return false;
		node=&(entity->nodes[desc.nodeIndex]);
		return true;
	}
	return false;
}

FXPluginRoutingParamValue::REntity *FXPluginRoutingParamValue::findEntity(const RNodeDesc &desc) const
{
	switch(desc.family)
	{
	case RNodeDesc::fSources:
		if(desc.entityIndex>=N_sources.size())
			return NULL;
		return const_cast<REntity *>(&(N_sources[desc.entityIndex]));

	case RNodeDesc::fInstances:
		if(desc.entityIndex>=N_instances.size())
			return NULL;
		return const_cast<REntity *>(&(N_instances[desc.entityIndex]));

	case RNodeDesc::fOutputs:
		if(desc.entityIndex>=1)
			return NULL;
		return const_cast<REntity *>(&N_output);
	}

	return NULL;
}

static bool within_distance(FXint x1,FXint y1,FXint x2,FXint y2)
{
	return( ((x2-x1)*(x2-x1))+((y2-y1)*(y2-y1)) <= (NODE_RADIUS*NODE_RADIUS) );
}

/*
 * finds the node based on the screen location on the canvas
 */
bool FXPluginRoutingParamValue::whereNode(FXint x,FXint y,RNodeDesc &desc)
{
	// check sources
	for(size_t t=0;t<N_sources.size();t++)
	{
		for(size_t i=0;i<N_sources[t].nodes.size();i++)
		{
			FXint nx,ny;
			getNodePosition(&(N_sources[t].nodes[i]),nx,ny);
			if(within_distance(x,y,nx,ny))
			{
				desc.family=RNodeDesc::fSources;
				desc.entityIndex=t;
				desc.nodeIndex=i;
				return true;
			}
		}
	}

	// check instances
	for(size_t t=0;t<N_instances.size();t++)
	{
		for(size_t i=0;i<N_instances[t].nodes.size();i++)
		{
			FXint nx,ny;
			getNodePosition(&(N_instances[t].nodes[i]),nx,ny);
			if(within_distance(x,y,nx,ny))
			{
				desc.family=RNodeDesc::fInstances;
				desc.entityIndex=t;
				desc.nodeIndex=i;
				return true;
			}
		}
	}

	// check output(s)
	for(size_t i=0;i<N_output.nodes.size();i++)
	{
		FXint nx,ny;
		getNodePosition(&(N_output.nodes[i]),nx,ny);
		if(within_distance(x,y,nx,ny))
		{
			desc.family=RNodeDesc::fOutputs;
			desc.entityIndex=0;
			desc.nodeIndex=i;
			return true;
		}
	}

	return false;
}

void FXPluginRoutingParamValue::enable()
{
	FXPacker::enable();
	enableAllChildren(this);
}

void FXPluginRoutingParamValue::disable()
{
	FXPacker::disable();
	disableAllChildren(this);
}

long FXPluginRoutingParamValue::onNewInstanceButton(FXObject *sender,FXSelector sel,void *ptr)
{
	newInstance();
	initialLayout();
	scrollWindow->recalc();
	return 0;
}

long FXPluginRoutingParamValue::onAddSourceButton(FXObject *sender,FXSelector sel,void *ptr)
{
	int index=soundList->getCurrentItem();
	if(soundList->isItemEnabled(index))
	{
		newSource(gSoundFileManager->getSound(index),index);
		initialLayout();
		scrollWindow->recalc();
	}
	return 0;
}

void FXPluginRoutingParamValue::removeEntity(vector<REntity> &familyMembers,RNodeDesc::Families family,FXWindow *entity)
{
	// find the entity that was clicked on
	for(size_t t=0;t<familyMembers.size();t++)
	{
		if(familyMembers[t].window==entity)
		{
			// remove the entity
			delete entity;
			familyMembers.erase(familyMembers.begin()+t);
			
			// remove connections involving this entity 
			for(size_t i=0;i<connections.size();i++)
			{
				if(
				   (connections[i].src.family==family && connections[i].src.entityIndex==t) ||
				   (connections[i].dest.family==family && connections[i].dest.entityIndex==t)
				  )
				{
					connections.erase(connections.begin()+i);
					i--;
				}
			}
		
			// aadjust entityIndexes of other connections as needed
			for(size_t i=0;i<connections.size();i++)
			{
				if(connections[i].src.family==family && connections[i].src.entityIndex>t)
					connections[i].src.entityIndex--;
				if(connections[i].dest.family==family && connections[i].dest.entityIndex>t)
					connections[i].dest.entityIndex--;
			}

			initialLayout();
			scrollWindow->recalc();
			break;
		}
	}
}

long FXPluginRoutingParamValue::onRemoveInstance(FXObject *sender,FXSelector sel,void *ptr)
{
	// disallow being able to remove an only instance
	if(N_instances.size()<=1)
			return 0;

	if(Question(_("Are you sure you want to remove this instance?"),yesnoQues)!=yesAns)
		return 0;

	// get the parent widget that's directly in the canvas
	FXWindow *entity=NULL;
	if(FXSELID(sel)==ID_INSTANCE)
		entity=(FXWindow *)sender;
	else if(FXSELID(sel)==ID_INSTANCE_TITLE)
		entity=((FXWindow *)sender)->getParent();
	else if(FXSELID(sel)==ID_INSTANCE_PORT)
		entity=((FXWindow *)sender)->getParent();
	else
		throw runtime_error(string(__func__)+" -- internal error -- unhandled case");


	removeEntity(N_instances,RNodeDesc::fInstances,entity);

	return 0;
}

long FXPluginRoutingParamValue::onRemoveSource(FXObject *sender,FXSelector sel,void *ptr)
{
	if(Question(_("Are you sure you want to remove this source?"),yesnoQues)!=yesAns)
		return 0;

	// get the parent widget that's directly in the canvas
	FXWindow *entity=NULL;
	if(FXSELID(sel)==ID_SOURCE)
		entity=(FXWindow *)sender;
	else if(FXSELID(sel)==ID_SOURCE_TITLE)
		entity=((FXWindow *)sender)->getParent();
	else if(FXSELID(sel)==ID_SOURCE_CHANNEL)
		entity=((FXWindow *)sender)->getParent()->getParent();
	else
		throw runtime_error(string(__func__)+" -- internal error -- unhandled case");

	// re-enable this item in the sound list
	for(size_t t=0;t<N_sources.size();t++)
	{
		if(N_sources[t].window==entity)
		{
			soundList->enableItem(N_sources[t].nodes[0].u.source.soundFileManagerIndex);
			break;
		}
	}

	removeEntity(N_sources,RNodeDesc::fSources,entity);

	return 0;
}

long FXPluginRoutingParamValue::onRemoveConnection(FXObject *sender,FXSelector sel,void *ptr)
{
	connections.erase(connections.begin()+clickedConnection);
	canvas->update();
	return 0;
}

long FXPluginRoutingParamValue::onSetConnectionGainSlider(FXObject *sender,FXSelector sel,void *ptr)
{
	FXSlider *slider=(FXSlider *)sender;
	if(FXSELTYPE(sel)==SEL_RIGHTBUTTONRELEASE)
		slider->setValue(100);
		
	float g=slider->getValue()/100.0;
	connections[clickedConnection].gain=g;
	connectionGainLabel->setText((istring(g,3,2,true)+"x").c_str());

	return 0;
}

long FXPluginRoutingParamValue::onOutputPopup(FXObject *sender,FXSelector sel,void *ptr)
{
	FXEvent *event=(FXEvent*)ptr;

	FXMenuPane menu(this);
		// ??? make sure that these get deleted when menu is deleted
		new FXMenuCommand(&menu,_("Append New Channel"),NULL,this,ID_APPEND_NEW_OUTPUT_CHANNEL);
		new FXMenuCommand(&menu,_("Remove Last Channel"),NULL,this,ID_REMOVE_LAST_OUTPUT_CHANNEL);

	menu.create();
	menu.popup(NULL,event->root_x,event->root_y);
	getApp()->runModalWhileShown(&menu);
	return 0;
}

#include "../backend/CSound_defs.h"
long FXPluginRoutingParamValue::onAppendNewOutputChannel(FXObject *sender,FXSelector sel,void *ptr)
{
	if(N_output.nodes.size()<MAX_CHANNELS-1)
	{
		createOutputChannel();
		N_output.nodes[N_output.nodes.size()-1].window->create();
		initialLayout(); // :-/ didn't really want to do this, but I guess I have to since things are LAYOUT_EXPLICIT
	}
	else
		Error(_("Cannot exceed maximum number of channels: ")+istring(MAX_CHANNELS));
	return 0;
}

long FXPluginRoutingParamValue::onRemoveLastOutputChannel(FXObject *sender,FXSelector sel,void *ptr)
{
	if(N_output.nodes.size()>1)
	{
		removeOutputChannel();
		initialLayout(); // :-/ didn't really want to do this, but I guess I have to since things are LAYOUT_EXPLICIT
	}
	else
		Error(_("Cannot remove the last output channel"));
	return 0;
}

long FXPluginRoutingParamValue::onResetButton(FXObject *sender,FXSelector sel,void *ptr)
{
	if(connections.size()!=0 && Question(_("Are you sure you want to clear the routing?"),yesnoQues)!=yesAns)
		return 0;
	RESET_WIRE_COLOR
	prvSetSound(actionSound);
	initialLayout();
	canvas->update();
	return 0;
}

long FXPluginRoutingParamValue::onDefaultButton(FXObject *sender,FXSelector sel,void *ptr)
{
	if(connections.size()!=0 && Question(_("Are you sure you want to discard the current routing and guess at the desired routing?"),yesnoQues)!=yesAns)
		return 0;

	try
	{
		RESET_WIRE_COLOR
		guessAtRouting();
	}
	catch(exception &e)
	{
		Warning(e.what());
	}

	return 0;
}

void FXPluginRoutingParamValue::guessAtRouting()
{
	const CPluginMapping mapping=getLADSPADefaultMapping(pluginDesc,actionSound);

	clearAll();

	// create the one source of actionSound
	for(size_t t=0;t<gSoundFileManager->getOpenedCount();t++)
	{
		if(gSoundFileManager->getSound(t)->sound==actionSound)
		{
			newSource(gSoundFileManager->getSound(t),t);
			break;
		}
	}

	createOutput();

	// count the number of input ports since we have to add that to the nodeIndex when processing the output mappings
	size_t inputPortCount=0;
	for(size_t i=0;i<pluginDesc->PortCount;i++)
	{
		const LADSPA_PortDescriptor pd=pluginDesc->PortDescriptors[i];
		if(LADSPA_IS_PORT_INPUT(pd) && LADSPA_IS_PORT_AUDIO(pd))
			inputPortCount++;
	}

	// create the connections from the source to the instance(s) and instance(s) to the ouput
	for(size_t t=0;t<mapping.inputMappings.size();t++)
	{
		newInstance();

		const vector<vector<CPluginMapping::RInputDesc> > &inputMapping=mapping.inputMappings[t];

		// assuming that the soundFileManagerIndex and wdro values can be ignored
		for(size_t i=0;i<inputMapping.size();i++)
		{
			for(size_t k=0;k<inputMapping[i].size();k++)
			{
				connections.push_back(RConnection(
					RNodeDesc(RNodeDesc::fSources,0,inputMapping[i][k].channel),
					RNodeDesc(RNodeDesc::fInstances,t,i),
					inputMapping[i][k].gain
				));
				connections[connections.size()-1].wireColor=NEXT_WIRE_COLOR;
			}
		}

		const vector<vector<CPluginMapping::ROutputDesc> > &outputMapping=mapping.outputMappings[t];
		for(size_t i=0;i<outputMapping.size();i++)
		{
			for(size_t k=0;k<outputMapping[i].size();k++)
			{
				connections.push_back(RConnection(
					RNodeDesc(RNodeDesc::fInstances,t,outputMapping[i][k].channel+inputPortCount),
					RNodeDesc(RNodeDesc::fOutputs,0,i),
					outputMapping[i][k].gain
				));
				connections[connections.size()-1].wireColor=NEXT_WIRE_COLOR;
			}
		}
	}
	// see note: ZZZ
	resize(getWidth()+1,getHeight()+1);
	resize(getWidth()-1,getHeight()-1);

	canvas->update();
}

void FXPluginRoutingParamValue::clearAll()
{
	draggingWire=false;
	mouseIsOverNode=false;

	connections.clear();

	N_sources.clear();
	N_instances.clear();
	N_output.window=NULL;
	N_output.nodes.clear();

	while(canvas->numChildren()>0)
		delete canvas->getFirst();
}

void FXPluginRoutingParamValue::resetSoundList()
{
	soundList->clearItems();

	for(size_t t=0;t<gSoundFileManager->getOpenedCount();t++)
	{
		soundList->appendItem(gSoundFileManager->getSound(t)->getFilename().c_str());

		// select the action sound.. we will put it up there already
		if(gSoundFileManager->getSound(t)->sound==actionSound)
		{
			soundList->setCurrentItem(soundList->getNumItems()-1);
			//soundList->disableItem(soundList->getNumItems()-1); done in newSource
		}
	}
}

void FXPluginRoutingParamValue::readFromFile(const string &prefix,CNestedDataFile *f)
{
	RESET_WIRE_COLOR
	const string key=prefix DOT getName();
	clearAll();
	resetSoundList();
	canvas->update();
	getApp()->forceRefresh(); // redraw NOW

	// read sources
	const size_t sourceCount=f->getValue<size_t>(key DOT "sourceCount");
	for(size_t t=0;t<sourceCount;t++)
	{
		bool sourceCreated=false;
		const string source_key=key DOT "source"+istring(t);

		const size_t nodeCount=f->getValue<size_t>(source_key DOT "nodeCount");

		if(f->getValue<bool>(source_key DOT "actionSound"))
		{ // this source was the action Sound
			// find the soundFileManagerIndex of actionSound
			size_t actionSoundIndex=0xffffffff;
			for(size_t k=0;k<gSoundFileManager->getOpenedCount();k++)
			{
				if(gSoundFileManager->getSound(k)->sound==actionSound)
				{
					actionSoundIndex=k;
					break;
				}
			}
			if(actionSoundIndex==0xffffffff)
				throw runtime_error(string(__func__)+" -- internal error -- could not find action sound in the gSoundFileManager");
			newSource(gSoundFileManager->getSound(actionSoundIndex),actionSoundIndex);
			sourceCreated=true;
		}
		else
		{ // try to find one with the filename that was saved with the preset
			const string filename=f->getValue<string>(source_key DOT "filename");
			bool found=false;
			for(size_t k=0;k<gSoundFileManager->getOpenedCount();k++)
			{
				if(gSoundFileManager->getSound(k)->getFilename()==filename)
				{
					if(gSoundFileManager->getSound(k)->sound==actionSound)
					{
						Warning(filename+string("\n\n")+_("This source referenced in the preset which was not the action sound when the preset was saved is now the action sound.  Some connections will likely be missing or wrong."));
						found=true;
						sourceCreated=false;
					}
					else
					{
						newSource(gSoundFileManager->getSound(k),k);
						sourceCreated=true;
						found=true;
					}
					break;
				}
			}

			if(!found)
			{ // filename wasn't found loaded in the soundFileManager
				if(CPath(filename).exists())
				{ // file exists, so prompt to load it
					if(Question(string(_("When this preset was saved there was a file loaded named:"))+"\n     "+filename+"\n"+_("This file is currently not loaded.\nWould you like to load it now?")+"\n"+_("If you do not, then the preset cannot not be fully recreated."),yesnoQues)==yesAns)
					{ // attempt to load a file by that name
						if(openSound(gSoundFileManager,filename))
						{ // a new file was loaded, so use it in place of the one not found
							soundList->appendItem(filename.c_str());
							newSource(gSoundFileManager->getSound(gSoundFileManager->getOpenedCount()-1),gSoundFileManager->getOpenedCount()-1);
							sourceCreated=true;
						}
						else
						{
							Message(_("The preset cannot be fully recreated."));
							return;
						}
					}
				}
				else
				{
					if(Question(string(_("When this preset was saved there was a file loaded named:"))+"\n     "+filename+"\n"+_("This file now does not exist.\nWould you like to load another file in its place?")+"\n"+_("If you do not, then the preset cannot not be fully recreated."),yesnoQues)==yesAns)
					{ // load a new file in its place
						if(openSound(gSoundFileManager))
						{ // a new file was loaded, so use it in place of the one not found
							soundList->appendItem(filename.c_str());
							newSource(gSoundFileManager->getSound(gSoundFileManager->getOpenedCount()-1),gSoundFileManager->getOpenedCount()-1);
							sourceCreated=true;
						}
						else
						{
							Message(_("The preset cannot be fully recreated."));
						}
					}
				}
			}
		}

		if(sourceCreated)
		{
			if(N_sources[N_sources.size()-1].nodes.size()<nodeCount)
				Warning(gSoundFileManager->getSound(N_sources[N_sources.size()-1].nodes[0].u.source.soundFileManagerIndex)->getFilename()+"\n\n"+_("This file has fewer channels than the one used when the preset was saved.  It may not be possible to restore some of the connections."));

			// the howMuch combobox
			if(N_sources[N_sources.size()-1].howMuch)
				N_sources[N_sources.size()-1].howMuch->setCurrentItem(f->getValue<int>(source_key DOT "howMuch"));

			// set wdro (when data runs out)
			for(size_t t=0;t<nodeCount;t++)
			{
				if(t>=N_sources[N_sources.size()-1].nodes.size())
					break;
				const int wdro=f->getValue<int>(source_key DOT "wdro"+istring(t));
				if(wdro<N_sources[N_sources.size()-1].nodes[t].u.source.wdro->getNumItems())
					N_sources[N_sources.size()-1].nodes[t].u.source.wdro->setCurrentItem(wdro);
			}
		}
	}
	
	// read instances
	const size_t instanceCount=f->getValue<size_t>(key DOT "instanceCount");
	for(size_t t=0;t<instanceCount;t++)
		newInstance();

	// read output
	createOutput();
	const size_t outputChannelCount=f->getValue<size_t>(key DOT "outputChannelCount");
	for(size_t t=actionSound->getChannelCount();t<outputChannelCount;t++)
	{
		createOutputChannel();
		N_output.nodes[N_output.nodes.size()-1].window->create();
	}
	/* probably don't want to do this; it could be annoying
	for(size_t t=outputChannelCount;t<actionSound->getChannelCount();t++)
		removeOutputChannel();
	*/

	// read connections
	const string connections_key=key DOT "connections";
	const size_t connectionCount=f->getValue<size_t>(connections_key DOT "connectionCount");
	for(size_t t=0;t<connectionCount;t++)
	{
		const string key=connections_key DOT "c"+istring(t);

		const float gain=f->getValue<float>(key DOT "gain");

		RNodeDesc src;
		src.family=(RNodeDesc::Families)f->getValue<int>(key DOT "srcFamily");
		src.entityIndex=f->getValue<size_t>(key DOT "srcEntity");
		src.nodeIndex=f->getValue<size_t>(key DOT "srcNode");

		RNodeDesc dest;
		dest.family=(RNodeDesc::Families)f->getValue<int>(key DOT "destFamily");
		dest.entityIndex=f->getValue<size_t>(key DOT "destEntity");
		dest.nodeIndex=f->getValue<size_t>(key DOT "destNode");

		if(isValidConnection(src,dest))
		{ // nodes exist, connection would be valid, so connect
			connections.push_back(RConnection(src,dest,gain));
			connections[connections.size()-1].wireColor=NEXT_WIRE_COLOR;
		}
	}


	// see note ZZZ
	resize(getWidth()+1,getHeight()+1);
	resize(getWidth()-1,getHeight()-1);
	canvas->update();
}

void FXPluginRoutingParamValue::writeToFile(const string &prefix,CNestedDataFile *f) const
{
	/*
	 * We definately want to save how the current action sound was routed
	 * Secondly, the user might have brought in sources other than the 
	 * current actionSound.  To recall these, I will save the filename and
	 * loaded index.  The next time the plugin, if a sound is loaded with the
	 * filename, I'll use it.  If not, I'll try the loaded Index, and finally
	 * I will just leave it out.
	 */
	const string key=prefix DOT getName();

	// write sources
	f->createValue<size_t>(key DOT "sourceCount",N_sources.size());
	for(size_t t=0;t<N_sources.size();t++)
	{
		const string source_key=key DOT "source"+istring(t);

		f->createValue<size_t>(source_key DOT "nodeCount",N_sources[t].nodes.size());

		if(gSoundFileManager->getSound(N_sources[t].nodes[0].u.source.soundFileManagerIndex)->sound==actionSound)
			f->createValue<bool>(source_key DOT "actionSound",true);
		f->createValue<string>(source_key DOT "filename",gSoundFileManager->getSound(N_sources[t].nodes[0].u.source.soundFileManagerIndex)->getFilename());
		if(N_sources[t].howMuch)
			f->createValue<int>(source_key DOT "howMuch",N_sources[t].howMuch->getCurrentItem());

		for(size_t k=0;k<N_sources[t].nodes.size();k++)
		{ // write all attributes of each node
			f->createValue<int>(source_key DOT "wdro"+istring(k),N_sources[t].nodes[k].u.source.wdro->getCurrentItem());
		}
	}

	// write instances
	f->createValue<size_t>(key DOT "instanceCount",N_instances.size());

	// write output
	f->createValue<size_t>(key DOT "outputChannelCount",N_output.nodes.size());

	// write information about connections between nodes
	const string connections_key=key DOT "connections";
	f->createValue<size_t>(connections_key DOT "connectionCount",connections.size());
	for(size_t t=0;t<connections.size();t++)
	{
		const string key=connections_key DOT "c"+istring(t);
		f->createValue<float>(key DOT "gain",connections[t].gain);

		f->createValue<int>(key DOT "srcFamily",connections[t].src.family);
		f->createValue<size_t>(key DOT "srcEntity",connections[t].src.entityIndex);
		f->createValue<size_t>(key DOT "srcNode",connections[t].src.nodeIndex);

		f->createValue<int>(key DOT "destFamily",connections[t].dest.family);
		f->createValue<size_t>(key DOT "destEntity",connections[t].dest.entityIndex);
		f->createValue<size_t>(key DOT "destNode",connections[t].dest.nodeIndex);
	}
}


