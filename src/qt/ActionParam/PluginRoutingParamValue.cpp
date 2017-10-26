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

#warning there are sizing glitches with this implementation.. it was ported from the FOX implementation and some things did not work right in Qt.  I want to create a more modern-behaving interface anyway (animations, animated splines for patch cables, arbitrary layout, etc).  Therefore, I will not take time to fix these issues now.

#include "PluginRoutingParamValue.h"

#include <assert.h>

#include <QMenu>
#include <QToolButton>
#include <QLabel>
#include <QComboBox>
#include <QPainter>
#include <QPaintEvent>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCursor>

#include <map>

#include <CPath.h>
#include <istring>
#include <CNestedDataFile/CNestedDataFile.h>

#include "SoundFileManager.h"
#include "../../backend/CLoadedSound.h"
#include "../../backend/CSound.h"
#include "../../backend/AStatusComm.h"
#include "../../backend/main_controls.h"
#include "../../backend/AAction.h" // for EUserMessage
#include "../backend/CSound_defs.h"

#include "../../backend/LADSPA/ladspa.h" /* ??? eventually don't want LADSPA specific code in here */
#include "../../backend/LADSPA/utils.h"

//#include "CFOXIcons.h"

#define COL_BACKGROUND QColor(44,44,64)
#define COL_FILENAME QColor(164,164,184)
#define COL_FILENAME_STRING "#a4a4b8"

#include <QColor>


static int wire_color_index=0;
static QColor wire_colors[]={QColor(255,255,0),QColor(255,0,0),QColor(128,128,255),QColor(0,255,0),QColor(255,0,255),QColor(0,255,255),QColor(255,128,0),QColor(128,255,0),QColor(0,255,128),QColor(0,128,255),QColor(255,0,128),QColor(128,0,255)};
#define N_WIRE_COLORS (sizeof(wire_colors)/sizeof(*wire_colors))
#define RESET_WIRE_COLOR {wire_color_index=0;}
#define CURRENT_WIRE_COLOR (wire_colors[wire_color_index%N_WIRE_COLORS])
#define NEXT_WIRE_COLOR (wire_colors[(wire_color_index++)%N_WIRE_COLORS])

#define COL_INPUT_NODE QColor(0,255,255)
#define COL_OUTPUT_NODE QColor(255,128,0)

#define NODE_RADIUS 4
#define WIRE_THICKNESS 5

#define MAX_INSTANCES 1024

/*
 * ??? I could store all sources, instances and outputs in a single vector or map
 * with a enum tag as to what family it belonged to.. then I wouldn't have to 
 * loop 3 times in most of the code stanzas
 *
 * ??? add gettext hooks wherever necessary
 *
 * ??? if this were more generic, the instance's could be created from a list of loaded plugins and input could be fed to any 
 */


PluginRoutingParamValue::PluginRoutingParamValue(const char *_name,const LADSPA_Descriptor *_desc) :
	name(_name),
	pluginDesc(_desc),
	actionSound(NULL),

	firstTime(true),

	N_output(this,NULL),

	mouseIsOverNode(false),
	draggingWire(false)
{
	setupUi(this);
	canvas->prpv=this;
	
	// since we store pointers to within the vector, we must reserve space to keep the pointers from changing.
	// it's not a great way of storing the instances and sources, I have notes on a better way of doing it
	N_sources.reserve(MAX_INSTANCES);
	N_instances.reserve(MAX_INSTANCES);
	N_output.paramValue=this;

	// re-parent canvas to be within a QScrollArea
	{
		scrollArea=new QScrollArea;
		scrollArea->setWidget(canvas);
		scrollArea->setWidgetResizable(true);
		((QVBoxLayout *)layout())->insertWidget(0,scrollArea);
	}

	textFont=font();
	textFont.setPointSize(8);

	setMinimumSize(690,230);

	

	//FOXIcons->plugin_wave->setTransparentColor(QColor(255,0,255));
}

PluginRoutingParamValue::~PluginRoutingParamValue()
{
}

/*
 * Gives you the screen position on the canvas given a node
 */
void PluginRoutingParamValue::REntity::RNode::getPosition(QWidget *canvas,int &x,int &y)
{
// ??? need to map to coordinate just outside of entity's window really.. fix me
	const QPoint given(
		type==REntity::RNode::tInput ? -(2*NODE_RADIUS) : (window->width()+(2*NODE_RADIUS)), // left or right side
		window->height()/2
	);
	QPoint ret=window->mapTo(canvas,given);

	x=ret.x();
	y=ret.y();
}

void PluginRoutingParamValue::REntity::RNode::draw(QPainter &p)
{
	// get x and y on canvas of the left (or right) edge of a node
	int x1,y;
	getPosition(entity->paramValue->canvas,x1,y);

	int x2=x1+(type==REntity::RNode::tInput ? NODE_RADIUS*2 : -NODE_RADIUS*2);

	p.save();

	// draw wire to node
	{
		QPen pen(wire_colors[0]);
		pen.setWidth(WIRE_THICKNESS);
		p.setPen(pen);
		p.drawLine(x1,y,x2,y);
	}

	// draw node
	{
		QPen pen(type==tInput ? COL_INPUT_NODE : COL_OUTPUT_NODE);
		pen.setWidth(1);
		p.setPen(pen);
		p.drawEllipse(x1-NODE_RADIUS,y-NODE_RADIUS,NODE_RADIUS*2,NODE_RADIUS*2);
	}

	p.restore();
}

