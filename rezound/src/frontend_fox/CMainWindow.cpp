/* vim:nowrap
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

#include "CMainWindow.h"

#include <stdexcept>
#include <algorithm>
#include <map>
#include <string>

#include <CPath.h>

#include "CActionMenuCommand.h"

#include "CSoundFileManager.h"

#include <CNestedDataFile/CNestedDataFile.h>
#include "settings.h"

#include "CFOXIcons.h"

#include "CAboutDialog.h"

#include "../backend/main_controls.h"

#include "../backend/CLoadedSound.h"
#include "../backend/AAction.h"
#include "../backend/ASoundClipboard.h"

#include "../backend/CSoundPlayerChannel.h"

#include "../backend/File/COpenAudioFileAction.h"

#include "CSoundWindow.h"

#include "CMetersWindow.h"

#include "CUserNotesDialog.h"
#include "CCrossfadeEdgesDialog.h"

#include "CKeyBindingsDialog.h"

#include "rememberShow.h"

#define RECORDING_MACRO_TIMER_INTERVAL 1000

extern void enableAutoRepeat(void *display,bool enable); // QQQ defined in main.cpp

/* TODO:
 * 	- it is necesary for the owner to specifically delete the FXMenuPane objects it creates
 */

FXDEFMAP(CMainWindow) CMainWindowMap[]=
{
	//Message_Type				ID							Message_Handler
	FXMAPFUNC(SEL_CLOSE,			0,							CMainWindow::onQuit),

		// file actions
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_REOPEN_FILE,				CMainWindow::onFileAction),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_CLOSE_FILE,				CMainWindow::onFileAction),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_REVERT_FILE,				CMainWindow::onFileAction),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_RECORD_MACRO,				CMainWindow::onFileAction),
	FXMAPFUNC(SEL_TIMEOUT,			CMainWindow::ID_RECORDING_MACRO_TIMER,			CMainWindow::onRecordingMacroTimer),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_EDIT_USERNOTES,				CMainWindow::onFileAction),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SHOW_ABOUT,				CMainWindow::onFileAction),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SETUP_KEY_BINDINGS,			CMainWindow::onSetupKeyBindings),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_QUIT,					CMainWindow::onQuit),
	

		// play/record/transport controls
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_RECORD,					CMainWindow::onControlAction),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_PLAY_ALL_ONCE,				CMainWindow::onControlAction),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_PLAY_ALL_LOOPED,			CMainWindow::onControlAction),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_PLAY_SELECTION_ONCE,			CMainWindow::onControlAction),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_PLAY_SELECTION_START_TO_END,		CMainWindow::onControlAction),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_PLAY_SELECTION_LOOPED,			CMainWindow::onControlAction),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_PLAY_SELECTION_LOOPED_SKIP_MOST,	CMainWindow::onControlAction),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_PLAY_SELECTION_LOOPED_GAP_BEFORE_REPEAT,CMainWindow::onControlAction),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_PLAY_LEFT_EDGE_OF_SCREEN_TO_END,	CMainWindow::onControlAction),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_STOP,					CMainWindow::onControlAction),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_PAUSE,					CMainWindow::onControlAction),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_JUMP_TO_BEGINNING,			CMainWindow::onControlAction),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_JUMP_TO_SELECTION_START,		CMainWindow::onControlAction),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_JUMP_TO_NEXT_CUE,			CMainWindow::onControlAction),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_JUMP_TO_PREV_CUE,			CMainWindow::onControlAction),

	FXMAPFUNC(SEL_LEFTBUTTONRELEASE,	CMainWindow::ID_SHUTTLE_DIAL,				CMainWindow::onShuttleReturn),
	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,	CMainWindow::ID_SHUTTLE_DIAL,				CMainWindow::onShuttleReturn),
	FXMAPFUNC(SEL_CHANGED,			CMainWindow::ID_SHUTTLE_DIAL,				CMainWindow::onShuttleChange),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SHUTTLE_DIAL_SPRING_BUTTON,		CMainWindow::onShuttleDialSpringButton),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SHUTTLE_DIAL_SCALE_BUTTON,		CMainWindow::onShuttleDialScaleButton),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SHUTTLE_RETURN,				CMainWindow::onShuttleReturn),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SHUTTLE_BACKWARD,			CMainWindow::onKeyboardShuttle),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SHUTTLE_INCREASE_RATE,			CMainWindow::onKeyboardShuttle),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SHUTTLE_FORWARD,			CMainWindow::onKeyboardShuttle),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_FIND_SELECTION_START,			CMainWindow::onControlAction),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_FIND_SELECTION_STOP,			CMainWindow::onControlAction),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_ZOOM_IN,				CMainWindow::onControlAction),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_ZOOM_FIT_SELECTION,			CMainWindow::onControlAction),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_ZOOM_OUT,				CMainWindow::onControlAction),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_ZOOM_OUT_FULL,				CMainWindow::onControlAction),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_TOGGLE_LEVEL_METERS,			CMainWindow::onControlAction),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_TOGGLE_STEREO_PHASE_METERS,		CMainWindow::onControlAction),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_TOGGLE_FREQUENCY_ANALYZER,		CMainWindow::onControlAction),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_UNDO_EDIT,				CMainWindow::onControlAction),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_CLEAR_UNDO_HISTORY,			CMainWindow::onControlAction),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_REDRAW,					CMainWindow::onControlAction),
	
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_DEFRAG_MENUITEM,			CMainWindow::onDebugButton),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_PRINT_SAT_MENUITEM,			CMainWindow::onDebugButton),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_VERIFY_SAT_MENUITEM,			CMainWindow::onDebugButton),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_FOLLOW_PLAY_POSITION_TOGGLE,		CMainWindow::onFollowPlayPositionButton),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_RENDER_CLIPPING_WARNING_TOGGLE,		CMainWindow::onRenderClippingWarningButton),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_DRAW_VERTICAL_CUE_POSITIONS_TOGGLE,	CMainWindow::onDrawVerticalCuePositionsButton),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_CROSSFADE_EDGES_COMBOBOX,		CMainWindow::onCrossfadeEdgesComboBox),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_CROSSFADE_EDGES_SETTINGS_BUTTON,	CMainWindow::onCrossfadeEdgesSettings),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_CLIPBOARD_COMBOBOX,			CMainWindow::onClipboardComboBox),

	FXMAPFUNC(SEL_CHANGED,			CMainWindow::ID_SOUND_LIST,				CMainWindow::onSoundListChange),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SOUND_LIST_HOTKEY_PREV,			CMainWindow::onSoundListHotKey),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SOUND_LIST_HOTKEY1,			CMainWindow::onSoundListHotKey),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SOUND_LIST_HOTKEY2,			CMainWindow::onSoundListHotKey),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SOUND_LIST_HOTKEY3,			CMainWindow::onSoundListHotKey),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SOUND_LIST_HOTKEY4,			CMainWindow::onSoundListHotKey),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SOUND_LIST_HOTKEY5,			CMainWindow::onSoundListHotKey),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SOUND_LIST_HOTKEY6,			CMainWindow::onSoundListHotKey),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SOUND_LIST_HOTKEY7,			CMainWindow::onSoundListHotKey),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SOUND_LIST_HOTKEY8,			CMainWindow::onSoundListHotKey),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SOUND_LIST_HOTKEY9,			CMainWindow::onSoundListHotKey),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SOUND_LIST_HOTKEY10,			CMainWindow::onSoundListHotKey),

	FXMAPFUNC(SEL_KEYPRESS,			CMainWindow::ID_SOUND_LIST,				CMainWindow::onHotKeyFocusFixup),
	FXMAPFUNC(SEL_KEYPRESS,			CMainWindow::ID_CROSSFADE_EDGES_COMBOBOX,		CMainWindow::onHotKeyFocusFixup),
	FXMAPFUNC(SEL_KEYPRESS,			CMainWindow::ID_CLIPBOARD_COMBOBOX,			CMainWindow::onHotKeyFocusFixup),

	FXMAPFUNC(SEL_KEYPRESS,			0,							CMainWindow::onKeyPress),
	FXMAPFUNC(SEL_KEYRELEASE,		0,							CMainWindow::onKeyRelease),
	FXMAPFUNC(SEL_ENTER,			0,							CMainWindow::onMouseEnter),
};

FXIMPLEMENT(CMainWindow,FXMainWindow,CMainWindowMap,ARRAYNUMBER(CMainWindowMap))

#include "drawPortion.h" // for backgroundColor

#include "custom_cursors.h"

