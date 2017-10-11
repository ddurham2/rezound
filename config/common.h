#ifndef rezound_COMMON_H
#define rezound_COMMON_H

/* for things like UINT64_C() and PRIx64 from stdint.h */
#define __STDC_CONSTANT_MACROS
#define __STDC_FORMAT_MACROS

/* common.h -- This file will deal with low-level portability problems. It
 * should be included at the top of every package file. */

// TODO work to remove this global namespace directive and support things properly in the .h and .cpp
namespace std {}
using namespace std;

#include "config/config.h"

#include <stddef.h>

#if defined(__clang__)
#	define STATIC_TPL

#elif defined(__GNUC__)
#	ifndef GCC_VERSION
#		define GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#	endif

// requirement of static template functions changed in gcc-4.3
#	if GCC_VERSION >= 403
#		define STATIC_TPL
#	else
#		define STATIC_TPL static
#	endif
#endif




#ifdef __GNUC__ //Using gcc
# define __func__ __PRETTY_FUNCTION__
#endif //__GNUC__

/* include code that determines the platform, and may supply missing function implementations */
#include "platform/platform.h"

/* 
 * Include this now so that it can't be included later and then the #define gettext(a)-to-nothing 
 * below won't mutate the header file's declaration 
 */
#ifdef HAVE_LIBINTL
	#include <libintl.h>
#endif 

#ifdef ENABLE_NLS
	/* this avoids having gettext("") return junk instead of a simple "" */
	#define gettext(String) ((String)==NULL ? NULL : ( (String)[0]==0 ? "" : gettext(String) ))
	#define _(String) gettext (String)
#else
	#define _(String) String
	#define gettext(String) String
#endif

// NOOP for gettext
#define N_(String) String

/* determine if pitch/tempo changing is available */
#ifdef HAVE_LIBSOUNDTOUCH
	#define AVAIL_PITCH_CHANGE
	#define AVAIL_TEMPO_CHANGE
#endif


#endif /* COMMON_H */
