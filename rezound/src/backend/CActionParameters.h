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

class ASoundFileManager;


/*
 * One of these is always the input to an action
 * Could could be streamed to disk to repeat actions later with the same parameters 
 */
#include "CGraphParamValueNode.h"
class CActionParameters
{
public:
	CActionParameters(ASoundFileManager *soundFileManager);
	CActionParameters(const CActionParameters &src);
	virtual ~CActionParameters();

	void clear();

	const bool containsParameter(const string name) const;

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
