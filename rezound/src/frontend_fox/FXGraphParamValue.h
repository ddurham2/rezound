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

#ifndef __FXGraphParamValue_H__
#define __FXGraphParamValue_H__

#include "../../config/common.h"


#include <fox/fx.h>
#include "FXPackerCanvas.h"

#include "../backend/ASound.h"
#include "../backend/CGraphParamValueNode.h"

class FXGraphParamNode;

class FXGraphParamValue : public FXPacker
{
	FXDECLARE(FXGraphParamValue);
public:
	typedef const double (*f_at_xs)(const double x,const int s);

	// minScalar and maxScalar define the min and max spinner values, if they're the same, no spinner is shown
	FXGraphParamValue(f_at_xs interpretValue,f_at_xs uninterpretValue,const int minScalar,const int maxScalar,const int initScalar,FXComposite *p,int opts,int x=0,int y=0,int w=0,int h=0);

	void setSound(ASound *sound,sample_pos_t start,sample_pos_t stop);
	void clearNodes();

	virtual void layout();

	long onGraphPanelPaint(FXObject *sender,FXSelector sel,void *ptr);
	long onCreateNode(FXObject *sender,FXSelector sel,void *ptr);

	long onDragNodeAfterCreate(FXObject *sender,FXSelector sel,void *ptr);
	long onStopDraggingNodeAfterCreate(FXObject *sender,FXSelector sel,void *ptr);

	long onScalarSpinnerChange(FXObject *sender,FXSelector sel,void *ptr);

	long onPatternButton(FXObject *sender,FXSelector sel,void *ptr);

	void updateNumbers();

	void setUnits(FXString units);

	const CGraphParamValueNodeList &getNodes() const;

	enum
	{
		ID_GRAPH_PANEL=FXPacker::ID_LAST,

		ID_SCALAR_SPINNER,
		ID_CLEAR_BUTTON,

		ID_LAST
	};


protected:
	FXGraphParamValue() {}

private:
	friend class FXGraphParamNode;
	friend class FXValueRuler;

	ASound *sound;
	sample_pos_t start,stop;
	bool firstTime;

	FXString units;

	FXComposite *buttonPanel;
		FXLabel *scalarLabel;
		FXSpinner *scalarSpinner;
	FXComposite *vRuler;
	FXComposite *statusPanel;
		FXLabel *positionLabel;
		FXLabel *valueLabel;
		FXLabel *unitsLabel;
	FXComposite *graphPanelParent;
	FXComposite *graphPanel;

	FXGraphParamNode *removeNode; // set when onDestroy is invoked for a node
	FXGraphParamNode *draggingNodeAfterCreate;

	CGraphParamValueNodeList nodes;
	mutable CGraphParamValueNodeList retNodes; // a copy of nodes that is returned

	int getGraphPanelHeight() const; // always returns an even value

	int findNode(FXGraphParamNode *node) const;
	void moveAndRecalcNode(FXGraphParamNode *node,int x,int y);
	void insertIntoNodes(CGraphParamValueNode &n);

	void drawPortion(int left,int width,FXDCWindow *dc);

	double screenToNodePosition(int x);
	double screenToNodeValue(int y);

	f_at_xs interpretValue;
	f_at_xs uninterpretValue;

	// we draw on this one time and blit from it anytime we need to update the canvas
	FXImage *backBuffer;

};

#endif
