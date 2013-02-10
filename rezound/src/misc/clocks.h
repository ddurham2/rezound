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
#ifndef __misc_clocks_H__
#define __misc_clocks_H__
#pragma once

#include <stdint.h>
//#include <limits>

#include "../../config/common.h"

namespace clocks {

typedef int64_t  sec_t;
typedef int64_t msec_t;
typedef int64_t usec_t;

//                                              avoiding #include <limits> conflicts with win32 headers
const sec_t max_sec =   0x7fffffffffffffffll; //std::numeric_limits<sec_t>::max();
const msec_t max_msec = 0x7fffffffffffffffll; //std::numeric_limits<msec_t>::max();
const usec_t max_usec = 0x7fffffffffffffffll; //std::numeric_limits<usec_t>::max();

// --- returns system-set calendar time since 00:00:00 UTC, January 1, 1970
//
//     The return value is subject to change based on the system clock setting.  Hence, it can
//     "go backward" if the system time is set back in the past or a clock correction is made.
//
//     time_sec() is essentially equivalent to the posix function call, time(NULL)
//
 sec_t time_sec();
msec_t time_msec();
usec_t time_usec();


// --- returns a steadily non-decreasing value relative to some arbirary but fixed point in time
//
//     The value can never "go backward" during a process, but it may be reset each time the 
//     process is started.
//
 sec_t fixed_sec();
msec_t fixed_msec();
usec_t fixed_usec();


#if defined(__linux) || defined(__APPLE__)
// --- returns the CPU time of the current process, that is, the amount of time that the process
//     has been scheduled to run on the CPU.
// TODO find out if this counts twice as fast if two threads are scheduled simultanous on a multiprocessor machine and make sure it behaves the same on all platforms
//      Also, implement a thread-specific version (thread_cpu_xxx()) if we can.. this function is helpful for unit-tests that test performance.
//
 sec_t cpu_sec();
msec_t cpu_msec();
usec_t cpu_usec();

#endif


enum Sources {
	sTime = 1,
	sFixed = 2,
#if defined(__linux) || defined(__APPLE__)
	sCPU = 3,
#endif
};

// --- can be used to get the value of one of the supported clocks given a source identifier
 sec_t source_sec(Sources source);
msec_t source_msec(Sources source);
usec_t source_usec(Sources source);

// --- can be used to convert from one time source to another time source
//     Do not use with sCPU.
//     These functions suffer from inaccuracies due to internal function ordering.
 sec_t convert_sec(Sources from,  sec_t time, Sources to);
msec_t convert_msec(Sources from, msec_t time, Sources to);
usec_t convert_usec(Sources from, usec_t time, Sources to);


// --- sleeps for the specific number of seconds + milliseconds
//
//     It returns false if the sleep wakes up early
//
bool sleep(sec_t sec, msec_t msec = 0);



// --- returns the timezone offset for this system [-12, +12]
//     DST does not have any affect on the return value
int get_timezone_offset();


} // namespace clocks

#endif