CMainWindow::CMainWindow(FXApp* a) :
	FXMainWindow(a,"ReZound",FOXIcons->icon_logo_32,FOXIcons->icon_logo_16,DECOR_ALL,10,20,799,600, 0,0,0,0, 0,0),
	shuttleFont(NULL),
	soundListFont(NULL),
	soundListHeaderFont(NULL),

	playMouseCursor(NULL),

	toggleLevelMetersMenuItem(NULL),
	toggleStereoPhaseMetersMenuItem(NULL),
	toggleFrequencyAnalyzerMenuItem(NULL)
{
					// I'm aware of these two memory leaks, but I'm not concerned
	playMouseCursor=new FXCursor(a,bytesToBits(playMouseCursorSource,16*16),bytesToBits(playMouseCursorMask,16*16),16,16,14,8);
	playMouseCursor->create();

	FXFontDesc d;

	menubar=new FXMenuBar(this,LAYOUT_SIDE_TOP|LAYOUT_FILL_X|FRAME_RAISED|FRAME_THICK,0,0,0,0, 0,0,0,0);
	dummymenu=new FXMenuPane(this);

	contents=new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 1,0);

	metersWindow=new CMetersWindow(contents);

	FXPacker *s,*t,*u;

	s=new FXHorizontalFrame(contents,LAYOUT_FILL_X|FRAME_RAISED|FRAME_THICK, 0,0,0,0, 0,0,0,0, 2,0);

	#define BUTTON_STYLE FRAME_RAISED|LAYOUT_EXPLICIT
	// build play control buttons
	FXPacker *playControlsFrame=new FXPacker(new FXPacker(s,LAYOUT_FILL_Y,0,0,0,0, 4,4,2,2),LAYOUT_FILL_Y|LAYOUT_FIX_WIDTH, 0,0,(32*5)+10/*+10 to make room for 'semitones'*/,0, 0,0,0,0, 0,0);
		#define PLAY_CONTROLS_BUTTON_STYLE BUTTON_STYLE
		new FXButton(playControlsFrame,FXString("\t")+_("Play All Once"),FOXIcons->play_all_once,this,ID_PLAY_ALL_ONCE,PLAY_CONTROLS_BUTTON_STYLE, 32,0,32,32);
		new FXButton(playControlsFrame,FXString("\t")+_("Play Selection Once"),FOXIcons->play_selection_once,this,ID_PLAY_SELECTION_ONCE,PLAY_CONTROLS_BUTTON_STYLE, 32+32,0,32,32);
		new FXButton(playControlsFrame,FXString("\t")+_("Play From Selection Start to End"),FOXIcons->play_selection_start_to_end,this,ID_PLAY_SELECTION_START_TO_END,PLAY_CONTROLS_BUTTON_STYLE, 32+32+32,0,32,32);
		new FXButton(playControlsFrame,FXString("\t")+_("Play From Left Edge of Screen to End"),FOXIcons->play_screen,this,ID_PLAY_LEFT_EDGE_OF_SCREEN_TO_END,PLAY_CONTROLS_BUTTON_STYLE, 32+32+32+32,0,32,32);
		new FXButton(playControlsFrame,FXString("\t")+_("Play All Looped"),FOXIcons->play_all_looped,this,ID_PLAY_ALL_LOOPED,PLAY_CONTROLS_BUTTON_STYLE, 32,32,32,32);
		new FXButton(playControlsFrame,FXString("\t")+_("Play Selection Looped"),FOXIcons->play_selection_looped,this,ID_PLAY_SELECTION_LOOPED,PLAY_CONTROLS_BUTTON_STYLE, 32+32,32,32,32);
		new FXButton(playControlsFrame,FXString("\t")+_("Play Selection Looped but Skip Most of the Middle"),FOXIcons->play_selection_looped_skip_most,this,ID_PLAY_SELECTION_LOOPED_SKIP_MOST,PLAY_CONTROLS_BUTTON_STYLE, 32+32+32,32,32,32);
		new FXButton(playControlsFrame,FXString("\t")+_("Play Selection Looped and Play a Gap Before Repeating"),FOXIcons->play_selection_looped_gap_before_repeat,this,ID_PLAY_SELECTION_LOOPED_GAP_BEFORE_REPEAT,PLAY_CONTROLS_BUTTON_STYLE, 32+32+32+32,32,32,32);

		new FXButton(playControlsFrame,FXString("\t")+_("Stop"),FOXIcons->stop,this,ID_STOP,PLAY_CONTROLS_BUTTON_STYLE, 0,0,32,32),
		new FXButton(playControlsFrame,FXString("\t")+_("Pause"),FOXIcons->pause,this,ID_PAUSE,PLAY_CONTROLS_BUTTON_STYLE, 0,32,32,32),

		new FXButton(playControlsFrame,FXString("\t")+_("Record"),FOXIcons->record,this,ID_RECORD,PLAY_CONTROLS_BUTTON_STYLE, 32+32+32+32,32+32,32,32),

		new FXButton(playControlsFrame,FXString("\t")+_("Jump to Beginning"),FOXIcons->jump_to_beginning,this,ID_JUMP_TO_BEGINNING,PLAY_CONTROLS_BUTTON_STYLE, 0,32+32,32+32,16);
		new FXButton(playControlsFrame,FXString("\t")+_("Jump to Start Position"),FOXIcons->jump_to_selection,this,ID_JUMP_TO_SELECTION_START,PLAY_CONTROLS_BUTTON_STYLE, 32+32,32+32,32+32,16);

		new FXButton(playControlsFrame,FXString("\t")+_("Jump to Previous Cue"),FOXIcons->jump_to_previous_q,this,ID_JUMP_TO_PREV_CUE,PLAY_CONTROLS_BUTTON_STYLE, 0,32+32+16,32+32,16);
		new FXButton(playControlsFrame,FXString("\t")+_("Jump to Next Cue"),FOXIcons->jump_to_next_q,this,ID_JUMP_TO_NEXT_CUE,PLAY_CONTROLS_BUTTON_STYLE, 32+32,32+32+16,32+32,16);

		// shuttle controls
		t=new FXHorizontalFrame(playControlsFrame,FRAME_NONE | LAYOUT_FIX_X|LAYOUT_FIX_Y, 0,32+32+16+16,0,0, 0,0,0,0, 0,0);
			u=new FXVerticalFrame(t,FRAME_NONE, 0,0,0,0, 0,0,0,0, 0,0);

			shuttleFont=getApp()->getNormalFont();
			shuttleFont->getFontDesc(d);
			d.weight=FONTWEIGHT_LIGHT;
			d.size=65;
			shuttleFont=new FXFont(getApp(),d);

			shuttleDial=new FXDial(u,this,ID_SHUTTLE_DIAL,DIAL_HORIZONTAL|DIAL_HAS_NOTCH|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_CENTER_Y, 0,0,32+32+32+32,20);
			shuttleDial->setRange(-((shuttleDial->getWidth())/2),(shuttleDial->getWidth())/2);
			shuttleDial->setRevolutionIncrement(shuttleDial->getWidth()*2-1);
			shuttleDial->setTipText(_("Shuttle Seek While Playing\n(right-click to return to middle)"));

			shuttleLabel=new FXLabel(u,"1x",NULL,LAYOUT_FILL_X | JUSTIFY_CENTER_X, 0,0,0,0, 0,0,0,0);
			shuttleLabel->setFont(shuttleFont);

			u=new FXVerticalFrame(t,FRAME_NONE,0,0,0,0, 0,0,0,0, 0,0);
				shuttleDialSpringButton=new FXToggleButton(u,_("free"),_("spring"),NULL,NULL,this,ID_SHUTTLE_DIAL_SPRING_BUTTON,LAYOUT_FILL_X|LAYOUT_SIDE_TOP | JUSTIFY_NORMAL | TOGGLEBUTTON_TOOLBAR | FRAME_RAISED,0,0,0,0, 1,1,0,0);
				shuttleDialSpringButton->setTipText(_("Set the Shuttle Wheel to Spring Back to the Middle or Not"));
				shuttleDialSpringButton->setState(true);
				shuttleDialSpringButton->setFont(shuttleFont);

				shuttleDialScaleButton=new FXButton(u,FXString("100x\t")+_("Set the Maximum Rate Change of the Shuttle Wheel"),NULL,this,ID_SHUTTLE_DIAL_SCALE_BUTTON,LAYOUT_FILL_X|LAYOUT_SIDE_TOP | JUSTIFY_NORMAL | TOGGLEBUTTON_TOOLBAR | FRAME_RAISED,0,0,0,0, 1,1,0,0);
				shuttleDialScaleButton->setFont(shuttleFont);

	new FXVerticalSeparator(s);

	// build miscellaneous buttons
	FXPacker *miscControlsFrame=new FXPacker(new FXPacker(s,LAYOUT_FILL_Y,0,0,0,0, 4,4,2,2),LAYOUT_FILL_Y|LAYOUT_FILL_X, 0,0,0,0, 0,0,0,0, 3,2);
		//t=new FXHorizontalFrame(miscControlsFrame,0, 0,0,0,0, 0,0,0,0);
		followPlayPositionButton=new FXCheckButton(miscControlsFrame,_("Follow Play Position"),this,ID_FOLLOW_PLAY_POSITION_TOGGLE);
		followPlayPositionButton->setPadLeft(0); followPlayPositionButton->setPadRight(0); followPlayPositionButton->setPadTop(0); followPlayPositionButton->setPadBottom(0);
		renderClippingWarningButton=new FXCheckButton(miscControlsFrame,_("Clipping Warning"),this,ID_RENDER_CLIPPING_WARNING_TOGGLE);
		renderClippingWarningButton->setPadLeft(0); renderClippingWarningButton->setPadRight(0); renderClippingWarningButton->setPadTop(0); renderClippingWarningButton->setPadBottom(0);
		drawVerticalCuePositionsButton=new FXCheckButton(miscControlsFrame,_("Draw Vertical Cue Positions"),this,ID_DRAW_VERTICAL_CUE_POSITIONS_TOGGLE);
		drawVerticalCuePositionsButton->setPadLeft(0); drawVerticalCuePositionsButton->setPadRight(0); drawVerticalCuePositionsButton->setPadTop(0); drawVerticalCuePositionsButton->setPadBottom(0);
		t=new FXHorizontalFrame(miscControlsFrame,0, 0,0,0,0, 0,0,0,0);
			crossfadeEdgesComboBox=new FXComboBox(t,8,this,ID_CROSSFADE_EDGES_COMBOBOX, FRAME_SUNKEN|FRAME_THICK | COMBOBOX_NORMAL|COMBOBOX_STATIC | LAYOUT_CENTER_Y);
				crossfadeEdgesComboBox->setNumVisible(3);
				crossfadeEdgesComboBox->setTipText(_("After Most Actions a Crossfade can be Performed at the Start and Stop \nPositions to Give a Smoother Transition in to and out of the Modified Selection"));
				crossfadeEdgesComboBox->appendItem(_("No Crossfade"));
				crossfadeEdgesComboBox->appendItem(_("Crossfade Inner Edges"));
				crossfadeEdgesComboBox->appendItem(_("Crossfade Outer Edges"));
				crossfadeEdgesComboBox->setCurrentItem(0);
			new FXButton(t,FXString("...\t")+_("Change Crossfade Times"),NULL,this,ID_CROSSFADE_EDGES_SETTINGS_BUTTON, BUTTON_NORMAL & ~FRAME_THICK);
		clipboardComboBox=new FXComboBox(miscControlsFrame,8,this,ID_CLIPBOARD_COMBOBOX, FRAME_SUNKEN|FRAME_THICK | COMBOBOX_NORMAL|COMBOBOX_STATIC);
		clipboardComboBox->setNumVisible(8);
		recordMacroButton=new FXButton(miscControlsFrame,(_("Record Macro")+string(" ")).c_str(),FOXIcons->OffLED1,this,ID_RECORD_MACRO, BUTTON_NORMAL|ICON_BEFORE_TEXT);
			// have to 'create' both icons before using them
			FOXIcons->OffLED1->create();
			FOXIcons->RedLED1->create();


	new FXVerticalSeparator(s);

	// build sound list 
	t=new FXPacker(s,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0, 4,4,2,3, 0,0);
		t=new FXPacker(t,LAYOUT_FILL_X|LAYOUT_FILL_Y | FRAME_SUNKEN|FRAME_THICK, 0,0,0,0, 0,0,0,0, 0,0);
			soundList=new FXIconList(t,this,ID_SOUND_LIST,HSCROLLER_NEVER|ICONLIST_BROWSESELECT|LAYOUT_FILL_X|LAYOUT_FILL_Y);

				soundListFont=getApp()->getNormalFont();
				shuttleFont->getFontDesc(d);
				d.weight=FONTWEIGHT_NORMAL;
				d.size=80;
				soundListFont=new FXFont(getApp(),d);

				soundList->setFont(soundListFont);

				soundListHeaderFont=getApp()->getNormalFont();
				shuttleFont->getFontDesc(d);
				d.weight=FONTWEIGHT_BOLD;
				d.size=80;
				soundListHeaderFont=new FXFont(getApp(),d);

				soundList->getHeader()->setFont(soundListHeaderFont);
				soundList->getHeader()->setPadLeft(2);
				soundList->getHeader()->setPadRight(2);
				soundList->getHeader()->setPadTop(0);
				soundList->getHeader()->setPadBottom(0);

				soundList->appendHeader(" #",NULL,25);
				soundList->appendHeader("M",NULL,20);
				soundList->appendHeader(_("Name"),NULL,200);
				soundList->appendHeader(_("Path"),NULL,9999);

	soundWindowFrame=new FXPacker(contents,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_RAISED|FRAME_THICK,0,0,0,0, 0,0,0,0, 0,0);

	reopenActionFactory=new COpenAudioFileActionFactory(NULL);
}

CMainWindow::~CMainWindow()
{
	delete playMouseCursor;
	delete shuttleFont;
	delete soundListFont;
	delete soundListHeaderFont;
	delete reopenActionFactory;
}

void CMainWindow::show()
{
	FXint wantedWidth=getDefaultWidth();
	FXint wantedHeight=getDefaultHeight();
	rememberShow(this,"ReZound");
	resize(max(getWidth(),wantedWidth),max(getHeight(),wantedHeight));

	FXMainWindow::show();

	followPlayPositionButton->setCheck(gFollowPlayPosition);

	renderClippingWarningButton->setCheck(gRenderClippingWarning);

	drawVerticalCuePositionsButton->setCheck(gDrawVerticalCuePositions);

	if(gCrossfadeEdges>=cetNone && gCrossfadeEdges<=cetOuter)
		crossfadeEdgesComboBox->setCurrentItem((FXint)gCrossfadeEdges);
	else
		crossfadeEdgesComboBox->setCurrentItem(0);


	// populate combo box to select clipboard
	for(size_t t=0;t<AAction::clipboards.size();t++)
		clipboardComboBox->appendItem(AAction::clipboards[t]->getDescription().c_str());

	if(gWhichClipboard>=AAction::clipboards.size())
		gWhichClipboard=1;

	clipboardComboBox->setCurrentItem(gWhichClipboard);

#if REZ_FOX_VERSION>=10119
	dynamic_cast<FXMenuCheck *>(toggleLevelMetersMenuItem)->setCheck(gLevelMetersEnabled);
	dynamic_cast<FXMenuCheck *>(toggleStereoPhaseMetersMenuItem)->setCheck(gStereoPhaseMetersEnabled);
	dynamic_cast<FXMenuCheck *>(toggleFrequencyAnalyzerMenuItem)->setCheck(gFrequencyAnalyzerEnabled);
#else // older than 1.1.19 used FXMenuCommand
	if(gLevelMetersEnabled)
		toggleLevelMetersMenuItem->check();
	if(gStereoPhaseMetersEnabled)
		toggleStereoPhaseMetersMenuItem->check();
	if(gFrequencyAnalyzerEnabled)
		toggleFrequencyAnalyzerMenuItem->check();
#endif

}

