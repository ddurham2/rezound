/* 
 * Copyright (C) 2002 - David W. Durham
 * 
 * This file is not part of any particular application.
 * 
 * CNestedDataFile is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 * 
 * istring is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */


/*
 * I could have used XML, to store this sort of information, but it's way too bulky for me.
 * So, here's my simplistic solution.
 * -- Davy
 */

#include "CNestedDataFile.h"

#include <stdlib.h>
#include <string.h>

#include <stdexcept>

#include <istring>

extern int cfg_parse();

CNestedDataFile *CNestedDataFile::parseTree;
const char *CNestedDataFile::initialFilename;

CNestedDataFile::CNestedDataFile(const string filename) :
	parentScope(NULL)
{
	#ifdef THREAD_SAFE_CSCOPE
	mutex.lock();
	#endif
	try
	{
		initialFilename=filename.c_str();
		parseTree=this;
		if(cfg_parse())
			throw(runtime_error("CNestedDataFile::CNestedDataFile -- error while parsing file: "+filename));

		//free(scanString);
		#ifdef THREAD_SAFE_CSCOPE
		mutex.unlock();
		#endif
	}
	catch(...)
	{
		//free(scanString);
		#ifdef THREAD_SAFE_CSCOPE
		mutex.unlock();
		#endif
		throw;
	}

}

CNestedDataFile::CNestedDataFile(const CNestedDataFile &src)
{
	throw(runtime_error("CNestedDataFile::CNestedDataFile -- invalid to call this copy constructor"));
}

CNestedDataFile::CNestedDataFile(const char *_name,CNestedDataFile *_parentScope) :
	name(_name),
	parentScope(_parentScope)
{
}

CNestedDataFile::CNestedDataFile(CNestedDataFile *_parentScope,const CNestedDataFile &src) :
	name(src.name),
	parentScope(_parentScope)
{
	for(map<string,CVariant *>::const_iterator i=src.values.begin();i!=src.values.end();i++)
		values[i->first]=new CVariant(parentScope,*i->second);
}

CNestedDataFile::~CNestedDataFile()
{
	for(map<string,CVariant *>::iterator i=values.begin();i!=values.end();i++)
		delete i->second;
	values.clear();
}

const string CNestedDataFile::getValue(const char *key) const
{
	CVariant *value;
	if(!prvGetValue(value,key,0,true))
		return("");

	switch(value->type)
	{
	case vtString:
		return(value->stringValue);
	case vtFloat:
		return(istring(value->floatValue));
	case vtScope:
		throw(runtime_error("CNestedDataFile::getValue -- "+string(key)+" resolves to a scope, not a value"));
	default:
		throw(runtime_error("CNestedDataFile::getValue -- internal error: unhandled type: "+istring(value->type)));
	}
}

bool CNestedDataFile::prvGetValue(CVariant *&retValue,const char *key,int offset,bool throwOnError) const
{
	// look for a dot in the key
	int pos=strchr(key+offset,'.')-(key+offset);
	if(pos<0)
	{ // no dot found, then we must be now asking for a value
	
		map<string,CVariant *>::const_iterator i=values.find(string(key+offset));
		if(i==values.end())
		{
			retValue=NULL;
			return(false);
		}
		else
		{
			retValue=i->second;
			return(true);
		}
	}
	else
	{ // dot found, then we must be asking for a nested scope
		map<string,CVariant *>::const_iterator i=values.find(string(key+offset,pos));
		if(i==values.end())
		{
			retValue=NULL;
			return(false);
		}
		else
		{
			if(i->second->type!=vtScope)
			{
				if(throwOnError)
					throw(runtime_error("CNestedDataFile::getValue -- "+string(key)+" resolves too soon to a value at: "+string(key,offset+pos)));
				else
				{
					retValue=NULL;
					return(false);
				}
			}
			else
				return(i->second->scopeValue->prvGetValue(retValue,key,offset+pos+1,throwOnError));
		}
	}
}


CNestedDataFile::CVariant *CNestedDataFile::upwardsScopeLookup(const char *key) const
{
/*
 * This method is used to resolve names like "aaa.bbb.c" when used in expressions
 * For example:
 * 	aaa
 * 	{
 * 		bbb
 * 		{
 * 			c=10;
 * 		}
 *
 * 		ddd
 * 		{
 * 			e=aaa.bbb.c;
 * 		}
 * 	}
 *
 * When The "aaa.bbb.c" token is encountered it should attempt to resolve that
 * name as a value.  The correct way to do this is: Starting within the scope, ddd
 * attempt to look up the name "aaa.bbb.c".  It won't be found, so go out a scope to
 * ddd's parent, aaa.  Then attempt to look up the name "aaa.bbb.c".  It again won't 
 * be found, so go out a scope again to aaa's parent, the root scope.  Then attempt
 * to look up the name "aaa.bbb.c".  This time it will resolve to the value, 10.
 *
 * The name upwardsScopeLookup means each time the lookup faile, it goes UP the 
 * scope tree to the searched scope's parent.
 */
	const CNestedDataFile *scope=this; // start with the current scope
	while(scope!=NULL)
	{
		CVariant *value;
		if(scope->prvGetValue(value,key,0,false))
			return(value);

		scope=scope->parentScope;
	}
	// not found -> exception on error???
	return(NULL);
}



CNestedDataFile::CVariant::CVariant() :
	type(vtInvalid)
{
}

CNestedDataFile::CVariant::CVariant(CNestedDataFile *value) :
	type(vtScope),
	scopeValue(value)
{
}

CNestedDataFile::CVariant::CVariant(const string value) :
	type(vtString),
	stringValue(value)
{
}

CNestedDataFile::CVariant::CVariant(float value) :
	type(vtFloat),
	floatValue(value)
{
}

CNestedDataFile::CVariant::CVariant(const CVariant &src)
{
	throw(runtime_error("CNestedDataFile::CVariant::CVariant -- invalid to call this constructor"));
}

CNestedDataFile::CVariant::CVariant(CNestedDataFile *parentScope,const CVariant &src)
{
	//operator=(src);
	type=src.type;
	switch(type)
	{
	case vtScope:
		scopeValue=new CNestedDataFile(parentScope,*src.scopeValue);
		break;
	case vtString:
		stringValue=src.stringValue;
		break;
	case vtFloat:
		floatValue=src.floatValue;
		break;
	default:
		throw(runtime_error("CNestedDataFile::CVariant::CVariant -- internal error"));
	}
}

CNestedDataFile::CVariant::~CVariant()
{
	switch(type)
	{
	case vtScope:
		delete scopeValue;
		break;
	}
}

CNestedDataFile::CVariant &CNestedDataFile::CVariant::operator=(const CVariant &rhs)
{
	throw(runtime_error("CNestedDataFile::CVariant::operator= -- invalid to call this operator="));
	/*
	type=rhs.type;
	switch(type)
	{
	case vtScope:
		scopeValue=new CNestedDataFile(NULL,*rhs.scopeValue);
		break;
	case vtString:
		stringValue=rhs.stringValue;
		break;
	case vtFloat:
		floatValue=rhs.floatValue;
		break;
	default:
		throw(runtime_error("CNestedDataFile::CVariant::operator= -- internal error"));
	}

	return(*this);
	*/
}

