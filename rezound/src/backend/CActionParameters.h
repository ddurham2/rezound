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

#ifndef __CActionParameters_H__
#define __CActionParameters_H__

#include "../../config/common.h"

class CActionParameters;

#include <string>
#include <vector>
#include <map>
#include <utility>

#include "CSound_defs.h"
#include "ALFO.h"
#include "CPluginMapping.h"
#include "CGraphParamValueNode.h"

class ASoundFileManager;
#include <CNestedDataFile/CNestedDataFile.h>


class CActionParameters : public CNestedDataFile
{
public:
	CActionParameters(ASoundFileManager *soundFileManager);
	CActionParameters(const CActionParameters &src);
	virtual ~CActionParameters();

#warning eventually remove these convenience methods and call the base class methods directly from the calling code

	const bool containsParameter(const string name) const { return keyExists(name)==ktValue; };
	void removeParameter(const string name,bool throwIfNotExists=false) { removeKey(name,throwIfNotExists); }

	const bool getBoolParameter(const string name) const				{ return getValue<bool>(name); }
	const string getStringParameter(const string name) const			{ return getValue<string>(name); }
	const unsigned getUnsignedParameter(const string name) const			{ return getValue<unsigned>(name); }
	const sample_pos_t getSamplePosParameter(const string name) const		{ return getValue<sample_pos_t>(name); }
	const double getDoubleParameter(const string name) const			{ return getValue<double>(name); }
	const CGraphParamValueNodeList getGraphParameter(const string name) const	{ return getValue<CGraphParamValueNodeList>(name); }
	const CLFODescription getLFODescription(const string name) const		{ return getValue<CLFODescription>(name); }
	const CPluginMapping getPluginMapping(const string name) const			{ return getValue<CPluginMapping>(name); }

	void setBoolParameter(const string name,const bool v)				{ setValue<bool>(name,v); }
	void setStringParameter(const string name,const string v)			{ setValue<string>(name,v); }
	void setUnsignedParameter(const string name,const unsigned v)			{ setValue<unsigned>(name,v); }
	void setSamplePosParameter(const string name,const sample_pos_t v)		{ setValue<sample_pos_t>(name,v); }
	void setDoubleParameter(const string name,const double v)			{ setValue<double>(name,v); }
	void setGraphParameter(const string name,const CGraphParamValueNodeList &v)	{ setValue<CGraphParamValueNodeList>(name,v); }
	void setLFODescription(const string name,const CLFODescription &v)		{ setValue<CLFODescription>(name,v); }
	void setPluginMapping(const string name,const CPluginMapping &v)		{ setValue<CPluginMapping>(name,v); }

	ASoundFileManager *getSoundFileManager() const;

private:
	ASoundFileManager *soundFileManager;

};

#if 0
/*
 * One of these is always the input to an action
 * Could could be streamed to disk to repeat actions later with the same parameters 
 */
class CActionParameters
{
public:

to change this to use or be a CNestedDataFile I will need to create a method in CNestedDataFile that 
can merge it's keys with another file's so that I can call writeToFile and also tell it that the root value in the file it should write to.. basically prepend that to all keys
also I should be able to extract .. er readFromFile starting at a given key where "" reads the whole file
I will also need to be able to construct CNestedDataFiles with no file at all

	CActionParameters(ASoundFileManager *soundFileManager);
	CActionParameters(const CActionParameters &src);
	virtual ~CActionParameters();

	void writeToFile(CNestedDataFile *f,const string key) const;
	void readFromFile(const CNestedDataFile *f,const string key);

	void clear();

	const bool containsParameter(const string name) const;
	bool removeParameter(const string name,bool throwIfNotExists=false);

	// I would avoid implementing all these different types by using a template method
	// but I need to be able to copy construct the values in the copy constructor,
	// so I don't think I can do this

	void addBoolParameter(const string name,const bool v);
	void addStringParameter(const string name,const string v);
	void addUnsignedParameter(const string name,const unsigned v);
	void addSamplePosParameter(const string name,const sample_pos_t v);
	void addDoubleParameter(const string name,const double v);
	void addGraphParameter(const string name,const CGraphParamValueNodeList &v);
	void addLFODescription(const string name,const CLFODescription &v);
	void addPluginMapping(const string name,const CPluginMapping &v);

	const bool getBoolParameter(const string name) const;
	const string getStringParameter(const string name) const;
	const unsigned getUnsignedParameter(const string name) const;
	const sample_pos_t getSamplePosParameter(const string name) const;
	const double getDoubleParameter(const string name) const;
	const CGraphParamValueNodeList getGraphParameter(const string name) const;
	const CLFODescription &getLFODescription(const string name) const;
	const CPluginMapping &getPluginMapping(const string name) const;

	void setBoolParameter(const string name,const bool v);
	void setStringParameter(const string name,const string v);
	void setUnsignedParameter(const string name,const unsigned v);
	void setSamplePosParameter(const string name,const sample_pos_t v);
	void setDoubleParameter(const string name,const double v);
	void setGraphParameter(const string name,const CGraphParamValueNodeList &v);
	void setLFODescription(const string name,const CLFODescription &v);
	void setPluginMapping(const string name,const CPluginMapping &v);

	ASoundFileManager *getSoundFileManager() const;

private:

	// These used to be the public methods for accessing parameters until I went to 
	// and access-by-name form.  These still could be used, but I left them private
	// because, as of yet, I shouldn't need them (except they do all the work in the
	// actual implementation. Thus, they are necessary at least as private)
	unsigned getParameterCount() const;

	const bool getBoolParameter(const unsigned i) const;
	const string getStringParameter(const unsigned i) const;
	const unsigned getUnsignedParameter(const unsigned i) const;
	const sample_pos_t getSamplePosParameter(const unsigned i) const;
	const double getDoubleParameter(const unsigned i) const;
	const CGraphParamValueNodeList getGraphParameter(const unsigned i) const;
	const CLFODescription &getLFODescription(const unsigned i) const;
	const CPluginMapping &getPluginMapping(const unsigned i) const;

	void setBoolParameter(const unsigned i,const bool v);
	void setStringParameter(const unsigned i,const string v);
	void setUnsignedParameter(const unsigned i,const unsigned v);
	void setSamplePosParameter(const unsigned i,const sample_pos_t v);
	void setDoubleParameter(const unsigned i,const double v);
	void setGraphParameter(const unsigned i,const CGraphParamValueNodeList &v);
	void setLFODescription(const unsigned i,const CLFODescription &v);
	void setPluginMapping(const unsigned i,const CPluginMapping &v);



	ASoundFileManager *soundFileManager;

	enum ParameterTypes 
	{
		ptBool,
		ptString,
		ptUnsigned,
		ptSamplePos,
		ptDouble,
		ptGraph,
		ptLFODescription,
		ptPluginMapping
	};

	//            type          value
	typedef pair<ParameterTypes,void *> parameter_t;
	vector<parameter_t> parameters;

	map<string,unsigned> parameterNames; // index into the parameters vector

};

#endif
#endif
