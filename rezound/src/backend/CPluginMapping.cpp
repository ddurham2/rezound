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

#include "CPluginMapping.h"


CPluginMapping::CPluginMapping() :
	outputAppendCount(0),
	outputRemoveCount(0)
{
}

CPluginMapping::CPluginMapping(const CPluginMapping &src) :
	outputAppendCount(src.outputAppendCount),
	inputMappings(src.inputMappings),
	outputMappings(src.outputMappings),
	passThrus(src.passThrus),
	//outputSwapChannels(src.outputSwapChannels),
	outputRemoveCount(src.outputRemoveCount)
{
}

CPluginMapping &CPluginMapping::operator=(const CPluginMapping &rhs)
{
	outputAppendCount=rhs.outputAppendCount;
	inputMappings=rhs.inputMappings;
	outputMappings=rhs.outputMappings;
	passThrus=rhs.passThrus;
	//outputSwapChannels=rhs.outputSwapChannels;
	outputRemoveCount=rhs.outputRemoveCount;

	return *this;
}

CPluginMapping::~CPluginMapping()
{
}


#include "CSound.h"
const CPluginMapping CPluginMapping::getDefaultMapping(const string pluginName,unsigned pluginInputPorts,unsigned pluginOutputPorts,const CSound *sound)
{
#warning the case (but generalize it) that there is one input, one channel, and two outputs, append a channel and map both outputs 
	CPluginMapping m;

	// locate which index to default things to by looking at what index is sound at
	size_t soundFileManagerIndex=0;
	/* don't have it to look for.. so just leave as zero for now
	for(size_t t=0;t<soundFileManager->getOpenedCount();t++)
	{
		if(soundFileManager->getSound(t)->sound==sound)
		{
			soundFileManagerindex=t;
			break;
		}
	}
	*/

	if(pluginInputPorts>pluginOutputPorts)
	{
		throw runtime_error(string(__func__)+" -- unimplemented that input ports is greater than output ports");
	}
	else if(pluginInputPorts==pluginOutputPorts)
	{
		if(pluginInputPorts==0)
			throw runtime_error("Both input and output ports for this plugin are 0");

		if((sound->getChannelCount()%pluginInputPorts)==0)
		{
			unsigned srcChannel=0;
			unsigned destChannel=0;
			for(unsigned t=0;t<sound->getChannelCount()/pluginInputPorts;t++)
			{
				// create a new iteration of the plugin (for instance if the plugin has 1 input port (and same # outputs) and the sound has 2 channels, then we would iterate the plugin twice)
				m.inputMappings.push_back(vector<vector<vector<CPluginMapping::RInputDesc> > >::value_type());
				m.outputMappings.push_back(vector<vector<vector<CPluginMapping::ROutputDesc> > >::value_type());

				vector<vector<CPluginMapping::RInputDesc> > &inputMapping=m.inputMappings[t];
				vector<vector<CPluginMapping::ROutputDesc> > &outputMapping=m.outputMappings[t];

				// make the input ports (for this iteration) point to the next N src channels (where N is the number of input ports on the plugin)
				for(unsigned i=0;i<pluginInputPorts;i++)
				{
					vector<CPluginMapping::RInputDesc> v;
					v.push_back(CPluginMapping::RInputDesc(soundFileManagerIndex,srcChannel++,CPluginMapping::RInputDesc::wdroSilence,CPluginMapping::RInputDesc::hmSelection,1));
					inputMapping.push_back(v);

				}


				// make the sound's next N channels map to the output ports (where N is the numbetr of output ports on the plugin (same # as inputs in this case))
				for(unsigned i=0;i<sound->getChannelCount();i++)
					outputMapping.push_back(vector<vector<CPluginMapping::ROutputDesc> >::value_type());
				for(unsigned i=0;i<pluginInputPorts;i++)
					outputMapping[destChannel+i].push_back(CPluginMapping::ROutputDesc(i,1));
				destChannel+=pluginInputPorts;
			}
		}
		else
			throw runtime_error("the plugin contains the same number of inputs and outputs, but the number of channels in the action sound is not a multiple of the number of inputs of the plugin");
	}
	else // if(pluginInputPorts>pluginOutputPorts)
	{
		if(pluginInputPorts==0)
		{ // sound generation plugin (no audio inputs, only outputs)
			// add more channels if needed
			if(pluginOutputPorts>sound->getChannelCount())
				m.outputAppendCount=sound->getChannelCount()-pluginOutputPorts;

			// ??? possibly remove channels if there are more channels in the action sound that the plugin's output?

			// create one iteration of the plugin
			// 	and even tho there is no input, there needs to be a single empty mapping (moreover, the inputMappings and outputMappings vectors are supposed to be parallel)
			m.inputMappings.push_back(vector<vector<vector<CPluginMapping::RInputDesc> > >::value_type());
			m.outputMappings.push_back(vector<vector<vector<CPluginMapping::ROutputDesc> > >::value_type());

			// this is the output mapping of the single iteration
			vector<vector<CPluginMapping::ROutputDesc> > &outputMapping=m.outputMappings[0];

			// for each output port on the plugin, map it to the same indexed channel on the action sound (and there should be enough channels in the action sound)
			for(unsigned i=0;i<pluginOutputPorts;i++)
			{
				outputMapping.push_back(vector<vector<CPluginMapping::ROutputDesc> >::value_type());
				outputMapping[i].push_back(CPluginMapping::ROutputDesc(i,1));
			}
		}
		else
		{
			throw runtime_error(string(__func__)+" -- unimplemented that input ports is greater than output ports");
		}
	}

	return m;
}

