/* 
 * Copyright (C) 2002 - David W. Durham
 * 
 * This file is part of ReZound, an audio editing application, but
 * could be used for other applications which could use the PoolFile
 * class's functionality.
 * 
 * PoolFile is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 * 
 * PoolFile is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */

#ifndef __CStringDiskTable_H__
#define __CStringDiskTable_H__

#include "../../../config/common.h"

#include "TDiskTable.h"

#include <stdexcept>
#include <string>

class CStringDiskTable
{
#define _CSDT_STRING_SIZE 1024
public:
	CStringDiskTable(TPoolFile<unsigned,unsigned> &poolFile,const string poolName) :
		diskTable(poolFile,poolName)
	{
	}

	const string getValue(const string &_key) const
	{
		if(_key.length()>=_CSDT_STRING_SIZE)
			throw(runtime_error(string(__func__)+" -- key value is too large: '"+_key+"'"));

		StringKey key;
		strcpy(key.data,_key.c_str());
		StringValue value;
	
		if(!diskTable.getValue(key,value))
			return("");
	
		return(string(value.data));
	}

	void setValue(const string &_key,const string &_value)
	{
		if(_key.length()>=_CSDT_STRING_SIZE)
			throw(runtime_error(string(__func__)+" -- key value is too large: '"+_key+"'"));
		if(_value.length()>=_CSDT_STRING_SIZE)
			throw(runtime_error(string(__func__)+" -- value is too large: '"+_value+"'"));

		StringKey key;
		strcpy(key.data,_key.c_str());
		StringValue value;
		strcpy(value.data,_value.c_str());
	
		diskTable.setValue(key,value);
	}

	void removeValue(const string &_key)
	{
		if(_key.length()>=_CSDT_STRING_SIZE)
			throw(runtime_error(string(__func__)+" -- key value is too large: '"+_key+"'"));

		StringKey key;
		strcpy(key.data,_key.c_str());
	
		diskTable.removeValue(key);
	}

	bool contains(const string &_key) const
	{
		if(_key.length()>=_CSDT_STRING_SIZE)
			throw(runtime_error(string(__func__)+" -- key value is too large: '"+_key+"'"));

		StringKey key;
		strcpy(key.data,_key.c_str());
	
		return(diskTable.contains(key));
	}


	size_t getSize() const
	{
		return(diskTable.getSize());
	}

	void get(size_t index,string &_key,string &_value) const
	{
		StringKey key;
		StringValue value;
	
		diskTable.get(index,key,value);
		_key=key.data;
		_value=value.data;
	}

	void remove(size_t index)
	{
		diskTable.remove(index);
	}

	void clear()
	{
		diskTable.clear();
	}


private:

	struct StringKey
	{
		char data[_CSDT_STRING_SIZE];

		bool operator==(const StringKey &rhs) const
		{
			return(strcmp(data,rhs.data)==0);
		}

		bool operator<(const StringKey &rhs) const
		{
			return(strcmp(data,rhs.data)<0);
		}

		bool operator>(const StringKey &rhs) const
		{
			return(strcmp(data,rhs.data)>0);
		}

	};

	struct StringValue
	{
		char data[_CSDT_STRING_SIZE];
	};


	TDiskTable<StringKey,StringValue> diskTable;

};

#endif