void PluginRoutingParamValue::drawSolidNode(QPainter &p,const RNodeDesc &desc)
{
	REntity *entity;
	REntity::RNode *node;
	findNode(desc,entity,node);

	int x,y;
	node->getPosition(canvas,x,y);

	p.save();

	p.setBrush(node->type==REntity::RNode::tInput ? COL_INPUT_NODE : COL_OUTPUT_NODE);
	p.drawEllipse(x-NODE_RADIUS,y-NODE_RADIUS,NODE_RADIUS*2,NODE_RADIUS*2);

	p.restore();
}

int PluginRoutingParamValue::findNearestHoleWithinInstancesColumn(int instanceX,int fromx,int fromy)
{
	int canvasHeight=canvas->height();
	/*
	const int v_spacing=calc_spacing(canvasHeight,saved_minInstancesColumnHeight,N_instances.size());
	const int y=calc_initial_offset(canvasHeight,saved_minInstancesColumnHeight,N_instances.size());
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
		int midy1=N_instances[0].window->y()/2;
		int midy2=(N_instances[0].window->y()+N_instances[0].window->height()) + ((canvasHeight-(N_instances[0].window->y()+N_instances[0].window->height()))/2);


		if( distance(instanceX,midy1,fromx,fromy)<distance(instanceX,midy2,fromx,fromy))
			return midy1;
		else
			return midy2;
	}
	else if(N_instances.size()==2)
	{ // we have two spaces above and below the two instances and one in between
		int midy1=N_instances[0].window->y()/2;
		int midy2=(N_instances[0].window->y()+N_instances[0].window->height()) + ((N_instances[1].window->y()-(N_instances[0].window->y()+N_instances[0].window->height()))/2);
		int midy3=(N_instances[1].window->y()+N_instances[1].window->height()) + ((canvasHeight-(N_instances[1].window->y()+N_instances[1].window->height()))/2);

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
		int min_distance=0x7fffffff;
		int min_midy=0;
		for(size_t t=0;t<N_instances.size()-1;t++)
		{
			int midy=(N_instances[t].window->y()+N_instances[t].window->height()) + ((N_instances[t+1].window->y()-(N_instances[t].window->y()+N_instances[t].window->height()))/2);
			int dist=distance(instanceX,midy,fromx,fromy);
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

void PluginRoutingCanvas::paintEvent(QPaintEvent *ev) { prpv->drawCanvas(ev); }

void PluginRoutingParamValue::drawCanvas(QPaintEvent *e)
{
	QPainter p(canvas);

	const int width=this->width();
	const int height=this->height();

	p.fillRect(e->rect(),COL_BACKGROUND);


	// draw all the node 'connectors'
	for(size_t t=0;t<N_sources.size();t++)
		for(size_t i=0;i<N_sources[t].nodes.size();i++)
			N_sources[t].nodes[i].draw(p);
	for(size_t t=0;t<N_instances.size();t++)
		for(size_t i=0;i<N_instances[t].nodes.size();i++)
			N_instances[t].nodes[i].draw(p);
	for(size_t i=0;i<N_output.nodes.size();i++)
		N_output.nodes[i].draw(p);


	// draw all the connections
	map<int,int> y_store; // stores the number of times in the value how many times the key has been used
	for(size_t t=0;t<connections.size();t++)
	{
		RConnection &c=connections[t];

		// figure out output node screen location
		REntity *srcEntity;
		REntity::RNode *srcNode;
		findNode(c.src,srcEntity,srcNode);

		int x1,y1;
		srcNode->getPosition(canvas,x1,y1);


		// figure out input node screen location
		REntity *destEntity;
		REntity::RNode *destNode;
		findNode(c.dest,destEntity,destNode);

		int x2,y2;
		destNode->getPosition(canvas,x2,y2);

	
		QPen pen;

		pen.setWidth(WIRE_THICKNESS);
		pen.setStyle(Qt::SolidLine);
		pen.setColor(c.wireColor);
		
		p.setPen(pen);

		// draw wire
		{
			const int g=(int)(min(255.0f,c.gain*255.0f));

			if(N_instances.size()>0 && c.src.family==RNodeDesc::fSources && c.dest.family==RNodeDesc::fOutputs)
			{ // if the wire goes from a source all the way to the output, I'll dodge instances 
				// the idea here is to aim from the source's node to the nearest 
				// hole between or outside of the instances then from that point 
				// to the output node
				

				int instance_x=saved_instanceColumnX+saved_instancesColumnWidth/2;
				int instance_y=findNearestHoleWithinInstancesColumn(instance_x,x1,y1);
	
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

				p.drawLine(c.line1.x1,c.line1.y1,c.line1.x2,c.line1.y2);
				p.drawLine(c.line2.x1,c.line2.y1,c.line2.x2,c.line2.y2);
				p.drawLine(c.line3.x1,c.line3.y1,c.line3.x2,c.line3.y2);

				pen.setWidth(1);
				pen.setStyle(Qt::DashDotLine);
				//pen.setDashes(0,"\xf0",8);
				pen.setColor(QColor(g,g,g));
				p.setPen(pen);

				p.drawLine(c.line1.x1,c.line1.y1,c.line1.x2,c.line1.y2);
				p.drawLine(c.line2.x1,c.line2.y1,c.line2.x2,c.line2.y2);
				p.drawLine(c.line3.x1,c.line3.y1,c.line3.x2,c.line3.y2);

			}
			else
			{ // otherwise draw the line straight there
				c.line1.x1=x1;
				c.line1.y1=y1;
				c.line1.x2=x2;
				c.line1.y2=y2;
				c.line2.x1=0x7fffffff;
				c.line3.x1=0x7fffffff;

				p.drawLine(c.line1.x1,c.line1.y1,c.line1.x2,c.line1.y2);

				pen.setWidth(1);
				pen.setStyle(Qt::DashDotLine);
				//pen.setDashes(0,"\xf0",8);
				pen.setColor(QColor(g,g,g));
				p.setPen(pen);

				p.drawLine(c.line1.x1,c.line1.y1,c.line1.x2,c.line1.y2);
			}
		}

		pen.setWidth(1);
		pen.setStyle(Qt::SolidLine);
		pen.setColor(COL_OUTPUT_NODE);
		p.setPen(pen);

		p.save();

		// draw output node solid (??? could call drawSolidNode())
		p.setBrush(COL_OUTPUT_NODE);
		p.drawEllipse(x1-NODE_RADIUS,y1-NODE_RADIUS,NODE_RADIUS*2,NODE_RADIUS*2);

		// draw output node solid (??? could call drawSolidNode())
		p.setBrush(COL_INPUT_NODE);
		p.drawEllipse(x2-NODE_RADIUS,y2-NODE_RADIUS,NODE_RADIUS*2,NODE_RADIUS*2);

		p.restore();
	}

	// draw what's being dragged
	if(draggingWire)
	{
		// draw wire
		{
			REntity *entity;
			REntity::RNode *node;
			findNode(draggingFrom,entity,node);


			int x1,y1;
			node->getPosition(canvas,x1,y1);

			QPoint mp=mapFromGlobal(QCursor::pos());
			int x2=mp.x();
			int y2=mp.y();

			QPen pen;
			pen.setStyle(Qt::SolidLine);
			pen.setWidth(WIRE_THICKNESS);
			pen.setColor(CURRENT_WIRE_COLOR);
			p.setPen(pen);
			p.drawLine(x1,y1,x2,y2);
		}

		// draw solid node
		drawSolidNode(p,draggingFrom);
	}

	if(mouseIsOverNode)
		drawSolidNode(p,mouseOverNode);
}

bool PluginRoutingParamValue::isValidConnection(const RNodeDesc &desc1,const RNodeDesc &desc2)
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

// ??? this needs to be events on the canvas
void PluginRoutingParamValue::mouseMoveEvent(QMouseEvent *ev)
{
	int x=ev->x();
	int y=ev->y();

#warning ??? reimplement me
#if 0
	if(draggingWire)
	{
		/* ??? I don't know what, but this autoscrolling stuff isn't working correctly, although I can't see any difference between this situation and the on in FXWaveScrollArea which works fine */
#if REZ_FOX_VERSION>=10125
		if(scrollArea->startAutoScroll(ev))
#else
		if(scrollArea->startAutoScroll(x,y))
#endif
			return 0;
	}
