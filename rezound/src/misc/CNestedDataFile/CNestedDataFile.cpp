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

#warning see about retaining the order that things were parsed in the file
	// if I did this, I could remove the 'names' array in presets.dat because I could call getChildKeys to know the preset's names and they would remain in the order they were parsed
	

/*
 * I could have used XML, to store this sort of information, but it's way too bulky for me.
 * So, here's my more simplistic solution.
 * -- Davy
 *
 * I could store a typeid in CVariant and enforce strict types, except then I wouldn't know what
 * it was exactly when I parsed it (i.e. is '25' a short or int?)
 *  not can you copy construct a type_info per ISO/C++
 */

#include "CNestedDataFile.h"

#include <stdio.h>
#include <ctype.h>

#include <stdexcept>
#include <algorithm>

#include <istring>


extern int cfg_parse();

CNestedDataFile *CNestedDataFile::parseTree;
string CNestedDataFile::initialFilename;
vector<string> CNestedDataFile::s2at_return_value;
string CNestedDataFile::s2at_string;

CMutex CNestedDataFile::cfg_parse_mutex;


const string CNestedDataFile::delim="|"; // !!!NOTE!!! Change the qualified_ident rule in cfg.y also!!!

CNestedDataFile::CNestedDataFile(const string _filename,bool _saveOnEachEdit) :
	root(NULL),
	saveOnEachEdit(_saveOnEachEdit)
{
	clear();
	if(_filename!="")
		parseFile(_filename);
}

CNestedDataFile::~CNestedDataFile()
{
	delete root;
}

void CNestedDataFile::clear()
{
	delete root;
	root=new CVariant(ktScope);
}

void CNestedDataFile::parseFile(const string _filename,bool clearExisting)
{
	if(clearExisting)
		clear();

	try
	{
		CMutexLocker l(cfg_parse_mutex); // because bison/flex aren't thread-safe

		initialFilename=_filename;
		s2at_string="";		 // make sure the yacc parser not in s2at mode
		filename=_filename;
		parseTree=this;

		if(cfg_parse())
			throw runtime_error(string(__func__)+" -- error while parsing "+_filename);
	}
	catch(...)
	{
		if(clearExisting)
			clear();

		throw;
	}
}

void CNestedDataFile::setFilename(const string _filename)
{
	filename=_filename;
}


//template<class type> const type CNestedDataFile::getValue(const string &key,bool throwIfNotExists) const
//	in .h file

CNestedDataFile::KeyTypes CNestedDataFile::keyExists(const string &key) const
{
	CVariant *value;
	if(findVariantNode(value,key,0,false,root))
		return value->type;
	else
		return ktNotExists;
}

const vector<string> CNestedDataFile::getChildKeys(const string &parentKey,bool throwIfNotExists) const
{
	vector<string> childKeys;
	CVariant *scope;

	if(parentKey=="")
		scope=root;
	else
	{
		if(!findVariantNode(scope,parentKey,0,true,root))
		{
			if(throwIfNotExists)
				throw runtime_error(string(__func__)+" -- parent key '"+parentKey+"' does not exist from "+filename);
			else
				return childKeys;
		}

		if(scope->type!=ktScope)
		{
			if(throwIfNotExists) // it DID actually exist, but it wasn't a scope containing more child values
				throw runtime_error(string(__func__)+" -- parent key '"+parentKey+"' resolved to a value from "+filename);
			else
				return childKeys;

		}
	}

	for(map<string,CVariant>::const_iterator i=scope->members.begin();i!=scope->members.end();i++)
		childKeys.push_back(i->first);

	return childKeys;
}

