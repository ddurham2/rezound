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


CActionParameters::CActionParameters(ASoundFileManager *_soundFileManager) :
	soundFileManager(_soundFileManager)
{
}

CActionParameters::CActionParameters(const CActionParameters &src)
{
	for(unsigned t=0;t<src.parameters.size();t++)
	{
		switch(src.parameters[t].first)
		{
		case ptBool:
			parameters.push_back(parameter_t(src.parameters[t].first, new bool(*((bool *)src.parameters[t].second))));
			break;

		case ptString:
			parameters.push_back(parameter_t(src.parameters[t].first,new string(*((string *)src.parameters[t].second))));
			break;

		case ptUnsigned:
			parameters.push_back(parameter_t(src.parameters[t].first,new unsigned(*((unsigned *)src.parameters[t].second))));
			break;

		case ptSamplePos:
			parameters.push_back(parameter_t(src.parameters[t].first,new sample_pos_t(*((sample_pos_t *)src.parameters[t].second))));
			break;

		case ptDouble:
			parameters.push_back(parameter_t(src.parameters[t].first,new double(*((double *)src.parameters[t].second))));
			break;

		case ptGraph:
			parameters.push_back(parameter_t(src.parameters[t].first,new CGraphParamValueNodeList(*((CGraphParamValueNodeList *)src.parameters[t].second))));
			break;

		case ptLFODescription:
			parameters.push_back(parameter_t(src.parameters[t].first,new CLFODescription(*((CLFODescription *)src.parameters[t].second))));
			break;

		default:
			throw(string(__func__)+" -- internal error -- unhandled parameter type: "+istring(src.parameters[t].first));
		}
	}
}

CActionParameters::~CActionParameters()
{
	clear();
}

ASoundFileManager *CActionParameters::getSoundFileManager() const
{
	return soundFileManager;
}

void CActionParameters::clear()
{
	for(unsigned t=0;t<parameters.size();t++)
	{
		switch(parameters[t].first)
		{
		case ptBool:
			delete ((bool *)parameters[t].second);
			break;

		case ptString:
			delete ((string *)parameters[t].second);
			break;

		case ptUnsigned:
			delete ((unsigned *)parameters[t].second);
			break;

		case ptSamplePos:
			delete ((sample_pos_t *)parameters[t].second);
			break;

		case ptDouble:
			delete ((double *)parameters[t].second);
			break;

		case ptGraph:
			delete ((CGraphParamValueNodeList *)parameters[t].second);
			break;

		case ptLFODescription:
			delete ((CLFODescription *)parameters[t].second);
			break;

		default:
			throw(string(__func__)+" -- internal error -- unhandled parameter type: "+istring(parameters[t].first));
		}
	}
	parameters.clear();
}

unsigned CActionParameters::getParameterCount() const
{
	return(parameters.size());
}

const bool CActionParameters::containsParameter(const string name) const
{
	return(parameterNames.find(name)!=parameterNames.end());
}

#define VERIFY_IS_NEW(name)	 												\
{																\
	const map<string,unsigned>::const_iterator iter=parameterNames.find((name));						\
	if(iter!=parameterNames.end())												\
		throw(runtime_error(string(__func__)+" -- parameter already exists found with name '"+string(name)+"'"));	\
}

void CActionParameters::addBoolParameter(const string name,const bool v)
{
	VERIFY_IS_NEW(name)
	parameterNames[name]=parameters.size();
	parameters.push_back(parameter_t(ptBool,new bool(v)));
}

void CActionParameters::addStringParameter(const string name,const string v)
{
	VERIFY_IS_NEW(name)
	parameterNames[name]=parameters.size();
	parameters.push_back(parameter_t(ptString,new string(v)));
}

void CActionParameters::addUnsignedParameter(const string name,const unsigned v)
{
	VERIFY_IS_NEW(name)
	parameterNames[name]=parameters.size();
	parameters.push_back(parameter_t(ptUnsigned,new unsigned(v)));
}

