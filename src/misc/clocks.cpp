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
#include "clocks.h"
#include <stdio.h>

#if defined(_WIN32)

#include <windows.h>
#include <mmsystem.h> // timeGetTime
#include <tchar.h>
#include <climits> // CHAR_BIT
#include "CMutex.h" // for protecting fixed_msec fallback

namespace clocks {

sec_t time_sec()
{
	return static_cast<sec_t>(time_msec() / 1000);
}

msec_t time_msec()
{
	union bigTime_t {
		FILETIME fTime; 
		ULONGLONG int64time; 
	} bigTime;

	GetSystemTimeAsFileTime( &bigTime.fTime );

	// Subtract the value for 1970-01-01 00:00 (UTC)
	bigTime.int64time -= 0x19db1ded53e8000; 

	// Convert to milliseconds 
	bigTime.int64time /= 10000; 

	return bigTime.int64time; 
}

usec_t time_usec()
{
	// Windows currently has no greater precision than 1ms
	return (time_msec() * 1000);
}

sec_t fixed_sec()
{
	return static_cast<sec_t>(fixed_msec() / 1000);
}

msec_t fixed_msec()
{
	return GetTickCount64();
}

usec_t fixed_usec()
{
	// Windows currently has no greater precision than 1ms
	return (fixed_msec() * 1000);
}

bool sleep(const sec_t sec, const msec_t msec)
{
	Sleep(static_cast<DWORD>(sec * 1000 + msec));
	return true;
}

int get_timezone_offset()
{
	TIME_ZONE_INFORMATION tz;
	DWORD result = GetTimeZoneInformation(&tz);

	// convert minutes to seconds
	int ret = (tz.Bias + tz.StandardBias) * 60;

	// negate for our purposes
	ret = -ret;

	// "clamp-and-shift" to convert values outside the appropriate
	// range to their correct locations within the appropriate range.
	// e.g. -14 hours really belongs at +10 hours
	// e.g. +13 hours really belongs at -11 hours
	if(ret < (-12*3600)) {
		ret = (ret % (12*3600)) + (12*3600);
	} else if(ret > (12*3600)) {
		ret = (ret % (12*3600)) - (12*3600);
	}

	return ret;
}

} // namespace clocks

#else

#include <time.h>
#include <sys/times.h>
#include <unistd.h>

#if TARGET_OS_IPHONE
#import "CAHostTimeBase.h"
#elif defined(__APPLE__)
#include <sys/time.h>
#include <mach/mach_time.h>
#include <CoreServices/CoreServices.h>
#include <Foundation/Foundation.h>
#endif

