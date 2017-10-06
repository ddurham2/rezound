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

#ifndef __FXPluginRoutingParamValue_H__
#define __FXPluginRoutingParamValue_H__

#include "../../config/common.h"
#include "fox_compat.h"

#include <string>
#include <vector>
#include <utility>

class CNestedDataFile;
class CLoadedSound;
class CSound;

class FXMyScrollWindow;

#include "../backend/CPluginMapping.h"


/* ??? don't want 'LADSPA' specific references in here at some point */
typedef struct _LADSPA_Descriptor LADSPA_Descriptor;

class FXPluginRoutingParamValue : public FXPacker
{
	FXDECLARE(FXPluginRoutingParamValue);
public:
	FXPluginRoutingParamValue(FXComposite *p,int opts,const char *name,const LADSPA_Descriptor *desc);
	virtual ~FXPluginRoutingParamValue();

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

	const string getName() const;

	void setSound(CSound *sound);

	void enable();
	void disable();

	const CPluginMapping getValue() const;
	//void setValue(const CPluginMapping &value) const;

	void readFromFile(const string &prefix,CNestedDataFile *f);
	void writeToFile(const string &prefix,CNestedDataFile *f) const;

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


protected:
	FXPluginRoutingParamValue() {}

private:
	const string name;
	const LADSPA_Descriptor *pluginDesc;
	CSound *actionSound;

	bool firstTime; /* this is true at the beginning, but after the first time the widget has been show, then ->create and ->show need to be called on the stuff in the canvas */

	FXPacker *contents;
		FXMyScrollWindow *scrollWindow;
			FXPacker *canvasParent;
				FXComposite *canvas;

	FXList *soundList;

	struct REntity
	{
		struct RNode
		{
			FXWindow *window;

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
					FXComboBox *wdro;
				} source;

				// used when this is a node of a plugin instance
				struct RInstance
				{
					int pluginPortIndex;
				} instance;
			} u;

			// ------------------------------

			RNode(FXWindow *_window,Types _type) :
				window(_window),
				type(_type)
			{
			}

			RNode(const RNode &src) :
				window(src.window),
				type(src.type),
				u(src.u)
			{
			}

			RNode &operator=(const RNode &rhs)
			{
				window=rhs.window;
				type=rhs.type;
				u=rhs.u;
				return *this;
			}
		};

		FXComposite *window;
		FXComboBox *howMuch;
		vector<RNode> nodes;

		// ------------------------------

		REntity() :
			window(NULL),
			howMuch(NULL)
		{
		}

		REntity(FXComposite *_window) :
			window(_window),
			howMuch(NULL)
		{
		}

		REntity(const REntity &src) :
			window(src.window),
			howMuch(src.howMuch),
			nodes(src.nodes)
		{
		}

		REntity &operator=(const REntity &rhs)
		{
			window=rhs.window;
			howMuch=rhs.howMuch;
			nodes=rhs.nodes;
			return *this;
		}
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
		size_t nodeIndex; /* index into the nodes once the REntiiy is located */

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
		FXColor wireColor;

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
	void getNodePosition(const REntity::RNode *node,FXint &x,FXint &y);
	bool findNode(const RNodeDesc &desc,REntity *&entity,REntity::RNode *&node) const;
	REntity *findEntity(const RNodeDesc &desc) const;
	bool whereNode(FXint x,FXint y,RNodeDesc &desc);

	void drawNodeConnector(FXDCWindow &dc,const REntity::RNode &node);
	void drawSolidNode(FXDCWindow &dc,const RNodeDesc &desc);

	void newInstance();
	void newSource(CLoadedSound *loaded,size_t soundFileManagerIndex);

	void removeEntity(vector<REntity> &familyMembers,RNodeDesc::Families family,FXWindow *entity);

	vector<RConnection> connections;

	bool mouseIsOverNode;
	RNodeDesc mouseOverNode;

	bool draggingWire;
	RNodeDesc draggingFrom;

	FXFont *textFont;

	// hold temporary values when a connection wire has been right clicked
	size_t clickedConnection;
	FXLabel *connectionGainLabel;


	void prvSetSound(CSound *sound);
	void initialLayout();
	void createOutput();
		void createOutputChannel();
		void removeOutputChannel();
	void clearAll();
	void resetSoundList();
	void guessAtRouting();

	// saved in initialLayout for drawing wires purposes
	FXint saved_minInstancesColumnHeight; 
	FXint saved_instancesColumnWidth;
	FXint saved_instanceColumnX;
	FXint findNearestHoleWithinInstancesColumn(FXint instanceX,FXint fromx,FXint fromy);
};

#endif