#endif

	const bool oldMouseIsOverNode=mouseIsOverNode;
	mouseIsOverNode=(ev->buttons()&Qt::LeftButton) && whereNode(x,y,mouseOverNode);

	// don't consider mouse is over the node if it's not a valid connection
	if(mouseIsOverNode && draggingWire)
		mouseIsOverNode=isValidConnection(mouseOverNode,draggingFrom);

	if(draggingWire || mouseIsOverNode!=oldMouseIsOverNode)
		canvas->update();
}

void PluginRoutingParamValue::mousePressEvent(QMouseEvent *ev)
{
	int x=ev->x();
	int y=ev->y();

	if(ev->buttons()&Qt::LeftButton)
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
}

void PluginRoutingParamValue::mouseReleaseEvent(QMouseEvent *ev)
{
	if(ev->button()==Qt::LeftButton)
	{ // left button up
#warning reimplement me ???
#if 0
		scrollArea->stopAutoScroll();
#endif

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
	else if(ev->button()==Qt::RightButton)
	{ // right button up
		int x=ev->x();
		int y=ev->y();

		// detect a right-click on a wire
		for(size_t t=0;t<connections.size();t++)
		{
			const RConnection &c=connections[t];
			if(
				                           point_to_line_segment_distance(c.line1.x1,c.line1.y1,c.line1.x2,c.line1.y2,x,y)<=((float)WIRE_THICKNESS)/2  || 
				(c.line2.x1!=0x7fffffff && point_to_line_segment_distance(c.line2.x1,c.line2.y1,c.line2.x2,c.line2.y2,x,y)<=((float)WIRE_THICKNESS)/2) || 
				(c.line3.x1!=0x7fffffff && point_to_line_segment_distance(c.line3.x1,c.line3.y1,c.line3.x2,c.line3.y2,x,y)<=((float)WIRE_THICKNESS)/2)
			)
			{ // found one
				clickedConnection=t;

				QMenu menu;
				menu.setLayout(new QVBoxLayout);
				menu.layout()->setContentsMargins(0,0,0,0);
				menu.layout()->setSpacing(1);
					QAction *removeAction=menu.addAction(_("Remove Connection"));
					menu.addSeparator();
					menu.popup(ev->globalPos()); // if we don't popup() now, then it doesn't know geometry of action(s)
					((QBoxLayout*)(menu.layout()))->addSpacing(menu.actionGeometry(removeAction).height());

					// create gain control: "Gain [---#---] 32.2x [Reset]"
					QWidget *w=new QWidget(&menu);
						w->setLayout(new QHBoxLayout);
						w->layout()->setContentsMargins(3,3,3,3);
						w->layout()->setSpacing(3);
						w->layout()->addWidget(new QLabel(_("Gain")));
						connectionGainSlider=new QSlider(Qt::Horizontal);
						connect(connectionGainSlider,SIGNAL(valueChanged(int)),this,SLOT(onConnectionGainChanged(int)));
						connectionGainSlider->setMinimumWidth(150);
						connectionGainSlider->setRange(0,200);
						w->layout()->addWidget(connectionGainSlider);
						w->layout()->addWidget(connectionGainLabel=new QLabel(""));
						QToolButton *b=new QToolButton;
						b->setText("Reset");
						connect(b,SIGNAL(clicked()),this,SLOT(onResetConnectionGain()));
						w->layout()->addWidget(b);
						connectionGainSlider->setValue((int)(connections[clickedConnection].gain*100.0));
					menu.layout()->addWidget(w);

				menu.hide();
				QAction *a=menu.exec(ev->globalPos());
				if(a==removeAction)
				{
					connections.erase(connections.begin()+t);
					canvas->update();
				}

				//??? check slider position too and set gain

				break; // found one
			}
		}
	}
}

void PluginRoutingParamValue::onConnectionGainChanged(int v)
{
	float g=v/100.0;
	connections[clickedConnection].gain=g;
	connectionGainLabel->setText((istring(g,3,2,true)+"x").c_str());
}

void PluginRoutingParamValue::onResetConnectionGain()
{
	connectionGainSlider->setValue(100);
}

void PluginRoutingParamValue::resizeEvent(QResizeEvent *e)
{
	initialLayout();
}

const CPluginMapping PluginRoutingParamValue::getValue() const
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
							(CPluginMapping::RInputDesc::WhenDataRunsOut)(srcNode->u.source.wdro->currentIndex()),
							srcEntity->howMuch ? (CPluginMapping::RInputDesc::HowMuch)(srcEntity->howMuch->currentIndex()) : (CPluginMapping::RInputDesc::HowMuch)0,
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
					(CPluginMapping::RInputDesc::WhenDataRunsOut)(srcNode->u.source.wdro->currentIndex()),
					srcEntity->howMuch ? (CPluginMapping::RInputDesc::HowMuch)(srcEntity->howMuch->currentIndex()) : (CPluginMapping::RInputDesc::HowMuch)0,
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

const string PluginRoutingParamValue::getName() const
{
	return name;
}

void PluginRoutingParamValue::setSound(CSound *_actionSound)
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
			if(loaded->getFilename()!=N_sources[t].data)
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

void PluginRoutingParamValue::prvSetSound(CSound *_actionSound)
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

#if 0
	/* ZZZ
	 * This is a stupid way to do it, but I can't figure out how else to do it:
	 * but the second time the widget is shown on a dialog the children of the 
	 * scroll window aren't sized or positioned right, but this makes it work
	 * I've tried calling recalc() and layout() in several combinations but i 
	 * couldn't get it to work... this worked...
	 */
	resize(getWidth()+1,getHeight()+1);
	resize(getWidth()-1,getHeight()-1);
#endif
	initialLayout();
}

