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

#ifndef __CPluginRoutingParamValue_H__
#define __CPluginRoutingParamValue_H__

#include "../../config/common.h"
#include "qt_compat.h"

#include <QComboBox>

#include "ui_CPluginRoutingParamValue.h"

#include <string>
#include <vector>
#include <utility>

class QLabel;
class QScrollArea;
class QSlider;

class CNestedDataFile;
class CLoadedSound;
class CSound;

#include "../backend/CPluginMapping.h"


/* ??? don't want 'LADSPA' specific references in here at some point */
typedef struct _LADSPA_Descriptor LADSPA_Descriptor;

class CPluginRoutingParamValue : public QWidget, public Ui::CPluginRoutingParamValue
{
	Q_OBJECT
public:
	CPluginRoutingParamValue(const char *name,const LADSPA_Descriptor *desc);
	virtual ~CPluginRoutingParamValue();

#if 0
	long onPaint(FXObject *sender,FXSelector sel,void *ptr);
	long onMouseMove(FXObject *sender,FXSelector sel,void *ptr);
	long onMouseDown(FXObject *sender,FXSelector sel,void *ptr);
	long onMouseUp(FXObject *sender,FXSelector sel,void *ptr);

	long onConfigure(FXObject *sender,FXSelector sel,void *ptr);

	long onNewInstanceButton(FXObject *sender,FXSelector sel,void *ptr);
	long onAddSourceButton(FXObject *sender,FXSelector sel,void *ptr);

	long onRemoveInstance(FXObject *sender,FXSelector sel,void *ptr);
	long onRemoveSource(FXObject *sender,FXSelector sel,void *ptr);

	long onRemoveConnection(FXObject *sender,FXSelector sel,void *ptr);
	long onSetConnectionGainSlider(FXObject *sender,FXSelector sel,void *ptr);

	long onOutputPopup(FXObject *sender,FXSelector sel,void *ptr);
	long onAppendNewOutputChannel(FXObject *sender,FXSelector sel,void *ptr);
	long onRemoveLastOutputChannel(FXObject *sender,FXSelector sel,void *ptr);

	long onResetButton(FXObject *sender,FXSelector sel,void *ptr);
	long onDefaultButton(FXObject *sender,FXSelector sel,void *ptr);
#endif

	const string getName() const;

	void setSound(CSound *sound);

	void enable() { setEnabled(true); }
	void disable() { setEnabled(false); }

	const CPluginMapping getValue() const;
	//void setValue(const CPluginMapping &value) const;

	void readFromFile(const string &prefix,CNestedDataFile *f);
	void writeToFile(const string &prefix,CNestedDataFile *f) const;

#if 0
	enum
	{
		ID_CANVAS=FXPacker::ID_LAST,
		ID_NEW_INSTANCE_BUTTON,
		ID_ADD_SOURCE_BUTTON,

		ID_INSTANCE,
		ID_INSTANCE_TITLE,
		ID_INSTANCE_PORT,

		ID_SOURCE,
		ID_SOURCE_TITLE,
		ID_SOURCE_CHANNEL,

		ID_REMOVE_CONNECTION,
		ID_SET_CONNECTION_GAIN,
		ID_SET_CONNECTION_GAIN_SLIDER,

		ID_OUTPUT,
			ID_APPEND_NEW_OUTPUT_CHANNEL,
			ID_REMOVE_LAST_OUTPUT_CHANNEL,

		ID_RESET_BUTTON,
		ID_DEFAULT_BUTTON,

		ID_LAST
	};
#endif

	void drawCanvas(QPaintEvent *e);

protected:
	//CPluginRoutingParamValue() {}
	
	//void paintEvent(QPaintEvent *e);
	void mouseMoveEvent(QMouseEvent *e);
	void mousePressEvent(QMouseEvent *e);
	void mouseReleaseEvent(QMouseEvent *e);

	void resizeEvent(QResizeEvent *e);

	bool eventFilter(QObject *watched,QEvent *e);

private Q_SLOTS:

	void on_newInstanceButton_clicked();
	void on_useSoundButton_clicked();

	void onConnectionGainChanged(int value);
	void onResetConnectionGain();

	//long onRemoveInstance(FXObject *sender,FXSelector sel,void *ptr);
	//long onRemoveSource(FXObject *sender,FXSelector sel,void *ptr);

	//long onRemoveConnection(FXObject *sender,FXSelector sel,void *ptr);
	//long onSetConnectionGainSlider(FXObject *sender,FXSelector sel,void *ptr);

	//long onOutputPopup(FXObject *sender,FXSelector sel,void *ptr);
	//long onAppendNewOutputChannel(FXObject *sender,FXSelector sel,void *ptr);
	//long onRemoveLastOutputChannel(FXObject *sender,FXSelector sel,void *ptr);

	void on_clearButton_clicked();
	void on_defaultButton_clicked();
	

private:
	const string name;
	const LADSPA_Descriptor *pluginDesc;
	CSound *actionSound;

	bool firstTime; /* this is true at the beginning, but after the first time the widget has been show, then ->create and ->show need to be called on the stuff in the canvas */

#if 0
	FXPacker *contents;
		FXMyScrollWindow *scrollWindow;
			FXPacker *canvasParent;
				FXComposite *canvas;

	FXList *soundList;
#endif

	QScrollArea *scrollArea;

	struct REntity // ??? I might ought to have subclasses of this for RSourceEntity, RInstanceEntity and ROutputEntity types
	{
		struct RNode
		{
			void draw(QPainter &p);
#if 0
			class CMyWidget : public QWidget
			{
			public:
				CMyWidget(RNode *_node) : node(_node) {}
			protected:
				RNode *node;
				void onPaint(QPaintEvent *e);
			};
			CMyWidget *window;
#endif

			REntity *entity;

			// the main widget for this node, used to know where to draw the node-connector 
			// on the routing window (outside of the REntity's widget)
			QWidget *window;

			void getPosition(QWidget *canvas,int &x,int &y);
			//void draw(QPainter &p);

			enum Types
			{
				tInput, /* meaning data goes into it */
				tOutput /* meaning data comes from it */
			} type;

			union
			{
				// used when this is a node of an audio file
				struct RSource
				{
					size_t soundFileManagerIndex;
					QComboBox *wdro;
				} source;

				// used when this is a node of a plugin instance
				struct RInstance
				{
					int pluginPortIndex;
				} instance;
			} u;

			// ------------------------------

			RNode(REntity *_entity,QWidget *_window,Types _type) :
				entity(_entity),
				window(_window),
				type(_type)
			{
			}

			RNode(const RNode &src) :
				entity(src.entity),
				window(src.window),
				type(src.type),
				u(src.u)
			{
			}

			RNode &operator=(const RNode &rhs)
			{
				entity=rhs.entity;
				window=rhs.window;
				type=rhs.type;
				u=rhs.u;
				return *this;
			}
		};

		string data; // multi-purpose use

		CPluginRoutingParamValue *paramValue;

		// the window that makes up source, plugin-instance or output that's a child of the routing window
		QWidget *window;

		QComboBox *howMuch; // user chooses all or selection for a source entity
		vector<RNode> nodes;

		// ------------------------------

/*
		REntity() :
			paramValue(NULL),
			window(NULL),
			howMuch(NULL)
		{
		}
*/

		REntity(CPluginRoutingParamValue *_paramValue,QWidget *_window) :
			paramValue(_paramValue),
			window(_window),
			howMuch(NULL)
		{
		}

		REntity(const REntity &src) :
			paramValue(src.paramValue),
			window(src.window),
			howMuch(src.howMuch),
			nodes(src.nodes)
		{
		}

		REntity &operator=(const REntity &rhs)
		{
			paramValue=rhs.paramValue;
			window=rhs.window;
			howMuch=rhs.howMuch;
			nodes=rhs.nodes;
			return *this;
		}
	private: 
		REntity() {}
	};