bool CPluginMapping::somethingMappedToAnOutput() const
{
	for(size_t t=0;t<outputMappings.size();t++)
	{
		for(size_t i=0;i<outputMappings[t].size();i++)
		{
			if(outputMappings[t][i].size()>0)
				return true;
		}
	}

	for(size_t t=0;t<passThrus.size();t++)
	{
		if(passThrus[t].size()>0)
			return true;
	}
	return false;
}

#include <stdio.h>
void CPluginMapping::print() const
{
	printf("outputAppendCount: %d\n",outputAppendCount);

	printf("inputMappings:\n");
	for(size_t t=0;t<inputMappings.size();t++)
	{
		for(size_t i=0;i<inputMappings[t].size();i++)
		{
			for(size_t j=0;j<inputMappings[t][i].size();j++)
			{
				printf("\tinstance:%3d  plugin_input:%3d n:%3d -- loadedIndex: %3d channel: %3d wdro: %1d howMuch: %1d gain: %3.2f\n",t,i,j,inputMappings[t][i][j].soundFileManagerIndex,inputMappings[t][i][j].channel,inputMappings[t][i][j].wdro,inputMappings[t][i][j].howMuch,inputMappings[t][i][j].gain);
			}
		}
	}

	printf("outputMapping:\n");
	for(size_t t=0;t<outputMappings.size();t++)
	{
		for(size_t i=0;i<outputMappings[t].size();i++)
		{
			for(size_t j=0;j<outputMappings[t][i].size();j++)
			{
				printf("\tinstance:%3d output_channel:%3d n:%3d -- plugin_output: %3d gain: %3.2f\n",t,i,j,outputMappings[t][i][j].channel,outputMappings[t][i][j].gain);
			}
		}
	}

	printf("passThrus:\n");
	for(size_t t=0;t<passThrus.size();t++)
	{
		printf("\tchannel: %3d\n",t);
		for(size_t i=0;i<passThrus[t].size();i++)
			printf("\t\t%3d -- loadedIndex: %3d channel: %3d wdro: %1d howMuch: %1d gain: %3.2f\n",i,passThrus[t][i].soundFileManagerIndex,passThrus[t][i].channel,passThrus[t][i].wdro,passThrus[t][i].howMuch,passThrus[t][i].gain);
	}

	/*
	printf("outputSwapChannels:\n");
	for(size_t t=0;t<outputSwapChannels.size();t++)
	{
		printf("\tswap channel: %3d and %3d\n",outputSwapChannels[t].first,outputSwapChannels[t].second);
	}
	*/

	printf("outputRemoveCount: %d\n",outputRemoveCount);

}

