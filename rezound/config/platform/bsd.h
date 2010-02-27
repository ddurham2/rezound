#ifndef __rez_platform_bsd_H__
#define __rez_platform_bsd_H__


#if defined(__FreeBSD__)
	#define rez_OS_BSD

#endif

#if defined(__NetBSD__)
	#define rez_OS_BSD
#endif

#if defined(__OpenBSD__)
	#define rez_OS_BSD
#endif

#if defined(__bsdi__)
	#define rez_OS_BSD
#endif



#if 0 // these functions are now available on BSD 5+
/* these functions are not included with BSD */
#ifdef rez_OS_BSD
	/*
 	 * This should really be a configure test because 'nearbyint() and round()' might 
 	 * exist one day on BSD I could also just use rint, but nearbyint is supposed to 
 	 * be slightly faster because it doesn't raise the inexact math exception.
 	 */
	#define nearbyint rint
	#define round rintf
	#define atoll(v)  strtoll((v),NULL,10)
#endif
#endif



#endif
