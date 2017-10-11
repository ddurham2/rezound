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

#warning add a read-only feature to TPoolFile and use it in CrezSoundTranslator::onLoadSound
#warning add ctrl-mousewheel  perform horizontal zoom into the cursor position (seems like something like this already exists)

#include "../../config/common.h"
#include "fox_compat.h"

#include <string.h>

#include <stdexcept>
#ifdef ENABLE_NLS
#include <clocale> // for gettext init
#endif

#include <CPath.h>
DECLARE_STATIC_CPATH // to declare CPath::dirDelim

#include "CMainWindow.h"
#include "CMetersWindow.h"
#include "CStatusComm.h"

#include "CSoundFileManager.h"
#include "CFrontendHooks.h"
#include "../backend/initialize.h"
#include "settings.h"

#include "CFOXIcons.h"

#include "CAboutDialog.h"

#include "CKeyBindingsDialog.h"

static void setupWindows(CMainWindow *mainWindow);

static void setupLocale(FXApp *application);

// QQQ
void enableAutoRepeat(void *display,bool enable);

// --- FOR TESTING
static void countWidgets(FXWindow *w);
static void printNormalFontProperties(FXApp *application);
static void listFonts();
// ---

// TODO put some of this stuff into a main.cpp which is frontend non-specific.. then define a frontend_main(argc, argv) which is defined

int main(int argc,char* argv[])
{
	readBackendSettings();  // just so gSysDataDirectory is populated
#ifdef ENABLE_NLS
	setlocale(LC_ALL,"");
	bindtextdomain(REZOUND_PACKAGE,(gSysDataDirectory + "/../locale").c_str());
	textdomain(REZOUND_PACKAGE);
#endif


	try
	{
		FXApp* application=new FXApp("ReZound","NLT");
		application->init(argc, argv);

		// QQQ
		if(argc==2 && string(argv[1])=="--fix-auto-repeat")
			enableAutoRepeat(application->getDisplay(),true);

#ifdef ENABLE_NLS
		setupLocale(application);
#endif

		//printNormalFontProperties(application);
		//listFonts();

		FOXIcons=new CFOXIcons(application);

#if FOX_MAJOR >= 1 // otherwise they just look funny which is okay i guess
		// these icons have a special property that they don't need a transparent color
		FOXIcons->logo->setTransparentColor(FXRGB(0,0,0));
		FOXIcons->icon_logo_16->setTransparentColor(FXRGB(0,0,0));
		FOXIcons->icon_logo_32->setTransparentColor(FXRGB(0,0,0));
#endif

		/*
		// set application wide colors
		application->setBaseColor(FXRGB(193,163,102));
		... there are others that could be set ...
		*/

		// you have to do this for hints to be activated
		new FXToolTip(application,TOOLTIP_VARIABLE);

		// create the main window 
		CMainWindow *mainWindow=new CMainWindow(application);
		application->create();

		// unfortunately we have to create main window before we can pop up error messages
		// which means we will have to load plugins and add their buttons to an already
		// created window... because there could be errors while loading
		// ??? I could fix this by delaying the creation of buttons after the creation of the main window.. and I should do this, since if I ever support loading plugins, I need to be able to popup error dialogs while loading them
		gStatusComm=new CStatusComm(mainWindow);
		gFrontendHooks=new CFrontendHooks(mainWindow);

		// from here on we can create error messages
		//   ??? I suppose I could atleast print to strerr if gStatusComm was not created yet

		ASoundPlayer *soundPlayer=NULL;
		if(!initializeBackend(soundPlayer, argc, argv))
			return 0;

		// the backend needed to be setup before this stuff was done
		static_cast<CFrontendHooks *>(gFrontendHooks)->doSetupAfterBackendIsSetup();


		gSoundFileManager=new CSoundFileManager(mainWindow,soundPlayer,gSettingsRegistry);

		mainWindow->getMetersWindow()->setSoundPlayer(soundPlayer);

		// create all the dialogs 
		setupWindows(mainWindow);

		// load any sounds that were from the previous session
		const vector<string> errors=gSoundFileManager->loadFilesInRegistry();
		for(size_t t=0;t<errors.size();t++)
			Error(errors[t]);

		// give the backend another oppertunity to handle arguments (like loading files)
		if(!handleMoreBackendArgs(gSoundFileManager,argc,argv))
			return 0;

		gAboutDialog->showOnStartup();
		mainWindow->show();
		fprintf(stderr,"If rezound dies unexpectedly while seeking with the keyboard, auto-repeat may be disabled.. if this happens run 'rezound --fix-auto-repeat'\n"); // QQQ
		application->run();

		//countWidgets(application->getRootWindow()); printf("wc: %d\n",wc);

		delete gSoundFileManager;

		deinitializeBackend();

		// ??? delete gFrontendHooks
		// ??? delete gStatusComm;

		delete FOXIcons;
		
		//delete application; dies while removing an accelerator while destroying an FXMenuCommand... I'm not sure why it's a problem
	}
	catch(exception &e)
	{
		if(gStatusComm!=NULL)
			Error(e.what());
		else
			fprintf(stderr,"exception -- %s\n",e.what());

		return 1;
	}
	catch(...)
	{
		if(gStatusComm!=NULL)
			Error("unknown exception caught\n");
		else
			fprintf(stderr,"unknown exception caught\n");

		return 1;
	}

	return 0;
}



