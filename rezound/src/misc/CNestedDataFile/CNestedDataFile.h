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

// define this if creating a multithreaded program
// the reason is, bison and flex create not thread-safe code, to a mutex is needed
//#define THREAD_SAFE_CSCOPE

#include <string>
#include <vector>
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
	CNestedDataFile(const string filename="",bool saveOnEachEdit=false);
	virtual ~CNestedDataFile();

	void clear();

	void parseFile(const string filename,bool clearExisting=true);
	void setFilename(const string filename);

	bool keyExists(const char *key) const;

	const string getValue(const char *key,bool throwIfNotExists=false) const;

	const string getArrayValue(const char *key,size_t index,bool throwIfNotExists=false) const;
	const size_t getArraySize(const char *key,bool throwIfNotExists=false) const;

	// will create the parents of key as necessary
	void createKey(const char *key,const double value);		// double
	void createKey(const char *key,const string &value);		// string
	void removeKey(const char *key,bool throwOnError=false);

	void createArrayKey(const char *key,size_t index,const string &value);	// string
	void removeArrayKey(const char *key,size_t index,bool throwOnError=false);

	// CAUTION: these collaps all arithmetic expressions to the evaluated value and throws away all comments from the original file
	void save() const;
	void writeFile(const string filename) const;

private:

	class CVariant;
	friend class CVariant;

	string filename;
	CVariant *root;
	bool saveOnEachEdit;

	enum VariantTypes
	{
		vtInvalid,
		vtScope,
		vtString,
		vtFloat,
		vtArray
	};

	class CVariant
	{
	public:	
		CVariant();
		CVariant(const string name);			// vtScope
			// ??? I think these name parameters are never used
		CVariant(const string name,const string value);	// vtString
		CVariant(const string name,const double value);	// vtFloat
		CVariant(const vector<CVariant> &value);	// vtArray
		CVariant(const CVariant &src);
		virtual ~CVariant();

		const CVariant &operator=(const CVariant &src);

		string name;

		// would use a union, but you can't have constructor-ed classes in a union (could use void *)
		VariantTypes type; // depending on this we use one of the following data-members
		map<string,CVariant> members; // I could be a bit more efficient if I were to use CVariant *'s, but this is a quick implementation right now
		string stringValue;
		double floatValue;
		vector<CVariant> arrayValue;
	};

	// I would have to implement this if I were to allow qualified idents in the input file which aren't always fully qualified... I would also need to have a parent * in CVariant to be able to implement this (unless I suppose I wanted to search more than I had to.. which I would do.. okay.. ya)
	//CVariant *upwardsScopeLookup(const char *key) const;

	// this could be a method of CVariant
	bool findVariantNode(CVariant *&retValue,const char *key,int offset,bool throwOnError,const CVariant *variant) const;


	void createKey(const char *key,const vector<CVariant> &value);	// array

	// this could be a method of CVariant
	void prvCreateKey(const char *key,int offset,CVariant &value,CVariant *variant);

	// this could be a method of CVariant
	void prvWriteData(void *f,int indent,const CVariant *variant) const;

	// used in cfg_parse
	static CNestedDataFile *parseTree;
	static const char *initialFilename;

	friend void checkForDupMember(int line,const char *key);
	friend int cfg_parse();
	friend void cfg_init();

	friend struct RKeyValue;
	friend union cfg_parse_union;

};

#endif