	vector<REntity> N_sources;
	vector<REntity> N_instances;
	REntity N_output;

	struct RNodeDesc
	{
		enum Families
		{	/* this indicates what REntity set (from above) the node is in */
			fSources,
			fInstances,
			fOutputs
		};

	       	/* an REntity::RNode::tOutput node */
		Families family;
		size_t entityIndex; /* index into the vector for the family */
		size_t nodeIndex; /* index into the nodes once the REntity is located */

		RNodeDesc() :
			family(fSources),
			entityIndex(0),
			nodeIndex(0)
		{ 
		}

		RNodeDesc(Families _family,size_t _entityIndex,size_t _nodeIndex) :
			family(_family),
			entityIndex(_entityIndex),
			nodeIndex(_nodeIndex)
		{
		}

		RNodeDesc(const RNodeDesc &src) :
			family(src.family),
			entityIndex(src.entityIndex),
			nodeIndex(src.nodeIndex)
		{
		}

		RNodeDesc &operator=(const RNodeDesc &rhs)
		{
			family=rhs.family;
			entityIndex=rhs.entityIndex;
			nodeIndex=rhs.nodeIndex;
			return *this;
		}

		bool operator==(const RNodeDesc &rhs) const
		{
			return family==rhs.family && entityIndex==rhs.entityIndex && nodeIndex==rhs.nodeIndex;
		}
	};

	struct RConnection
	{
		RNodeDesc src;
		RNodeDesc dest;
		float gain;
		QColor wireColor;

		RConnection() :
			gain(1.0)
		{
		}

		RConnection(const RNodeDesc &_src,const RNodeDesc &_dest,const float _gain) :
			src(_src),
			dest(_dest),
			gain(_gain)
		{
		}

		RConnection(const RConnection &_src) :
			src(_src.src),
			dest(_src.dest),
			gain(_src.gain),
			wireColor(_src.wireColor)
		{
		}

		RConnection &operator=(const RConnection &rhs)
		{
			src=rhs.src;
			dest=rhs.dest;
			gain=rhs.gain;
			wireColor=rhs.wireColor;
			return *this;
		}

		bool operator==(const RConnection &rhs) const
		{
			return src==rhs.src && dest==rhs.dest; /* not comparing gain */
		}

		struct RLine
		{
			int x1,y1;
			int x2,y2;
		};

		RLine line1,line2,line3;
	};

	bool isValidConnection(const RNodeDesc &desc1,const RNodeDesc &desc2);
	//void getNodePosition(const REntity::RNode *node,int &x,int &y);
	bool findNode(const RNodeDesc &desc,REntity *&entity,REntity::RNode *&node) const;
	REntity *findEntity(const RNodeDesc &desc) const;
	bool whereNode(int x,int y,RNodeDesc &desc);

	//void drawNodeConnector(FXDCWindow &dc,const REntity::RNode &node);
	void drawSolidNode(QPainter &p,const RNodeDesc &desc);

	void newInstance();
	void newSource(CLoadedSound *loaded,size_t soundFileManagerIndex);

	void removeEntity(vector<REntity> &familyMembers,RNodeDesc::Families family,QWidget *entity);

	vector<RConnection> connections;

	bool mouseIsOverNode;
	RNodeDesc mouseOverNode;

	bool draggingWire;
	RNodeDesc draggingFrom;

	QFont textFont;

	// hold temporary values when a connection wire has been right clicked
	size_t clickedConnection;
	QLabel *connectionGainLabel;
	QSlider *connectionGainSlider;


	void prvSetSound(CSound *sound);
	void initialLayout();
	void createOutput();
		void createOutputChannel();
		void removeOutputChannel();
	void clearAll();
	void resetSoundList();
	void guessAtRouting();

	// saved in initialLayout for drawing wires purposes
	int saved_minInstancesColumnHeight; 
	int saved_instancesColumnWidth;
	int saved_instanceColumnX;
	int findNearestHoleWithinInstancesColumn(int instanceX,int fromx,int fromy);
};

#endif