#include "CChannelSelectDialog.h"
#include "CPasteChannelsDialog.h"
#include "CCueDialog.h"
#include "CCueListDialog.h"
#include "CUserNotesDialog.h"
#include "CCrossfadeEdgesDialog.h"

void setupWindows(CMainWindow *mainWindow)
{
		gAboutDialog=new CAboutDialog(mainWindow);

		gKeyBindingsDialog=new CKeyBindingsDialog(mainWindow);

		// create the channel select dialog that AActionFactory is given to use often
		gChannelSelectDialog=new CChannelSelectDialog(mainWindow);

		// create the channel select dialog that AActionFactory uses for selecting channels to paste to
		gPasteChannelsDialog=new CPasteChannelsDialog(mainWindow);

		// create the dialog used to obtain the name for a new cue
		gCueDialog=new CCueDialog(mainWindow);

		// create the dialog used to manipulate a list of cues
		gCueListDialog=new CCueListDialog(mainWindow);

		// create the dialog used to make user notes saved with the sound
		gUserNotesDialog=new CUserNotesDialog(mainWindow);

		// create the dialog used to set the length of the crossfade on the edges
		gCrossfadeEdgesDialog=new CCrossfadeEdgesDialog(mainWindow);

		// create the tool bars in CMainWindow
		mainWindow->createMenus();
}

#ifdef ENABLE_NLS
void setupLocale(FXApp *application)
{
	/*??? it'd be nice if I knew if the font choice was overridden in the FOX registry... I should also have a font dialog for choosing in the config system later */
	string lang=setlocale(LC_MESSAGES,NULL);

	// it's important to set the default locale for STL streams too (but remove the numeric punctuation because the STL putting punctuation in a number doesn't jive when atoi/f parses it)
	locale loc(lang.c_str());
	std::locale::global(std::locale(loc,std::locale::classic(),std::locale::numeric));
}
#endif


// --- FUNCTIONS ONLY USED WHEN TESTING THINGS ---

#if 0
int wc=1;
void countWidgets(FXWindow *w)
{
	w=w->getFirst();
	if(w)
	{
		do
		{
			wc++;
			countWidgets(w);
		}
		while((w=w->getNext()));
	}
}
#endif

#if 0
void printNormalFontProperties(FXApp *application)
{
	FXFontDesc desc;
	application->getNormalFont()->getFontDesc(desc);
	printf("normal font {\n");
	printf("\tface: '%s'\n",desc.face);
	printf("\tsize: %d\n",desc.size);
	printf("\tweight: %d\n",desc.weight);
	printf("\tslant: %d\n",desc.slant);
	printf("\tencoding: %d\n",desc.encoding);
	printf("\tsetwidth: %d\n",desc.setwidth);
	printf("\tflags: %d\n",desc.flags);
	printf("}\n");
}

void listFonts()
{
	printf("font_listing {\n");

	FXFontDesc *fonts;
	FXuint numfonts;
	if(FXFont::listFonts(fonts,numfonts,"helvetica"))
	{
		printf("\tnum fonts: %d\n",numfonts);
		for(FXuint t=0;t<numfonts;t++)
			printf("\t%d: face: '%s' encoding: %d\n",t,fonts[t].face,fonts[t].encoding);
	}
	
	printf("}\n");
}
#endif



// --- QQQ ---------------------------------------------
// QQQ: HACK: My use of these is a hack (first, the parameter is actually "Display *display").. since we can't (or I don't know 
// how to) distinguish between a real key press and an autogenerated key-release -> key-press event (coming from keyboard repeats) 
// I just disable auto-repeat while the seek forward or seek backward events are happening.  Unfortunately it disables it for the
// whole X display.  I was originally very careful to arrange that XAutoRepeatOn() be called in the event of an unexpected termination,
// but calling it is apparently not enough because it didn't get restored even though I know it was being called
extern "C" int XAutoRepeatOn(void *display);
extern "C" int XAutoRepeatOff(void *display);
static void *gDisplay=NULL;

void enableAutoRepeat(void *display,bool enable)
{
	gDisplay=display; // save this for if we die
	if(enable)
		XAutoRepeatOn(display); // QQQ
	else
		XAutoRepeatOff(display); // QQQ
}

/*
#include <signal.h>

extern "C" void enable_autorepeat_sighandler(int sig)
{
	printf("new sig handler called\n");
	if(gDisplay!=NULL)
	{
		printf("enabling auto repeat again\n");
		XAutoRepeatOn(gDisplay);
	}

	// now do the default action
	signal(sig,SIG_DFL);
	raise(sig);
}

void setupAutoRepeatFixers()
{
	for(int t=1;t<32;t++) // handle all signals 
	{
		if(t==9 || t==20 || t==17 || t==18) continue; // ignore these
		signal(t,enable_autorepeat_sighandler);
	}
}
*/