void PluginRoutingParamValue::createOutput()
{
		// ??? need mouse click events
	QFrame *output=new QFrame(canvas);
	output->setAutoFillBackground(true);
	output->setFrameShape(QFrame::Panel);
	output->setFrameShadow(QFrame::Raised);
	output->installEventFilter(this);
	output->setProperty("output entity",1);
	output->setLayout(new QVBoxLayout);
	output->layout()->setContentsMargins(3,3,3,3);
	output->layout()->setSpacing(3);

	QLabel *label=new QLabel(_("Output"));
	label->setStyleSheet("border:1px solid black; background-color: "COL_FILENAME_STRING);
	output->layout()->addWidget(label);

	N_output.paramValue=this;
	N_output.window=output;
	for(unsigned t=0;t<actionSound->getChannelCount();t++)
		createOutputChannel();

	output->show();
}

// creates a new output channel and appends it
void PluginRoutingParamValue::createOutputChannel()
{
	QLabel *win=new QLabel(istring(N_output.nodes.size()+1).c_str()/*,FOXIcons->plugin_wave*/); // ??? need icon and text to both display
	N_output.window->layout()->addWidget(win);
		// ??? need to cause mouse clicks to go to window underneath
	//win->setTarget(this);
	//win->setSelector(ID_OUTPUT);

	N_output.nodes.push_back(REntity::RNode(&N_output,win,REntity::RNode::tInput));
}

// removes an output channel from the end.. this also handles removing any connections involving that channel
void PluginRoutingParamValue::removeOutputChannel()
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