void CMainWindow::hide()
{
	rememberHide(this,"ReZound");
	FXMainWindow::hide();
}

void CMainWindow::rebuildSoundWindowList()
{
	FXint xpos,ypos;
	soundList->getPosition(xpos,ypos);

	soundList->clearItems();
	for(size_t t=0;t<gSoundFileManager->getOpenedCount();t++)
	{
		CSoundWindow *win=gSoundFileManager->getSoundWindow(t);

		// add to sound window list 
		CPath p(win->loadedSound->getFilename());

		soundList->appendItem(
			(
			istring(t+1,2,false)+"\t"+
			(win->loadedSound->sound->isModified() ? "*" : "")+"\t"+
			p.baseName()+"\t"+
			p.dirName()
			).c_str(),
			NULL,NULL,win);
	}

	soundList->forceRefresh();

	soundList->setPosition(xpos,ypos); // scroll to the same position we were at before

	// if it isn't visible make the active sound visible in the sound list
	CSoundWindow *active=gSoundFileManager->getActiveWindow();
	if(active!=NULL)
	{
		for(FXint t=0;t<soundList->getNumItems();t++)
		{
			if(soundList->getItemData(t)==(void *)active)
			{
				soundList->setCurrentItem(t);
				soundList->makeItemVisible(t);
				break;
			}
		}
	}

}

long CMainWindow::onSoundListChange(FXObject *sender,FXSelector sel,void *ptr)
{
	FXint index=(FXint)ptr;

	if(index>=0 && index<soundList->getNumItems())
	{
		((CSoundWindow *)soundList->getItemData(index))->setActiveState(true);

		if(AActionFactory::macroRecorder.isRecording())
		{
			Warning(_("You have changed the selected audio file while a macro is being recorded to position: ")+istring(index+1)+"\n\n"+_("Note that when the macro is played back the audio file loaded in the same position in the loaded sounds list will be selected at this step in the macro."));
			AActionFactory::macroRecorder.pushActiveSoundChange(index);
		}
	}


	return 1;
}

extern CSoundWindow *previousActiveWindow;
long CMainWindow::onSoundListHotKey(FXObject *sender,FXSelector _sel,void *ptr)
{
	FXEvent *ev=(FXEvent *)ptr;
	int sel=FXSELID(_sel);
	
	if(sel==ID_SOUND_LIST_HOTKEY_PREV)
	{ // switch to previously active window
		if(previousActiveWindow!=NULL)
		{
			int index=0;
			for(;index<soundList->getNumItems();index++)
			{ // find the index in the sound list of the previous active window so we can set the current item
				if(((CSoundWindow *)soundList->getItemData(index))==previousActiveWindow)
					break;
			}
			soundList->setCurrentItem(index);
			soundList->makeItemVisible(soundList->getCurrentItem());
			return onSoundListChange(NULL,0,(void *)index);
		}
		return 1;
	}
	else
	{
		FXint index=0;
		switch(sel) // not assuming enum assigned continous increasing values (although it would probably be a safe assumption)
		{
			case ID_SOUND_LIST_HOTKEY1: index=0; break;
			case ID_SOUND_LIST_HOTKEY2: index=1; break;
			case ID_SOUND_LIST_HOTKEY3: index=2; break;
			case ID_SOUND_LIST_HOTKEY4: index=3; break;
			case ID_SOUND_LIST_HOTKEY5: index=4; break;
			case ID_SOUND_LIST_HOTKEY6: index=5; break;
			case ID_SOUND_LIST_HOTKEY7: index=6; break;
			case ID_SOUND_LIST_HOTKEY8: index=7; break;
			case ID_SOUND_LIST_HOTKEY9: index=8; break;
			case ID_SOUND_LIST_HOTKEY10: index=9; break;
	 		default: return 0;
		}

		if(index<soundList->getNumItems())
		{
			soundList->setCurrentItem(index);
			soundList->makeItemVisible(soundList->getCurrentItem());
			return onSoundListChange(NULL,0,(void *)index);
		}
		else
			return 0;
	}
}

/*
	This handler steals the key press events from the soundList FXIconList, clipboard 
	FXComboBox, and the crossfade method FXComboBox because when focused they will take 
	all my accelerator keys and search the handle them rather than pass them on to be 
	handled by the accelerator table.  So, I steal all the keys except for keys like 
	up, down, tab, et al.
	
*/

long CMainWindow::onHotKeyFocusFixup(FXObject *sender,FXSelector sel,void *ptr)
{
	switch(((FXEvent*)ptr)->code)
	{
	case KEY_Up:
	case KEY_KP_Up:
	case KEY_Down:
	case KEY_KP_Down:

	case KEY_Page_Up:
	case KEY_KP_Page_Up:
	case KEY_Page_Down:
	case KEY_KP_Page_Down:

	case KEY_Home:
	case KEY_KP_Home:
	case KEY_End:
	case KEY_KP_End:

	case KEY_Tab:
	case KEY_KP_Tab:
		return 0;
	}

	// kill the focus (so the generated event won't pass the event back to the 
	// sender), then generate a key press event to the main window, then set 
	// the focus again on the sender
	dynamic_cast<FXWindow *>(sender)->killFocus();
	this->handle(sender,FXSEL(SEL_KEYPRESS,0),ptr);
	static_cast<FXWindow *>(sender)->setFocus();
	return 1;
}






// --- stuff for handling that pressing ctrl should cause a play cursor to show on the wave canvas ---------------

// goofy.. I don't like this, but it's the easiest thing I can think to do right now.. any other way seems to be graceless or rediculus
#include "FXWaveCanvas.h"
void setMouseCursorForFXWaveCanvas(FXWindow *p,FXCursor *cursor)
{
	if(dynamic_cast<FXWaveCanvas *>(p)!=NULL)
	{
		static_cast<FXWaveCanvas *>(p)->setDefaultCursor(cursor);
		static_cast<FXWaveCanvas *>(p)->setDragCursor(cursor);
	}
	else
	{
		for(int t=0;t<p->numChildren();t++)
			setMouseCursorForFXWaveCanvas(p->childAtIndex(t),cursor);
	}
}

long CMainWindow::onKeyPress(FXObject *sender,FXSelector sel,void *ptr)
{
	if(((FXEvent *)ptr)->code==KEY_Control_L || ((FXEvent *)ptr)->code==KEY_Control_R)
		// set play cursor
		setMouseCursorForFXWaveCanvas(soundWindowFrame,playMouseCursor);

	return FXMainWindow::handle(sender,sel,ptr); // behave as normal, just intercept ctrl presses
}

long CMainWindow::onKeyRelease(FXObject *sender,FXSelector sel,void *ptr)
{
	if(((FXEvent *)ptr)->code==KEY_Control_L || ((FXEvent *)ptr)->code==KEY_Control_R)
		// unset play cursor
		setMouseCursorForFXWaveCanvas(soundWindowFrame,getApp()->getDefaultCursor(DEF_ARROW_CURSOR));

	return FXMainWindow::handle(sender,sel,ptr); // behave as normal, just intercept ctrl releases
}

long CMainWindow::onMouseEnter(FXObject *sender,FXSelector sel,void *ptr)
{
	FXint dummy;
	FXuint keyboardModifierState;
	getCursorPosition(dummy,dummy,keyboardModifierState);

	if(keyboardModifierState&CONTROLMASK)
		// set play cursor
		setMouseCursorForFXWaveCanvas(soundWindowFrame,playMouseCursor);
	else
		// unset play cursor
		setMouseCursorForFXWaveCanvas(soundWindowFrame,getApp()->getDefaultCursor(DEF_ARROW_CURSOR));

	return 1;
}

// ---------------------------------------------------------------------------------------------------------------






extern const string escapeAmpersand(const string i); // defined in CStatusComm.cpp

/*
   This is the class for the reopen submenu.  It intercepts calls to FXMenuPane::popup ()
   so it can create the menu items which can change between each popup.
*/
class CReopenPopup : public FXMenuPane
{
public:
	CReopenPopup(FXWindow *owner) :
		FXMenuPane(owner)
	{
	}

	virtual ~CReopenPopup()
	{
	}

	virtual void popup(FXWindow* grabto, FXint x, FXint y, FXint w=0, FXint h=0)
	{
		// clear from previous popup 
		// I can't do this on popdown because the event won't have happened yet needing the menu item's text for the filename)
		for(size_t t=0;t<items.size();t++)
			delete items[t];
		items.clear();

		// create menu items
		size_t reopenSize=gSoundFileManager->getReopenHistorySize();
		if(reopenSize<=0)
			return;
		for(size_t t=0;t<reopenSize;t++)
		{
			FXMenuCommand *item=new FXMenuCommand(this,escapeAmpersand(gSoundFileManager->getReopenHistoryItem(t)).c_str(),NULL,getOwner(),CMainWindow::ID_REOPEN_FILE);
			item->create();
			items.push_back(item);
		}

		FXMenuPane::popup(grabto,x,y,w,h);
	}

	vector<FXMenuCommand *> items;
};

/*
   This is the class for the recent action submenu.  It intercepts calls to FXMenuPane::popup ()
   so it can create the menu items which can change between each popup.
*/
class CRecentActionsPopup : public FXMenuPane
{
public:
	CRecentActionsPopup(FXWindow *owner) :
		FXMenuPane(owner)
	{
		mainWindow=(CMainWindow *)owner;
	}

	virtual ~CRecentActionsPopup()
	{
	}

	virtual void popup(FXWindow* grabto, FXint x, FXint y, FXint w=0, FXint h=0)
	{
		// clear from previous popup 
		// I can't do this on popdown because the event won't have happened yet needing the menu item's text for the filename)
		for(size_t t=0;t<items.size();t++)
			delete items[t];
		items.clear();

		// create menu items
		const vector<CActionMenuCommand *> &recentActions=mainWindow->recentActions;
		if(recentActions.size()<=0)
			return;
		for(size_t t=0;t<recentActions.size();t++)
		{
			CActionMenuCommand *item=new CActionMenuCommand(this,*recentActions[t]);
			item->create();
			items.push_back(item);
		}

		FXMenuPane::popup(grabto,x,y,w,h);
	}

	vector<CActionMenuCommand *> items;
	CMainWindow *mainWindow;
};

void CMainWindow::actionMenuCommandTriggered(CActionMenuCommand *actionMenuCommand)
{
	for(vector<CActionMenuCommand *>::iterator i=recentActions.begin();i!=recentActions.end();i++)
	{
		if((*i)->getText()==actionMenuCommand->getText())
		{
			CActionMenuCommand *t=*i;
			recentActions.erase(i);
			recentActions.insert(recentActions.begin(),t);
			return;
		}
	}

	if(recentActions.size()>=10)
		recentActions.pop_back();
	recentActions.insert(recentActions.begin(),actionMenuCommand);
}

void CMainWindow::setWhichClipboard(size_t whichClipboard)
{
	if(whichClipboard<AAction::clipboards.size())
	{
		clipboardComboBox->setCurrentItem(whichClipboard);
		gWhichClipboard=whichClipboard;
	}
}

#include "CChannelSelectDialog.h"
#include "CPasteChannelsDialog.h"

#include "../backend/CRunMacroAction.h"

#include "../backend/File/FileActions.h"
#include "FileActionDialogs.h"

#include "../backend/Edits/EditActions.h"
#include "EditActionDialogs.h"

#include "../backend/Effects/EffectActions.h"
#include "EffectActionDialogs.h"

#include "../backend/Filters/FilterActions.h"
#include "FilterActionDialogs.h"

#include "../backend/Looping/LoopingActions.h"
#include "LoopingActionDialogs.h"

#include "../backend/Remaster/RemasterActions.h"
#include "RemasterActionDialogs.h"

