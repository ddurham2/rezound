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


// some deformations on a node list
const CGraphParamValueNodeList flipGraphNodesHorizontally(const CGraphParamValueNodeList &nodes)
{
	// reverse the order and 1.0-x each node
	CGraphParamValueNodeList tmp;
	for(unsigned t=0;t<nodes.size();t++)
	{
		tmp.push_back(nodes[nodes.size()-t-1]);
		tmp[tmp.size()-1].x=1.0-tmp[tmp.size()-1].x;
	}

	return tmp;
}

const CGraphParamValueNodeList flipGraphNodesVertically(const CGraphParamValueNodeList &_nodes)
{
	CGraphParamValueNodeList nodes=_nodes;
	for(unsigned t=0;t<nodes.size();t++)
		nodes[t].y=1.0-nodes[t].y;
	return nodes;
}

const CGraphParamValueNodeList smoothGraphNodes(const CGraphParamValueNodeList &nodes)
{
	CGraphParamValueNodeList tmp;

	for(int t=1;t<(int)nodes.size();t++)
	{
		const CGraphParamValueNode &n0=nodes[max(0,t-2)];
		const CGraphParamValueNode &n1=nodes[max(0,t-1)];
		const CGraphParamValueNode &n2=nodes[t];
		const CGraphParamValueNode &n3=nodes[min((int)nodes.size()-1,t+1)];

		//double x0=n0.x;
		double y0=n0.y;
		double x1=n1.x;
		double y1=n1.y;
		double x2=n2.x;
		double y2=n2.y;
		//double x3=n3.x;
		double y3=n3.y;

		// if it's already less than 1/500th between x1 and x2, then don't smooth any more
		if((x2-x1)<=0.005)
		{
			tmp.push_back(n1);
			continue;
		}

		const double bias=0;
		const double tension=0;

		for(double x=x1;x<x2;x+=(x2-x1)/2.0)
		{
			const double mu=(double)(x-x1)/(double)(x2-x1);

			// Hermite interpolation -- http://astronomy.swin.edu.au/~pbourke/analysis/interpolation/index.html

			double mu2 = mu * mu;
			double mu3 = mu2 * mu;
			double m0  = (y1-y0)*(1+bias)*(1-tension)/2;
			m0 += (y2-y1)*(1-bias)*(1-tension)/2;
			double m1  = (y2-y1)*(1+bias)*(1-tension)/2;
			m1 += (y3-y2)*(1-bias)*(1-tension)/2;
			double a0 =  2*mu3 - 3*mu2 + 1;
			double a1 =    mu3 - 2*mu2 + mu;
			double a2 =    mu3 -   mu2;
			double a3 = -2*mu3 + 3*mu2;

			const double y=a0*y1+a1*m0+a2*m1+a3*y2;


			tmp.push_back(CGraphParamValueNode(x,y));
		}

	}

	// make sure first one does come out to be zero
	tmp[0].x=0;

	// maintain the same endpoint
	tmp.push_back(nodes[nodes.size()-1]);

	return tmp;
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



