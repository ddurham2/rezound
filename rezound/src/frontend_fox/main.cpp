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

#include "../../config/common.h"

#include <fox/fx.h>
#include <stdexcept>

#include "CMainWindow.h"
#include "CStatusComm.h"

#include "CSoundFileManager.h"
#include "../backend/initialize.h"
#include "settings.h"

#include "../images/images.cpp"

void setupWindows(CMainWindow *mainWindow);
void setupAccels(CMainWindow *mainWindow);


int main(int argc,char *argv[])
{
	try
	{
		FXApp* application=new FXApp("ReZound","NLT");
		application->init(argc,argv);

		/*
		// set application wide colors
		application->setBaseColor(FXRGB(193,163,102));
		... there are others that could be set ...
		*/

		// you have to do this for hints to be activated
		new FXTooltip(application);

		// create the main window 
		CMainWindow *mainWindow=new CMainWindow(application);
		application->create();

		// unfortunately we have to create main window before we can pop up error messages
		// which means we will have to load plugins and add their buttons to an already
		// created window... because there could be errors while loading
		// ??? I could fix this by delaying the creation of buttons after the creation of the main window.. and I should do this, since if I ever support loading plugins, I need to be able to popup error dialogs while loading them
		gStatusComm=new CStatusComm(mainWindow);

		// from here on we can create error messages
		//   ??? I suppose I could atleast print to strerr if gStatusComm was not created yet

		ASoundPlayer *soundPlayer=NULL;
		initializeBackend(soundPlayer);

		gSoundFileManager=new CSoundFileManager(mainWindow,soundPlayer,gSettingsRegistry);

		// create all the dialogs 
		setupWindows(mainWindow);

		// setup an FXAccelTable which allows for keys to invoke actions
		setupAccels(mainWindow);

		// load any sounds that were from the previous session
		const vector<string> errors=gSoundFileManager->loadFilesInRegistry();
		for(size_t t=0;t<errors.size();t++)
			Error(errors[t]);

		application->run();


		delete gSoundFileManager;

		deinitializeBackend();

		// ??? delete CStatusComm;
	}
	catch(exception &e)
	{
		if(gStatusComm!=NULL)
			Error(e.what());
		else
			fprintf(stderr,"exception -- %s\n",e.what());

		return(1);
	}
	catch(...)
	{
		if(gStatusComm!=NULL)
			Error("unknown exception caught\n");
		else
			fprintf(stderr,"unknown exception caught\n");

		return(1);
	}

	return(0);
}



#include "CChannelSelectDialog.h"
#include "CPasteChannelsDialog.h"
#include "CNewSoundDialog.h"
#include "CRecordDialog.h"
#include "CCueDialog.h"
#include "CCueListDialog.h"
#include "CUserNotesDialog.h"
#include "CCrossfadeEdgesDialog.h"
#include "CEditToolbar.h"
#include "CSoundListWindow.h"

void setupWindows(CMainWindow *mainWindow)
{
		// create the channel select dialog that AActionFactory is given to use often
		gChannelSelectDialog=new CChannelSelectDialog(mainWindow);

		// create the channel select dialog that AActionFactory uses for selecting channels to paste to
		gPasteChannelsDialog=new CPasteChannelsDialog(mainWindow);

		// create the dialog used to select parameters when the use creates a new sound
		gNewSoundDialog=new CNewSoundDialog(mainWindow);

		// create the dialog used to record to a new sound
		gRecordDialog=new CRecordDialog(mainWindow);

		// create the dialog used to obtain the name for a new cue
		gCueDialog=new CCueDialog(mainWindow);

		// create the dialog used to manipulate a list of cues
		gCueListDialog=new CCueListDialog(mainWindow);

		// create the dialog used to make user notes saved with the sound
		gUserNotesDialog=new CUserNotesDialog(mainWindow);

		// create the dialog used to set the length of the crossfade on the edges
		gCrossfadeEdgesDialog=new CCrossfadeEdgesDialog(mainWindow);

		// create the editing toolbar and set the global gEditToolbar variable
		gEditToolbar=new CEditToolbar(mainWindow);
		gEditToolbar->create();

		if(gFocusMethod==fmSoundWindowList)
		{
			gSoundListWindow=new CSoundListWindow(mainWindow);
			gSoundListWindow->create();
		}

		// create the tool bars in CMainWindow
		mainWindow->createToolbars();

		mainWindow->show();
		gEditToolbar->show();
}

#include <fox/fxkeys.h>
void setupAccels(CMainWindow *mainWindow)
{
	FXAccelTable *at=mainWindow->getAccelTable();


	// play controls
	at->addAccel(KEY_a ,mainWindow,MKUINT(CMainWindow::ID_PLAY_SELECTION_ONCE_BUTTON,SEL_COMMAND));
	at->addAccel(KEY_s ,mainWindow,MKUINT(CMainWindow::ID_STOP_BUTTON,SEL_COMMAND));



	// edits
	at->addAccel(MKUINT(KEY_a,CONTROLMASK) ,gEditToolbar->selectAllButton,MKUINT(CActionButton::ID_KEY,SEL_COMMAND));
	
	at->addAccel(MKUINT(KEY_c,CONTROLMASK), gEditToolbar->copyButton,MKUINT(CActionButton::ID_KEY,SEL_COMMAND));
	at->addAccel(MKUINT(KEY_C,CONTROLMASK|SHIFTMASK), gEditToolbar->copyButton,MKUINT(CActionButton::ID_KEY,SEL_COMMAND));
	at->addAccel(MKUINT(KEY_x,CONTROLMASK), gEditToolbar->cutButton,MKUINT(CActionButton::ID_KEY,SEL_COMMAND));
	at->addAccel(MKUINT(KEY_X,CONTROLMASK|SHIFTMASK), gEditToolbar->cutButton,MKUINT(CActionButton::ID_KEY,SEL_COMMAND));
	at->addAccel(MKUINT(KEY_d,CONTROLMASK), gEditToolbar->delButton,MKUINT(CActionButton::ID_KEY,SEL_COMMAND));
	at->addAccel(MKUINT(KEY_D,CONTROLMASK|SHIFTMASK), gEditToolbar->delButton,MKUINT(CActionButton::ID_KEY,SEL_COMMAND));
	at->addAccel(MKUINT(KEY_r,CONTROLMASK), gEditToolbar->cropButton,MKUINT(CActionButton::ID_KEY,SEL_COMMAND));
	at->addAccel(MKUINT(KEY_R,CONTROLMASK|SHIFTMASK), gEditToolbar->cropButton,MKUINT(CActionButton::ID_KEY,SEL_COMMAND));
	at->addAccel(MKUINT(KEY_v,CONTROLMASK), gEditToolbar->pasteInsertButton,MKUINT(CActionButton::ID_KEY,SEL_COMMAND));
	at->addAccel(MKUINT(KEY_V,CONTROLMASK|SHIFTMASK), gEditToolbar->pasteInsertButton,MKUINT(CActionButton::ID_KEY,SEL_COMMAND));

	at->addAccel(MKUINT(KEY_z,CONTROLMASK) ,mainWindow,MKUINT(CMainWindow::ID_UNDO_BUTTON,SEL_COMMAND));
}

