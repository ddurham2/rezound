#ifndef __fox_compat_H__
#define __fox_compat_H__

#include <fox/fxver.h>

#define REZ_FOX_VERSION ((FOX_MAJOR*10000)+(FOX_MINOR*100)+FOX_LEVEL)

/*
 * FOX renamed some things at version 1.1.8 and added namespaces
 * so if the version is older then I rename them back.
 * Some few years and I can remove these defines
 */
#if REZ_FOX_VERSION<10108

	#define FOX_NO_NAMESPACE

	#define FXToolTip FXTooltip
	#define FXToolBar FXToolbar
	#define FXScrollBar FXScrollbar
	#define FXMenuBar FXMenubar
	#define FXStatusBar FXStatusbar
	#define FXStatusLine FXStatusline
	#define FXToolBarShell FXToolbarShell
	#define FXToolBarGrip FXToolbarGrip
	#define horizontalScrollBar horizontalScrollbar
	#define verticalScrollBar verticalScrollbar
	#define getRootWindow getRoot
#else
	
	#define FOX_RESTORE_WINDOW_POSITIONS

#endif

#if REZ_FOX_VERSION<10110
	#define getModality modalModality
#endif

#if REZ_FOX_VERSION<10117
	#define FXSELID(x) SELID(x)
	#define FXSELTYPE(x) SELTYPE(x)
	#define setFont setTextFont
#endif




#endif
