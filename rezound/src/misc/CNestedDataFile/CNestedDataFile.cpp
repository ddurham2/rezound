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
 * CNestedDataFile is distributed in the hope that it will be useful, but
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
#include <stdio.h>

#include <stdexcept>
#include <algorithm>

#include <istring>

extern int cfg_parse();

CNestedDataFile *CNestedDataFile::parseTree;
const char *CNestedDataFile::initialFilename;

CNestedDataFile::CNestedDataFile(const string filename)
{
	#ifdef THREAD_SAFE_CSCOPE
	mutex.lock();
	#endif
	try
	{
		initialFilename=filename.c_str();
		this->filename=filename;
		root=new CVariant("root");
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

CNestedDataFile::~CNestedDataFile()
{
	delete root;
}

const string CNestedDataFile::getValue(const char *key,bool throwIfNotExists) const
{
	CVariant *value;
	if(!prvGetValue(value,key,0,true,root))
	{
		if(throwIfNotExists)
			throw(runtime_error("CNestedDataFile::getValue -- key '"+string(key)+"' does not exist from file: "+filename));
		else
			return("");
	}

	switch(value->type)
	{
	case vtString:
		return(value->stringValue);
	case vtFloat:
		return(istring(value->floatValue));
	case vtScope:
		throw(runtime_error("CNestedDataFile::getValue -- '"+string(key)+"' resolves to a scope, not a value from file: "+filename));
	default:
		throw(runtime_error("CNestedDataFile::getValue -- internal error: unhandled type: '"+istring(value->type)+"' from file: "+filename));
	}
}

bool CNestedDataFile::keyExists(const char *key) const
{
	CVariant *value;
	return(prvGetValue(value,key,0,false,root));
}

bool CNestedDataFile::prvGetValue(CVariant *&retValue,const char *key,int offset,bool throwOnError,const CVariant *variant) const
{
	if(variant->type!=vtScope)
	{
		if(throwOnError)
			throw(runtime_error("CNestedDataFile::getValue -- '"+string(key)+"' resolves too soon to a value at: '"+string(key,max(0,offset-1))+"' from file: "+filename));
		else
			return(false);
	}

	// look for a dot in the key
	int pos=strchr(key+offset,'.')-(key+offset);
	if(pos<0)
	{ // no dot found, then we must be now asking for a value
		map<string,CVariant>::const_iterator i=variant->members.find(string(key+offset));
		if(i==variant->members.end())
			return(false);
		else
		{
			retValue=const_cast<CVariant *>(&(i->second));
			return(true);
		}
	}
	else
	{ // dot found, then we must be asking for a nested scope
		map<string,CVariant>::const_iterator i=variant->members.find(string(key+offset,pos));
		if(i==variant->members.end())
			return(false);
		else
			return(prvGetValue(retValue,key,offset+pos+1,throwOnError,&i->second));
	}
}

#if 0
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
#endif

void CNestedDataFile::createKey(const char *key,const double value)
{
	CVariant newVariant("",value);
	prvCreateKey(key,0,newVariant,root);
}

void CNestedDataFile::createKey(const char *key,const string value)
{
	CVariant newVariant("",value);
	prvCreateKey(key,0,newVariant,root);
}

void CNestedDataFile::prvCreateKey(const char *key,int offset,CVariant &value,CVariant *variant)
{
	// look for a dot in the key
	int pos=strchr(key+offset,'.')-(key+offset);
	if(pos<0)
	{ // no dot found, then we must be now creating a value
		//printf("creating new value: %s\n",string(key+offset).c_str());
		value.name=string(key+offset);
		variant->members[string(key+offset)]=value;
	}
	else
	{ // dot found, then we must be asking for a nested scope

		if(variant->type!=vtScope)
			throw(runtime_error("CNestedDataFile::createKey -- '"+string(key)+"' resolves too soon to a value at: '"+string(key,offset+pos)+"' from file: "+filename));

		map<string,CVariant>::const_iterator i=variant->members.find(string(key+offset,pos));
		if(i==variant->members.end())
		{ // create a new scope
			//printf("creating new scope: %s\n",string(key+offset,pos).c_str());
			variant->members[string(key+offset,pos)]=CVariant(string(key+offset,pos));
			prvCreateKey(key,offset+pos+1,value,&variant->members[string(key+offset,pos)]);
		}
		else
		{
			//printf("while creating key, decending into: %s\n",string(key+offset,pos).c_str());
			prvCreateKey(key,offset+pos+1,value,const_cast<CVariant *>(&(i->second)));
		}
	}
}

void CNestedDataFile::writeFile(const string filename)
{
	// ??? do I wanna reassign this->filename ?
	FILE *f=fopen(filename.c_str(),"wt");
	if(f==NULL)
		throw(runtime_error("CNestedDataFile::writeFile -- error opening file for write: "+filename));
	try
	{
		prvWriteData(f,-1,root);
		fclose(f);
	}
	catch(...)
	{
		fclose(f);
		throw;
	}
}

void CNestedDataFile::prvWriteData(void *_f,int indent,const CVariant *variant)
{
	FILE *f=(FILE *)_f; // to avoid including stdio.h in the .h file

	for(int t=0;t<indent;t++)
		fprintf(f,"\t");

	string name=variant->name;

	// convert " " to "\ "
	for(size_t t=0;t<name.length();t++)
	{
		if(name[t]==' ')
		{
			name.insert(t,"\\");
			t++;
		}
	}

	switch(variant->type)
	{
	case vtString:
		fprintf(f,"%s=\"%s\";\n",name.c_str(),variant->stringValue.c_str());
		break;

	case vtFloat:
		// ??? I may want to do a better job of making sure I don't truncate any necessary percision on outputing the value
		fprintf(f,"%s=%f;\n",name.c_str(),variant->floatValue);
		break;

	case vtScope:
		if(indent>=0) // not root scope
		{
			fprintf(f,"%s\n",name.c_str());

			for(int t=0;t<indent;t++)
				fprintf(f,"\t");
			fprintf(f,"{\n");
		}

		for(map<string,CVariant>::const_iterator i=variant->members.begin();i!=variant->members.end();i++)
			prvWriteData(f,indent+1,&i->second);

		if(indent>=0) // not root scope
		{
			for(int t=0;t<indent;t++)
				fprintf(f,"\t");
			fprintf(f,"}\n");
		}
		break;

	default:
		throw(runtime_error("CNestedDataFile::prvWriteData -- internal error: unhandled type: "+istring(variant->type)+" from file: "+filename));
	}
}



// -----------------------------------------------------------------------------

CNestedDataFile::CVariant::CVariant() :
	type(vtInvalid)
{
}

CNestedDataFile::CVariant::CVariant(const string _name) :
	name(_name),
	type(vtScope)
{
}

CNestedDataFile::CVariant::CVariant(const string _name,const string value) :
	name(_name),
	type(vtString),
	stringValue(value)
{
}

CNestedDataFile::CVariant::CVariant(const string _name,const double value) :
	type(vtFloat),
	floatValue(value)
{
}

CNestedDataFile::CVariant::CVariant(const CVariant &src) :
	name(src.name),
	type(src.type),
	members(src.members),
	stringValue(src.stringValue),
	floatValue(src.floatValue)
{
}

const CNestedDataFile::CVariant &CNestedDataFile::CVariant::operator=(const CVariant &rhs)
{
	name=rhs.name;
	type=rhs.type;
	members=rhs.members;
	stringValue=rhs.stringValue;
	floatValue=rhs.floatValue;

	return(*this);
}

CNestedDataFile::CVariant::~CVariant()
{
}