bool CNestedDataFile::findVariantNode(CVariant *&retValue,const string &key,int offset,bool throwOnError,const CVariant *variant) const
{
	if(variant->type!=ktScope)
	{
		if(throwOnError)
			throw runtime_error(string(__func__)+" -- '"+key+"' resolves too soon to a value at: '"+string(key,max(0,offset-(int)delim.length()))+"' from "+filename);
		else
			return false;
	}

	// look for the first dot in the key
	size_t pos=key.find(delim,offset);
	if(pos==string::npos)
	{ // no dot found, then we're now asking for a member of the current scope
		map<string,CVariant>::const_iterator i=variant->members.find(key.substr(offset));
		if(i==variant->members.end())
			return false;
		else
		{
			retValue=const_cast<CVariant *>(&(i->second));
			return true;
		}
	}
	else
	{ // dot found, then we must be asking for a nested scope
		const string subKey=key.substr(offset,pos-offset);
		map<string,CVariant>::const_iterator i=variant->members.find(subKey);
		if(i==variant->members.end())
			return false;
		else
			return findVariantNode(retValue,key,pos+delim.length(),throwOnError,&i->second);
	}
}

//template<class type> void CNestedDataFile::createKey(const string &key,const type value)
//	in .h file

void CNestedDataFile::prvCreateKey(const string &key,int offset,const CVariant &value,CVariant *variant)
{
	if(variant==root) // only verify once
		verifyKey(key);

	// look for a dot in the key
	size_t pos=key.find(delim,offset);
	if(pos==string::npos)
	{ // no dot found, then we're now creating a value
		variant->members[key.substr(offset)]=value;
	}
	else
	{ // dot found, then we're now asking for a nested scope
		if(variant->type!=ktScope)
			throw runtime_error(string(__func__)+" -- '"+key+"' resolves too soon to a value at: '"+string(key,offset+pos)+"' from "+filename);

		const string subKey=key.substr(offset,pos-offset);
		map<string,CVariant>::const_iterator i=variant->members.find(subKey);
		if(i==variant->members.end())
		{ // create a new scope
			variant->members[subKey]=CVariant(ktScope);
			prvCreateKey(key,pos+delim.length(),value,&variant->members[subKey]);
		}
		else
		{
			prvCreateKey(key,pos+delim.length(),value,const_cast<CVariant *>(&(i->second)));
		}
	}
}

void CNestedDataFile::removeKey(const string &key,bool throwOnError)
{
	CVariant *parent=NULL;;

	string parentKey;
	string childKey;
	size_t lastDot=key.rfind(delim);
	if(lastDot==string::npos)
	{
		parent=root;
		childKey=key;
	}
	else
	{
		parentKey=key.substr(0,lastDot);
		childKey=key.substr(lastDot+delim.length());
	}

	if(parent==root || findVariantNode(parent,parentKey,0,throwOnError,root))
	{
		if(parent->members.find(childKey)==parent->members.end())
		{
			if(throwOnError)
				throw runtime_error(string(__func__)+" -- '"+key+"' key not found from "+filename);
			return;
		}
		parent->members.erase(childKey);
	}

	if(saveOnEachEdit)
		save();
}

void CNestedDataFile::save() const
{
	writeFile(filename);
}

void CNestedDataFile::writeFile(const string filename) const
{
	if(filename=="")
		return;

	CMutexLocker l(cfg_parse_mutex);  // just to keep two threads from writing the file at the same time

	FILE *f=fopen(filename.c_str(),"wt");
	if(f==NULL)
		throw runtime_error(string(__func__)+" -- error opening file for write: "+filename);

	fprintf(f,"// ReZound program generated data; be careful if modifying.  Modify ONLY when ReZound is NOT running.  Consider making a backup before modifying!\n\n");

	try
	{
		prvWriteData(f,-1,"",root);
		fclose(f);
	}
	catch(...)
	{
		fclose(f);
		throw;
	}
}

/* translate \ to \\ and " to \" in the given filename */
static const string fixEscapes(const string _s)
{
	string s=_s;
	for(size_t t=0;t<s.size();t++)
	{
		if(s[t]=='\\' || s[t]=='"')
		{
			s.insert(t,"\\");
			t++;
		}
	}
	return s;
}

