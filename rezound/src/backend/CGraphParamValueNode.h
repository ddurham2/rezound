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
	double x;
	double y; // ??? could be float -- need to change to typedef probably

	CGraphParamValueNode();
	CGraphParamValueNode(double _x,double _y);
	CGraphParamValueNode(const CGraphParamValueNode &src);

	CGraphParamValueNode &operator=(const CGraphParamValueNode &rhs);
};

//typedef TBasicList<CGraphParamValueNode> CGraphParamValueNodeList;
typedef vector<CGraphParamValueNode> CGraphParamValueNodeList;



// - Simply creates a list of two nodes, (x: 0.0, y: v) and (x: 1.0, y: v)
//
// - Useful if a simple, single value input dialog needs to create an input parameter for 
//   an action which accepts a CGraphParamValueNodeList as that input paramter.  Hence the
//   advanced dialog for that action returns the curve that the user plotted
//
extern const CGraphParamValueNodeList singleValueToGraph(const double v);

extern void interpretGraphNodes(const CGraphParamValueNodeList &nodes,const unsigned i,const sample_pos_t totalLength,sample_pos_t &segmentStartPosition,double &segmentStartValue,sample_pos_t &segmentStopPosition,double &segmentStopValue,sample_pos_t &segmentLength);


class CGraphParamValueIterator
{
public:
	CGraphParamValueIterator(const CGraphParamValueNodeList &nodes,const sample_pos_t iterationLength);
	virtual ~CGraphParamValueIterator();

	const double next();

private:
	const CGraphParamValueNodeList nodes;
	const sample_pos_t iterationLength;
	unsigned nodeIndex;
	double t,segmentLength,segmentLengthSub1,segmentStartValue,segmentStopValueStartValueDiff;
};

#endif
