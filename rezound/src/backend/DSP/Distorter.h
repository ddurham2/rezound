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

#ifndef __DSP_Distorter_H__
#define __DSP_Distorter_H__

#include "../../config/common.h"

#include <math.h>

#include <istring>

#include "../CGraphParamValueNode.h"
#include "../unit_conv.h"

/* --- TDSPDistorter ------------------------------------
 *	This class is a DSP block to distort the samples according to a given curve
 */
template<class sample_t,int maxSample> class TDSPDistorter
{
public:
	// the curves' nodes' x's and y's should be in dBFS
	TDSPDistorter(const CGraphParamValueNodeList &_positiveCurve,const CGraphParamValueNodeList &_negativeCurve) :
		positiveCurve(dBFS_to_scalar(_positiveCurve)),
		positiveCurveSizeSub1(_positiveCurve.size()-1),
		negativeCurve(dBFS_to_scalar(_negativeCurve)),
		negativeCurveSizeSub1(_negativeCurve.size()-1)
	
	{
	}

	virtual ~TDSPDistorter()
	{
	}

	/*
	...Curve is essentially a graph whose domain and range are [0,1]
	The graph represents how to map the input sample value to the 
	output sample value. a graph with nodes (0,0) and (1,1) does not
	affect the output.
	*/

	const sample_t processSample(const sample_t input) const
	{
		// ??? if sample_t were greater precision than float, this would need to change
		// ??? a flat curve (0,0)-(1,1) does not produce identical output because of truncation errors when going between 0..MAX_SAMPLE and 0..1
		if(input>=0)
		{
			const float inputSample=(float)input/(float)maxSample;
			for(size_t t=0;t<positiveCurveSizeSub1;t++)
			{
				if(inputSample<=(positiveCurve[t+1].x))
					return (sample_t)(maxSample*remap(inputSample,positiveCurve[t].x,positiveCurve[t].y,positiveCurve[t+1].x,positiveCurve[t+1].y));
			}
		}
		else
		{
			const float inputSample=(float)-input/(float)maxSample;
			for(size_t t=0;t<negativeCurveSizeSub1;t++)
			{
				if(inputSample<=(negativeCurve[t+1].x))
					return -(sample_t)(maxSample*remap(inputSample,negativeCurve[t].x,negativeCurve[t].y,negativeCurve[t+1].x,negativeCurve[t+1].y));
			}
		}
		// catch all
		return input;
	}

private:
	const CGraphParamValueNodeList positiveCurve;
	const size_t positiveCurveSizeSub1;
	const CGraphParamValueNodeList negativeCurve;
	const size_t negativeCurveSizeSub1;

	static const float remap(const float sample,const float x1,const float y1,const float x2,const float y2)
	{
		if(x2==x1)
			return sample*y2;
		else
		{
			const float m=(y2-y1)/(x2-x1);
			return m*(sample-x1)+y1;
		}
	}

	static const CGraphParamValueNodeList dBFS_to_scalar(CGraphParamValueNodeList nodes)
	{
		for(size_t t=0;t<nodes.size();t++)
		{
			nodes[t].x=dB_to_scalar(nodes[t].x);
			nodes[t].y=dB_to_scalar(nodes[t].y);
		}
		return nodes;
	}
};

#endif