void CNestedDataFile::prvWriteData(void *_f,int indent,const string &_name,const CVariant *variant) const
{
	FILE *f=(FILE *)_f; // to avoid including stdio.h in the .h file

	for(int t=0;t<indent;t++)
		fprintf(f,"\t");

	string name=_name; // make copy to be able to escape certain chars

	// escape non-alnum to '\'non-alnum (and also don't make '_' into '\_')
	// and escape the first char if it's a digit
	for(size_t t=0;t<name.length();t++)
	{
		const char c=name[t];
		if( 	
			// corrisponding with the RE for IDENT in cfg.l, escape non-[_0-9A-Za-z]
			(!(c=='_' || (c>='a' && c<='z') || (c>='A' && c<='Z') || isdigit(c)))
			||
			// OR escape first char if it's a 0-9 (so it won't match the LIT_NUMBER ruler in the lexer)
			(t==0 && isdigit(c))
		)
		{
			name.insert(t,"\\");
			t++;
		}
	}

	switch(variant->type)
	{
	case ktValue:
		fprintf(f,"%s=%s;\n",name.c_str(),variant->stringValue.c_str());
		break;

	case ktScope:
	{
		if(indent>=0) // not root scope
		{
			fprintf(f,"%s\n",name.c_str());

			for(int t=0;t<indent;t++)
				fprintf(f,"\t");
			fprintf(f,"{\n");
		}

		// write non scopes first (just to look nicer)
		bool more=false;
		for(map<string,CVariant>::const_iterator i=variant->members.begin();i!=variant->members.end();i++)
		{
			if(i->second.type!=ktScope)
				prvWriteData(f,indent+1,i->first,&i->second);
			else
				more=true;
		}
		if(more) // there will be scopes written
			fprintf(f,"\n");

		// write scopes after non-scopes
		for(map<string,CVariant>::const_iterator i=variant->members.begin();i!=variant->members.end();i++)
		{
			if(i->second.type==ktScope)
				prvWriteData(f,indent+1,i->first,&i->second);
		}

		if(indent>=0) // not root scope
		{
			for(int t=0;t<indent;t++)
				fprintf(f,"\t");
			fprintf(f,"}\n");
			if(indent==0)
				fprintf(f,"\n");
		}
		break;
	}

	default:
		throw runtime_error(string(__func__)+" -- internal error: unhandled type: "+istring(variant->type)+" from "+filename);
	}
}

/*
 * This method makes sure that when creating a key in the file that it
 * does not contain invalid characters that would cause a parse error
 * if the file were parsed again with this supposed invalid key.
 */
void CNestedDataFile::verifyKey(const string &key)
{
	const size_t l=key.length();
	if(l==0)
		throw runtime_error(string(__func__)+" -- key is blank");

	for(size_t t=0;t<l;t++)
	{
		// accept only graphic characters (i.e. not control chars), space and tab
		// 	??? will using isgraph cause problems when locales changed between runs (someone saved a key in one language with chars that aren't graphic in another language
		if(!(isgraph(key[t]) || key[t]==' ' || key[t]=='\t'))
			throw runtime_error(string(__func__)+" -- invalid character in key: '"+key+"' at position: "+istring(t)+" while creating key in "+filename);
	}
}




// -----------------------------------------------------------------------------

CNestedDataFile::CVariant::CVariant(KeyTypes _keyType) :
	type(_keyType)
{
}

CNestedDataFile::CVariant::CVariant(const string &value) :
	type(ktValue),
	stringValue(value)
{
}

CNestedDataFile::CVariant::CVariant(const CVariant &src) :
	type(ktNotExists)
{
	operator=(src);
}

const CNestedDataFile::CVariant &CNestedDataFile::CVariant::operator=(const CVariant &rhs)
{
	if(this!=&rhs)
	{
		type=rhs.type;

		if(type==ktScope)
			members=rhs.members;
		else if(type==ktValue)
			stringValue=rhs.stringValue;
	}

	return *this;
}

CNestedDataFile::CVariant::~CVariant()
{
}

