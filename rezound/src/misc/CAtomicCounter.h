/* 
 * Copyright (C) 2002 - David W. Durham
 * 
 * This file is part of ReZound, an audio editing application.
 * 
 * ReZound is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 * 
 * ReZound is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */
#ifndef __CAtomicCounter_H__
#define __CAtomicCounter_H__

#include "../../config/common.h"

#if defined(__linux) && defined(__i386__) && ((__GNUC__ * 100) + __GNUC_MINOR__) < 401

	// using linux's exposed atomic operations on Intel-32bit
	#define LINUX_ATOMIX
	#include <asm/atomic.h>

#elif defined(__GNUC__) && ((__GNUC__ * 100) + __GNUC_MINOR__) >= 405 && defined(__GXX_EXPERIMENTAL_CXX0X__)/*for now detect --std=c++0x.. will be removed later*/

	// gcc-4.5 uses the atomic header (c++v0)
	#define STL_ATOMIX
	#include <atomic>

#elif defined(__GNUC__) && ((__GNUC__ * 100) + __GNUC_MINOR__) >= 404 && defined(__GXX_EXPERIMENTAL_CXX0X__)/*for now detect --std=c++0x.. will be removed later*/

	// gcc-4.4 introduced atomic operations into the STL (c++v0)
	#define STL_ATOMIX
	#include <cstdatomic>
#else

	// could use this only and nix the above three implementations.. need look into the cost tho
	#define BOOST_ATOMIX
	#include <boost/detail/atomic_count.hpp>

#endif

class CAtomicCounter {
public:
	CAtomicCounter()
#if defined(BOOST_ATOMIX)
		: counter(0)
#endif
	{
#if defined(LINUX_ATOMIX)
		atomic_set(&counter, 0);
#elif defined(STL_ATOMIX) || defined(MUTEX_ATOMIX)
		counter = 0;
#elif defined(BOOST_ATOMIX)
		// handled in ctor init list
#else
		#error unimplemented
#endif
	}

	int inc() {
#if defined(LINUX_ATOMIX)
		return atomic_inc_return(&counter);
#elif defined(STL_ATOMIX) || defined(BOOST_ATOMIX)
		return ++counter;
#elif defined(MUTEX_ATOMIX)
		CMutexLocker ml(mMutex);
		return ++counter;
#else	
		#error unimplemented
#endif
	}

	int dec() {
#if defined(LINUX_ATOMIX)
		return atomic_dec_return(&counter);
#elif defined(STL_ATOMIX) || defined(BOOST_ATOMIX)
		return --counter;
#elif defined(MUTEX_ATOMIX)
		CMutexLocker ml(mMutex);
		return --counter;
#else	
		#error unimplemented
#endif
	}

	int value() const {
#if defined(LINUX_ATOMIX)
		return atomic_read(&counter);
#elif defined(STL_ATOMIX) || defined(BOOST_ATOMIX) || defined(MUTEX_ATOMIX)
		return (int)counter;
#endif
	}

private:
#if defined(LINUX_ATOMIX)
	atomic_t counter;
#elif defined(STL_ATOMIX)
	std::atomic<int> counter;
#elif defined(BOOST_ATOMIX)
	boost::detail::atomic_count counter;
#elif defined(MUTEX_ATOMIX)
	CMutex mMutex;
	volatile int counter;
#else
	#error unimplemented
#endif
	
};

#endif
