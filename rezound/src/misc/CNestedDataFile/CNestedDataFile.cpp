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

#warning parseFile doesnt need to set the filename, only the constructor and setFilename should do that
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
#include <CPath.h>


extern int cfg_parse();

CNestedDataFile *CNestedDataFile::parseTree;
string CNestedDataFile::initialFilename;
vector<string> CNestedDataFile::s2at_return_value;
string CNestedDataFile::s2at_string;

CMutex CNestedDataFile::cfg_parse_mutex;

static const string escapeIdent(const string &_name)
{
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
	return name;
}


const string CNestedDataFile::delim="|"; // !!!NOTE!!! Change the qualified_ident rule in cfg.y also!!!

const string CNestedDataFile::stripLeadingDOTs(const string &key)
{
	// look until we don't see a DOT
	size_t t=0;
	while(t<key.length() && key[t]==delim[0])
		t++;

	// return everything past where we found leading DOTs
	return key.substr(t);
}

CNestedDataFile::CNestedDataFile(const string _filename,bool _saveOnEachEdit) :
	root(NULL),
	saveOnEachEdit(_saveOnEachEdit),
	alternate(NULL)
{
	clear();
	if(_filename!="")
		parseFile(_filename);
}

CNestedDataFile::CNestedDataFile(const CNestedDataFile &src) :
	filename(src.filename),
	root(new CVariant(*(src.root))),
	saveOnEachEdit(src.saveOnEachEdit),
	alternate(src.alternate)
{
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

void CNestedDataFile::parseString(const string str,bool clearExisting)
{
	if(clearExisting)
		clear();

	try
	{
		CMutexLocker l(cfg_parse_mutex); // because bison/flex aren't thread-safe

		initialFilename="";	// make sure the lexer is not in read-file mode
		s2at_string=str;
		parseTree=this;

		if(cfg_parse())
			throw runtime_error(string(__func__)+" -- error while parsing string");
	}
	catch(...)
	{
		if(clearExisting)
			clear();

		throw;
	}
}

const string CNestedDataFile::asString() const
{
	string acc;
	root->asString(acc,-1,"");
	return acc;
}

void CNestedDataFile::parseFile(const string _filename,bool clearExisting)
{
	if(clearExisting)
		clear();

	try
	{
		CMutexLocker l(cfg_parse_mutex); // because bison/flex aren't thread-safe
		filename=_filename;

		if(CPath(filename).exists())
		{

			initialFilename=_filename;
			s2at_string="";		 // make sure the lexer is not in s2at mode
			parseTree=this;

			if(cfg_parse())
				throw runtime_error(string(__func__)+" -- error while parsing "+_filename);
		}
		else
		{
#warning make this a flag .. otherwise throw an exception
			// create an empty file
			CPath(filename).touch();
		}
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

CNestedDataFile::KeyTypes CNestedDataFile::keyExists(const string &_key) const
{
	const string key=stripLeadingDOTs(_key);
	CVariant *value;
	if(findVariantNode(value,key,0,false,root))
		return value->type;
	else
	{
		if(alternate && /*search alternate too -->*/alternate->findVariantNode(value,key,0,false,alternate->root))
			return value->type;
		else
			return ktNotExists;
	}
}

const vector<string> CNestedDataFile::getChildKeys(const string &parentKey,bool throwIfNotExists) const
{
	if(alternate==NULL)
		return prvGetChildKeys(parentKey,throwIfNotExists);

	if(throwIfNotExists)
	{
		// if the key is a value in this file OR the key doesn't exist in this file, but resolves to a value in the alternate file, then throw the exception
		if(keyExists(parentKey)==ktValue || (keyExists(parentKey)==ktNotExists && alternate->keyExists(parentKey)!=ktScope))
			throw runtime_error(string(__func__)+" -- parent key '"+parentKey+"' did not resolve to a scope from "+filename);
	}

	// have to merge the values in the two files when both are scopes (or either one doesn't exist)
	vector<string> keys1=prvGetChildKeys(parentKey,false);
	sort(keys1.begin(),keys1.end());
	vector<string> keys2=alternate->prvGetChildKeys(parentKey,false);
	sort(keys2.begin(),keys2.end());

	vector<string> keys3(keys1.size() + keys2.size());
	set_union(keys1.begin(),keys1.end(), keys2.begin(),keys2.begin(), keys3.begin());

	return keys3;
}

const vector<string> CNestedDataFile::prvGetChildKeys(const string &_parentKey,bool throwIfNotExists) const
{
	const string parentKey=stripLeadingDOTs(_parentKey);
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
				throw runtime_error(string(__func__)+" -- parent key '"+parentKey+"' did not resolve to a scope from "+filename);
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

void CNestedDataFile::removeKey(const string &_key,bool throwOnError)
{
	const string key=stripLeadingDOTs(_key);
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

	string acc;
	root->asString(acc,-1,"");

	CMutexLocker l(cfg_parse_mutex);  // just to keep two threads from writing the file at the same time

	FILE *f=fopen(filename.c_str(),"wt");
	if(f==NULL)
		throw runtime_error(string(__func__)+" -- error opening file for write: "+filename);

	fprintf(f,"// ReZound program generated data; be careful if modifying.  Modify ONLY when ReZound is NOT running.  Consider making a backup before modifying!\n\n");

	size_t ret;
	if((ret=fwrite(acc.data(),1,acc.length(),f))!=acc.length())
	{
		fclose(f);
		throw runtime_error(string(__func__)+" -- fwrite did not write everything: "+istring(ret)+"!="+istring(acc.length()));
	}
	fclose(f);
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

void CNestedDataFile::writeToFile(CNestedDataFile *f,const string key) const
{
	root->writeToFile(f,key);
}

void CNestedDataFile::readFromFile(const CNestedDataFile *f,const string key,bool clearExisting)
{
	if(clearExisting)
		clear();
	root->readFromFile(f,key);
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

CNestedDataFile::CVariant::~CVariant()
{
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

void CNestedDataFile::CVariant::writeToFile(CNestedDataFile *f,const string key) const
{
	// make sure the scope exists (create a value in it)
	f->setValue<int>(key DOT "__bogus__",0);

	// find the scope (that already existed or we just created)
	CVariant *v;
	if(!f->findVariantNode(v,key,0,false,f->root))
		throw runtime_error(string(__func__)+" -- internal error");

	// merge our contents into theirs
	v->type=type;
	v->stringValue=stringValue;
	for(map<string,CVariant>::const_iterator i=members.begin(); i!=members.end(); i++)
		v->members[i->first]=i->second;
	
	// remove the value we created to make sure key existed
	f->removeKey(key DOT "__bogus__");
}

void CNestedDataFile::CVariant::readFromFile(const CNestedDataFile *f,const string key)
{
	// find the scope within f
	CVariant *v;
	if(!f->findVariantNode(v,key,0,false,f->root))
		return;

	// merge their contents with ours
	type=v->type;
	stringValue=v->stringValue;
	for(map<string,CVariant>::const_iterator i=v->members.begin(); i!=v->members.end(); i++)
		members[i->first]=i->second;
}

void CNestedDataFile::CVariant::asString(string &acc,int indent,const string &_name) const
{
	for(int t=0;t<indent;t++)
		acc+='\t';

	string name=escapeIdent(_name);

	switch(type)
	{
	case ktValue:
		acc+=name+"="+stringValue+";\n";
		break;

	case ktScope:
	{
		if(indent>=0) // not root scope
		{
			acc+=name+"\n";

			for(int t=0;t<indent;t++)
				acc+='\t';
			acc+="{\n";
		}

		// write non scopes first (just to look nicer)
		bool more=false;
		for(map<string,CVariant>::const_iterator i=members.begin();i!=members.end();i++)
		{
			if(i->second.type!=ktScope)
				i->second.asString(acc,indent+1,i->first);
			else
				more=true;
		}
		if(more) // there will be scopes written
			acc+='\n';

		// write scopes after non-scopes
		for(map<string,CVariant>::const_iterator i=members.begin();i!=members.end();i++)
		{
			if(i->second.type==ktScope)
				i->second.asString(acc,indent+1,i->first);
		}

		if(indent>=0) // not root scope
		{
			for(int t=0;t<indent;t++)
				acc+='\t';
			acc+="}\n";
			if(indent==0)
				acc+='\n';
		}
		break;
	}

	default:
		throw runtime_error(string(__func__)+" -- internal error: unhandled type: "+istring(type));
	}
}