void PluginRoutingParamValue::newInstance()
{
	if(N_instances.size()>=MAX_INSTANCES) // necessary hard limit
		return;

	// ??? need right click events
	QFrame *instance=new QFrame(canvas);
	instance->setAutoFillBackground(true);
	instance->setFrameShape(QFrame::Panel);
	instance->setFrameShadow(QFrame::Raised);
	instance->installEventFilter(this);
	instance->setProperty("instance entity",1);
	instance->setLayout(new QVBoxLayout);
	instance->layout()->setContentsMargins(3,3,3,3);
	instance->layout()->setSpacing(3);

	N_instances.push_back(REntity(this,instance));
	REntity &N_instance=N_instances[N_instances.size()-1];//*(N_instances.rbegin());
	{
		/*??? eventually wouldn't want LADSPA specific code here.. LADSPA would be wrapped in my native plugin API */
			// ??? need mouse clicks to go to window underneath
		QLabel *label=new QLabel(pluginDesc->Name/*,NULL,FRAME_LINE|LABEL_NORMAL | LAYOUT_FILL_X*/);
		//label->setTarget(this);
		//label->setSelector(ID_INSTANCE_TITLE);
		label->setStyleSheet("border:1px solid black; background-color: "COL_FILENAME_STRING);
		instance->layout()->addWidget(label);

		// input audio ports
		int inputPortCount=0;
		for(unsigned t=0;t<pluginDesc->PortCount;t++)
		{
			const LADSPA_PortDescriptor pd=pluginDesc->PortDescriptors[t];
			if(LADSPA_IS_PORT_INPUT(pd) && LADSPA_IS_PORT_AUDIO(pd))
			{
				QLabel *win=new QLabel(pluginDesc->PortNames[t]/*,NULL,JUSTIFY_LEFT|ICON_AFTER_TEXT | LAYOUT_FILL_X*/);
				win->setAlignment(Qt::AlignLeft);
					// ??? pass mouse events underneath
				//win->setTarget(this);
				//win->setSelector(ID_INSTANCE_PORT);
				instance->layout()->addWidget(win);

				N_instance.nodes.push_back(REntity::RNode(&N_instance,win,REntity::RNode::tInput));
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
				QLabel *win=new QLabel(pluginDesc->PortNames[t]/*,NULL,JUSTIFY_RIGHT|ICON_BEFORE_TEXT | LAYOUT_FILL_X*/);
				win->setAlignment(Qt::AlignRight);
					// ??? pass mouse events underneath
				//win->setTarget(this);
				//win->setSelector(ID_INSTANCE_PORT);
				instance->layout()->addWidget(win);

				N_instance.nodes.push_back(REntity::RNode(&N_instance,win,REntity::RNode::tOutput));
				N_instance.nodes[N_instance.nodes.size()-1].u.instance.pluginPortIndex=outputPortCount++;
			}
		}
	}
	instance->show();
}

void PluginRoutingParamValue::newSource(CLoadedSound *loaded,size_t soundFileManagerIndex)
{
	if(N_sources.size()>=MAX_INSTANCES) // necessary hard limit
		return;

	const CSound &sound=*(loaded->sound);
	const string basename=CPath(loaded->getFilename()).baseName();

	// ??? need right click events
	QFrame *source=new QFrame(canvas);
	source->setAutoFillBackground(true);
	source->setFrameShape(QFrame::Panel);
	source->setFrameShadow(QFrame::Raised);
	source->installEventFilter(this);
	source->setProperty("source entity",1);
	source->setLayout(new QVBoxLayout);
	source->layout()->setContentsMargins(3,3,3,3);
	source->layout()->setSpacing(3);

	N_sources.push_back(REntity(this,source));
	REntity &N_source=*(N_sources.rbegin());
	{
		QLabel *label=new QLabel(basename.c_str()/*,NULL,FRAME_LINE|LABEL_NORMAL | LAYOUT_FILL_X*/);
			// ??? passthru mouse events underneath
		//label->setTarget(this);
		//label->setSelector(ID_SOURCE_TITLE);
		//label->setBackColor(COL_FILENAME);
		N_source.data=loaded->getFilename();
		label->setStyleSheet("border:1px solid black; background-color: "COL_FILENAME_STRING);
		source->layout()->addWidget(label);

		if(loaded->sound!=actionSound)
		{
			QWidget *w=new QWidget;
			w->setLayout(new QHBoxLayout);
			w->layout()->setContentsMargins(0,0,0,0);
			w->layout()->setSpacing(3);

			source->layout()->addWidget(w);
	
			QLabel *label=new QLabel(_("Use"));
			label->setFont(textFont);
			w->layout()->addWidget(label);

			QComboBox *c=new QComboBox;
				// these have to maintain the order in CPluginMapping::RInputDesc::WhenDataRunsOut because the index ends up being the value of that enum
			//c->setMaxVisibleItems(2);
			c->addItem(_("Selection"));
			c->addItem(_("All"));
			c->setFont(textFont);
			w->layout()->addWidget(c);


			N_source.howMuch=c;
		}

		for(unsigned i=0;i<sound.getChannelCount();i++)
		{
			QWidget *nodeParent=new QWidget;
			nodeParent->setLayout(new QHBoxLayout);
			nodeParent->layout()->setContentsMargins(0,0,0,0);
			nodeParent->layout()->setSpacing(3);
			
			source->layout()->addWidget(nodeParent);

			QComboBox *wdroControl=new QComboBox;
			wdroControl->setMaxVisibleItems(2);
			wdroControl->setFont(textFont);
			wdroControl->setToolTip(_("When Data Runs Out ..."));
				// these have to maintain the order in CPluginMapping::RInputDesc::WhenDataRunsOut because the index ends up being the value of that enum
			wdroControl->addItem(_("Silence"));
			wdroControl->addItem(_("Loop"));
			nodeParent->layout()->addWidget(wdroControl);


			QLabel *label=new QLabel(istring(i+1).c_str()/*,FOXIcons->plugin_wave,JUSTIFY_RIGHT|ICON_BEFORE_TEXT | LAYOUT_FILL_X*/); // ??? need text and icon
				// ??? pass events underneath
			//label->setTarget(this);
			//label->setSelector(ID_SOURCE_CHANNEL);
			nodeParent->layout()->addWidget(label);


			N_source.nodes.push_back(REntity::RNode(&N_source,nodeParent,REntity::RNode::tOutput));
			N_source.nodes[N_source.nodes.size()-1].u.source.soundFileManagerIndex=soundFileManagerIndex;
			N_source.nodes[N_source.nodes.size()-1].u.source.wdro=wdroControl;
		}
	}

	// disable the item
	soundList->item(soundFileManagerIndex)->setFlags(soundList->item(soundFileManagerIndex)->flags() & ~Qt::ItemIsEnabled);
	source->show();
}

