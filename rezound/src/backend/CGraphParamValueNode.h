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

#ifndef __CGraphParamValueNode_H__
#define __CGraphParamValueNode_H__


#include "../../config/common.h"

#include <vector>

#include "CSound_defs.h"

class CGraphParamValueNode
{
public:
	double position;
	double value; // ??? could be float -- need to change to typedef probably

	void *userData; // for use in the front-end code for the graphical representation of this node

	CGraphParamValueNode();
	CGraphParamValueNode(double _position,double _value,void *_userData=NULL);
	CGraphParamValueNode(const CGraphParamValueNode &src);

	CGraphParamValueNode &operator=(const CGraphParamValueNode &rhs);
};

//typedef TBasicList<CGraphParamValueNode> CGraphParamValueNodeList;
typedef vector<CGraphParamValueNode> CGraphParamValueNodeList;



// - Simply creates a list of two nodes, (position: 0.0, value: v) and (position: 1.0, value: v)
//
// - Useful if a simple, single value input dialog needs to create an input parameter for 
//   an action which accepts a CGraphParamValueNodeList as that input paramter.  Hence the
//   advanced dialog for that action returns the curve that the user plotted
//
extern const CGraphParamValueNodeList singleValueToGraph(const double v);

extern void interpretGraphNodes(const CGraphParamValueNodeList &nodes,const unsigned i,const sample_pos_t totalLength,sample_pos_t &segmentStartPosition,double &segmentStartValue,sample_pos_t &segmentStopPosition,double &segmentStopValue,sample_pos_t &segmentLength);

#endif
