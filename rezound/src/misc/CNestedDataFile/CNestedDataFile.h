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

#ifndef __CNestedDataFile_H__
#define __CNestedDataFile_H__

// define this if creating a multithreaded program
// the reason is, bison and flex create not thread-safe code, to a mutex is needed
//#define THREAD_SAFE_CSCOPE

#include <string>
#include <map>

#ifdef THREAD_SAFE_CSCOPE
#error no no.. I need to convert this to use CommonC++ if I am to actually support threadsafe-ness
#include <CMutex.h>
#endif

extern int cfg_parse(void);
extern void cfg_init(void);

class CNestedDataFile
{
public:
	// create a scope from this filename
	CNestedDataFile(const string filename);
	virtual ~CNestedDataFile();

	const string getValue(const char *key) const;


private:

	class CVariant;
	friend class CVariant;

	CNestedDataFile(const CNestedDataFile &src);
	CNestedDataFile(const char *name,CNestedDataFile *parentScope);
	CNestedDataFile(CNestedDataFile *parentScope,const CNestedDataFile &src);

	bool prvGetValue(CVariant *&retValue,const char *key,int offset,bool throwOnError) const;
	CVariant *upwardsScopeLookup(const char *key) const;

	enum VariantTypes
	{
		vtInvalid,
		vtScope,
		vtString,
		vtFloat
	};
		
	/*
	 * Although Jack only wants all values as strings, internally
	 * the types are maintained in case someday we do want to enforce
	 * the type of a value by having to call getInt, getFloat, getString, etc
	 */
	class CVariant
	{
	public:	
		CVariant();
		CVariant(CNestedDataFile *value);
		CVariant(const string value);
		CVariant(float value);
		CVariant(const CVariant &src);
		CVariant(CNestedDataFile *parentScope,const CVariant &src);
		virtual ~CVariant();

		CVariant &operator=(const CVariant &rhs);

		// this ain't too efficient if say entire scenes were loaded with this
		VariantTypes type;
		CNestedDataFile *scopeValue;
		string stringValue;
		float floatValue;
	};

	const string name;
	CNestedDataFile *parentScope;
	map<string,CVariant *> values;

	#ifdef THREAD_SAFE_CSCOPE
	mutable CMutex mutex; // used to make cfg_parse thread-safe
	#endif

	// used in cfg_parse
	static CNestedDataFile *parseTree;
	static const char *initialFilename;

	friend void addScopeMember(int line,CNestedDataFile *scope,const char *key,CVariant *value);
	friend int cfg_parse();
	friend void cfg_init();

	friend struct RKeyValue;
	friend union cfg_parse_union;

};

#endif
