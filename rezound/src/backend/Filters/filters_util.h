#ifndef __filters_util_H__
#define __filters_util_H__

#include "../CGraphParamValueNode.h"

static const CGraphParamValueNodeList normalizeFrequencyResponse(const CGraphParamValueNodeList &_freqResponse,unsigned sampleRate)
{
	CGraphParamValueNodeList freqResponse;

	// The range of freqResponse[].x is firstOctaveFrequency -> firstOctaveFrequency*pow(2,numberOfOctaves) so I have to fabricate values from 0 to baseFrequency
	// and either fabricate values above the max (if the sampleRate/2 is higher), or ignore some of the values  (if the sampleRate/2 is less than the max)
	freqResponse.push_back(CGraphParamValueNode(0,_freqResponse[0].y)); // fabricate the values from 0 to the base the same as the first element defined
	for(size_t t=0;t<_freqResponse.size();t++)
	{
		//printf("%d: %f, %f\n",t,_freqResponse[t].x,_freqResponse[t].y);

		if(_freqResponse[t].x>(sampleRate/2))
		{
			if(t<1)
			{ // shouldn't happen, but I suppose it could
				freqResponse.push_back(CGraphParamValueNode(1.0, _freqResponse[t].y ));
			}
			else
			{
				const double x1=_freqResponse[t-1].x;
				const double x2=_freqResponse[t].x;
				const double y1=_freqResponse[t-1].y;
				const double y2=_freqResponse[t].y;
				const double x=sampleRate/2;
									// interpolate where the truncation point to fall on the last line segment
				const double y=((y2-y1)/(x2-x1))*(x-x1)+y1;
				freqResponse.push_back(CGraphParamValueNode(1.0,y));
			}
			break;
		}
		else
			freqResponse.push_back(CGraphParamValueNode(_freqResponse[t].x/(sampleRate/2),_freqResponse[t].y));
	}

	// fabricate values above the highest defined frequency in the given response if the sampleRate/2 is higher than it
	if(_freqResponse[_freqResponse.size()-1].x<=sampleRate/2)
		freqResponse.push_back(CGraphParamValueNode(1.0,_freqResponse[_freqResponse.size()-1].y));

/* might need to print the kernel nodes in debugging
	for(size_t t=0;t<freqResponse.size();t++)
		printf("%d: %f, %f\n",t,freqResponse[t].x,freqResponse[t].y);
*/

	return freqResponse;
}

#endif
