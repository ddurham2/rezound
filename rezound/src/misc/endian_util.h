/* 
 * Copyright (C) 2003 - David W. Durham
 * 
 * This file is not part of any particular application.
 * 
 * endian_util.h is free software; you can redistribute it and/or modify
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

#ifndef __endian_util_H__
#define __endian_util_H__

/*
 * This header files given functionality to convenient change endian-ness of values
 * and through the use of very few function names regardless of the type.
 *
 * To use this header file, WORDS_BIGENDIAN must be #defined if the platform is a 
 * big-endian platform and NOT defined if it's a little-endian platform.
 *
 * Though there is only one function name with the use of C++ templates the compiler 
 * automatically chooses the implementation depending on the size of the type of the 
 * value given.  It should even be free to call the functions on say a 1 byte value 
 * that doesn't need conversion just to be complete in the code.
 *
 * Having one function name for all types makes mantenence easier since for instance: 
 * changing the type size of a variable won't break the endian conversion when someone 
 * forgets to look further down the code at the specific uses of that variable.
 *
 * As a word of advice, whenever writing data to stable storage or to a network, use
 * the types defined in the now standard header file, stdint.h   It defines fixed 
 * sized typenames so that you don't have to assume the sizes of int, long, short, etc 
 *
 * float and doubles should be implemented as the same size on any ANSI C compiler that
 * has them as types.  They are supposed to follow  IEEE standards.
 *
 *
 *
 * 4 macros are defined to do endian conversion
 *
 *   lethe - given a little-endian value returns the same value in the host endian
 *   lethe - given a pointer to a little-endian value modifies the value to be the host endian
 *
 *   hetle - reverse of lethe
 *
 *   bethe - given a big-endian value returns the same value in the host endian
 *   bethe - given a pointer to a big-endian value modifies the value to be the host endian
 *
 *   hetbe - reverse of bethe
 *
 * These macros both call on swap_endian() which does the work.  This function will swap the
 * endian of the given value, or swap the endian of the value at the given pointer.  Again,
 * the parameter type determines the correct implmenation.
 *
 */


/* 'lethe' means little-endian to host-endian */
/* 'hetle' means host-endian to little-endian */
#ifdef WORDS_BIGENDIAN
	#define lethe(value) swap_endian(value)
	#define hetle(value) swap_endian(value)
#else // assuming now a little-endian platform
	#define lethe(value) (value)
	#define hetle(value) (value)
#endif

/* 'bethe' means big-endian to host-endian */
/* 'hetbe' means host-endian to big-endian */
#ifdef WORDS_BIGENDIAN
	#define bethe(value) (value)
	#define hetbe(value) (value)
#else // assuming now a little-endian platform
	#define bethe(value) swap_endian(value)
	#define hetbe(value) swap_endian(value)
#endif

#ifndef __cplusplus
	#error this header file requires a C++ compiler
#endif





#include <stdint.h>
namespace endian_util
{
	// --- generic implementation
	template<unsigned Size> inline static void really_swap_endian(void *_value,unsigned size)
	{
		uint8_t * const value=(uint8_t *)_value;
		register int k=(size+1)/2;
		for(register int t=(size/2)-1;t>=0;t--,k++)
		{
			const uint8_t temp=value[t];
			value[k]=value[t];
			value[k]=temp;
		}
	}

	// --- implementation for 1 byte quantities (nothing)
	template<> inline static void really_swap_endian<1>(void *value,unsigned size)
	{
		// nothing to do
	}

	// --- implementation for 2 byte quantities
	template<> inline static void really_swap_endian<2>(void *value,unsigned size)
	{
		const register uint16_t v=((uint16_t *)value)[0];
		((uint16_t *)value)[0]=
		// of 2, swap upper and lower 8 octets
		   ((v>>8)&0x00ff) | ((v<<8)&0xff00);
		
	}

	// --- implementation for 4 byte quantities
	template<> inline static void really_swap_endian<4>(void *value,unsigned size)
	{
		const register uint32_t v=((uint32_t *)value)[0];
		((uint32_t *)value)[0]=
		// of 4, swap upper most and lower most octets then swap two middle octets
		   ((v>>24)&0x000000ff) | ((v<<24)&0xff000000) | 
		   ((v>> 8)&0x0000ff00) | ((v<< 8)&0x00ff0000);
	}

	// --- implementation for 8 byte quantities
	template<> inline static void really_swap_endian<8>(void *value,unsigned size)
	{
		const register uint64_t v=((uint64_t *)value)[0];
		// of 8, swap upper most and lower most octets then the next two inward, and so on ..
		((uint64_t *)value)[0]=
		   ((v>>56)&0x00000000000000ff) | ((v<<56)&0xff00000000000000) | 
		   ((v>>40)&0x000000000000ff00) | ((v<<40)&0x00ff000000000000) |
		   ((v>>24)&0x0000000000ff0000) | ((v<<24)&0x0000ff0000000000) |
		   ((v>> 8)&0x00000000ff000000) | ((v<< 8)&0x000000ff00000000);
	}

}; // namespace endian_util


/* 
 * These two are implemented so that if you pass it a pointer, it modifies the data 
 * you're pointing to but if you pass it a value, then it returns the modified value 
 */

template<typename Type> inline static const Type swap_endian(Type value)
{
	endian_util::really_swap_endian<sizeof(Type)>(&value,sizeof(Type));
	return value;
}

template<typename Type> inline static void swap_endian(Type *value)
{
	endian_util::really_swap_endian<sizeof(Type)>(value,sizeof(Type));
}

#endif
