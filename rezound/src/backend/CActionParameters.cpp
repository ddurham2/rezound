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

#include "CActionParameters.h"

#include <stdexcept>

#include <istring>


CActionParameters::CActionParameters()
{
}

CActionParameters::CActionParameters(const CActionParameters &src)
{
	for(unsigned t=0;t<src.parameters.size();t++)
	{
		parameterTypes.push_back(src.parameterTypes[t]);
		switch(parameterTypes[parameterTypes.size()-1])
		{
		case ptBool:
			parameters.push_back(new bool(*((bool *)src.parameters[t])));
			break;

		case ptString:
			parameters.push_back(new string(*((string *)src.parameters[t])));
			break;

		case ptUnsigned:
			parameters.push_back(new unsigned(*((unsigned *)src.parameters[t])));
			break;

		case ptSamplePos:
			parameters.push_back(new sample_pos_t(*((sample_pos_t *)src.parameters[t])));
			break;

		case ptDouble:
			parameters.push_back(new double(*((double *)src.parameters[t])));
			break;

		case ptGraph:
			parameters.push_back(new CGraphParamValueNodeList(*((CGraphParamValueNodeList *)src.parameters[t])));
			break;

		default:
			throw(string(__func__)+" -- internal error -- unhandled parameter type: "+istring(parameterTypes[parameterTypes.size()-1]));
		}
	}
}

CActionParameters::~CActionParameters()
{
	clear();
}

void CActionParameters::clear()
{
	for(unsigned t=0;t<parameters.size();t++)
	{
		switch(parameterTypes[t])
		{
		case ptBool:
			delete ((bool *)parameters[t]);
			break;

		case ptString:
			delete ((string *)parameters[t]);
			break;

		case ptUnsigned:
			delete ((unsigned *)parameters[t]);
			break;

		case ptSamplePos:
			delete ((sample_pos_t *)parameters[t]);
			break;

		case ptDouble:
			delete ((double *)parameters[t]);
			break;

		case ptGraph:
			delete ((CGraphParamValueNodeList *)parameters[t]);
			break;

		default:
			throw(string(__func__)+" -- internal error -- unhandled parameter type: "+istring(parameterTypes[parameterTypes.size()-1]));
		}
	}
	parameterTypes.clear();
	parameters.clear();
}

unsigned CActionParameters::getParameterCount() const
{
	return(parameters.size());
}

void CActionParameters::addBoolParameter(const bool v)
{
	parameterTypes.push_back(ptBool);
	parameters.push_back(new bool(v));
}

void CActionParameters::addStringParameter(const string v)
{
	parameterTypes.push_back(ptString);
	parameters.push_back(new string(v));
}

void CActionParameters::addUnsignedParameter(const unsigned v)
{
	parameterTypes.push_back(ptUnsigned);
	parameters.push_back(new unsigned(v));
}

void CActionParameters::addSamplePosParameter(const sample_pos_t v)
{
	parameterTypes.push_back(ptSamplePos);
	parameters.push_back(new sample_pos_t(v));
}

void CActionParameters::addDoubleParameter(const double v)
{
	parameterTypes.push_back(ptDouble);
	parameters.push_back(new double(v));
}

void CActionParameters::addGraphParameter(const CGraphParamValueNodeList &v)
{
	parameterTypes.push_back(ptGraph);
	parameters.push_back(new CGraphParamValueNodeList(v));
}

const bool CActionParameters::getBoolParameter(const unsigned i) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameterTypes[i]!=ptBool)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptBool"));
	else
		return(*((bool *)parameters[i]));
}

const string CActionParameters::getStringParameter(const unsigned i) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameterTypes[i]!=ptString)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptString"));
	else
		return(*((string *)parameters[i]));
}

const unsigned CActionParameters::getUnsignedParameter(const unsigned i) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameterTypes[i]!=ptUnsigned)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptUnsigned"));
	else
		return(*((unsigned *)parameters[i]));
}

const sample_pos_t CActionParameters::getSamplePosParameter(const unsigned i) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameterTypes[i]!=ptSamplePos)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptSamplePos"));
	else
		return(*((sample_pos_t *)parameters[i]));
}

const double CActionParameters::getDoubleParameter(const unsigned i) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameterTypes[i]!=ptDouble)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptDouble"));
	else
		return(*((double *)parameters[i]));
}

const CGraphParamValueNodeList CActionParameters::getGraphParameter(const unsigned i) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameterTypes[i]!=ptGraph)
	{
		if(parameterTypes[i]==ptDouble)
			return(singleValueToGraph(*((double *)parameters[i])));
		else
			throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptDouble or ptGraph"));
	}
	else
		return(*((CGraphParamValueNodeList *)parameters[i]));
}



void CActionParameters::setBoolParameter(const unsigned i,const bool v) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameterTypes[i]!=ptBool)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptBool"));
	else
		(*((bool *)parameters[i]))=v;
}

void CActionParameters::setStringParameter(const unsigned i,const string v) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameterTypes[i]!=ptString)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptString"));
	else
		(*((string *)parameters[i]))=v;
}

void CActionParameters::setUnsignedParameter(const unsigned i,const unsigned v) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameterTypes[i]!=ptUnsigned)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptUnsigned"));
	else
		(*((unsigned *)parameters[i]))=v;
}

void CActionParameters::setSamplePosParameter(const unsigned i,const sample_pos_t v) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameterTypes[i]!=ptSamplePos)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptSamplePos"));
	else
		(*((sample_pos_t *)parameters[i]))=v;
}

void CActionParameters::setDoubleParameter(const unsigned i,const double v) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameterTypes[i]!=ptDouble)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptDouble"));
	else
		(*((double *)parameters[i]))=v;
}

void CActionParameters::setGraphParameter(const unsigned i,const CGraphParamValueNodeList &v) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameterTypes[i]!=ptGraph)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptGraph"));
	else
		(*((CGraphParamValueNodeList *)parameters[i]))=v;
}


