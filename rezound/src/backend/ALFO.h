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

#ifndef __ALFO_H__
#define __ALFO_H__

#include "../../config/common.h"

#include "CSound_defs.h"

#include <string>
class CNestedDataFile;

/* --- ALFO ---------------------
 *  - This is an abstract class used to generate an LFO value
 *  - DSP algorithms can be designed to use one of these instead of a hard coded function or constant value
 *  - The derived LFO class simply overrides the nextValue method and DSP algorithms call it not needing to know the LFO function
 *  	- Sometimes, however,  it is not possible to write the algorithm using an LFO and not knowing the LFO function because
 *  	  some algorithms may need to determine things from the integral or derivative of the function.  I could have each 
 *  	  LFO implementation to supply these related functions too, but as of now, I haven't needed it.
 */
class ALFO
{
public:
	virtual ~ALFO();

	virtual const float nextValue()=0;
	virtual const float getValue(sample_pos_t time) const=0; // time is in samples, that is not seconds or ms

protected:
	ALFO();

};


class CLFODescription
{
public:
	CLFODescription() :
		amp(0),
		freq(0),
		phase(0),
		LFOType(0)
	{
	}

	CLFODescription(const float _amp,const float _freq,const float _phase,const size_t _LFOType) :
		amp(_amp),
		freq(_freq),
		phase(_phase),
		LFOType(_LFOType)
	{
	}

	CLFODescription(const CLFODescription &src) :
		amp(src.amp),
		freq(src.freq),
		phase(src.phase),
		LFOType(src.LFOType)
	{
	}

	const CLFODescription &operator=(const CLFODescription &rhs)
	{
		amp=rhs.amp;
		freq=rhs.freq;
		phase=rhs.phase;
		LFOType=rhs.LFOType;
		return(*this);
	}

	float amp,freq,phase;
	size_t LFOType;

	void writeToFile(CNestedDataFile *f,const string key) const;
	void readFromFile(const CNestedDataFile *f,const string key);
};

#include <CNestedDataFile/anytype.h>
template<> STATIC_TPL const CLFODescription string_to_anytype<CLFODescription>(const string &str,CLFODescription &ret) 
{
	 CNestedDataFile f;
	 f.parseString(s2at::remove_surrounding_quotes(str));
	 ret.readFromFile(&f,"");
	 return ret;
}

template<> STATIC_TPL const string anytype_to_string<CLFODescription>(const CLFODescription &any) 
{
	CNestedDataFile f; any.writeToFile(&f,"");
	return "\""+s2at::escape_chars(istring(f.asString()).searchAndReplace("\n"," ",true))+"\"";
}

class CLFORegistry
{
public:
	CLFORegistry();
	virtual ~CLFORegistry();
	
	const size_t getCount() const;
	const string getName(const size_t index) const;
	const bool isBipolar(const size_t index) const;
	const size_t getIndexByName(const string name) const;

	// the return value should be deleted by the caller
	ALFO *createLFO(const CLFODescription &desc,const unsigned sampleRate) const;
};

extern const CLFORegistry gLFORegistry;


#endif
