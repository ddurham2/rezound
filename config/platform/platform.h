#ifndef __platform_H__
#define __platform_H__

#include "linux.h"
#include "solaris.h"
#include "bsd.h"


#if !defined(rez_OS_LINUX) && !defined(rez_OS_SOLARIS) && !defined(rez_OS_BSD)
	#warning no platform determined!
#endif

#endif
