#ifndef __rez_platform_solaris_H__
#define __rez_platform_solaris_H__

#if defined(__sun) || defined(sun)
        #define rez_OS_SOLARIS
#endif


/* these functions are not included with solaris */
#ifdef rez_OS_SOLARIS
	/* This should really be a configure test because they might exist one day on solaris */

	#define nearbyint rint
	#define round rint

	/* and apparently on some version of solaris/gcc the ...f() math functions aren't present */
	#define floorf(a) ((float)floor((double)(a)))
	#define ceilf(a) ((float)ceil((double)(a)))
	/* modf has a modff on solaris */
	#define sinf(a) ((float)sin((double)(a)))
	#define cosf(a) ((float)cos((double)(a)))
	#define tanf(a) ((float)tan((double)(a)))
	#define powf(a) ((float)pow((double)(a)))
	#define isnanf(a) ((float)isnan((double)(a)))

	/*
	 * kludge: fox is redefining these in fxdef.h when __USE_ISOC99 isn't defined
	 * I don't think it's proper for it to do that in public header files (sure do
	 * it in the private implemenation.  So I define this so it won't #define these
	 * (then again, I may be doing sort of the same thing since things such as
	 * istring include common.h (but don't have to)
	 */
	#define __USE_ISOC99
#endif


#endif
