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

#include "CSound_defs.h"

/*
 * One of these is always the input to an action
 * Could could be streamed to disk to repeat actions later with the same parameters 
 */
#include "CGraphParamValueNode.h"
class CActionParameters
{
public:
	CActionParameters();
	CActionParameters(const CActionParameters &src);
	virtual ~CActionParameters();

	void clear();

	unsigned getParameterCount() const;

	// I would avoid implementing all these different types by using a template method
	// but I need to be able to copy construct the values in the copy constructor,
	// so I don't think I can do this

	void addBoolParameter(const bool v);
	void addStringParameter(const string v);
	void addUnsignedParameter(const unsigned v);
	void addSamplePosParameter(const sample_pos_t v);
	void addDoubleParameter(const double v);
	void addGraphParameter(const CGraphParamValueNodeList &v);

	const bool getBoolParameter(const unsigned i) const;
	const string getStringParameter(const unsigned i) const;
	const unsigned getUnsignedParameter(const unsigned i) const;
	const sample_pos_t getSamplePosParameter(const unsigned i) const;
	const double getDoubleParameter(const unsigned i) const;
	const CGraphParamValueNodeList getGraphParameter(const unsigned i) const;

	void setBoolParameter(const unsigned i,const bool v) const;
	void setStringParameter(const unsigned i,const string v) const;
	void setUnsignedParameter(const unsigned i,const unsigned v) const;
	void setSamplePosParameter(const unsigned i,const sample_pos_t v) const;
	void setDoubleParameter(const unsigned i,const double v) const;
	void setGraphParameter(const unsigned i,const CGraphParamValueNodeList &v) const;

private:
	enum ParameterTypes { ptBool,ptString,ptUnsigned,ptSamplePos,ptDouble,ptGraph };

	// these two vectors are parallel
	vector<ParameterTypes> parameterTypes;
	vector<void *> parameters;

};

#endif
