/* 
 * Copyright (C) 2006 - David W. Durham
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

#include <QApplication>

#include "qt_compat.h"

#include "CMainWindow.h"
#include "CStatusComm.h"
#include "CSoundFileManager.h"
#include "CFrontendHooks.h"
#include "../backend/initialize.h"
#include "settings.h"

#include <CPath.h>
DECLARE_STATIC_CPATH // to declare CPath::dirDelim

/*
#include "CAboutDialog.h"
*/

static void setupWindows(CMainWindow *mainWindow);

int main( int argc, char ** argv )
{
	QApplication a( argc, argv );
	try
	{
		CMainWindow mainWindow;

		// unfortunately we have to create main window before we can pop up error messages
		// which means we will have to load plugins and add their buttons to an already
		// created window... because there could be errors while loading
		// ??? I could fix this by delaying the creation of buttons after the creation of the main window.. and I should do this, since if I ever support loading plugins, I need to be able to popup error dialogs while loading them
		gStatusComm=new CStatusComm(&mainWindow);
		gFrontendHooks=new CFrontendHooks(&mainWindow);

		// from here on we can create error messages
		//   ??? I suppose I could atleast print to strerr if gStatusComm was not created yet

		ASoundPlayer *soundPlayer=NULL;
		if(!initializeBackend(soundPlayer,argc,argv))
			return 0;

		mainWindow.backendInitialized();

		// the backend needed to be setup before this stuff was done
		static_cast<CFrontendHooks *>(gFrontendHooks)->doSetupAfterBackendIsSetup();

		gSoundFileManager=new CSoundFileManager(&mainWindow,soundPlayer,gSettingsRegistry);

		mainWindow.setSoundPlayerForMeters(soundPlayer);

		// create all the dialogs and stuff
		setupWindows(&mainWindow);

		// load any sounds that were from the previous session
		const vector<string> errors=gSoundFileManager->loadFilesInRegistry();
		for(size_t t=0;t<errors.size();t++)
			Error(errors[t]);

		// give the backend another oppertunity to handle arguments (like loading files)
		if(!handleMoreBackendArgs(gSoundFileManager,argc,argv))
			return 0;

		/*
		gAboutDialog->showOnStartup();
		*/

		mainWindow.show();
		a.connect( &a, SIGNAL( lastWindowClosed() ), &a, SLOT( quit() ) );

		int r=a.exec();

		delete gSoundFileManager;

		deinitializeBackend();

		return r;
	}
	catch(exception &e)
	{
// ??? I think it does no good to use statuscomm here
		if(gStatusComm!=NULL)
		{
			((CStatusComm*)gStatusComm)->noMainWindow();
			Error(e.what());
		}
		else
			fprintf(stderr,"exception -- %s\n",e.what());

		return 1;
	}
	catch(...)
	{
		if(gStatusComm!=NULL)
		{
			((CStatusComm*)gStatusComm)->noMainWindow();
			Error("unknown exception caught\n");
		}
		else
			fprintf(stderr,"unknown exception caught\n");

		return 1;
	}
}

#include "ActionParam/CChannelSelectDialog.h"
#include "ActionParam/CPasteChannelsDialog.h"
#include "CUserNotesDialog.h"
CUserNotesDialog *gUserNotesDialog;
#include "CKeyBindingsDialog.h"
CKeyBindingsDialog *gKeyBindingsDialog;
/*
#include "CCueDialog.h"
#include "CCueListDialog.h"
*/
#include "CCrossfadeEdgesDialog.h"

void setupWindows(CMainWindow *mainWindow)
{
	/*
		gAboutDialog=new CAboutDialog(mainWindow);
	*/

		// create the channel select dialog that AActionFactory is given to use often
		gChannelSelectDialog=new CChannelSelectDialog(mainWindow);

		// create the channel select dialog that AActionFactory uses for selecting channels to paste to
		gPasteChannelsDialog=new CPasteChannelsDialog(mainWindow);

	/* ???
		// create the dialog used to obtain the name for a new cue
		gCueDialog=new CCueDialog(mainWindow);

		// create the dialog used to manipulate a list of cues
		gCueListDialog=new CCueListDialog(mainWindow);
	*/

		// create the dialog used to make user notes saved with the sound
		gUserNotesDialog=new CUserNotesDialog(mainWindow);

		// create the dialog use for the user to re-bind keys to actions
		gKeyBindingsDialog=new CKeyBindingsDialog(mainWindow);

		// create the dialog used to set the length of the crossfade on the edges
		gCrossfadeEdgesDialog=new CCrossfadeEdgesDialog(mainWindow);

		// create the tool bars in CMainWindow
		mainWindow->createMenus();
}
