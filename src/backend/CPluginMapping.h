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

#ifndef __CPluginMapping_H__
#define __CPluginMapping_H__

#include "../../config/common.h"

#include <string>
#include <vector>
#include <utility>
#include <stdexcept>

class CSound;
class CNestedDataFile;

class CPluginMapping
{
public:
	CPluginMapping();
	CPluginMapping(const CPluginMapping &src);

	virtual ~CPluginMapping();

	void print() const;

	CPluginMapping &operator=(const CPluginMapping &rhs);

	void writeToFile(CNestedDataFile *f,const string key) const;
	void readFromFile(const CNestedDataFile *f,const string key);


	// First this values should be used to modify the output channels by appending 
	// this many channels to the output sound
	unsigned outputAppendCount;


	// After possibly having modified the output these vectors indicate how to map
	// plugin's ins and outs to the source sound(s) and (modified) destination sound

	// inputMapping and outputMapping are in parallel indicating how many times the 
	// plugin should be  performed since the plugin might have 1 input port and the 
	// action sound have 2  channels so the plugin should perhaps be performed twice, 
	// once for each channel  in the sound

	struct RInputDesc
	{
		unsigned soundFileManagerIndex; /*??? could make this a string*/
		unsigned channel;
		enum WhenDataRunsOut
		{
			// the order and values of these are important to be synced with what the frontend expects
			wdroSilence=0,	// when data runs out on this input just produce silence
			wdroLoop=1	// when data runs out on this input loop back to beginning
		} wdro;
		enum HowMuch
		{
			// the order and values of these are important to be synced with what the frontend expects
			hmSelection=0,
			hmAll=1
		} howMuch;
		float gain;
		//??? probably need others describing start/stop positions

		RInputDesc(unsigned _soundFileManagerIndex,unsigned _channel,WhenDataRunsOut _wdro,HowMuch _howMuch,float _gain) :
			soundFileManagerIndex(_soundFileManagerIndex),
			channel(_channel),
			wdro(_wdro),
			howMuch(_howMuch),
			gain(_gain)
		{
		}

		// default is fine: RInputDesc(const RInputDesc &src) (??? not if I make something a string though)

		RInputDesc &operator=(const RInputDesc &rhs)
		{
			soundFileManagerIndex=rhs.soundFileManagerIndex;
			channel=rhs.channel;
			wdro=rhs.wdro;
			howMuch=rhs.howMuch;
			gain=rhs.gain;
			return *this;
		}

		void writeToFile(CNestedDataFile *f,const string key) const;
		void readFromFile(const CNestedDataFile *f,const string key);
	};

	vector<
		/*
		 * This is used to map channels from multiple sounds to the input ports 
		 * of a plugin
		 * 
		 * Suppose a plugin had 6 input audio ports and we had 3 sounds each with 
		 * 4 channels that needed to be mapped to the plugin's 6 inputs.  
		 *
		 * To use this data-member, the outer vector would contain an element for 
		 * each input on the plugin (so 6 elements in the outer vector).  
		 * 
		 * The inner vector defines which channels map to that input port on the 
		 * plugin (multiple channels are allowed to be mapped to an input and they 
		 * should be mixed together when there is more than 1).  The data type of 
		 * the inner vector describes which sound and which channel the input should
		 * come from and what to do when the data runs out.
		 *
		 * Hence, there would be 6 elements in the outer vector.  The inner vector
		 * (at one of the outer vector's positions) should possibly contained only
		 * 12 values max.  The first part of the pair could have a range of 0 to 2 
		 * and the second part of the pair could have a range of 0 to 3.
		 *
		 */
		vector<vector<RInputDesc> >
	> inputMappings;


	struct ROutputDesc
	{
		unsigned channel;
		float gain;

		ROutputDesc(unsigned _channel,float _gain) :
			channel(_channel),
			gain(_gain)
		{
		}

		ROutputDesc &operator=(const RInputDesc &rhs)
		{
			channel=rhs.channel;
			gain=rhs.gain;
			return *this;
		}

		void writeToFile(CNestedDataFile *f,const string key) const;
		void readFromFile(const CNestedDataFile *f,const string key);
	};

	vector<
		/*
 		 * This is used to map channels in the destination back to output ports on 
		 * the plugin, possibly mapping to a destination channel more than one output
		 * port on the plugin.
		 * 
 		 * Suppose a plugin had 5 channels of output (5 output ports) to be mapped to
		 * a destination sound with 4 channels 
		 * 
 		 * To use this type, the outer vector would contain an element for each 
		 * channel in the destination sound (so there would be 4 elements in the outer
		 * vector).  
		 * 
 		 * The inner vector defines which channels of the plugin's outputs should be 
		 * mixed together and put in the destination channel at that element in the 
		 * outer vector.  Associated with each is a gain to apply as well.
		 * 
 		 * Hence, there would be 4 elements in the outer vector.  The inner vector (at 
		 * one of the outer vector's position) should possibly contain only 5 values 
		 * max.  
		 *
		 */
		vector<vector<ROutputDesc> >
	> outputMappings;

	// NOTE that the outer vectors and inner vectors talked about in the two comments above 
	// are inverted in the two desciptions.
	

	/*
	 * This contains the connections of inputs directly to channels in the output
	 * There is an element in the outer vector for each channel in the output
	 * The bool in the pair indicates whether to clear the output channel's data 
	 * before writing to it or not.
	 *
	 * The inner vector (.second of the pair) lists all inputs that should mix into 
	 * that output channel.  The data members of RInputDesc define what sound and
	 * channel, and what to do when data runs out of that input
	 */
	vector<vector<RInputDesc> > passThrus;


/* not going to do this for now.. I think it can be done with user chosen passThrus and routing
	// Finally, this vector indicates which channels to swap around and the removeCount is 
	// how many channels to remove from the end of the sound
	vector<pair<unsigned,unsigned> > outputSwapChannels;
*/

	unsigned outputRemoveCount;


	// This returns a default mapping that should be able to be used for most plugins it
	// will throw a runtime_error with a message to the user if a default mapping cannot
	// be determined.
	// 
	// The plugin name is passed so default mappings can be developed for often used pluings
	static const CPluginMapping getDefaultMapping(const string pluginName,unsigned pluginInputPorts,unsigned pluginOutputPorts,const CSound *sound);

	bool somethingMappedToAnOutput() const;
};

#include <CNestedDataFile/anytype.h>
template<> STATIC_TPL const CPluginMapping string_to_anytype<CPluginMapping>(const string &str,CPluginMapping &ret)
{
	CNestedDataFile f;
	f.parseString(s2at::remove_surrounding_quotes(str));
	ret.readFromFile(&f,"");
	return ret;
}

template<> STATIC_TPL const string anytype_to_string<CPluginMapping>(const CPluginMapping &any)
{
	CNestedDataFile f;
	any.writeToFile(&f,"");
	return "\""+s2at::escape_chars(istring(f.asString()).searchAndReplace("\n"," ",true))+"\"";
}

#endif
