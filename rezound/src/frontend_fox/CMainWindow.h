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
#include "fox_compat.h"

#include <map>
#include <string>

#include <fox/fx.h>

class CSoundWindow;
class CMetersWindow;

class CMainWindow : public FXMainWindow
{
	FXDECLARE(CMainWindow)
public:

	CMainWindow(FXApp* a);
	virtual ~CMainWindow();

	virtual void show();
	virtual void hide();

	void createMenus();

	void showAbout(); // temporary for the alpha and beta stages

	long onQuit(FXObject *sender,FXSelector sel,void *ptr);

	long onFollowPlayPositionButton(FXObject *sender,FXSelector sel,void *ptr);

	long onCrossfadeEdgesComboBox(FXObject *sender,FXSelector sel,void *ptr);
	long onCrossfadeEdgesSettings(FXObject *sender,FXSelector sel,void *ptr);

	long onClipboardComboBox(FXObject *sender,FXSelector sel,void *ptr);

	// file action events
	long onFileAction(FXObject *sender,FXSelector sel,void *ptr);

	// play control events
	long onPlayControlButton(FXObject *sender,FXSelector sel,void *ptr);

	long onUserNotesButton(FXObject *sender,FXSelector sel,void *ptr);

	long onUndoButton(FXObject *sender,FXSelector sel,void *ptr);
	long onClearUndoHistoryButton(FXObject *sender,FXSelector sel,void *ptr);

	long onShuttleReturn(FXObject *sender,FXSelector sel,void *ptr);
	long onShuttleChange(FXObject *sender,FXSelector sel,void *ptr);
	void positionShuttleGivenSpeed(double seekSpeed,const string shuttleControlScalar,bool springBack);
	long onShuttleDialSpringButton(FXObject *sender,FXSelector sel,void *ptr);
	long onShuttleDialScaleButton(FXObject *sender,FXSelector sel,void *ptr);

	long onSoundListChange(FXObject *sender,FXSelector sel,void *ptr);
	long onSoundListHotKey(FXObject *sender,FXSelector sel,void *ptr);

	long onHotKeyFocusFixup(FXObject *sender,FXSelector sel,void *ptr);

	// used to control the shuttle control with the keyboard
	long onKeyboardSeek(FXObject *sender,FXSelector sel,void *ptr);

	long onViewKey(FXObject *sender,FXSelector sel,void *ptr); // main window gets view-change key presses because the sound window changes and we can't bind a key to any particualar object pointer

	long onKeyPress(FXObject *sender,FXSelector sel,void *ptr);
	long onKeyRelease(FXObject *sender,FXSelector sel,void *ptr);
	long onMouseEnter(FXObject *sender,FXSelector sel,void *ptr);

	long onDebugButton(FXObject *sender,FXSelector sel,void *ptr);


	enum
	{
		ID_FILE_NEW_MENUITEM=FXMainWindow::ID_LAST,
		ID_FILE_OPEN_MENUITEM,
		ID_FILE_REOPEN_MENUITEM,
		ID_FILE_SAVE_MENUITEM,
		ID_FILE_SAVE_AS_MENUITEM,
		ID_FILE_CLOSE_MENUITEM,
		ID_FILE_REVERT_MENUITEM,
		ID_FILE_RECORD_MENUITEM,

		ID_FILE_QUIT_MENUITEM,

		ID_ABOUT_MENUITEM,


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
		ID_SHUTTLE_DIAL_SPRING_BUTTON,
		ID_SHUTTLE_DIAL_SCALE_BUTTON,

		// used for key bindings
		ID_SEEK_NORMAL,
		ID_SEEK_LEFT,
		ID_SEEK_MODIFY,
		ID_SEEK_RIGHT,


		ID_CENTER_START_POS,
		ID_CENTER_STOP_POS,

		ID_NOTES_MENUITEM,

		ID_UNDO_MENUITEM,
		ID_CLEAR_UNDO_HISTORY_MENUITEM,

		ID_DEFRAG_MENUITEM,
		ID_PRINT_SAT_MENUITEM,
		ID_VERIFY_SAT_MENUITEM,

		ID_FOLLOW_PLAY_POSITION_BUTTON,

		ID_CROSSFADE_EDGES_COMBOBOX,
		ID_CROSSFADE_EDGES_SETTINGS,

		ID_CLIPBOARD_COMBOBOX,

		ID_SOUND_LIST,
		ID_SOUND_LIST_HOTKEY,

		ID_LAST
	};

	void rebuildSoundWindowList();
	FXComposite *getParentOfSoundWindows() { return soundWindowFrame; }

	CMetersWindow *getMetersWindow() { return metersWindow; }

protected:

	CMainWindow() {}


private:

	FXMenuBar 		*menubar;
	FXPacker		*contents;		// top horizontal main frame which contains play controls and action controls
	FXPacker 		*soundWindowFrame; 	// parent of all sound windows (only one is visible though)
	CMetersWindow		*metersWindow;

	FXFont *shuttleFont;
	FXFont *soundListFont;
	FXFont *soundListHeaderFont;

	FXDial *shuttleDial;
	FXToggleButton *shuttleDialSpringButton;
	FXButton *shuttleDialScaleButton;

	FXCheckButton	*followPlayPositionButton;
	FXComboBox	*crossfadeEdgesComboBox;
	FXComboBox	*clipboardComboBox; // ??? it would however make sense to put this on the edit dialog.. it's just a little wide

	FXIconList *soundList;

	FXCursor *playMouseCursor;

};



#endif
