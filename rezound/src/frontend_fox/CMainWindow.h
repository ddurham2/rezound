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

#ifndef __CMainWindow_H__
#define __CMainWindow_H__

#include "../../config/common.h"

#include <map>

#include <fox/fx.h>

class CMainWindow : public FXMainWindow
{
	FXDECLARE(CMainWindow)
public:

	CMainWindow(FXApp* a);

	virtual void show();
	virtual void hide();

	void createToolbars();
	

	long onQuit(FXObject *sender,FXSelector sel,void *ptr);

	long onActionControlTabMouseMove(FXObject *sender,FXSelector sel,void *ptr);

	long onFollowPlayPositionButton(FXObject *sender,FXSelector sel,void *ptr);

	long onCrossfadeEdgesCheckbox(FXObject *sender,FXSelector sel,void *ptr);
	long onCrossfadeEdgesSettings(FXObject *sender,FXSelector sel,void *ptr);

	// file action events
	long onFileButton(FXObject *sender,FXSelector sel,void *ptr);

	long onReopenMenuPopup(FXObject *sender,FXSelector sel,void *ptr);
	long onReopenMenuSelect(FXObject *sender,FXSelector sel,void *ptr);

	// play control events
	long onPlayControlButton(FXObject *sender,FXSelector sel,void *ptr);

	long onRedrawButton(FXObject *sender,FXSelector sel,void *ptr);

	long onUndoButton(FXObject *sender,FXSelector sel,void *ptr);
	long onClearUndoHistoryButton(FXObject *sender,FXSelector sel,void *ptr);

	long onUserNotesButton(FXObject *sender,FXSelector sel,void *ptr);

	long onShuttleReturn(FXObject *sender,FXSelector sel,void *ptr);
	long onShuttleChange(FXObject *sender,FXSelector sel,void *ptr);

	long onDefragButton(FXObject *sender,FXSelector sel,void *ptr);
	long onPrintSATButton(FXObject *sender,FXSelector sel,void *ptr);

	enum
	{
		ID_FILE_NEW_BUTTON=FXMainWindow::ID_LAST,
		ID_FILE_OPEN_BUTTON,
		ID_FILE_SAVE_BUTTON,
		ID_FILE_SAVE_AS_BUTTON,
		ID_FILE_CLOSE_BUTTON,
		ID_FILE_REVERT_BUTTON,
		ID_FILE_RECORD_BUTTON,

		ID_REOPEN_MENU_SELECT,

		ID_PLAY_ALL_ONCE_BUTTON,
		ID_PLAY_ALL_LOOPED_BUTTON,
		ID_PLAY_SELECTION_ONCE_BUTTON,
		ID_PLAY_SELECTION_LOOPED_BUTTON,

		ID_STOP_BUTTON,
		ID_PAUSE_BUTTON,

		ID_JUMP_TO_BEGINNING_BUTTON,
		ID_JUMP_TO_START_POSITION_BUTTON,
	
		ID_JUMP_TO_PREV_CUE_BUTTON,
		ID_JUMP_TO_NEXT_CUE_BUTTON,
	
		ID_SHUTTLE_DIAL,

		ID_REDRAW_BUTTON,

		ID_UNDO_BUTTON,
		ID_CLEAR_UNDO_HISTORY_BUTTON,

		ID_NOTES_BUTTON,

		ID_DEFRAG_BUTTON,
		ID_PRINT_SAT_BUTTON,

		ID_ACTIONCONTROL_TAB,

		ID_FOLLOW_PLAY_POSITION_BUTTON,

		ID_CROSSFADE_EDGES_CHECKBOX,
		ID_CROSSFADE_EDGES_SETTINGS,

		ID_LAST
	};
							  

protected:

	CMainWindow() {}


private:

	FXHorizontalFrame	*contents;		// top horizontal main frame which contains play controls and action controls
	FXPacker   		*playControlsFrame;	// frame that contains the play control buttons
		FXDial *shuttleDial;
	FXPacker		*miscControlsFrame;	// frame that contains the undo/redo buttons, follow-play-position checkbox, etc
		FXCheckButton	*followPlayPositionButton;
		FXCheckButton	*crossfadeEdgesCheckbox;
	FXTabBook   		*actionControlsFrame;	// frame that is the tab layout for the other actions
		FXTabItem 	*fileTab;	
			FXPacker *fileTabFrame;
				FXButton *fileNewButton;
				FXButton *fileOpenButton;
				FXButton *fileSaveButton;
				FXButton *fileSaveAsButton;
				FXButton *fileCloseButton;
				FXButton *fileRevertButton;
				FXButton *fileRecordButton;
				FXButton *notesButton;
		FXTabItem 	*effectsTab;	
			FXPacker *effectsTabFrame;
		FXTabItem 	*loopingTab;	
			FXPacker *loopingTabFrame;
		FXTabItem 	*remasterTab;	
			FXPacker *remasterTabFrame;


	map<void *,int> actionControlTabOrdering; // is a mapping from FXTabItem pointers to the index for FXTabBar::setCurrent() needing to give it the object created
	void *mouseMoveLastTab; // contains the last tab for which the mouse move even was called, onActionControlTabMouseMove

};



#endif