void CActionParameters::addSamplePosParameter(const string name,const sample_pos_t v)
{
	VERIFY_IS_NEW(name)
	parameterNames[name]=parameters.size();
	parameters.push_back(parameter_t(ptSamplePos,new sample_pos_t(v)));
}

void CActionParameters::addDoubleParameter(const string name,const double v)
{
	VERIFY_IS_NEW(name)
	parameterNames[name]=parameters.size();
	parameters.push_back(parameter_t(ptDouble,new double(v)));
}

void CActionParameters::addGraphParameter(const string name,const CGraphParamValueNodeList &v)
{
	VERIFY_IS_NEW(name)
	parameterNames[name]=parameters.size();
	parameters.push_back(parameter_t(ptGraph,new CGraphParamValueNodeList(v)));
}

void CActionParameters::addLFODescription(const string name,const CLFODescription &v)
{
	VERIFY_IS_NEW(name)
	parameterNames[name]=parameters.size();
	parameters.push_back(parameter_t(ptLFODescription,new CLFODescription(v)));
}



#define PARM_INDEX_BY_NAME(name,index) 										\
{														\
	const map<string,unsigned>::const_iterator iter=parameterNames.find((name));				\
	if(iter==parameterNames.end())										\
		throw(runtime_error(string(__func__)+" -- parameter not found with name '"+string(name)+"'"));	\
	index=iter->second;											\
}

const bool CActionParameters::getBoolParameter(const string name) const
{
	unsigned i;
	PARM_INDEX_BY_NAME(name,i)
	return(getBoolParameter(i));
}

const string CActionParameters::getStringParameter(const string name) const
{
	unsigned i;
	PARM_INDEX_BY_NAME(name,i)
	return(getStringParameter(i));
}

const unsigned CActionParameters::getUnsignedParameter(const string name) const
{
	unsigned i;
	PARM_INDEX_BY_NAME(name,i)
	return(getUnsignedParameter(i));
}

const sample_pos_t CActionParameters::getSamplePosParameter(const string name) const
{
	unsigned i;
	PARM_INDEX_BY_NAME(name,i)
	return(getSamplePosParameter(i));
}

const double CActionParameters::getDoubleParameter(const string name) const
{
	unsigned i;
	PARM_INDEX_BY_NAME(name,i)
	return(getDoubleParameter(i));
}

const CGraphParamValueNodeList CActionParameters::getGraphParameter(const string name) const
{
	unsigned i;
	PARM_INDEX_BY_NAME(name,i)
	return(getGraphParameter(i));
}

const CLFODescription &CActionParameters::getLFODescription(const string name) const
{
	unsigned i;
	PARM_INDEX_BY_NAME(name,i)
	return(getLFODescription(i));
}





void CActionParameters::setBoolParameter(const string name,const bool v)
{
	unsigned i;
	PARM_INDEX_BY_NAME(name,i)
	setBoolParameter(i,v);
}

void CActionParameters::setStringParameter(const string name,const string v)
{
	unsigned i;
	PARM_INDEX_BY_NAME(name,i)
	setStringParameter(i,v);
}

void CActionParameters::setUnsignedParameter(const string name,const unsigned v)
{
	unsigned i;
	PARM_INDEX_BY_NAME(name,i)
	setUnsignedParameter(i,v);
}

void CActionParameters::setSamplePosParameter(const string name,const sample_pos_t v)
{
	unsigned i;
	PARM_INDEX_BY_NAME(name,i)
	setSamplePosParameter(i,v);
}

void CActionParameters::setDoubleParameter(const string name,const double v)
{
	unsigned i;
	PARM_INDEX_BY_NAME(name,i)
	setDoubleParameter(i,v);
}

void CActionParameters::setGraphParameter(const string name,const CGraphParamValueNodeList &v)
{
	unsigned i;
	PARM_INDEX_BY_NAME(name,i)
	setGraphParameter(i,v);
}