/* 
 * given the actual size of the canvas and the minumal size needed for all items and the number items
 * this returns the amount of space between each item
 */
static int calc_spacing(int actual,int minimal,size_t num_items)
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
static int calc_initial_offset(int actual,int minimal,size_t num_items)
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
static int calc_extra_spacing(int spacing,size_t num_items)
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

void PluginRoutingParamValue::initialLayout()
{
	#define PADDING 20
	#define H_SPACING 60	// minimal space between columns
	#define V_SPACING 15	// minimal space between objects in columns vertically

	// layout is:  sources in a column, instances in a column, output(s) in a column
	// spread out all the items in each column across the height and width of the 
	// area, and let there be a minimum that the area will shink, otherwise defined
	// by the current size of the canvas

	// calculate width/height of sources column
	int sourcesColumnWidth=0;
	int minSourcesColumnHeight=0;
	{
		for(size_t t=0;t<N_sources.size();t++)
			sourcesColumnWidth=max(sourcesColumnWidth,N_sources[t].window->sizeHint().width());

		for(size_t t=0;t<N_sources.size();t++)
			minSourcesColumnHeight+=N_sources[t].window->sizeHint().height();
/*
		if(N_sources.size()>1)
			minSourcesColumnHeight+= N_sources.size()-1*V_SPACING;
*/
	}

	// calculate width/height of instances column
	int instancesColumnWidth=0;
	int minInstancesColumnHeight=0;
	{
		for(size_t t=0;t<N_instances.size();t++)
			instancesColumnWidth=max(instancesColumnWidth,N_instances[t].window->sizeHint().width());

		for(size_t t=0;t<N_instances.size();t++)
			minInstancesColumnHeight+=N_instances[t].window->sizeHint().height();
/*
		if(N_instances.size()>1)
			minInstancesColumnHeight+= N_instances.size()-1*V_SPACING;
*/
	}

	// calculate width/height of output(s) column
	int outputsColumnWidth=0;
	int minOutputsColumnHeight=0;
	if(N_output.window)
	{
		outputsColumnWidth=max(outputsColumnWidth,N_output.window->sizeHint().width());

		minOutputsColumnHeight+=N_output.window->sizeHint().height();
	}

	// ----------------------------------------

	const int total_H_SPACING=calc_extra_spacing(H_SPACING,3);
	const int total_V_SPACING=calc_extra_spacing(V_SPACING,max(N_sources.size(),max(N_instances.size(),(size_t)1)));

	// calc the size that the canvas would like to be with no scroll bars
	const int nowCanvasWidth=scrollArea->maximumViewportSize().width()-(2*PADDING); //scrollWindow->getWidth()-(canvasParent->getPadLeft()+canvasParent->getPadRight());
	const int nowCanvasHeight=scrollArea->maximumViewportSize().height()-(2*PADDING); //scrollWindow->getHeight()-(canvasParent->getPadTop()+canvasParent->getPadBottom());

	// calc the minimum we will allow the canvas to get
	const int minCanvasWidth=sourcesColumnWidth+instancesColumnWidth+outputsColumnWidth;
	const int minCanvasHeight=max(minSourcesColumnHeight,max(minInstancesColumnHeight,minOutputsColumnHeight));

	// now determine what the canvas size will be (adding spacing so that there will always be at least that much between items)
	const int canvasWidth=max(nowCanvasWidth,minCanvasWidth + total_H_SPACING);
	const int canvasHeight=max(nowCanvasHeight,minCanvasHeight + total_V_SPACING);
	//canvas->setFixedSize(canvasWidth,canvasHeight);

	// ----------------------------------------

	const int h_spacing=calc_spacing(canvasWidth,minCanvasWidth,3);
	int x=calc_initial_offset(canvasWidth,minCanvasWidth,3)+PADDING;

	// resize and position sources
	{
		int v_spacing=calc_spacing(canvasHeight,minSourcesColumnHeight,N_sources.size());
		int y=calc_initial_offset(canvasHeight,minSourcesColumnHeight,N_sources.size())+PADDING;
		for(size_t t=0;t<N_sources.size();t++)
		{
			QWidget *win=N_sources[t].window;

			win->setGeometry(x,y,sourcesColumnWidth,win->sizeHint().height());
			y+=win->sizeHint().height();
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

		int v_spacing=calc_spacing(canvasHeight,minInstancesColumnHeight,N_instances.size());
		int y=calc_initial_offset(canvasHeight,minInstancesColumnHeight,N_instances.size())+PADDING;
		for(size_t t=0;t<N_instances.size();t++)
		{
			QWidget *win=N_instances[t].window;

			win->setGeometry(x,y,instancesColumnWidth,win->sizeHint().height());
			y+=win->sizeHint().height();
			y+=v_spacing;
		}
	}
	x+=instancesColumnWidth+h_spacing;

	// resize and position output(s)
	{
		int v_spacing=calc_spacing(canvasHeight,minOutputsColumnHeight,1);
		int y=calc_initial_offset(canvasHeight,minOutputsColumnHeight,1)+PADDING;
		// loop for all outputs (just 1 right now)
		if(N_output.window)
		{
			QWidget *win=N_output.window;

			win->setGeometry(x,y,outputsColumnWidth,win->sizeHint().height());
			y+=win->sizeHint().height();
			y+=v_spacing;
		}
	}

	canvas->setFixedSize(scrollArea->maximumViewportSize());
	canvas->setMinimumSize(minCanvasWidth,minCanvasHeight);
	canvas->update();
}

bool PluginRoutingParamValue::findNode(const RNodeDesc &desc,REntity *&entity,REntity::RNode *&node) const
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

PluginRoutingParamValue::REntity *PluginRoutingParamValue::findEntity(const RNodeDesc &desc) const
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

static bool within_distance(int x1,int y1,int x2,int y2)
{
	return( ((x2-x1)*(x2-x1))+((y2-y1)*(y2-y1)) <= (NODE_RADIUS*NODE_RADIUS) );
}

/*
 * finds the node based on the screen location on the canvas
 */
bool PluginRoutingParamValue::whereNode(int x,int y,RNodeDesc &desc)
{
	// check sources
	for(size_t t=0;t<N_sources.size();t++)
	{
		for(size_t i=0;i<N_sources[t].nodes.size();i++)
		{
			int nx,ny;
			N_sources[t].nodes[i].getPosition(canvas,nx,ny);
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
			int nx,ny;
			N_instances[t].nodes[i].getPosition(canvas,nx,ny);
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
		int nx,ny;
		N_output.nodes[i].getPosition(canvas,nx,ny);
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

void PluginRoutingParamValue::on_newInstanceButton_clicked()
{
	newInstance();
	initialLayout();
	//scrollArea->recalc();
}

void PluginRoutingParamValue::on_useSoundButton_clicked()
{
	QListWidgetItem *item=soundList->currentItem();
	if(item && item->flags()&Qt::ItemIsEnabled)
	{
		int index=soundList->currentRow();
		newSource(gSoundFileManager->getSound(index),index);
		initialLayout();
		//scrollArea->recalc();
	}
}

void PluginRoutingParamValue::removeEntity(vector<REntity> &familyMembers,RNodeDesc::Families family,QWidget *entity)
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
			//scrollArea->recalc();
			break;
		}
	}
}

bool PluginRoutingParamValue::eventFilter(QObject *watched,QEvent *e)
{
	if(e->type()==QEvent::MouseButtonRelease)
	{
		QMouseEvent *me=(QMouseEvent *)e;
		if(me->button()==Qt::RightButton)
		{
			QVariant v;

			v=watched->property("source entity");
			if(v.isValid())
			{
				if(Question(_("Are you sure you want to remove this source?"),yesnoQues)!=yesAns)
					return true;

				// re-enable this item in the sound list
				for(size_t t=0;t<N_sources.size();t++)
				{
					if(N_sources[t].window==watched)
					{
						QListWidgetItem *item=soundList->item(N_sources[t].nodes[0].u.source.soundFileManagerIndex);
						item->setFlags(item->flags() | Qt::ItemIsEnabled);
						break;
					}
				}

				removeEntity(N_sources,RNodeDesc::fSources,(QWidget *)watched);
				return true;
			}

			v=watched->property("instance entity");
			if(v.isValid())
			{	
				// disallow being able to remove an only instance
				if(N_instances.size()<=1)
						return true;

				if(Question(_("Are you sure you want to remove this instance?"),yesnoQues)!=yesAns)
					return true;

				removeEntity(N_instances,RNodeDesc::fInstances,(QWidget *)watched);
				return true;
			}

			v=watched->property("output entity");
			if(v.isValid())
			{ // output popup menu
				QMenu menu;
				QAction *a;

				a=menu.addAction(_("Append New Channel"));
				a->setProperty("op","append");

				a=menu.addAction(_("Remove Last Channel"));
				a->setProperty("op","remove");

				a=menu.exec(QCursor::pos());
				if(a)
				{
					const QString op=a->property("op").toString();
					if(op=="append")
					{
						if(N_output.nodes.size()<MAX_CHANNELS-1)
						{
							createOutputChannel();
							initialLayout(); // :-/ didn't really want to do this, but I guess I have to since things are LAYOUT_EXPLICIT
						}
						else
							Error(_("Cannot exceed maximum number of channels: ")+istring(MAX_CHANNELS));
					}
					else if(op=="remove")
					{
						if(N_output.nodes.size()>1)
						{
							removeOutputChannel();
							initialLayout(); // :-/ didn't really want to do this, but I guess I have to since things are LAYOUT_EXPLICIT
						}
						else
							Error(_("Cannot remove the last output channel"));
					}
				}
			}
		}
	}
	return false;
}

void PluginRoutingParamValue::on_clearButton_clicked()
{
	if(connections.size()!=0 && Question(_("Are you sure you want to clear the routing?"),yesnoQues)!=yesAns)
		return;
	RESET_WIRE_COLOR
	prvSetSound(actionSound);
	initialLayout();
	canvas->update();
}

void PluginRoutingParamValue::on_defaultButton_clicked()
{
	if(connections.size()!=0 && Question(_("Are you sure you want to discard the current routing and guess at the desired routing?"),yesnoQues)!=yesAns)
		return;

	try
	{
		RESET_WIRE_COLOR
		guessAtRouting();
	}
	catch(exception &e)
	{
		Warning(e.what());
	}
}

void PluginRoutingParamValue::guessAtRouting()
{
#ifdef ENABLE_LADSPA
	const CPluginMapping mapping=getLADSPADefaultMapping(pluginDesc,actionSound);
#else
	const CPluginMapping mapping;
#endif

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
/*
	// see note: ZZZ
	resize(getWidth()+1,getHeight()+1);
	resize(getWidth()-1,getHeight()-1);
*/

	initialLayout();
}

void PluginRoutingParamValue::clearAll()
{
	draggingWire=false;
	mouseIsOverNode=false;

	connections.clear();

	N_sources.clear();
	N_instances.clear();
	N_output.window=NULL;
	N_output.nodes.clear();

	qDeleteAll(canvas->children());
}

void PluginRoutingParamValue::resetSoundList()
{
	soundList->clear();

	for(size_t t=0;t<gSoundFileManager->getOpenedCount();t++)
	{
		soundList->addItem(gSoundFileManager->getSound(t)->getFilename().c_str());

		// select the action sound.. we will put it up there already
		if(gSoundFileManager->getSound(t)->sound==actionSound)
		{
			soundList->setCurrentRow(soundList->count()-1);
			//soundList->disableItem(soundList->getNumItems()-1); done in newSource
		}
	}
}

void PluginRoutingParamValue::readFromFile(const string &prefix,CNestedDataFile *f)
{
	RESET_WIRE_COLOR
	const string key=prefix DOT getName();
	clearAll();
	resetSoundList();
	canvas->update();
	//getApp()->forceRefresh(); // redraw NOW

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
							soundList->addItem(filename.c_str());
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
							soundList->addItem(filename.c_str());
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
				N_sources[N_sources.size()-1].howMuch->setCurrentIndex(f->getValue<int>(source_key DOT "howMuch"));

			// set wdro (when data runs out)
			for(size_t t=0;t<nodeCount;t++)
			{
				if(t>=N_sources[N_sources.size()-1].nodes.size())
					break;
				const int wdro=f->getValue<int>(source_key DOT "wdro"+istring(t));
				if(wdro<N_sources[N_sources.size()-1].nodes[t].u.source.wdro->count())
					N_sources[N_sources.size()-1].nodes[t].u.source.wdro->setCurrentIndex(wdro);
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
		//N_output.nodes[N_output.nodes.size()-1].window->create();
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


#if 0
	// see note ZZZ
	resize(getWidth()+1,getHeight()+1);
	resize(getWidth()-1,getHeight()-1);
	canvas->update();
#endif
	initialLayout();
}

void PluginRoutingParamValue::writeToFile(const string &prefix,CNestedDataFile *f) const
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
	f->setValue<size_t>(key DOT "sourceCount",N_sources.size());
	for(size_t t=0;t<N_sources.size();t++)
	{
		const string source_key=key DOT "source"+istring(t);

		f->setValue<size_t>(source_key DOT "nodeCount",N_sources[t].nodes.size());

		if(gSoundFileManager->getSound(N_sources[t].nodes[0].u.source.soundFileManagerIndex)->sound==actionSound)
			f->setValue<bool>(source_key DOT "actionSound",true);
		f->setValue<string>(source_key DOT "filename",gSoundFileManager->getSound(N_sources[t].nodes[0].u.source.soundFileManagerIndex)->getFilename());
		if(N_sources[t].howMuch)
			f->setValue<int>(source_key DOT "howMuch",N_sources[t].howMuch->currentIndex());

		for(size_t k=0;k<N_sources[t].nodes.size();k++)
		{ // write all attributes of each node
			f->setValue<int>(source_key DOT "wdro"+istring(k),N_sources[t].nodes[k].u.source.wdro->currentIndex());
		}
	}

	// write instances
	f->setValue<size_t>(key DOT "instanceCount",N_instances.size());

	// write output
	f->setValue<size_t>(key DOT "outputChannelCount",N_output.nodes.size());

	// write information about connections between nodes
	const string connections_key=key DOT "connections";
	f->setValue<size_t>(connections_key DOT "connectionCount",connections.size());
	for(size_t t=0;t<connections.size();t++)
	{
		const string key=connections_key DOT "c"+istring(t);
		f->setValue<float>(key DOT "gain",connections[t].gain);

		f->setValue<int>(key DOT "srcFamily",connections[t].src.family);
		f->setValue<size_t>(key DOT "srcEntity",connections[t].src.entityIndex);
		f->setValue<size_t>(key DOT "srcNode",connections[t].src.nodeIndex);

		f->setValue<int>(key DOT "destFamily",connections[t].dest.family);
		f->setValue<size_t>(key DOT "destEntity",connections[t].dest.entityIndex);
		f->setValue<size_t>(key DOT "destNode",connections[t].dest.nodeIndex);
	}
}


