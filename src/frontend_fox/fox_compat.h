#ifndef __fox_compat_H__
#define __fox_compat_H__

#include "../../config/common.h"

#if defined(HAVE_LIBFOX)
	#include <fx.h>
	#include <fxkeys.h>
#else
	#error no HAVE_LIBFOX defined
#endif


#define REZ_FOX_VERSION ((FOX_MAJOR*10000)+(FOX_MINOR*100)+FOX_LEVEL)

#if FOX_MAJOR<1
	// no control over ticks before 1.0
	#define SLIDER_TICKS_LEFT 0
	#define SLIDER_TICKS_RIGHT 0
	#define SLIDER_TICKS_TOP 0
	#define SLIDER_TICKS_BOTTOM 0
#endif

/* fox-1.1.39 change FXApp::runWhileEvents to FXApp::runModalWhileEvents */
#if REZ_FOX_VERSION<10139
	#define runModalWhileEvents runWhileEvents
#endif

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
	#define FXSEL(x,y) MKUINT(y,x)
	#define compat_setFont(f) setTextFont(f)
#else
	#define compat_setFont(f) setFont(f)
#endif

#if REZ_FOX_VERSION<10125
	#define addTimeout(tgt,sel,ms) addTimeout(ms,tgt,sel)
#endif

#if REZ_FOX_VERSION<10146
	// 1.1.46 removed the nvis parameter, so I have to supply a zero if fox is older
	// I always call setNumVisible() now after construction 
	#define FXList(p,tgt,sel,opts) FXList((p),0,(tgt),(sel),(opts))
	// ditto (but wasn't mentioned in the News about 1.1.46)
	#define FXComboBox(p,cols,tgt,sel,opts) FXComboBox((p),(cols),0,(tgt),(sel),(opts))
	// ditto (but wasn't mentioned in the News about 1.1.46)
	#define FXListBox(p,tgt,sel,opts) FXListBox((p),0,(tgt),(sel),(opts))
#endif

#if REZ_FOX_VERSION>=10506
	// ~1.5.6 changed the FONTWEIGHT_xxx defines to enums
	#define FONTWEIGHT_BOLD FXFont::Bold
	#define FONTWEIGHT_LIGHT FXFont::Light
	#define FONTWEIGHT_NORMAL FXFont::Normal
	#define FONTSLANT_REGULAR FXFont::Straight
#endif


#if REZ_FOX_VERSION<=10600
	// ~1.6.0 changed fxparseAccel to just FX::parseAccel
	#define parseAccel fxparseAccel
	#define unparseAccel fxunparseAccel
#endif

#if REZ_FOX_VERSION<10600
	// 1.6.0 changed FXbool to bool through the headesr
	#define fx_bool FXbool
#else
	#define fx_bool bool
#endif




#endif