#include "../backend/Generate/GenerateActions.h"
#include "GenerateActionDialogs.h"

#include "../backend/LADSPA/LADSPAActions.h"


static const string stripAmpersand(const string str)
{
	string stripped;
	for(size_t t=0;t<str.length();t++)
		if(str[t]!='&') stripped+=str[t];
	return stripped;
}

static const string stripElipsis(const string str)
{
	if(str.rfind("...")==str.length()-3 && str.length()>3/*don't strip if that's the whole string*/)
		return str.substr(0,str.length()-3);
	return str;
}


/* the usual case -- adding a CActionMenuCommand to the menu item registry */
static void addToActionMap(CActionMenuCommand *item,map<const string,FXMenuCaption *> &menuItemRegistry)
{
		// I use the text right off the item because I don't want to pass a separate string to addToActionMap() just for the name
	const string strippedItemName=stripElipsis(stripAmpersand(item->getUntranslatedText()));
	if(menuItemRegistry.find(strippedItemName)!=menuItemRegistry.end()) // something a developer would want to know
		printf("NOTE: duplicate item name in menu item registry '%s'\n",strippedItemName.c_str());
	menuItemRegistry[strippedItemName]=item;
}

/* the less usual case -- adding a general menu item to the menu item registry */
static void addToActionMap(const char *itemText,FXMenuCaption *item,map<const string,FXMenuCaption *> &menuItemRegistry)
{ 	/* because I can't get the original text of the menu item with the hotkey in it, I pass it in separately and set the menuitem's text to it (prepended to what was already there).*/

		// I use the text right off the item because I don't want to pass a separate string to addToActionMap() just for the name
	const string strippedItemName=stripElipsis(stripAmpersand(string(itemText)+item->getText().text()));
	if(menuItemRegistry.find(strippedItemName)!=menuItemRegistry.end()) // something a developer would want to know
		printf("NOTE: duplicate item name in menu item registry '%s'\n",strippedItemName.c_str());

	if(itemText[0]) // not blank
		item->setText(gettext(itemText)+item->getText());

	menuItemRegistry[strippedItemName]=item;
}

// this registry is used to know what menu items exist that we might set key bindings for
map<string,FXMenuCommand *> gKeyBindingRegistry; // ??? could make this a datamemeber

void CMainWindow::addActionToKeyBindingRegistery(const string name,FXMenuCommand *mc)
{
	gKeyBindingRegistry[name]=mc;
}

void CMainWindow::buildActionMap() 
{
	// This initializes the menu action map, creating the collection of all available menu actions.
	// This allows menus to be dynamically laid out using a configuration file.
	
	// I know it's very wide, but it's (supposed to be) neat (tabs need to be set to 8 for it to look right)

	// File 
	addToActionMap(								new CActionMenuCommand(new CNewAudioFileActionFactory(new CNewAudioFileActionDialog(this)),dummymenu,							FOXIcons->file_new),													menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new COpenAudioFileActionFactory(new COpenAudioFileActionDialog(this)),dummymenu,							FOXIcons->file_open),													menuItemRegistry);
	addToActionMap(N_("&Reopen"),						new FXMenuCascade(dummymenu,														"",		FOXIcons->file_open,						new CReopenPopup(this)),				menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CSaveAudioFileActionFactory,dummymenu,											FOXIcons->file_save),													menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CSaveAsAudioFileActionFactory(new CSaveAsAudioFileActionDialog(this)),dummymenu,						FOXIcons->file_save_as),												menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CSaveSelectionAsActionFactory(),dummymenu,											FOXIcons->file_save_as),												menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CSaveAsMultipleFilesActionFactory(new CSaveAsMultipleFilesDialog(this)),dummymenu,						FOXIcons->file_save_as),												menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CBurnToCDActionFactory(new CBurnToCDDialog(this)),dummymenu,									FOXIcons->file_burn),													menuItemRegistry);
		// not converting close or revert to AAction classes because they pull the sound object out from under the calling code in AAction
	addToActionMap(N_("&Close"),						new FXMenuCommand(dummymenu,														"",		FOXIcons->file_close,						this,	ID_CLOSE_FILE),					menuItemRegistry);
	addToActionMap(N_("Re&vert"),						new FXMenuCommand(dummymenu,														"",		FOXIcons->file_revert,						this,	ID_REVERT_FILE),				menuItemRegistry);
	// -
	addToActionMap(								new CActionMenuCommand(new CRunMacroActionFactory(new CRunMacroDialog(this)),dummymenu,									NULL),															menuItemRegistry);
	addToActionMap(N_("Record Macro..."),					new FXMenuCommand(dummymenu,														"",		FOXIcons->normal_action_buff,					this,	ID_RECORD_MACRO),				menuItemRegistry);
	// -
	addToActionMap(N_("User No&tes..."),					new FXMenuCommand(dummymenu,														"",		FOXIcons->notes,						this,	ID_EDIT_USERNOTES),				menuItemRegistry);
	// -
	addToActionMap(N_("&About ReZound"),					new FXMenuCommand(dummymenu,														"",		NULL,								this,	ID_SHOW_ABOUT),					menuItemRegistry);
	addToActionMap(N_("&Setup Hotkeys..."),					new FXMenuCommand(dummymenu,														"",		NULL,								this,	ID_SETUP_KEY_BINDINGS),				menuItemRegistry);
	// -
	addToActionMap("- Just for testing",					new FXMenuCaption(dummymenu,														""),																	menuItemRegistry);
	addToActionMap("Defrag",						new FXMenuCommand(dummymenu,														"",		NULL,								this,	ID_DEFRAG_MENUITEM),				menuItemRegistry);
	addToActionMap("PrintSAT",						new FXMenuCommand(dummymenu,														"",		NULL,								this,	ID_PRINT_SAT_MENUITEM),				menuItemRegistry);
	addToActionMap("VerifySAT",						new FXMenuCommand(dummymenu,														"",		NULL,								this,	ID_VERIFY_SAT_MENUITEM),			menuItemRegistry);
	// -
	addToActionMap(N_("&Quit"),						new FXMenuCommand(dummymenu,														"",		FOXIcons->exit,							this,	ID_QUIT),					menuItemRegistry);


	// Control
	addToActionMap(N_("Record..."),						new FXMenuCommand(dummymenu,														"",		FOXIcons->small_record,						this,	ID_RECORD),					menuItemRegistry);
	addToActionMap(N_("Play All Once"),					new FXMenuCommand(dummymenu,														"",		FOXIcons->small_play_all_once,					this,	ID_PLAY_ALL_ONCE),				menuItemRegistry);
	addToActionMap(N_("Play All Looped"),					new FXMenuCommand(dummymenu,														"",		FOXIcons->small_play_all_looped,				this,	ID_PLAY_ALL_LOOPED),				menuItemRegistry);
	addToActionMap(N_("Play Selection Once"),				new FXMenuCommand(dummymenu,														"",		FOXIcons->small_play_selection_once,				this,	ID_PLAY_SELECTION_ONCE),			menuItemRegistry);
	addToActionMap(N_("Play From Left Edge of Screen to End"),		new FXMenuCommand(dummymenu,														"",		FOXIcons->small_play_screen,	 				this,	ID_PLAY_LEFT_EDGE_OF_SCREEN_TO_END),		menuItemRegistry);
	addToActionMap(N_("Play From Selection Start to End"),			new FXMenuCommand(dummymenu,														"",		FOXIcons->small_play_selection_start_to_end,			this,	ID_PLAY_SELECTION_START_TO_END),		menuItemRegistry);
	addToActionMap(N_("Play Selection Looped"),				new FXMenuCommand(dummymenu,														"",		FOXIcons->small_play_selection_looped,				this,	ID_PLAY_SELECTION_LOOPED),			menuItemRegistry);
	addToActionMap(N_("Loop Selection but Skip Most of the Middle"),	new FXMenuCommand(dummymenu,														"",		FOXIcons->small_play_selection_looped_skip_most,		this,	ID_PLAY_SELECTION_LOOPED_SKIP_MOST),		menuItemRegistry);
	addToActionMap(N_("Loop Selection and Play a Gap Before Repeating"),	new FXMenuCommand(dummymenu,														"",		FOXIcons->small_play_selection_looped_gap_before_repeat,	this,	ID_PLAY_SELECTION_LOOPED_GAP_BEFORE_REPEAT),	menuItemRegistry);
	addToActionMap(N_("Stop"),						new FXMenuCommand(dummymenu,														"",		FOXIcons->small_stop,						this,	ID_STOP),					menuItemRegistry);
	addToActionMap(N_("Pause"),						new FXMenuCommand(dummymenu,														"",		FOXIcons->small_pause,						this,	ID_PAUSE),					menuItemRegistry);
	// -
	addToActionMap(N_("Jump to Beginning"),					new FXMenuCommand(dummymenu,														"",		FOXIcons->small_jump_to_beginning,				this,	ID_JUMP_TO_BEGINNING),				menuItemRegistry);
	addToActionMap(N_("Jump to Selection Start"),				new FXMenuCommand(dummymenu,														"",		FOXIcons->small_jump_to_selection,				this,	ID_JUMP_TO_SELECTION_START),			menuItemRegistry);
	addToActionMap(N_("Jump to Previous Cue"),				new FXMenuCommand(dummymenu,														"",		FOXIcons->small_jump_to_previous_q,				this,	ID_JUMP_TO_PREV_CUE),				menuItemRegistry);
	addToActionMap(N_("Jump to Next Cue"),					new FXMenuCommand(dummymenu,														"",		FOXIcons->small_jump_to_next_q,					this,	ID_JUMP_TO_NEXT_CUE),				menuItemRegistry);
	// -
	addToActionMap(N_("Shuttle Rewind"),					new FXMenuCommand(dummymenu,														"",		FOXIcons->shuttle_backward,					this,	ID_SHUTTLE_BACKWARD),				menuItemRegistry);
	addToActionMap(N_("Shuttle Amount"),					new FXMenuCommand(dummymenu,														"",		FOXIcons->shuttle_normal,					this,	ID_SHUTTLE_INCREASE_RATE),			menuItemRegistry);
	addToActionMap(N_("Shuttle Forward"),					new FXMenuCommand(dummymenu,														"",		FOXIcons->shuttle_forward,					this,	ID_SHUTTLE_FORWARD),				menuItemRegistry);


	// View Controls
	addToActionMap(N_("Zoom Out F&ull"),					new FXMenuCommand(dummymenu,														"",		FOXIcons->zoom_out_full,					this,	ID_ZOOM_OUT_FULL),				menuItemRegistry);
	addToActionMap(N_("Zoom &Out"),						new FXMenuCommand(dummymenu,														"",		FOXIcons->zoom_out,						this,	ID_ZOOM_OUT),					menuItemRegistry);
	addToActionMap(N_("Zoom &In"),						new FXMenuCommand(dummymenu,														"",		FOXIcons->zoom_in,						this,	ID_ZOOM_IN),					menuItemRegistry);
	addToActionMap(N_("Zoom &Fit Selection"),				new FXMenuCommand(dummymenu,														"",		FOXIcons->zoom_fit,						this,	ID_ZOOM_FIT_SELECTION),				menuItemRegistry);
	// -
	addToActionMap(N_("Find &Start Position"),				new FXMenuCommand(dummymenu,														"",		FOXIcons->normal_action_buff,					this,	ID_FIND_SELECTION_START),			menuItemRegistry);
	addToActionMap(N_("Find Sto&p Position"),				new FXMenuCommand(dummymenu,														"",		FOXIcons->normal_action_buff,					this,	ID_FIND_SELECTION_STOP),			menuItemRegistry);
	// -
	addToActionMap(N_("&Redraw"),						new FXMenuCommand(dummymenu,														"",		FOXIcons->normal_action_buff,					this,	ID_REDRAW),					menuItemRegistry);
	// -