void CActionParameters::setLFODescription(const string name,const CLFODescription &v)
{
	unsigned i;
	PARM_INDEX_BY_NAME(name,i)
	setLFODescription(i,v);
}





const bool CActionParameters::getBoolParameter(const unsigned i) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameters[i].first!=ptBool)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptBool"));
	else
		return(*((bool *)parameters[i].second));
}

const string CActionParameters::getStringParameter(const unsigned i) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameters[i].first!=ptString)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptString"));
	else
		return(*((string *)parameters[i].second));
}

const unsigned CActionParameters::getUnsignedParameter(const unsigned i) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));

	if(parameters[i].first==ptUnsigned)
		return(*((unsigned *)parameters[i].second));
	else if(parameters[i].first==ptDouble)
		return((unsigned)(*((double *)parameters[i].second)));
	else
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptUnsigned nor ptDouble"));
}

const sample_pos_t CActionParameters::getSamplePosParameter(const unsigned i) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameters[i].first!=ptSamplePos)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptSamplePos"));
	else
		return(*((sample_pos_t *)parameters[i].second));
}

const double CActionParameters::getDoubleParameter(const unsigned i) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameters[i].first==ptDouble)
		return(*((double *)parameters[i].second));
	else if(parameters[i].first==ptUnsigned)
		return((double)(*((unsigned *)parameters[i].second)));
	else
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptDouble nor ptUnsigned"));
}

const CGraphParamValueNodeList CActionParameters::getGraphParameter(const unsigned i) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameters[i].first!=ptGraph)
	{
		if(parameters[i].first==ptDouble)
			return(singleValueToGraph(*((double *)parameters[i].second)));
		else
			throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptDouble or ptGraph"));
	}
	else
		return(*((CGraphParamValueNodeList *)parameters[i].second));
}

const CLFODescription &CActionParameters::getLFODescription(const unsigned i) const
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameters[i].first!=ptLFODescription)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptLFODescription"));
	else
		return(*((CLFODescription *)parameters[i].second));
}



void CActionParameters::setBoolParameter(const unsigned i,const bool v)
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameters[i].first!=ptBool)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptBool"));
	else
		(*((bool *)parameters[i].second))=v;
}

void CActionParameters::setStringParameter(const unsigned i,const string v)
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameters[i].first!=ptString)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptString"));
	else
		(*((string *)parameters[i].second))=v;
}

void CActionParameters::setUnsignedParameter(const unsigned i,const unsigned v)
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameters[i].first!=ptUnsigned)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptUnsigned"));
	else
		(*((unsigned *)parameters[i].second))=v;
}

void CActionParameters::setSamplePosParameter(const unsigned i,const sample_pos_t v)
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameters[i].first!=ptSamplePos)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptSamplePos"));
	else
		(*((sample_pos_t *)parameters[i].second))=v;
}

void CActionParameters::setDoubleParameter(const unsigned i,const double v)
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameters[i].first!=ptDouble)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptDouble"));
	else
		(*((double *)parameters[i].second))=v;
}

void CActionParameters::setGraphParameter(const unsigned i,const CGraphParamValueNodeList &v)
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameters[i].first!=ptGraph)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptGraph"));
	else
		(*((CGraphParamValueNodeList *)parameters[i].second))=v;
}

void CActionParameters::setLFODescription(const unsigned i,const CLFODescription &v)
{
	if(i>=parameters.size())
		throw(runtime_error(string(__func__)+" -- index out of bounds: "+istring(i)+" -- caller probably didn't pass enough parameters to the action"));
	if(parameters[i].first!=ptLFODescription)
		throw(runtime_error(string(__func__)+" -- parameter at index: "+istring(i)+" is not ptLFODescription"));
	else
		(*((CLFODescription *)parameters[i].second))=v;
}

