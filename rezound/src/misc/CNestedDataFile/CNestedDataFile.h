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

#ifndef __CNestedDataFile_H__
#define __CNestedDataFile_H__

#include "../../../config/common.h"

#include <string>
#include <vector>
#include <map>
using namespace std; // maybe see about removing this?

#include <CMutex.h>

class CNestedDataFile
{
public:
	static const string delim; // the scope separator character so it can be changed easily
	#define DOT +(CNestedDataFile::delim)+

	// create a scope from this filename
	CNestedDataFile(const string filename="",bool saveOnEachEdit=false);
	CNestedDataFile(const CNestedDataFile &src);
	virtual ~CNestedDataFile();

	// This method can be used to set a CNestedDataFile object to check for a value when it doesn't exist in this object
	// It is only used by getValue(), keyExists() and getChildKeys, thus the alternate file is read-only
	// NULL can be passed to unset an alternate object
	void setAlternateReadFile(const CNestedDataFile *_alternate) { alternate=_alternate; }

	enum KeyTypes
	{
		ktNotExists=0,
		ktScope,
		ktValue
	};

	// if(keyExists(...)) will tell you if a key does exist, but the return
	// value actually tells you if the key is a value, a child scope
	KeyTypes keyExists(const string &key) const;



	template<class type> const type getValue(const string &key,bool throwIfNotExists=false) const;
	template<class type> void setValue(const string &key,const type value,bool throwIfExists=false); // will create the parents of key as necessary, overwrites any existing value
	void removeKey(const string &key,bool throwOnError=false);

	void clear();

	// just pass this "" if you want everything in the root scope
	// or "foo" for a list of all keys under the scope named "foo"
	const vector<string> getChildKeys(const string &parentKey,bool throwIfNotExists=false) const;

// these methods may need to be organized/documented better ???
// 	the filename datamember is annoying .. would be nice if I just had to always pass the filename when saving
// 	also, then could remove the filename from the constructor, and just require them to call parseFile()
// 	ack, but then saveOnEveryEdit can't work

	// CAUTION: these collaps all arithmetic expressions to the evaluated value and throws away all comments in the original file
	void save() const;
	void writeFile(const string filename) const;

	// creates this data struction from a string
	void parseString(const string str,bool clearExisting=true);

	// returns this data struction as a string
	const string asString() const;

	void parseFile(const string filename,bool clearExisting=true);
	void setFilename(const string filename);
	const string getFilename() const { return filename; }

	// write our files to the given CNestedDataFile under the given key
	void writeToFile(CNestedDataFile *f,const string key) const;

	// read our values from the given CNestedDataFile that are under the given key (but the scope names within 'key' will not be included in this object's struction)
	void readFromFile(const CNestedDataFile *f,const string key,bool clearExisting=true);

private:

	class CVariant
	{
	public:
		CVariant(const KeyTypes type=ktNotExists);
		CVariant(const string &value);			// sets keyType to ktValue
		CVariant(const CVariant &src);
		virtual ~CVariant();

		const CVariant &operator=(const CVariant &src);

		void writeToFile(CNestedDataFile *f,const string key) const;
		void readFromFile(const CNestedDataFile *f,const string key);

		void asString(string &acc,int indent,const string &name) const;

		KeyTypes type;			// depending on this we use one of the following data-members
		//union {   would, but can't have constructor-ed classes in a union
			map<string,CVariant> members;	// I could be a bit more efficient if I were to use CVariant *'s, but this is a quick implementation right now
			string stringValue;
		//} u;

	};

	class CVariant;
	friend class CVariant;

	string filename;
	CVariant *root;
	bool saveOnEachEdit;

	const CNestedDataFile *alternate;

	// I would have to implement this if I were to allow qualified idents in the input file which aren't always fully qualified... I would also need to have a parent * in CVariant to be able to implement this (unless I suppose I wanted to search more than Iad to.. which I would do.. okay.. ya)
	//CVariant *upwardsScopeLookup(const string key) const;

	// this could be a method of CVariant
	bool findVariantNode(CVariant *&retValue,const string &key,int offset,bool throwOnError,const CVariant *variant) const;

	// this could be a method of CVariant
	void prvCreateKey(const string &key,int offset,const CVariant &value,CVariant *variant);

	const vector<string> prvGetChildKeys(const string &parentKey,bool throwIfNotExists=false) const;

	void verifyKey(const string &key);

	// used in cfg_parse/cfg_init
	static CNestedDataFile *parseTree;		// what the yacc parser should put things into when parsing not in s2at mode
	static string initialFilename;			// the file that the yacc parser should parse when not in s2at mode
	static vector<string> s2at_return_value;	// what the yacc parser should put things into when parsing s2at mode (NULL when not s2at mode)
	static string s2at_string;			// what the yacc parser should parse when in s2at mode

	static CMutex cfg_parse_mutex;

	static const string stripLeadingDOTs(const string &key);

	friend void checkForDupMember(int line,const char *key);
	friend int cfg_parse();
	friend void cfg_init();
	friend void cfg_error(int line,const char *msg);
	template<class type> friend const vector<type> &string_to_anytype(const string &str,vector<type> &ret);

	friend union cfg_parse_union;
};

#include "anytype.h"
#include <istring>
template<class type> const type CNestedDataFile::getValue(const string &_key,bool throwIfNotExists) const
{
	const string key=stripLeadingDOTs(_key);
	CVariant *value;
	if(!findVariantNode(value,key,0,true,root))
	{
		if(alternate==NULL || /*check alternate file for the value -->*/!alternate->findVariantNode(value,key,0,true,alternate->root))
		{
			if(throwIfNotExists)
				throw runtime_error(string(__func__)+" -- key '"+key+"' does not exist from "+filename);
			else
			{
				type r;
				return string_to_anytype("",r);
			}
		}
	}

	switch(value->type)
	{
	case ktValue:
		{
		type r;
		return string_to_anytype(value->stringValue,r);
		}
	case ktScope:
		throw runtime_error(string(__func__)+" -- '"+key+"' resolves to a scope, not a value from "+filename);
	default:
		throw runtime_error(string(__func__)+" -- internal error: unhandled type: '"+istring(value->type)+"' from "+filename);
	}
}

template<class type> void CNestedDataFile::setValue(const string &_key,const type value,bool throwIfExists)
{
	if(throwIfExists && keyExists(_key))
		throw runtime_error(string(__func__)+" -- key already exists: "+_key);

	const string key=stripLeadingDOTs(_key);
	CVariant newVariant(anytype_to_string(value));
	prvCreateKey(key,0,newVariant,root);

	if(saveOnEachEdit)
		save();
}

#endif