#if REZ_FOX_VERSION>=10119
	addToActionMap(N_("Toggle &Level Meters"),				toggleLevelMetersMenuItem=new FXMenuCheck(dummymenu,											"",										this,	ID_TOGGLE_LEVEL_METERS),			menuItemRegistry);
	addToActionMap(N_("Toggle &Stereo Phase Meters"),			toggleStereoPhaseMetersMenuItem=new FXMenuCheck(dummymenu,										"",										this,	ID_TOGGLE_STEREO_PHASE_METERS),			menuItemRegistry);
	addToActionMap(N_("Toggle Frequency &Analyzer"),			toggleFrequencyAnalyzerMenuItem=new FXMenuCheck(dummymenu,										"",										this,	ID_TOGGLE_FREQUENCY_ANALYZER),			menuItemRegistry);
#else // older than 1.1.19 used FXMenuCommand
	addToActionMap(N_("Toggle &Level Meters"),				toggleLevelMetersMenuItem=new FXMenuCommand(dummymenu,											"",		NULL,								this,	ID_TOGGLE_LEVEL_METERS),			menuItemRegistry);
	addToActionMap(N_("Toggle &Stereo Phase Meters"),			toggleStereoPhaseMetersMenuItem=new FXMenuCommand(dummymenu,										"",		NULL,								this,	ID_TOGGLE_STEREO_PHASE_METERS),			menuItemRegistry);
	addToActionMap(N_("Toggle Frequency &Analyzer"),			toggleFrequencyAnalyzerMenuItem=new FXMenuCommand(dummymenu,										"",		NULL,								this,	ID_TOGGLE_FREQUENCY_ANALYZER),			menuItemRegistry);
#endif
	// -
	addToActionMap(N_("Switch to Previously Viewed File"),			new FXMenuCommand(dummymenu,														"",		NULL,								this,	ID_SOUND_LIST_HOTKEY_PREV),			menuItemRegistry);
	addToActionMap(N_("Switch to Loaded File 1"),				new FXMenuCommand(dummymenu,														"",		NULL,								this,	ID_SOUND_LIST_HOTKEY1),				menuItemRegistry);
	addToActionMap(N_("Switch to Loaded File 2"),				new FXMenuCommand(dummymenu,														"",		NULL,								this,	ID_SOUND_LIST_HOTKEY2),				menuItemRegistry);
	addToActionMap(N_("Switch to Loaded File 3"),				new FXMenuCommand(dummymenu,														"",		NULL,								this,	ID_SOUND_LIST_HOTKEY3),				menuItemRegistry);
	addToActionMap(N_("Switch to Loaded File 4"),				new FXMenuCommand(dummymenu,														"",		NULL,								this,	ID_SOUND_LIST_HOTKEY4),				menuItemRegistry);
	addToActionMap(N_("Switch to Loaded File 5"),				new FXMenuCommand(dummymenu,														"",		NULL,								this,	ID_SOUND_LIST_HOTKEY5),				menuItemRegistry);
	addToActionMap(N_("Switch to Loaded File 6"),				new FXMenuCommand(dummymenu,														"",		NULL,								this,	ID_SOUND_LIST_HOTKEY6),				menuItemRegistry);
	addToActionMap(N_("Switch to Loaded File 7"),				new FXMenuCommand(dummymenu,														"",		NULL,								this,	ID_SOUND_LIST_HOTKEY7),				menuItemRegistry);
	addToActionMap(N_("Switch to Loaded File 8"),				new FXMenuCommand(dummymenu,														"",		NULL,								this,	ID_SOUND_LIST_HOTKEY8),				menuItemRegistry);
	addToActionMap(N_("Switch to Loaded File 9"),				new FXMenuCommand(dummymenu,														"",		NULL,								this,	ID_SOUND_LIST_HOTKEY9),				menuItemRegistry);
	addToActionMap(N_("Switch to Loaded File 10"),				new FXMenuCommand(dummymenu,														"",		NULL,								this,	ID_SOUND_LIST_HOTKEY10),			menuItemRegistry);


	// Edit
	addToActionMap(N_("Undo"),						new FXMenuCommand(dummymenu,														"",		FOXIcons->edit_undo,						this,	ID_UNDO_EDIT),					menuItemRegistry);
	addToActionMap(N_("Clear Undo History"),				new FXMenuCommand(dummymenu,														"",		NULL,								this,	ID_CLEAR_UNDO_HISTORY),				menuItemRegistry);
	// -
	addToActionMap(N_("&Recent Actions"),					new FXMenuCascade(dummymenu,														"",		NULL,								new CRecentActionsPopup(this)),				menuItemRegistry);
	// -
	addToActionMap(								new CActionMenuCommand(new CCopyEditFactory(gChannelSelectDialog),dummymenu,										FOXIcons->edit_copy),													menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CCopyToNewEditFactory(gChannelSelectDialog),dummymenu,									FOXIcons->edit_copy),													menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CCutEditFactory(gChannelSelectDialog),dummymenu,										FOXIcons->edit_cut),													menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CCutToNewEditFactory(gChannelSelectDialog),dummymenu,									FOXIcons->edit_cut),													menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CDeleteEditFactory(gChannelSelectDialog),dummymenu,										FOXIcons->edit_delete),													menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CCropEditFactory(gChannelSelectDialog),dummymenu,										FOXIcons->edit_crop),													menuItemRegistry);
	// -
	addToActionMap(								new CActionMenuCommand(new CInsertPasteEditFactory(gPasteChannelsDialog),dummymenu,									FOXIcons->edit_paste),													menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CReplacePasteEditFactory(gPasteChannelsDialog),dummymenu,									FOXIcons->edit_paste),													menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new COverwritePasteEditFactory(gPasteChannelsDialog),dummymenu,									FOXIcons->edit_paste),													menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CLimitedOverwritePasteEditFactory(gPasteChannelsDialog),dummymenu,								FOXIcons->edit_paste),													menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CMixPasteEditFactory(gPasteChannelsDialog),dummymenu,									FOXIcons->edit_paste),													menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CLimitedMixPasteEditFactory(gPasteChannelsDialog),dummymenu,									FOXIcons->edit_paste),													menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CFitMixPasteEditFactory(gPasteChannelsDialog),dummymenu,									FOXIcons->edit_paste),													menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CPasteAsNewEditFactory,dummymenu,												FOXIcons->edit_paste),													menuItemRegistry);
	// -
	addToActionMap(								new CActionMenuCommand(new CInsertSilenceEditFactory(gChannelSelectDialog,new CInsertSilenceDialog(this)),dummymenu,					NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CMuteEditFactory(gChannelSelectDialog),dummymenu,										NULL),															menuItemRegistry);
	// -
	addToActionMap(								new CActionMenuCommand(new CAddChannelsEditFactory(new CAddChannelsDialog(this)),dummymenu,								NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CDuplicateChannelEditFactory(new CDuplicateChannelDialog(this)),dummymenu,							NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CRemoveChannelsEditFactory(gChannelSelectDialog),dummymenu,									NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CSwapChannelsEditFactory(new CSwapChannelsDialog(this)),dummymenu,								NULL),															menuItemRegistry);
	// -
	addToActionMap(								new CActionMenuCommand(new CRotateLeftEditFactory(gChannelSelectDialog,new CRotateDialog(this)),dummymenu,						NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CRotateRightEditFactory(gChannelSelectDialog,new CRotateDialog(this)),dummymenu,						NULL),															menuItemRegistry);
	// -
	addToActionMap(								new CActionMenuCommand(new CSelectionEditFactory(sSelectAll),dummymenu,											NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CGrowOrSlideSelectionEditFactory(new CGrowOrSlideSelectionDialog(this)),dummymenu,						NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CSelectionEditFactory(sSelectToBeginning),dummymenu,										NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CSelectionEditFactory(sSelectToEnd),dummymenu,										NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CSelectionEditFactory(sFlopToBeginning),dummymenu,										NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CSelectionEditFactory(sFlopToEnd),dummymenu,											NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CSelectionEditFactory(sSelectToSelectStart),dummymenu,									NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CSelectionEditFactory(sSelectToSelectStop),dummymenu,									NULL),															menuItemRegistry);


	// Effects
	addToActionMap(								new CActionMenuCommand(new CReverseEffectFactory(gChannelSelectDialog),dummymenu,									NULL),															menuItemRegistry);
	// -
	addToActionMap(								new CActionMenuCommand(new CChangeVolumeEffectFactory(gChannelSelectDialog,new CNormalVolumeChangeDialog(this)),dummymenu,				NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CSimpleGainEffectFactory(gChannelSelectDialog,new CNormalGainDialog(this)),dummymenu,					NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CCurvedGainEffectFactory(gChannelSelectDialog,new CAdvancedGainDialog(this)),dummymenu,					NULL),															menuItemRegistry);
	// -
	addToActionMap(								new CActionMenuCommand(new CSimpleChangeRateEffectFactory(gChannelSelectDialog,new CNormalRateChangeDialog(this)),dummymenu,				NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CCurvedChangeRateEffectFactory(gChannelSelectDialog,new CAdvancedRateChangeDialog(this)),dummymenu,				NULL),															menuItemRegistry);
	// -
	addToActionMap(								new CActionMenuCommand(new CFlangeEffectFactory(gChannelSelectDialog,new CFlangeDialog(this)),dummymenu,						NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CSimpleDelayEffectFactory(gChannelSelectDialog,new CSimpleDelayDialog(this)),dummymenu,					NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CQuantizeEffectFactory(gChannelSelectDialog,new CQuantizeDialog(this)),dummymenu,						NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CDistortionEffectFactory(gChannelSelectDialog,new CDistortionDialog(this)),dummymenu,					NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CVariedRepeatEffectFactory(gChannelSelectDialog,new CVariedRepeatDialog(this)),dummymenu,					NULL),															menuItemRegistry);

	addToActionMap(								new CActionMenuCommand(new CTestEffectFactory(gChannelSelectDialog),dummymenu,										NULL),															menuItemRegistry);


	// Filter
	addToActionMap(								new CActionMenuCommand(new CConvolutionFilterFactory(gChannelSelectDialog,new CConvolutionFilterDialog(this)),dummymenu,				NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CArbitraryFIRFilterFactory(gChannelSelectDialog,new CArbitraryFIRFilterDialog(this)),dummymenu,				FOXIcons->filter_custom),												menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CMorphingArbitraryFIRFilterFactory(gChannelSelectDialog,new CMorphingArbitraryFIRFilterDialog(this)),dummymenu,		FOXIcons->filter_custom),												menuItemRegistry);
	// -
	addToActionMap(								new CActionMenuCommand(new CSinglePoleLowpassFilterFactory(gChannelSelectDialog,new CSinglePoleLowpassFilterDialog(this)),dummymenu,			FOXIcons->filter_lowpass),												menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CSinglePoleHighpassFilterFactory(gChannelSelectDialog,new CSinglePoleHighpassFilterDialog(this)),dummymenu,			FOXIcons->filter_highpass),												menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CBandpassFilterFactory(gChannelSelectDialog,new CBandpassFilterDialog(this)),dummymenu,					FOXIcons->filter_bandpass),												menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CNotchFilterFactory(gChannelSelectDialog,new CNotchFilterDialog(this)),dummymenu,						FOXIcons->filter_notch),												menuItemRegistry);
	// -
	addToActionMap(								new CActionMenuCommand(new CBiquadResLowpassFilterFactory(gChannelSelectDialog,new CBiquadResLowpassFilterDialog(this)),dummymenu,			FOXIcons->filter_lowpass),												menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CBiquadResHighpassFilterFactory(gChannelSelectDialog,new CBiquadResHighpassFilterDialog(this)),dummymenu,			FOXIcons->filter_highpass),												menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CBiquadResBandpassFilterFactory(gChannelSelectDialog,new CBiquadResBandpassFilterDialog(this)),dummymenu,			FOXIcons->filter_bandpass),												menuItemRegistry);


	// Looping
	addToActionMap(								new CActionMenuCommand(new CMakeSymetricActionFactory(gChannelSelectDialog),dummymenu,									NULL),															menuItemRegistry);
	// -
	addToActionMap(								new CActionMenuCommand(new CAddNCuesActionFactory(new CAddNCuesDialog(this)),dummymenu,									NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CAddTimedCuesActionFactory(new CAddTimedCuesDialog(this)),dummymenu,								NULL),															menuItemRegistry);


	// Remaster
	addToActionMap(								new CActionMenuCommand(new CSimpleBalanceActionFactory(NULL,new CSimpleBalanceActionDialog(this)),dummymenu,						NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CCurvedBalanceActionFactory(NULL,new CCurvedBalanceActionDialog(this)),dummymenu,						NULL),															menuItemRegistry);
	// -
	addToActionMap(								new CActionMenuCommand(new CMonoizeActionFactory(NULL,new CMonoizeActionDialog(this)),dummymenu,							NULL),															menuItemRegistry);
	// -
	addToActionMap(								new CActionMenuCommand(new CNoiseGateActionFactory(gChannelSelectDialog,new CNoiseGateDialog(this)),dummymenu,						NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CCompressorActionFactory(gChannelSelectDialog,new CCompressorDialog(this)),dummymenu,					NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CNormalizeActionFactory(gChannelSelectDialog,new CNormalizeDialog(this)),dummymenu,						NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CAdaptiveNormalizeActionFactory(gChannelSelectDialog,new CAdaptiveNormalizeDialog(this)),dummymenu,				NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CMarkQuietAreasActionFactory(new CMarkQuietAreasDialog(this)),dummymenu,							NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CShortenQuietAreasActionFactory(new CShortenQuietAreasDialog(this)),dummymenu,							NULL),															menuItemRegistry);
	// -
	addToActionMap(								new CActionMenuCommand(new CResampleActionFactory(gChannelSelectDialog,new CResampleDialog(this)),dummymenu,						NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CChangePitchActionFactory(gChannelSelectDialog,new CChangePitchDialog(this)),dummymenu,					NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CChangeTempoActionFactory(gChannelSelectDialog,new CChangeTempoDialog(this)),dummymenu,					NULL),															menuItemRegistry);
	// -
	addToActionMap(								new CActionMenuCommand(new CRemoveDCActionFactory(gChannelSelectDialog),dummymenu,									NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CInvertPhaseActionFactory(gChannelSelectDialog),dummymenu,									NULL),															menuItemRegistry);
	// -
	addToActionMap(								new CActionMenuCommand(new CUnclipActionFactory(gChannelSelectDialog),dummymenu,									NULL),															menuItemRegistry);

	// Generate
	addToActionMap(								new CActionMenuCommand(new CGenerateNoiseActionFactory(gChannelSelectDialog,new CGenerateNoiseDialog(this)),dummymenu,					NULL),															menuItemRegistry);
	addToActionMap(								new CActionMenuCommand(new CGenerateToneActionFactory(gChannelSelectDialog,new CGenerateToneDialog(this)),dummymenu,					NULL),															menuItemRegistry);



	// These are here simply so that xgettext will add entries for these in the rezound.pot file
	N_("&File");
	N_("&Edit");
	N_("Selection");
	N_("&View");
	N_("&Control");
	N_("Effec&ts");
	N_("&Looping");
	N_("F&ilters");
	N_("&Remaster");
	N_("&Generate");

	// create registry of all actions (for macros)
	gRegisteredActionFactories.clear();
	for(map<const string,FXMenuCaption *>::iterator i=menuItemRegistry.begin();i!=menuItemRegistry.end();i++)
	{
		if(dynamic_cast<CActionMenuCommand *>(i->second)!=NULL)
		{
			gRegisteredActionFactories[i->first]=((CActionMenuCommand *)i->second)->getActionFactory();
		}
	}

	// set key bindings
	for(map<const string,FXMenuCaption *>::iterator i=menuItemRegistry.begin();i!=menuItemRegistry.end();i++)
	{
		if(dynamic_cast<FXMenuCommand *>(i->second)!=NULL)
			addActionToKeyBindingRegistery(i->first,(FXMenuCommand *)i->second);
	}
		
}