namespace clocks {

static const long gClockTicks = sysconf(_SC_CLK_TCK);

sec_t time_sec()
{
#if TARGET_OS_IPHONE
	NSDate *d = [NSDate date];
	NSTimeInterval t = [d timeIntervalSince1970];
	return t;
#elif defined(__APPLE__)
	struct timeval t;
	if(gettimeofday(&t,NULL) == 0) {
		return (sec_t)t.tv_sec;
	}
#else
	struct timespec t;
	if(clock_gettime(CLOCK_REALTIME, &t) == 0) {
		return (sec_t)t.tv_sec;
	}
#endif

	// fall back
	return time(NULL);
}

msec_t time_msec()
{
#if TARGET_OS_IPHONE
	NSDate *d = [NSDate date];
	NSTimeInterval t = [d timeIntervalSince1970];
	return (t * 1000);
#elif defined(__APPLE__)
	struct timeval t;
	if(gettimeofday(&t,NULL) == 0) {
		return ((msec_t)t.tv_sec * 1000) + (t.tv_usec / 1000);
	}
#else
	struct timespec t;
	if(clock_gettime(CLOCK_REALTIME, &t) == 0) {
		return ((msec_t)t.tv_sec * 1000) + (t.tv_nsec / 1000000);
	}
#endif

	// fall back
	return time(NULL) * 1000;
}

usec_t time_usec()
{
#if TARGET_OS_IPHONE
	NSDate *d = [NSDate date];
	NSTimeInterval t = [d timeIntervalSince1970];
	return (t * 1000000);
#elif defined(__APPLE__)
	struct timeval t;
	if(gettimeofday(&t,NULL) == 0) {
		return ((msec_t)t.tv_sec * 1000000) + t.tv_usec;
	}
#else
	struct timespec t;
	if(clock_gettime(CLOCK_REALTIME, &t) == 0) {
		return ((usec_t)t.tv_sec * 1000000) + (t.tv_nsec / 1000);
	}
#endif

	// fall back
	return time(NULL) * 1000000;
}

sec_t fixed_sec()
{
#if TARGET_OS_IPHONE
	UInt64 t = CAHostTimeBase::GetCurrentTimeInNanos();
	return (t / 1000000000);
#elif defined(__APPLE__)
	uint64_t t = mach_absolute_time();
	Nanoseconds elapsedNano = AbsoluteToNanoseconds(*(AbsoluteTime *)&t);
	return (*(uint64_t *) &t) / 1000000000 ;
#else
	struct timespec t;
	if(clock_gettime(CLOCK_MONOTONIC, &t) == 0) {
		return (sec_t)t.tv_sec;
	}
#endif

	// fall back
	return time_sec();
}

msec_t fixed_msec()
{
#if TARGET_OS_IPHONE
	UInt64 t = CAHostTimeBase::GetCurrentTimeInNanos();
	return (t / 1000000);
#elif defined(__APPLE__)
	uint64_t t = mach_absolute_time();
	Nanoseconds elapsedNano = AbsoluteToNanoseconds(*(AbsoluteTime*)&t);
	return (*(uint64_t *) &t) / 1000000;
#else
	struct timespec t;
	if(clock_gettime(CLOCK_MONOTONIC, &t) == 0) {
		return ((sec_t)t.tv_sec * 1000) + (t.tv_nsec / 1000000);
	}
#endif

	// fall back
	return time_msec();
}

usec_t fixed_usec()
{
#if TARGET_OS_IPHONE
	UInt64 t = CAHostTimeBase::GetCurrentTimeInNanos();
	return (t / 1000);
#elif defined(__APPLE__)
	uint64_t t = mach_absolute_time();
	Nanoseconds elapsedNano = AbsoluteToNanoseconds(*(AbsoluteTime*)&t);
	return (*(uint64_t *) &t) / 1000;
#else
	struct timespec t;
	if(clock_gettime(CLOCK_MONOTONIC, &t) == 0) {
		return ((usec_t)t.tv_sec * 1000000) + (t.tv_nsec / 1000);
	}
#endif

	// fall back
	return time_usec();
}

sec_t cpu_sec()
{
	struct tms t;
	if(times(&t) != -1) {
		return (sec_t)(t.tms_utime + t.tms_stime) / gClockTicks;
	}
	return -1;
}

msec_t cpu_msec()
{
	struct tms t;
	if(times(&t) != -1) {
		return (msec_t)(t.tms_utime + t.tms_stime) * 1000 / gClockTicks;
	}
	return -1;
}

usec_t cpu_usec()
{
	struct tms t;
	if(times(&t) != -1) {
		return (usec_t)(t.tms_utime + t.tms_stime) * 1000000 / gClockTicks;
	}
	return -1;
}

bool sleep(sec_t sec, msec_t msec)
{
	// ensure that msec is within the proper range
	if(msec >= 1000) {
		sec += (msec / 1000);
		msec %= 1000;
	}

	struct timespec t = { 
		static_cast<time_t>(sec), 
		static_cast<long>(msec * 1000 * 1000)
	};

	if(nanosleep(&t, 0) == -1) {
		// assuming errno == EINTR, but there are other rare errors (particularly with bad input to nanosleep())
		return false;
	}
	return true;
}

int get_timezone_offset()
{
#ifdef __APPLE__
	int ret = [[NSTimeZone localTimeZone] secondsFromGMT];
#else
	time_t date = time(NULL);
	int ret = localtime(&date)->tm_gmtoff;
#endif
	
	// "clamp-and-shift" to convert values outside the appropriate
	// range to their correct locations within the appropriate range.
	// e.g. -14 hours really belongs at +10 hours
	// e.g. +13 hours really belongs at -11 hours
	if(ret < (-12*3600)) {
		ret = (ret % (12*3600)) + (12*3600);
	} else if(ret > (12*3600)) {
		ret = (ret % (12*3600)) - (12*3600);
	}

	return ret;
}

} // namespace clocks

#endif

namespace clocks {

sec_t source_sec(Sources source)
{
	switch(source) {
	case sTime:
		return time_sec();
	case sFixed:
		return fixed_sec();
#if defined(__linux) || defined(__APPLE__)
	case sCPU:
		return cpu_sec();
#endif
	}
	return 0;
}

msec_t source_msec(Sources source)
{
	switch(source) {
	case sTime:
		return time_msec();
	case sFixed:
		return fixed_msec();
#if defined(__linux) || defined(__APPLE__)
	case sCPU:
		return cpu_msec();
#endif
	}
	return 0;
}

usec_t source_usec(Sources source)
{
	switch(source) {
	case sTime:
		return time_usec();
	case sFixed:
		return fixed_usec();
#if defined(__linux) || defined(__APPLE__)
	case sCPU:
		return cpu_usec();
#endif
	}
	return 0;
}

sec_t convert_sec(Sources from, sec_t fromTime, Sources to)
{
	if (from == to) {
		return fromTime;
	}
	return fromTime - source_sec(from) + source_sec(to);
}

msec_t convert_msec(Sources from, msec_t fromTime, Sources to)
{
	if (from == to) {
		return fromTime;
	}
	return fromTime - source_msec(from) + source_msec(to);
}

usec_t convert_usec(Sources from, usec_t fromTime, Sources to)
{
	if (from == to) {
		return fromTime;
	}
	return fromTime - source_usec(from) + source_usec(to);
}

	

} // namespace clocks
