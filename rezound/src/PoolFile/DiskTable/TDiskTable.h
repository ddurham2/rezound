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

#ifndef __TDiskTable_H__
#define __TDiskTable_H__

#include "../../../config/common.h"

#include <string.h>

#include <stdexcept>
#include <string>

#include <TPoolFile.h>
#include <TPoolAccesser.h>

/*
        This template class has two parameters, KeyType and ValueType.
        NOTE: KeyType MUST be a fixed size and should be memcpy-able
        KeyType must have the following methods defined
                operator==
                operator>
                operator<

        NOTE: ValueType MUST be a fixed size and should be memcpy-able

        The class stores its data into the CPoolFile given at construction.
        The data will be stored in a pool with the name given at construction.
*/

template <class KeyType,class ValueType> class TDiskTable
{
public:
        TDiskTable(TPoolFile<unsigned,unsigned> &poolFile,const string poolName);

        bool getValue(const KeyType &key,ValueType &value) const;
        void setValue(const KeyType &key,const ValueType &value);
        void removeValue(const KeyType &key);
        bool contains(const KeyType &key) const;

        size_t getSize() const;
        void get(size_t index,KeyType &key,ValueType &value) const;
        void remove(size_t index);

        void clear();

private:
	typedef TPoolFile<unsigned,unsigned> CPoolFile;
        typedef TPoolAccesser<KeyType,TPoolFile<unsigned,unsigned> > CKeyPoolAccesser;
        typedef TPoolAccesser<ValueType,TPoolFile<unsigned,unsigned> > CValuePoolAccesser;

        TPoolFile<unsigned,unsigned> &poolFile;
        const string poolName;
        int keyPoolID,valuePoolID;
        mutable size_t bottomFromContains;
        size_t size;

};


// -----------------------------------------------------------------------------


// maybe it'll get fixed one day
// 	- I think it's a bug... but perhaps it's a feature.. I don't know
// 	- if I catch word that it's a feature (compile with other compilers) I will remove this macro
#define STUPID_GCC_WORKAROUND poolFile.TPoolFile<unsigned,unsigned>::

template <class KeyType,class ValueType> TDiskTable<KeyType,ValueType>::TDiskTable(TPoolFile<unsigned,unsigned> &_poolFile,const string _poolName) :
	poolFile(_poolFile),
	poolName(_poolName)
{
	CKeyPoolAccesser keys=STUPID_GCC_WORKAROUND createPool<KeyType>(poolName+"_KEY",false);
	STUPID_GCC_WORKAROUND createPool<ValueType>(poolName+"_VALUE",false);

	keyPoolID=poolFile.getPoolIdByName(poolName+"_KEY");
	valuePoolID=poolFile.getPoolIdByName(poolName+"_VALUE");

	size=keys.getSize();
}

template <class KeyType,class ValueType> bool TDiskTable<KeyType,ValueType>::getValue(const KeyType &key,ValueType &value) const
{
	if(contains(key))
	{
		memcpy(value.data,CValuePoolAccesser(STUPID_GCC_WORKAROUND getPoolAccesser<ValueType>(valuePoolID))[bottomFromContains].data,sizeof(ValueType));
		return(true);
	}
	return(false);
}

template <class KeyType,class ValueType> void TDiskTable<KeyType,ValueType>::setValue(const KeyType &key,const ValueType &value)
{
	// insert in sorted order
	const bool has=contains(key);

	CValuePoolAccesser values=STUPID_GCC_WORKAROUND getPoolAccesser<ValueType>(valuePoolID);

	if(!has)
	{
		CKeyPoolAccesser keys=STUPID_GCC_WORKAROUND getPoolAccesser<KeyType>(keyPoolID);

		keys.insert(bottomFromContains,1);
		values.insert(bottomFromContains,1);

		memcpy(keys[bottomFromContains].data,key.data,sizeof(KeyType));
		size++;
	}

	memcpy(values[bottomFromContains].data,value.data,sizeof(ValueType));

	poolFile.flushData();
}

template <class KeyType,class ValueType> void TDiskTable<KeyType,ValueType>::removeValue(const KeyType &key)
{
	const bool has=contains(key);
	if(has)
		remove(bottomFromContains);
}

template <class KeyType,class ValueType> bool TDiskTable<KeyType,ValueType>::contains(const KeyType &key) const
{
	// go ahead bail out if item is bigger than the bottom or less than the top
	// this helps when the data is coming in in already sorted order
	const CKeyPoolAccesser keys=STUPID_GCC_WORKAROUND getPoolAccesser<KeyType>(keyPoolID);
	const size_t _itemsCount=keys.getSize();

	if(_itemsCount>0)
	{
		if(key>keys[_itemsCount-1])
		{
			bottomFromContains=_itemsCount;
			return(false);
		}
		else if(key<keys[0])
		{
			bottomFromContains=0;
			return(false);
		}

		size_t middle;
		size_t bottom=0;
		size_t top=_itemsCount-1;
		while(top>=bottom)
		{
			middle=(top+bottom)/2;
			if(key<keys[middle])
				top=middle-1;
			else if(key>keys[middle])
				bottom=middle+1;
			else if(key==keys[middle])
			{	// equal
				bottomFromContains=middle;
				return(true);
			}
		}
		bottomFromContains=bottom;
	}
	else
		bottomFromContains=0;

	return(false);
}

template <class KeyType,class ValueType> size_t TDiskTable<KeyType,ValueType>::getSize() const
{
	return(size);
}

template <class KeyType,class ValueType> void TDiskTable<KeyType,ValueType>::get(size_t index,KeyType &key,ValueType &value) const
{
	if(index<0 || index>=getSize())
		throw(runtime_error(string(__func__)+" -- index is out of range: "+istring(index)));

	memcpy(key.data,CKeyPoolAccesser(STUPID_GCC_WORKAROUND getPoolAccesser<KeyType>(keyPoolID))[index].data,sizeof(KeyType));
	memcpy(value.data,CValuePoolAccesser(STUPID_GCC_WORKAROUND getPoolAccesser<ValueType>(valuePoolID))[index].data,sizeof(ValueType));
}

template <class KeyType,class ValueType> void TDiskTable<KeyType,ValueType>::remove(size_t index)
{
	if(index<0 || index>=getSize())
		throw(runtime_error(string(__func__)+" -- index is out of range: "+istring(index)));

	CKeyPoolAccesser keys=STUPID_GCC_WORKAROUND getPoolAccesser<KeyType>(keyPoolID);
	CValuePoolAccesser values=STUPID_GCC_WORKAROUND getPoolAccesser<ValueType>(valuePoolID);

	keys.remove(index,1);
	values.remove(index,1);

	size--;
	poolFile.flushData();
}

template <class KeyType,class ValueType> void TDiskTable<KeyType,ValueType>::clear()
{
	CKeyPoolAccesser keys=STUPID_GCC_WORKAROUND getPoolAccesser<KeyType>(keyPoolID);
	CValuePoolAccesser values=STUPID_GCC_WORKAROUND getPoolAccesser<ValueType>(valuePoolID);
	keys.clear();
	values.clear();
	poolFile.flushData();
	size=0;
}

#endif