void CMainWindow::buildMenu(FXMenuPane *menu,const CNestedDataFile *menuLayoutFile,const string menuKey,const string itemName)
{
	if(itemName=="-") 
	{
		if(menu) new FXMenuSeparator(menu);
		return;
	}

	// if the item is a submenu item, recur for each item in it; otherwise, add as normal menu item
	if(menuLayoutFile->keyExists(menuKey)==CNestedDataFile::ktScope)
	{	// add as a submenu
		FXMenuPane *submenu=NULL;

		const size_t DOTCount=istring(menuKey).count(CNestedDataFile::delim);
		if(DOTCount==0)
		{	// we've been passed just the layout name
			// this is the theorical place to create menubar, but for FOX's needs I have to create it in the constructor
		}
		else if(DOTCount==1)
		{	// we've been passed a top-level pull-down menu name
			submenu=new FXMenuPane(this);
			new FXMenuTitle(menubar,_(itemName.c_str()),NULL,submenu);
		}
		else if(DOTCount>1)
		{	// submenu of menu
			submenu=new FXMenuPane(this);
			new FXMenuCascade(menu,_(itemName.c_str()),NULL,submenu);
		}

		const string menuItemsKey=menuKey DOT "menuitems";
		if(menuLayoutFile->keyExists(menuItemsKey)==CNestedDataFile::ktValue)
		{
			const vector<string> menuItems=menuLayoutFile->getValue<vector<string> >(menuItemsKey);
			for(size_t t=0;t<menuItems.size();t++) 
			{
				const string name=menuItems[t];
				buildMenu(submenu,menuLayoutFile,menuKey DOT stripAmpersand(name),name);
			}
		} // else (if it's not an array) something may be screwed up in the layout definition
	}
	else
	{	// add as normal item
		const string strippedItemName=stripAmpersand(itemName);
		if(menuItemRegistry.find(strippedItemName)!=menuItemRegistry.end())
		{
			if(menuItemRegistry[strippedItemName]->getParent()!=dummymenu) // just a check
				printf("NOTE: registered menu item '%s' was mapped more than once in layout; only the last one will be effective\n",strippedItemName.c_str());
			menuItemRegistry[strippedItemName]->reparent(menu);

			// change menu name's caption if there is an alias defined: 'origMenuName="new menu name";' in the same scope as where menuitems is defined
			if(menuLayoutFile->keyExists(menuKey)==CNestedDataFile::ktValue) 
			{
				FXMenuCaption *menuItem=menuItemRegistry[strippedItemName];
				const string alias=menuLayoutFile->getValue<string>(menuKey);
				menuItem->setText(gettext(alias.c_str()));
			}
		}
		else
		{
			if(menu)
				new FXMenuCommand(menu,(itemName+" (unregistered)").c_str(),NULL,this,0);
			else
				// since menu is NULL, this is a top-level menu without a defined body
				new FXMenuTitle(menubar,(itemName+" (no subitems defined)").c_str(),NULL,new FXMenuPane(this));
		}
	}
}

#include <CNestedDataFile/CNestedDataFile.h>
void CMainWindow::createMenus()
{
	buildActionMap();	
       
	// Try to find out which set of menu information we should load and which file contains the menu information.
	// The default menu layout is 'default' in .../share/.../menu.dat

	// ??? make this a global setting instead of a lookup here 
	//	NOTE: this would be the first frontend specific global setting (but this isn't the only lookup (showAbout() also does it))
	string menuLayout;
        if(gSettingsRegistry->keyExists("MenuLayout"))
		menuLayout=gSettingsRegistry->getValue<string>("MenuLayout");
	else
		menuLayout="default";

	tryAgain:

	string menuLayoutFilename;
	if(menuLayout=="default")
		menuLayoutFilename=gSysDataDirectory+CPath::dirDelim+"menu.dat";
	else
		menuLayoutFilename=gUserDataDirectory+CPath::dirDelim+"menu.dat";
	
	const CNestedDataFile *menuLayoutFile=new CNestedDataFile(menuLayoutFilename);
	try
	{
		if(	menuLayoutFile->keyExists(menuLayout)!=CNestedDataFile::ktScope || 
			menuLayoutFile->keyExists(menuLayout DOT "menuitems")!=CNestedDataFile::ktValue)
		{
			Warning(menuLayout+".menuitems array does not exist in requested menu layout '"+ menuLayout+"' in '"+menuLayoutFilename+"'");
			menuLayout="default"; // go around again and use the default menu layout
			goto tryAgain;
		}

		buildMenu(NULL,menuLayoutFile,menuLayout,menuLayout);
		delete menuLayoutFile;
	}
	catch(...)
	{
		delete menuLayoutFile;
		throw;
	}

	// give feedback about actions that didn't get mapped to a menu on screen
	for(map<const string,FXMenuCaption *>::iterator i=menuItemRegistry.begin();i!=menuItemRegistry.end();i++)
	{
		if(i->second->getParent()==dummymenu)
			printf("NOTE: registered menu item '%s' was not mapped anywhere in '%s' in layout '%s'\n",i->first.c_str(),menuLayoutFilename.c_str(),menuLayout.c_str());
	}


	buildLADSPAMenus();

	setupKeyBindings();

	create(); // it is necessary to call create again which will call it for all new child windows
}

void CMainWindow::buildLADSPAMenus()
{
#ifdef USE_LADSPA
	// now stick the LADSPA menu in there if it needs to be
	// ??? (with dynamic menus, maybe let the layout define WHERE the ladspa submenu goes, (except it might not always be compiled for ladspa, so we wouldn't want to add it to the map if USE_LADSPA wasn't defined)
	FXMenuPane *menu=new FXMenuPane(this);
	new FXMenuTitle(menubar,"L&ADSPA",NULL,menu);
		const vector<CLADSPAActionFactory *> LADSPAActionFactories=getLADSPAActionFactories();
		if(LADSPAActionFactories.size()<=0)
		{
			new FXMenuCaption(menu,_("No LADSPA Plugins Found"));
			new FXMenuSeparator(menu);
			new FXMenuCaption(menu,_("Like PATH, set LADSPA_PATH to point"));
			new FXMenuCaption(menu,_("to a directory(s) containing LADSPA"));
			new FXMenuCaption(menu,_("plugin .so file at least once.  Or"));
			new FXMenuCaption(menu,_("edit the value in ~/.rezound/registry.dat"));
		}
		else
		{
			// register all the LADSPA action so they can be used in macros
			for(size_t t=0;t<LADSPAActionFactories.size();t++)
				gRegisteredActionFactories[LADSPAActionFactories[t]->getName()]=LADSPAActionFactories[t];
			

			// determine number of FXMenuCaption will fit on the screen (msh -> menu screen height)
			const FXuint msh=(getApp()->getRootWindow()->getHeight()/(7+getApp()->getNormalFont()->getFontHeight()))-1;

			if(LADSPAActionFactories.size()>20)
			{
				// add a submenu grouped by manufacturer
				map<const string,map<const string,CLADSPAActionFactory *> > makerGrouped;
				for(size_t t=0;t<LADSPAActionFactories.size();t++)
				{
					const string maker= LADSPAActionFactories[t]->getDescriptor()->Maker;
					makerGrouped[maker][LADSPAActionFactories[t]->getName()]=LADSPAActionFactories[t];
				}

				if(makerGrouped.size()>1)
				{ // more than one maker 
					FXMenuPane *makerMenu=new FXMenuPane(this);
					new FXMenuCascade(menu,"By Maker",NULL,makerMenu);

					for(map<const string,map<const string,CLADSPAActionFactory *> >::iterator i=makerGrouped.begin();i!=makerGrouped.end();i++)
					{
#if REZ_FOX_VERSION<10142
						FXMenuPane *submenu=new FXMenuPane(this);
#else
						// if menu will be vertically taller than the screen, then make the menu scrollable
						FXMenuPane *submenu=i->second.size()>msh ? new FXScrollPane(this,msh) : new FXMenuPane(this);
#endif
						new FXMenuCascade(makerMenu,i->first.c_str(),NULL,submenu);
	
						for(map<const string,CLADSPAActionFactory *>::iterator t=i->second.begin();t!=i->second.end();t++)
						{
							FXMenuCommand *mc=new CActionMenuCommand(t->second,submenu,NULL);
							addActionToKeyBindingRegistery(t->second->getName(),mc);
						}
					}
				}

				new FXMenuSeparator(menu);

				

				// group by the first letter
				map<const char,map<const string,CLADSPAActionFactory *> > nameGrouped;
				for(size_t t=0;t<LADSPAActionFactories.size();t++)
				{
					const char letter= *(istring(LADSPAActionFactories[t]->getName()).upper()).begin();
					nameGrouped[letter][LADSPAActionFactories[t]->getName()]=LADSPAActionFactories[t];
				}

				for(map<const char,map<const string,CLADSPAActionFactory *> >::iterator i=nameGrouped.begin();i!=nameGrouped.end();i++)
				{
#if REZ_FOX_VERSION<10142
					FXMenuPane *submenu=new FXMenuPane(this);
#else
					// if menu will be vertically taller than the screen, then make the menu scrollable
					FXMenuPane *submenu=i->second.size()>msh ? new FXScrollPane(this,msh) : new FXMenuPane(this);
#endif
					new FXMenuCascade(menu,string(&(i->first),1).c_str(),NULL,submenu);

					for(map<const string,CLADSPAActionFactory *>::iterator t=i->second.begin();t!=i->second.end();t++)
					{
						FXMenuCommand *mc=new CActionMenuCommand(t->second,submenu,NULL);
						addActionToKeyBindingRegistery(t->second->getName(),mc);
					}
				}
			}
			else
			{
				for(size_t t=0;t<LADSPAActionFactories.size();t++)
				{
					FXMenuCommand *mc=new CActionMenuCommand(LADSPAActionFactories[t],menu,NULL);
					addActionToKeyBindingRegistery(LADSPAActionFactories[t]->getName(),mc);
				}
			}
		}
#endif
}

