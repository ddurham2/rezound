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
#include <vector>
#include <string>

#include <fox/fx.h>

class CSoundWindow;
class CMetersWindow;
class CActionMenuCommand;

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

	long onRenderClippingWarningButton(FXObject *sender,FXSelector sel,void *ptr);

	long onCrossfadeEdgesComboBox(FXObject *sender,FXSelector sel,void *ptr);
	long onCrossfadeEdgesSettings(FXObject *sender,FXSelector sel,void *ptr);

	long onClipboardComboBox(FXObject *sender,FXSelector sel,void *ptr);

	// file action events
	long onFileAction(FXObject *sender,FXSelector sel,void *ptr);

	// play/record/transport/misc control events
	long onControlAction(FXObject *sender,FXSelector sel,void *ptr);

	long onShuttleReturn(FXObject *sender,FXSelector sel,void *ptr);
	long onShuttleChange(FXObject *sender,FXSelector sel,void *ptr);
	void positionShuttleGivenSpeed(double seekSpeed,const string shuttleControlScalar,bool springBack);
	long onShuttleDialSpringButton(FXObject *sender,FXSelector sel,void *ptr);
	long onShuttleDialScaleButton(FXObject *sender,FXSelector sel,void *ptr);

	long onSoundListChange(FXObject *sender,FXSelector sel,void *ptr);
	long onSoundListHotKey(FXObject *sender,FXSelector sel,void *ptr);

	long onHotKeyFocusFixup(FXObject *sender,FXSelector sel,void *ptr);

	// used to control the shuttle control with the keyboard
	long onKeyboardShuttle(FXObject *sender,FXSelector sel,void *ptr);

	long onViewKey(FXObject *sender,FXSelector sel,void *ptr); // main window gets view-change key presses because the sound window changes and we can't bind a key to any particualar object pointer

	long onKeyPress(FXObject *sender,FXSelector sel,void *ptr);
	long onKeyRelease(FXObject *sender,FXSelector sel,void *ptr);
	long onMouseEnter(FXObject *sender,FXSelector sel,void *ptr);

	long onDebugButton(FXObject *sender,FXSelector sel,void *ptr);


	enum
	{
		ID_NEW_FILE=FXMainWindow::ID_LAST,
		ID_OPEN_FILE,
		ID_REOPEN_FILE,
		ID_SAVE_FILE,
		ID_SAVE_FILE_AS,
		ID_CLOSE_FILE,
		ID_REVERT_FILE,

		ID_EDIT_USERNOTES,

		ID_RECENT_ACTION,

		ID_SHOW_ABOUT,

		ID_QUIT,

		ID_RECORD,

		ID_PLAY_ALL_ONCE,
		ID_PLAY_ALL_LOOPED,
		ID_PLAY_SELECTION_ONCE,
		ID_PLAY_SELECTION_LOOPED,
		ID_PLAY_SELECTION_LOOPED_SKIP_MOST,
		ID_PLAY_SELECTION_LOOPED_GAP_BEFORE_REPEAT,

		ID_STOP,
		ID_PAUSE,

		ID_JUMP_TO_BEGINNING,
		ID_JUMP_TO_SELECTION_START,
	
		ID_JUMP_TO_PREV_CUE,
		ID_JUMP_TO_NEXT_CUE,
	
		ID_SHUTTLE_DIAL,
		ID_SHUTTLE_DIAL_SPRING_BUTTON,
		ID_SHUTTLE_DIAL_SCALE_BUTTON,

		// used for key bindings
		ID_SHUTTLE_RETURN,
		ID_SHUTTLE_BACKWARD,
		ID_SHUTTLE_INCREASE_RATE,
		ID_SHUTTLE_FORWARD,


		ID_FIND_SELECTION_START,
		ID_FIND_SELECTION_STOP,

		ID_ZOOM_IN,
		ID_ZOOM_FIT_SELECTION,
		ID_ZOOM_OUT,
		ID_ZOOM_OUT_FULL,

		ID_TOGGLE_LEVEL_METERS,
		ID_TOGGLE_FREQUENCY_ANALYZER,

		ID_UNDO_EDIT,
		ID_CLEAR_UNDO_HISTORY,

		ID_REDRAW,

		ID_DEFRAG_MENUITEM,
		ID_PRINT_SAT_MENUITEM,
		ID_VERIFY_SAT_MENUITEM,

		ID_FOLLOW_PLAY_POSITION_TOGGLE,

		ID_RENDER_CLIPPING_WARNING_TOGGLE,

		ID_CROSSFADE_EDGES_COMBOBOX,
		ID_CROSSFADE_EDGES_SETTINGS_BUTTON,

		ID_CLIPBOARD_COMBOBOX,

		ID_SOUND_LIST,
		ID_SOUND_LIST_HOTKEY,

		ID_LAST
	};

	void rebuildSoundWindowList();
	FXComposite *getParentOfSoundWindows() { return soundWindowFrame; }

	CMetersWindow *getMetersWindow() { return metersWindow; }

	void actionMenuCommandTriggered(CActionMenuCommand *actionMenuCommand);

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
	FXCheckButton	*renderClippingWarningButton;
	FXComboBox	*crossfadeEdgesComboBox;
	FXComboBox	*clipboardComboBox; // ??? it would however make sense to put this on the edit dialog.. it's just a little wide

	FXIconList *soundList;

	FXCursor *playMouseCursor;

	FXMenuCommand *toggleLevelMetersMenuItem;
	FXMenuCommand *toggleFrequencyAnalyzerMenuItem;

	FXMenuPane *recentActionsMenu;

	friend class CRecentActionsPopup;
	vector<CActionMenuCommand *> recentActions;

};



#endif
