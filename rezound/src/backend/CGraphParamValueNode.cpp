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

#include "CGraphParamValueNode.h"

#include <stdexcept>
#include <string>

#include <istring>


CGraphParamValueNode::CGraphParamValueNode()
{
	x=y=0.0;
}

CGraphParamValueNode::CGraphParamValueNode(double _x,double _y)
{
	x=_x;
	y=_y;
}

CGraphParamValueNode::CGraphParamValueNode(const CGraphParamValueNode &src)
{
	operator=(src);
}

CGraphParamValueNode &CGraphParamValueNode::operator=(const CGraphParamValueNode &rhs)
{
	x=rhs.x;
	y=rhs.y;
	return *this;
}



// - Simply creates a list of two nodes, (x: 0.0, y: v) and (x: 1.0, y: v)
//
// - Useful if a simple, single value input dialog needs to create an input parameter for 
//   an action which accepts a CGraphParamValueNodeList as that input paramter.  Hence the
//   advanced dialog for that action returns the curve that the user plotted
//
const CGraphParamValueNodeList singleValueToGraph(const double v)
{
	CGraphParamValueNodeList l;
	//l.append(CGraphParamValueNode(0.0,v));
	l.push_back(CGraphParamValueNode(0.0,v));
	//l.append(CGraphParamValueNode(1.0,v));
	l.push_back(CGraphParamValueNode(1.0,v));
	return l;
}

#include <math.h>
void interpretGraphNodes(const CGraphParamValueNodeList &nodes,const unsigned i,const sample_pos_t totalLength,sample_pos_t &segmentStartPosition,double &segmentStartValue,sample_pos_t &segmentStopPosition,double &segmentStopValue,sample_pos_t &segmentLength)
{
	//if(i<0 || i>=nodes.getSize()-1)
	if(i<0 || i>=nodes.size()-1)
		throw runtime_error(string(__func__)+" -- i ("+istring(i)+") is out of range for "+istring(nodes.size())+" nodes");

	const CGraphParamValueNode &startNode=nodes[i];
	const CGraphParamValueNode &stopNode=nodes[i+1];

	// - This condition is here to prevent out of range problems should the 
	//   totalLength be less than the number of segments in the parameter curve
	// - Here, when the segment in the parameter curve to process is more than
	//   than the total length of the data being processed, we just act like 
	//   there's nothing left to do by returning segmentLength as 0
	if(i>=totalLength)
	{ 
		segmentStartValue=startNode.y;
		segmentStopValue=stopNode.y;

		segmentStartPosition=1;
		segmentStopPosition=0;
		segmentLength=0;

		return;
	}

	/*
	if(startNode.x==stopNode.x)
	{
		segmentStartPosition=segmentStopPosition=(sample_pos_t)ceil(startNode.x*(totalLength-1));
		segmentLength=0;
	}
	else if(startNode.x>stopNode.x)
		throw runtime_error(string(__func__)+" -- invalid node list -- node "+istring(i)+"'s x is greater than node "+istring(i+1)+"'s");
	*/

	segmentStartValue=startNode.y;
	segmentStopValue=stopNode.y;

	segmentStartPosition=(sample_pos_t)floor(startNode.x*(totalLength));
	segmentStopPosition=(sample_pos_t)floor(stopNode.x*(totalLength))-1;
	segmentLength=(segmentStopPosition-segmentStartPosition+1);
}


// ------------------------------------

CGraphParamValueIterator::CGraphParamValueIterator(const CGraphParamValueNodeList &_nodes,const sample_pos_t _iterationLength) :
	nodes(_nodes),
	iterationLength(_iterationLength),
	nodeIndex(0),
	t(0),
	segmentLength(0)
{
}

CGraphParamValueIterator::~CGraphParamValueIterator()
{
}

const double CGraphParamValueIterator::next()
{
	if(t<segmentLength)
			// ??? this multiplication may cause problems when the values are very large
		return segmentStartValue+(((segmentStopValueStartValueDiff)*(t++))/(segmentLengthSub1));
	else
	{
		if(nodeIndex>nodes.size()-2)
			return 0.0;

		sample_pos_t segmentStartPosition,segmentStopPosition,_segmentLength;
		double segmentStopValue;
		interpretGraphNodes(nodes,nodeIndex++,iterationLength,segmentStartPosition,segmentStartValue,segmentStopPosition,segmentStopValue,_segmentLength);
		segmentStopValueStartValueDiff=segmentStopValue-segmentStartValue;
		segmentLength=_segmentLength;
			// actually, only -1 when on the last segment
		segmentLengthSub1=segmentLength-(nodeIndex==nodes.size()-1 ? 1 : 0);

		t=0.0;
		return next();
	}
}