/* this function sets a key-binding if one has been defined for the given FXMenuCommand */
void CMainWindow::setupKeyBindings()
{
	
	// clear existing accelerator table (not doing this now.. see below)
	//delete getAccelTable();
	//setAccelTable(new FXAccelTable());

	// reassign all accelerators (would prefer to wipe the slate clean before doing this, but there's no good way to restore the alt-&letter menu key bindings after doing so)
	for(map<string,FXMenuCommand *>::iterator i=gKeyBindingRegistry.begin(); i!=gKeyBindingRegistry.end(); i++)
	{
		const string name=i->first;
		FXMenuCommand *mc=i->second;

		mc->setAccelText(""); // clear any existing accel text

		if(gKeyBindingsStore->keyExists(name)==CNestedDataFile::ktValue)
		{
			string value=gKeyBindingsStore->getValue<string>(name);
			if(value=="")
				continue; // no key bound to this action

			FXHotKey acckey=fxparseAccel(value.c_str());
			mc->setAccelText(value.c_str());

			/* these shuttle controls have to be handle specially because they require key up events to return the shuttle to the middle */
			if(i->first=="Shuttle Forward")
				getAccelTable()->addAccel(acckey,this,FXSEL(SEL_COMMAND,ID_SHUTTLE_FORWARD),FXSEL(SEL_COMMAND,ID_SHUTTLE_RETURN));
			else if(i->first=="Shuttle Rewind")
				getAccelTable()->addAccel(acckey,this,FXSEL(SEL_COMMAND,ID_SHUTTLE_BACKWARD),FXSEL(SEL_COMMAND,ID_SHUTTLE_RETURN));
			else
				getAccelTable()->addAccel(acckey,mc,FXSEL(SEL_COMMAND,FXMenuCommand::ID_ACCEL));
		}
	}
}

long CMainWindow::onQuit(FXObject *sender,FXSelector sel,void *ptr)
{

	if(getApp()->getModality()==MODAL_FOR_WINDOW)
	{ // don't allow a quit if there is a modal window showing
		gStatusComm->beep();
		return 1;
	}

	if(exitReZound(gSoundFileManager))
	{
		enableAutoRepeat(getApp()->getDisplay(),true); // QQQ (didn't work just putting this after application->run() in main.cpp)

		hide();
		getApp()->exit(0);
	}
	return 1;
}

long CMainWindow::onSetupKeyBindings(FXObject *sender,FXSelector sel,void *ptr)
{
	// create a list of the keys that are currently bound because we need to remove those from the current accel table before assigning the new key bindings
	// this is done to avoid having to delete and re-create the accel table each time in setupKeyBindings() which destroys the alt-&letter key bindings that
	// already exist there.
	vector<FXHotKey> removeKeyBindingsList;
	const vector<string> actionsWithKeyBindings=gKeyBindingsStore->getChildKeys("");
	for(size_t t=0;t<actionsWithKeyBindings.size();t++)
		removeKeyBindingsList.push_back(fxparseAccel(gKeyBindingsStore->getValue<string>(actionsWithKeyBindings[t]).c_str()));

	if(gKeyBindingsDialog->showIt(gKeyBindingRegistry))
	{
		for(size_t t=0;t<removeKeyBindingsList.size();t++)
			getAccelTable()->removeAccel(removeKeyBindingsList[t]);

		setupKeyBindings();
	}
	return 1;
}

long CMainWindow::onFollowPlayPositionButton(FXObject *sender,FXSelector sel,void *ptr)
{
	gFollowPlayPosition=followPlayPositionButton->getCheck();
	return 1;
}

long CMainWindow::onRenderClippingWarningButton(FXObject *sender,FXSelector sel,void *ptr)
{
	gRenderClippingWarning=renderClippingWarningButton->getCheck();
	if(gSoundFileManager->getActiveWindow())
		gSoundFileManager->getActiveWindow()->updateFromEdit();
	return 1;
}

long CMainWindow::onDrawVerticalCuePositionsButton(FXObject *sender,FXSelector sel,void *ptr)
{
	gDrawVerticalCuePositions=drawVerticalCuePositionsButton->getCheck();
	if(gSoundFileManager->getActiveWindow())
		gSoundFileManager->getActiveWindow()->updateFromEdit();
	return 1;
}

long CMainWindow::onCrossfadeEdgesComboBox(FXObject *sender,FXSelector sel,void *ptr)
{
	gCrossfadeEdges=(CrossfadeEdgesTypes)crossfadeEdgesComboBox->getCurrentItem();
	return 1;
}

long CMainWindow::onCrossfadeEdgesSettings(FXObject *sender,FXSelector sel,void *ptr)
{
	gCrossfadeEdgesDialog->showIt();
	return 1;
}


long CMainWindow::onClipboardComboBox(FXObject *sender,FXSelector sel,void *ptr)
{
	gWhichClipboard=clipboardComboBox->getCurrentItem();
	return 1;
}


// file action events
long CMainWindow::onFileAction(FXObject *sender,FXSelector sel,void *ptr)
{
	switch(FXSELID(sel))
	{
	case ID_REOPEN_FILE:
	{
		vector<string> filenames;
		filenames.push_back(dynamic_cast<FXMenuCommand *>(sender)->getText().text());

		CActionParameters actionParameters(gSoundFileManager);
		actionParameters.setValue<vector<string> >("filenames",filenames);

		reopenActionFactory->performAction(NULL,&actionParameters,false);
		break;
	}
	
	case ID_CLOSE_FILE:
		if(AActionFactory::macroRecorder.isRecording())
		{
			Message(_("Cannot close a file while recording a macro"));
			return 1;
		}
		closeSound(gSoundFileManager);
		break;

	case ID_REVERT_FILE:
		if(AActionFactory::macroRecorder.isRecording())
		{
			Message(_("Cannot revert a file while recording a macro"));
			return 1;
		}
		revertSound(gSoundFileManager);
		break;

	case ID_RECORD_MACRO:
		recordMacro();
		if(AActionFactory::macroRecorder.isRecording())
		{
			recordMacroButton->setState(1);
			recordMacroButton->setIcon(FOXIcons->RedLED1);
			// start blinking LED timer
			getApp()->addTimeout(this,CMainWindow::ID_RECORDING_MACRO_TIMER,RECORDING_MACRO_TIMER_INTERVAL);
		}
		else
		{
			recordMacroButton->setIcon(FOXIcons->OffLED1);
			recordMacroButton->setState(0);
		}
		break;

	case ID_SHOW_ABOUT:
		gAboutDialog->execute(PLACEMENT_SCREEN);
		break;


	case ID_EDIT_USERNOTES:
#warning ??? make this an action?
		try
		{
			CLoadedSound *s=gSoundFileManager->getActive();
			if(s!=NULL)
				gUserNotesDialog->show(s,PLACEMENT_CURSOR);
			else
				getApp()->beep();
		}
		catch(exception &e)
		{
			Error(e.what());
		}
		break;

	default:
		throw runtime_error(string(__func__)+" -- unhandled file button selector");
	}
	return 1;
}

long CMainWindow::onRecordingMacroTimer(FXObject *sender,FXSelector sel,void *ptr)
{
	static int counter=0;
	if(AActionFactory::macroRecorder.isRecording())
	{
		recordMacroButton->setIcon( (counter%2) ? FOXIcons->RedLED1 : FOXIcons->OffLED1);
		counter++;

		// schedule for the next timer
		getApp()->addTimeout(this,CMainWindow::ID_RECORDING_MACRO_TIMER,RECORDING_MACRO_TIMER_INTERVAL);
	}
	else 
		counter=0; 

	return 1;
}

// play control events
long CMainWindow::onControlAction(FXObject *sender,FXSelector sel,void *ptr)
{
	switch(FXSELID(sel))
	{
	case ID_PLAY_ALL_ONCE:
		metersWindow->resetGrandMaxPeakLevels();
		play(gSoundFileManager,CSoundPlayerChannel::ltLoopNone,false);
		break;

	case ID_PLAY_ALL_LOOPED:
		metersWindow->resetGrandMaxPeakLevels();
		play(gSoundFileManager,CSoundPlayerChannel::ltLoopNormal,false);
		break;

	case ID_PLAY_SELECTION_ONCE:
		metersWindow->resetGrandMaxPeakLevels();
		play(gSoundFileManager,CSoundPlayerChannel::ltLoopNone,true);
		break;

	case ID_PLAY_SELECTION_START_TO_END:
		metersWindow->resetGrandMaxPeakLevels();
		if(gSoundFileManager->getActive())
			play(gSoundFileManager,gSoundFileManager->getActive()->channel->getStartPosition());
		break;

	case ID_PLAY_SELECTION_LOOPED:
		metersWindow->resetGrandMaxPeakLevels();
		play(gSoundFileManager,CSoundPlayerChannel::ltLoopNormal,true);
		break;

	case ID_PLAY_SELECTION_LOOPED_SKIP_MOST:
		metersWindow->resetGrandMaxPeakLevels();
		play(gSoundFileManager,CSoundPlayerChannel::ltLoopSkipMost,true);
		break;

	case ID_PLAY_SELECTION_LOOPED_GAP_BEFORE_REPEAT:
		metersWindow->resetGrandMaxPeakLevels();
		play(gSoundFileManager,CSoundPlayerChannel::ltLoopGapBeforeRepeat,true);
		break;

	case ID_PLAY_LEFT_EDGE_OF_SCREEN_TO_END:
		if(gSoundFileManager->getActiveWindow())
		{
			metersWindow->resetGrandMaxPeakLevels();
			play(gSoundFileManager,gSoundFileManager->getActiveWindow()->getLeftEdgePosition());
		}
		break;

	case ID_STOP:
		stop(gSoundFileManager);
		break;

	case ID_PAUSE:
		pause(gSoundFileManager);
		break;

	case ID_RECORD:
		if(AActionFactory::macroRecorder.isRecording())
		{
			Message(_("Cannot record audio while recording a macro"));
			return 1;
		}
		recordSound(gSoundFileManager);
		break;

	case ID_JUMP_TO_BEGINNING:
		jumpToBeginning(gSoundFileManager);
		break;

	case ID_JUMP_TO_SELECTION_START:
		jumpToStartPosition(gSoundFileManager);
		break;

	case ID_JUMP_TO_PREV_CUE:
		jumpToPreviousCue(gSoundFileManager);
		break;

	case ID_JUMP_TO_NEXT_CUE:
		jumpToNextCue(gSoundFileManager);
		break;


	case ID_UNDO_EDIT:
		undo(gSoundFileManager);
		break;

	case ID_CLEAR_UNDO_HISTORY:
		clearUndoHistory(gSoundFileManager);
		break;


	case ID_FIND_SELECTION_START:
		if(gSoundFileManager->getActiveWindow())
			gSoundFileManager->getActiveWindow()->centerStartPos();
		break;

	case ID_FIND_SELECTION_STOP:
		if(gSoundFileManager->getActiveWindow())
			gSoundFileManager->getActiveWindow()->centerStopPos();
		break;


	case ID_TOGGLE_LEVEL_METERS:
#if REZ_FOX_VERSION>=10119
		metersWindow->enableLevelMeters(dynamic_cast<FXMenuCheck *>(sender)->getCheck());
#else // older than 1.1.19 used FXMenuCommand
		if(dynamic_cast<FXMenuCommand *>(sender)->isChecked())
			dynamic_cast<FXMenuCommand *>(sender)->uncheck();
		else
			dynamic_cast<FXMenuCommand *>(sender)->check();
		metersWindow->enableLevelMeters(dynamic_cast<FXMenuCommand *>(sender)->isChecked());
#endif
		break;

	case ID_TOGGLE_STEREO_PHASE_METERS:
#if REZ_FOX_VERSION>=10119
		metersWindow->enableStereoPhaseMeters(dynamic_cast<FXMenuCheck *>(sender)->getCheck());
#else // older than 1.1.19 used FXMenuCommand
		if(dynamic_cast<FXMenuCommand *>(sender)->isChecked())
			dynamic_cast<FXMenuCommand *>(sender)->uncheck();
		else
			dynamic_cast<FXMenuCommand *>(sender)->check();
		metersWindow->enableStereoPhaseMeters(dynamic_cast<FXMenuCommand *>(sender)->isChecked());
#endif
		break;

	case ID_TOGGLE_FREQUENCY_ANALYZER:
#if REZ_FOX_VERSION>=10119
		metersWindow->enableFrequencyAnalyzer(dynamic_cast<FXMenuCheck *>(sender)->getCheck());
#else // older than 1.1.19 used FXMenuCommand
		if(dynamic_cast<FXMenuCommand *>(sender)->isChecked())
			dynamic_cast<FXMenuCommand *>(sender)->uncheck();
		else
			dynamic_cast<FXMenuCommand *>(sender)->check();
		metersWindow->enableFrequencyAnalyzer(dynamic_cast<FXMenuCommand *>(sender)->isChecked());
#endif
		break;


	case ID_ZOOM_IN:
		if(gSoundFileManager->getActiveWindow())
			gSoundFileManager->getActiveWindow()->horzZoomInSome();
		break;

	case ID_ZOOM_FIT_SELECTION:
		if(gSoundFileManager->getActiveWindow())
			gSoundFileManager->getActiveWindow()->horzZoomSelectionFit();
		break;

	case ID_ZOOM_OUT:
		if(gSoundFileManager->getActiveWindow())
			gSoundFileManager->getActiveWindow()->horzZoomOutSome();
		break;

	case ID_ZOOM_OUT_FULL:
		if(gSoundFileManager->getActiveWindow())
			gSoundFileManager->getActiveWindow()->horzZoomOutFull();
		break;


	case ID_REDRAW:
		if(gSoundFileManager->getActiveWindow())
			gSoundFileManager->getActiveWindow()->redraw();
		break;

	default:
		throw runtime_error(string(__func__)+" -- unhandled play button selector");
	}
	return 1;
}

long CMainWindow::onShuttleReturn(FXObject *sender,FXSelector sel,void *ptr)
{
	if(((FXEvent *)ptr)->code==LEFTBUTTON && !shuttleDialSpringButton->getState())
		return 1; // this wasn't a left click release and where we're in spring-back mode

	enableAutoRepeat(getApp()->getDisplay(),true); // QQQ

	// return shuttle control to the middle
	shuttleDial->setValue(0);
	onShuttleChange(NULL,0,NULL);
	return 1;
}

long CMainWindow::onShuttleChange(FXObject *sender,FXSelector sel,void *ptr)
{
	CSoundWindow *w=gSoundFileManager->getActiveWindow();
	if(w!=NULL)
	{
		CLoadedSound *s=w->loadedSound;

		string labelString;

		FXint minValue,maxValue;
		shuttleDial->getRange(minValue,maxValue);

		const FXint shuttlePos=shuttleDial->getValue();
		float seekSpeed;

		const string text=shuttleDialScaleButton->getText().text();

		if(shuttlePos==0)
		{
			if(text==_("semitones"))
				labelString="+ 0";
			else
				labelString="1x";

			seekSpeed=1.0;
		}
		else
		{
			if(text=="1x")
			{ // 1x +/- (0..1]
				if(shuttlePos>0)
					seekSpeed=(double)shuttlePos/(double)maxValue;
				else //if(shuttlePos<0)
					seekSpeed=(double)-shuttlePos/(double)minValue;
				labelString=istring(seekSpeed,4,3)+"x";
			}
			else if(text=="2x")
			{ // 2x +/- [1..2]
				if(shuttlePos>0)
					seekSpeed=(double)shuttlePos/(double)maxValue+1.0;
				else //if(shuttlePos<0)
					seekSpeed=(double)-shuttlePos/(double)minValue-1.0;
				labelString=istring(seekSpeed,3,2)+"x";
			}
			else if(text=="100x")
			{ // 100x +/- [1..100]
						// I square the value to give a more useful range
				if(shuttlePos>0)
					seekSpeed=(pow((double)shuttlePos/(double)maxValue,2.0)*99.0)+1.0;
				else //if(shuttlePos<0)
					seekSpeed=(pow((double)shuttlePos/(double)minValue,2.0)*-99.0)-1.0;
				labelString=istring(seekSpeed,4,2)+"x";
			}
			else if(text==_("semitones"))
			{ // semitone + [0.5..2]
				float semitones = round((double)shuttlePos/(double)maxValue*12); // +/- 12 semitones
				if(shuttlePos>0) 
				{
					semitones = round((double)shuttlePos/(double)maxValue*12);
					seekSpeed=pow(2.0,semitones/12.0);
				}
				else //if(shuttlePos<0)
				{
					semitones = round((double)shuttlePos/(double)minValue*12);
					seekSpeed=pow(0.5,semitones/12.0);
				}
				labelString = (semitones>=0 ? "+" : "") + istring((int)semitones,2,false) + " ("+istring(seekSpeed,3,2)+"x)";
			}
			else
				throw runtime_error(string(__func__)+" -- internal error -- unhandled text for shuttleDialScaleButton: '"+text+"'");
		}

		shuttleLabel->setText(labelString.c_str());
		w->shuttleControlScalar=shuttleDialScaleButton->getText().text();
		w->shuttleControlSpringBack=shuttleDialSpringButton->getState();
		s->channel->setSeekSpeed(seekSpeed);
	}

	return 1;
}

void CMainWindow::positionShuttleGivenSpeed(double seekSpeed,const string shuttleControlScalar,bool springBack)
{
	FXint minValue,maxValue;
	shuttleDial->getRange(minValue,maxValue);

	FXint shuttlePos;
	if(seekSpeed==1.0)
		shuttlePos=0;
	else
	{
		const string &text=shuttleControlScalar;
		if(text=="1x")
		{
			if(seekSpeed>0.0)
				shuttlePos=(FXint)(seekSpeed*maxValue);
			else //if(seekSpeed<0.0)
				shuttlePos=(FXint)(-seekSpeed*minValue);
		}
		else if(text=="2x")
		{
			if(seekSpeed>0.0)
				shuttlePos=(FXint)((seekSpeed-1.0)*maxValue);
			else //if(seekSpeed<0.0)
				shuttlePos=(FXint)(-(seekSpeed+1.0)*minValue);
		}
		else if(text=="100x")
		{
			if(seekSpeed>0.0)
				shuttlePos=(FXint)(maxValue*sqrt((seekSpeed-1.0)/100.0));
			else //if(seekSpeed<0.0)
				shuttlePos=(FXint)(minValue*sqrt((seekSpeed+1.0)/-100.0));
		}
		else if(text==_("semitones"))
		{
			if(seekSpeed>0.0)
				shuttlePos=(FXint)(log(seekSpeed)/log(2.0)*maxValue);
			else //if(seekSpeed<0.0)
				shuttlePos=(FXint)(log(seekSpeed)/log(2.0)*minValue);
		}
		else
			throw runtime_error(string(__func__)+" -- internal error -- unhandled text for shuttleDialScaleButton: '"+text+"'");
		
	}

	shuttleDialScaleButton->setText(shuttleControlScalar.c_str());
	shuttleDialSpringButton->setState(springBack);
	shuttleDial->setValue(shuttlePos);
}

long CMainWindow::onShuttleDialSpringButton(FXObject *sender,FXSelector sel,void *ptr)
{
	shuttleDialSpringButton->killFocus();
	if(shuttleDialSpringButton->getState())
	{
		// return the shuttle control to the middle
		shuttleDial->setValue(0);
		onShuttleChange(NULL,0,NULL);
	}
	return 1;
}

long CMainWindow::onShuttleDialScaleButton(FXObject *sender,FXSelector sel,void *ptr)
{
	shuttleDialScaleButton->killFocus();
	const string text=shuttleDialScaleButton->getText().text();
	if(text=="100x")
		shuttleDialScaleButton->setText("1x");
	else if(text=="1x")
		shuttleDialScaleButton->setText("2x");
	else if(text=="2x")
		shuttleDialScaleButton->setText(_("semitones"));
	else if(text==_("semitones"))
		shuttleDialScaleButton->setText("100x");
	else
		throw runtime_error(string(__func__)+" -- internal error -- unhandled text for shuttleDialScaleButton: '"+text+"'");

	// return the shuttle control to the middle
	shuttleDial->setValue(0);
	onShuttleChange(NULL,0,NULL);

	return 1;
}

long CMainWindow::onKeyboardShuttle(FXObject *sender,FXSelector sel,void *ptr)
{
	FXint lo,hi;
	shuttleDial->getRange(lo,hi);

	FXint inc= (hi-lo)/14; // 7 positions surrounding 0 

	FXint pos=shuttleDial->getValue();

	if(pos==0 && FXSELID(sel)==ID_SHUTTLE_BACKWARD)
	{
		if((unsigned)ptr > 1/*came from keyboard event*/) enableAutoRepeat(getApp()->getDisplay(),false); // QQQ
		shuttleDial->setValue(pos-inc);
		onShuttleChange(sender,sel,ptr);
	}
	else if(pos==0 && FXSELID(sel)==ID_SHUTTLE_FORWARD)
	{
		if((unsigned)ptr > 1/*came from keyboard event*/) enableAutoRepeat(getApp()->getDisplay(),false); // QQQ
		shuttleDial->setValue(pos+inc);
		onShuttleChange(sender,sel,ptr);
	}
	else if(pos!=0 && FXSELID(sel)==ID_SHUTTLE_INCREASE_RATE)
	{
		if(pos<0)
		{ // go more leftward
			shuttleDial->setValue(pos-inc);
		}
		else if(pos>0)
		{ // go more rightward
			shuttleDial->setValue(pos+inc);
		}
		onShuttleChange(sender,sel,ptr);
	}

	return 1;
}

long CMainWindow::onDebugButton(FXObject *sender,FXSelector sel,void *ptr)
{
	CLoadedSound *s=gSoundFileManager->getActive();
	if(s!=NULL)
	{
		if(FXSELID(sel)==ID_DEFRAG_MENUITEM)
		{
			s->sound->defragPoolFile();
			gSoundFileManager->updateAfterEdit();
		}
		else if(FXSELID(sel)==ID_PRINT_SAT_MENUITEM)
			s->sound->printSAT();
		else if(FXSELID(sel)==ID_VERIFY_SAT_MENUITEM)
			s->sound->verifySAT();
	}
	else
		getApp()->beep();
	
	return 1;
}

