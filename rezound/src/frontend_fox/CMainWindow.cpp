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

#include "CMainWindow.h"

#include <stdexcept>
#include <algorithm>
#include <string>

#include <CPath.h>

#include "CActionMenuCommand.h"

#include "CSoundFileManager.h"

#include "settings.h"

#include "CFOXIcons.h"

#include "CAboutDialog.h"

#include "../backend/main_controls.h"

#include "../backend/CLoadedSound.h"
#include "../backend/AAction.h"
#include "../backend/ASoundClipboard.h"

#include "../backend/CSoundPlayerChannel.h"

#include "CSoundWindow.h"

#include "CMetersWindow.h"

#include "CUserNotesDialog.h"
#include "CCrossfadeEdgesDialog.h"

#include "rememberShow.h"


/* TODO:
 * 	- put gap after repeating and other toggles
 * 
 */

FXDEFMAP(CMainWindow) CMainWindowMap[]=
{
	//Message_Type				ID						Message_Handler

	FXMAPFUNC(SEL_CLOSE,			0,						CMainWindow::onQuit),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_FILE_QUIT_MENUITEM,		CMainWindow::onQuit),
	
		// file actions
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_FILE_NEW_MENUITEM,		CMainWindow::onFileAction),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_FILE_OPEN_MENUITEM,		CMainWindow::onFileAction),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_FILE_REOPEN_MENUITEM,		CMainWindow::onFileAction),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_FILE_SAVE_MENUITEM,		CMainWindow::onFileAction),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_FILE_SAVE_AS_MENUITEM,		CMainWindow::onFileAction),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_FILE_CLOSE_MENUITEM,		CMainWindow::onFileAction),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_FILE_REVERT_MENUITEM,		CMainWindow::onFileAction),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_FILE_RECORD_MENUITEM,		CMainWindow::onFileAction),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_ABOUT_MENUITEM,			CMainWindow::onFileAction),

		// play controls
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_PLAY_ALL_ONCE_BUTTON,		CMainWindow::onPlayControlButton),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_PLAY_ALL_LOOPED_BUTTON,		CMainWindow::onPlayControlButton),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_PLAY_SELECTION_ONCE_BUTTON,	CMainWindow::onPlayControlButton),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_PLAY_SELECTION_LOOPED_BUTTON,	CMainWindow::onPlayControlButton),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_STOP_BUTTON,			CMainWindow::onPlayControlButton),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_PAUSE_BUTTON,			CMainWindow::onPlayControlButton),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_JUMP_TO_BEGINNING_BUTTON,	CMainWindow::onPlayControlButton),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_JUMP_TO_START_POSITION_BUTTON,	CMainWindow::onPlayControlButton),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_JUMP_TO_NEXT_CUE_BUTTON,	CMainWindow::onPlayControlButton),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_JUMP_TO_PREV_CUE_BUTTON,	CMainWindow::onPlayControlButton),

	FXMAPFUNC(SEL_LEFTBUTTONRELEASE,	CMainWindow::ID_SHUTTLE_DIAL,			CMainWindow::onShuttleReturn),
	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,	CMainWindow::ID_SHUTTLE_DIAL,			CMainWindow::onShuttleReturn),
	FXMAPFUNC(SEL_CHANGED,			CMainWindow::ID_SHUTTLE_DIAL,			CMainWindow::onShuttleChange),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SHUTTLE_DIAL_SPRING_BUTTON,	CMainWindow::onShuttleDialSpringButton),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SHUTTLE_DIAL_SCALE_BUTTON,	CMainWindow::onShuttleDialScaleButton),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SEEK_NORMAL,			CMainWindow::onShuttleReturn),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SEEK_LEFT,			CMainWindow::onKeyboardSeek),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SEEK_MODIFY,			CMainWindow::onKeyboardSeek),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SEEK_RIGHT,			CMainWindow::onKeyboardSeek),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_CENTER_START_POS,		CMainWindow::onViewKey),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_CENTER_STOP_POS,		CMainWindow::onViewKey),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_NOTES_MENUITEM,			CMainWindow::onUserNotesButton),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_UNDO_MENUITEM,			CMainWindow::onUndoButton),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_CLEAR_UNDO_HISTORY_MENUITEM,	CMainWindow::onClearUndoHistoryButton),
	
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_DEFRAG_MENUITEM,		CMainWindow::onDebugButton),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_PRINT_SAT_MENUITEM,		CMainWindow::onDebugButton),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_VERIFY_SAT_MENUITEM,		CMainWindow::onDebugButton),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_FOLLOW_PLAY_POSITION_BUTTON,	CMainWindow::onFollowPlayPositionButton),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_CROSSFADE_EDGES_COMBOBOX,	CMainWindow::onCrossfadeEdgesComboBox),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_CROSSFADE_EDGES_SETTINGS,	CMainWindow::onCrossfadeEdgesSettings),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_CLIPBOARD_COMBOBOX,		CMainWindow::onClipboardComboBox),

	FXMAPFUNC(SEL_CHANGED,			CMainWindow::ID_SOUND_LIST,			CMainWindow::onSoundListChange),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SOUND_LIST_HOTKEY,		CMainWindow::onSoundListHotKey),
	FXMAPFUNC(SEL_KEYPRESS,			CMainWindow::ID_SOUND_LIST,			CMainWindow::onSoundListKeyPress),
};

FXIMPLEMENT(CMainWindow,FXMainWindow,CMainWindowMap,ARRAYNUMBER(CMainWindowMap))

#include <fox/fxkeys.h>

#include "drawPortion.h" // for backgroundColor

CMainWindow::CMainWindow(FXApp* a) :
	FXMainWindow(a,"ReZound",FOXIcons->icon_logo_32,FOXIcons->icon_logo_16,DECOR_ALL,10,20,800,600, 0,0,0,0, 0,0),
	shuttleFont(NULL),
	soundListFont(NULL),
	soundListHeaderFont(NULL)
{
	FXFontDesc d;

	menubar=new FXMenuBar(this,LAYOUT_SIDE_TOP|LAYOUT_FILL_X|FRAME_RAISED|FRAME_THICK,0,0,0,0, 0,0,0,0);

	contents=new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 1,0);

	metersWindow=new CMetersWindow(contents);

	FXPacker *s,*t;

	s=new FXHorizontalFrame(contents,LAYOUT_FILL_X|FRAME_RAISED|FRAME_THICK, 0,0,0,0, 0,0,0,0, 2,0);

	#define BUTTON_STYLE FRAME_RAISED|LAYOUT_EXPLICIT
	// build play control buttons
	FXPacker *playControlsFrame=new FXPacker(new FXPacker(s,LAYOUT_FILL_Y,0,0,0,0, 4,4,2,2),LAYOUT_FILL_Y|LAYOUT_FILL_X, 0,0,0,0, 0,0,0,0, 0,0);
		#define PLAY_CONTROLS_BUTTON_STYLE BUTTON_STYLE
		new FXButton(playControlsFrame,"\tPlay All Once",FOXIcons->play_all_once,this,ID_PLAY_ALL_ONCE_BUTTON,PLAY_CONTROLS_BUTTON_STYLE, 0,0,32,32);
		new FXButton(playControlsFrame,"\tPlay Selection Once",FOXIcons->play_selection_once,this,ID_PLAY_SELECTION_ONCE_BUTTON,PLAY_CONTROLS_BUTTON_STYLE, 32,0,32,32);
		new FXButton(playControlsFrame,"\tPlay All Looped",FOXIcons->play_all_looped,this,ID_PLAY_ALL_LOOPED_BUTTON,PLAY_CONTROLS_BUTTON_STYLE, 0,32,32,32);
		new FXButton(playControlsFrame,"\tPlay Selection Looped",FOXIcons->play_selection_looped,this,ID_PLAY_SELECTION_LOOPED_BUTTON,PLAY_CONTROLS_BUTTON_STYLE, 32,32,32,32);

		new FXButton(playControlsFrame,"\tStop",FOXIcons->stop,this,ID_STOP_BUTTON,PLAY_CONTROLS_BUTTON_STYLE, 32+32,0,32,32),
		new FXButton(playControlsFrame,"\tPause",FOXIcons->pause,this,ID_PAUSE_BUTTON,PLAY_CONTROLS_BUTTON_STYLE, 32+32,32,32,32),

		new FXButton(playControlsFrame,"\tJump to Beginning",FOXIcons->jump_to_beginning,this,ID_JUMP_TO_BEGINNING_BUTTON,PLAY_CONTROLS_BUTTON_STYLE, 0,32+32,32+32,16);
		new FXButton(playControlsFrame,"\tJump to Start Position",FOXIcons->jump_to_selection,this,ID_JUMP_TO_START_POSITION_BUTTON,PLAY_CONTROLS_BUTTON_STYLE, 32+32,32+32,32+32,16);

		new FXButton(playControlsFrame,"\tJump to Previous Cue",FOXIcons->jump_to_previous_q,this,ID_JUMP_TO_PREV_CUE_BUTTON,PLAY_CONTROLS_BUTTON_STYLE, 0,32+32+16,32+32,16);
		new FXButton(playControlsFrame,"\tJump to Next Cue",FOXIcons->jump_to_next_q,this,ID_JUMP_TO_NEXT_CUE_BUTTON,PLAY_CONTROLS_BUTTON_STYLE, 32+32,32+32+16,32+32,16);

		// shuttle controls
		t=new FXHorizontalFrame(playControlsFrame,FRAME_NONE|LAYOUT_FIX_X|LAYOUT_FIX_Y,0,32+32+16+16,0,0, 0,0,0,0, 0,0);

			shuttleFont=getApp()->getNormalFont();
			shuttleFont->getFontDesc(d);
			d.weight=FONTWEIGHT_LIGHT;
			d.size=65;
			shuttleFont=new FXFont(getApp(),d);

			shuttleDial=new FXDial(t,this,ID_SHUTTLE_DIAL,DIAL_HORIZONTAL|DIAL_HAS_NOTCH|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_CENTER_Y, 0,0,32+32+32+32,20);
			shuttleDial->setRange(-((shuttleDial->getWidth())/2),(shuttleDial->getWidth())/2);
			shuttleDial->setRevolutionIncrement(shuttleDial->getWidth()*2-1);
			shuttleDial->setTipText("Shuttle Seek While Playing\n(right-click to return to middle)");

			t=new FXVerticalFrame(t,FRAME_NONE,0,0,0,0, 0,0,0,0, 0,0);
				shuttleDialSpringButton=new FXToggleButton(t,"free","spring",NULL,NULL,this,ID_SHUTTLE_DIAL_SPRING_BUTTON,LAYOUT_FILL_X|JUSTIFY_NORMAL|TOGGLEBUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 1,1,0,0);
				shuttleDialSpringButton->setState(true);
				shuttleDialSpringButton->setFont(shuttleFont);

				shuttleDialScaleButton=new FXButton(t,"100x",NULL,this,ID_SHUTTLE_DIAL_SCALE_BUTTON,LAYOUT_FILL_X|JUSTIFY_NORMAL|TOGGLEBUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 1,1,0,0);
				shuttleDialScaleButton->setFont(shuttleFont);

	new FXVerticalSeparator(s);

	// build miscellaneous buttons
	FXPacker *miscControlsFrame=new FXPacker(new FXPacker(s,LAYOUT_FILL_Y,0,0,0,0, 4,4,2,2),LAYOUT_FILL_Y|LAYOUT_FILL_X, 0,0,0,0, 0,0,0,0, 3,2);
		t=new FXHorizontalFrame(miscControlsFrame,0, 0,0,0,0, 0,0,0,0);
		followPlayPositionButton=new FXCheckButton(miscControlsFrame,"Follow Play Position",this,ID_FOLLOW_PLAY_POSITION_BUTTON);
		followPlayPositionButton->setPadLeft(0); followPlayPositionButton->setPadRight(0); followPlayPositionButton->setPadTop(0); followPlayPositionButton->setPadBottom(0);
		t=new FXHorizontalFrame(miscControlsFrame,0, 0,0,0,0, 0,0,0,0);
			//new FXLabel(t,"Crossfade Edges: ");
			crossfadeEdgesComboBox=new FXComboBox(t,8,3, this,ID_CROSSFADE_EDGES_COMBOBOX, FRAME_SUNKEN|FRAME_THICK | COMBOBOX_NORMAL|COMBOBOX_STATIC | LAYOUT_CENTER_Y);
				crossfadeEdgesComboBox->setTipText("After Most Actions a Crossfade can be Performed at the Start and Stop \nPositions to Give a Smoother Transition in to and out of the Modified Selection");
				crossfadeEdgesComboBox->appendItem("No Crossfade");
				crossfadeEdgesComboBox->appendItem("Crossfade Inner Edges");
				crossfadeEdgesComboBox->appendItem("Crossfade Outer Edges");
				crossfadeEdgesComboBox->setCurrentItem(0);
			new FXButton(t,"...\tChange Crossfade Times",NULL,this,ID_CROSSFADE_EDGES_SETTINGS, BUTTON_NORMAL & ~FRAME_THICK);
		clipboardComboBox=new FXComboBox(miscControlsFrame,8,8, this,ID_CLIPBOARD_COMBOBOX, FRAME_SUNKEN|FRAME_THICK | COMBOBOX_NORMAL|COMBOBOX_STATIC);

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
				soundList->appendHeader("Name",NULL,200);
				soundList->appendHeader("Path",NULL,9999);

	soundWindowFrame=new FXPacker(contents,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_RAISED|FRAME_THICK,0,0,0,0, 0,0,0,0, 0,0);
}

CMainWindow::~CMainWindow()
{
	delete shuttleFont;
	delete soundListFont;
	delete soundListHeaderFont;
}

void CMainWindow::show()
{
	FXint wantedWidth=getDefaultWidth();
	FXint wantedHeight=getDefaultHeight();
	rememberShow(this);
	resize(max(getWidth(),wantedWidth),max(getHeight(),wantedHeight));

	FXMainWindow::show();

	followPlayPositionButton->setCheck(gFollowPlayPosition);

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

}

void CMainWindow::hide()
{
	rememberHide(this);
	FXMainWindow::hide();
}

#include <CNestedDataFile/CNestedDataFile.h>
#include <stdlib.h>
void CMainWindow::showAbout()
{
	// this is called whenever the application starts
	// I do it to make sure the user at least knows *why* this release was made for the alpha and beta stages
	// make the about dialog show up some fixed number of times every time the version changes
	string version=gSettingsRegistry->getValue("SeenAboutDialogVersion");
	string count=gSettingsRegistry->getValue("SeenAboutDialogCount");
	if(version!=REZOUND_VERSION)
	{ // different version
		gSettingsRegistry->createKey("SeenAboutDialogVersion",REZOUND_VERSION);
		gSettingsRegistry->createKey("SeenAboutDialogCount","1");

		// if the version has changed from the previous run, then forget all window positions/sizes and splitter positions
		gSettingsRegistry->removeKey("SplitterPositions");
		gSettingsRegistry->removeKey("WindowDimensions");
	}
	else
	{ // same version, now check count or increment count
		if(atoi(count.c_str())>2)
			return; // been seen 3 times already
		else
			gSettingsRegistry->createKey("SeenAboutDialogCount",istring(atoi(count.c_str())+1));
	}
	gAboutDialog->execute(PLACEMENT_SCREEN);
}

void CMainWindow::rebuildSoundWindowList()
{
	soundList->clearItems();
	for(size_t t=0;t<gSoundFileManager->getOpenedCount();t++)
	{
		CSoundWindow *win=gSoundFileManager->getSoundWindow(t);

		// add to sound window list 
		CPath p(win->loadedSound->getFilename().c_str());

		soundList->appendItem(
			(
			istring(t+1,2,false)+"\t"+
			p.baseName()+"\t"+
			p.dirName()
			).c_str(),
			NULL,NULL,win);
	}

	soundList->forceRefresh();

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
		((CSoundWindow *)soundList->getItemData(index))->setActiveState(true);

	return 1;
}

extern CSoundWindow *previousActiveWindow;
long CMainWindow::onSoundListHotKey(FXObject *sender,FXSelector sel,void *ptr)
{
	FXEvent *ev=(FXEvent *)ptr;
	
	if(ev->code=='`')
	{ // switch to previously active window
		if(previousActiveWindow!=NULL)
		{
			int index=0;
			for(;index<soundList->getNumItems();index++)
			{ // find the index in the sound list of the previous active window so we can set the current item
				if(((CSoundWindow *)soundList->getItemData(index))==previousActiveWindow)
					break;
			}
			previousActiveWindow->setActiveState(true);
			soundList->setCurrentItem(index);
			soundList->makeItemVisible(soundList->getCurrentItem());
		}
		return 1;
	}
	else
	{
		FXint index=ev->code-'0';

		// take care of 0 meaning 10 actualy (which is index 9)
		index--;
		if(index==-1)
			index=9;
		
		if(index>=0 && index<soundList->getNumItems())
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
	This handler steals the key press events from the soundList FXIconList because
	it will take all my accelerator keys and search the list rather than pass them
	on to be handled by the accelerators.  I do this for all keys except things like
	up, down, tab, et al.
	
*/
long CMainWindow::onSoundListKeyPress(FXObject *sender,FXSelector sel,void *ptr)
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
	// sound list), then generate an key even to the main window, then set the 
	// focus again on the sound list
	soundList->killFocus();
	this->handle(sender,FXSEL(SEL_KEYPRESS,0),ptr);
	soundList->setFocus();
	return 1;
}

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
			FXMenuCommand *item=new FXMenuCommand(this,escapeAmpersand(gSoundFileManager->getReopenHistoryItem(t)).c_str(),NULL,getOwner(),CMainWindow::ID_FILE_REOPEN_MENUITEM);
			item->create();
			items.push_back(item);
		}

		FXMenuPane::popup(grabto,x,y,w,h);
	}

	vector<FXMenuCommand *> items;
};

#include "CChannelSelectDialog.h"
#include "CPasteChannelsDialog.h"

#include "EditActionDialogs.h"
#include "../backend/Edits/EditActions.h"

#include "../backend/Effects/EffectActions.h"
#include "EffectActionDialogs.h"

#include "../backend/Filters/FilterActions.h"
#include "FilterActionDialogs.h"

#include "../backend/Looping/LoopingActions.h"
#include "LoopingActionDialogs.h"

#include "../backend/Remaster/RemasterActions.h"
#include "RemasterActionDialogs.h"


void CMainWindow::createMenus()
{
	// build the drop-down menus
	FXMenuPane *menu;

	menu=new FXMenuPane(this);
	new FXMenuTitle(menubar,"&File",NULL,menu);
		new FXMenuCommand(menu,"&New",FOXIcons->file_new,this,ID_FILE_NEW_MENUITEM);
		new FXMenuCommand(menu,"&Open\tCtrl+O",FOXIcons->file_open,this,ID_FILE_OPEN_MENUITEM);
		new FXMenuCascade(menu,"&Reopen",FOXIcons->file_open,new CReopenPopup(this));
		new FXMenuCommand(menu,"&Save\tCtrl+S",FOXIcons->file_save,this,ID_FILE_SAVE_MENUITEM);
		new FXMenuCommand(menu,"Save &As",FOXIcons->file_save_as,this,ID_FILE_SAVE_AS_MENUITEM);
		new CActionMenuCommand(new CSaveSelectionAsActionFactory(),menu,"",FOXIcons->file_save_as);
		new CActionMenuCommand(new CSaveAsMultipleFilesActionFactory(new CSaveAsMultipleFilesDialog(this)),menu,"",FOXIcons->file_save_as);
		new FXMenuCommand(menu,"&Close\tCtrl+W",FOXIcons->file_close,this,ID_FILE_CLOSE_MENUITEM);
		new FXMenuCommand(menu,"Re&vert",FOXIcons->file_revert,this,ID_FILE_REVERT_MENUITEM);

		new FXMenuSeparator(menu);
		new FXMenuCommand(menu,"Record",NULL,this,ID_FILE_RECORD_MENUITEM);

		new FXMenuSeparator(menu);
		new FXMenuCommand(menu,"User No&tes"/*\tUser notes about the sound (and preserved in the file if the format supports it)"*/,FOXIcons->notes,this,ID_NOTES_MENUITEM);

		new FXMenuSeparator(menu);
		new FXMenuCommand(menu,"&About ReZound\tF1",NULL,this,ID_ABOUT_MENUITEM);

		// just for testing ???
		new FXMenuSeparator(menu);
		new FXMenuCaption(menu,"- Just for testing");
		new FXMenuCommand(menu,"Defrag",NULL,this,ID_DEFRAG_MENUITEM);
		new FXMenuCommand(menu,"PrintSAT",NULL,this,ID_PRINT_SAT_MENUITEM);
		new FXMenuCommand(menu,"VerifySAT",NULL,this,ID_VERIFY_SAT_MENUITEM);

		new FXMenuSeparator(menu);
		new FXMenuCommand(menu,"&Quit\tCtrl+Q",FOXIcons->exit,this,ID_FILE_QUIT_MENUITEM);


		// ??? in CActionMenuItem I should be able to do something intelligent to 
		// have it figure out (based on past entries it its parent) what letter in
		// the name should have a & in front

	menu=new FXMenuPane(this);
	new FXMenuTitle(menubar,"&Edit",NULL,menu);
		new FXMenuCommand(menu,"Undo\tCtrl+Z",FOXIcons->edit_undo,this,ID_UNDO_MENUITEM);
		new FXMenuCommand(menu,"Clear Undo History",NULL,this,ID_CLEAR_UNDO_HISTORY_MENUITEM);

		new FXMenuSeparator(menu);
		new CActionMenuCommand(new CCopyEditFactory(gChannelSelectDialog),menu,"Ctrl+C",FOXIcons->edit_copy);
		new CActionMenuCommand(new CCutEditFactory(gChannelSelectDialog),menu,"Ctrl+X",FOXIcons->edit_cut);
		new CActionMenuCommand(new CDeleteEditFactory(gChannelSelectDialog),menu,"Ctrl+D",FOXIcons->edit_delete);
		new CActionMenuCommand(new CCropEditFactory(gChannelSelectDialog),menu,"Ctrl+R",FOXIcons->edit_crop);

		new FXMenuSeparator(menu);
		new CActionMenuCommand(new CInsertPasteEditFactory(gPasteChannelsDialog),menu,"Ctrl+V",FOXIcons->edit_paste);
		new CActionMenuCommand(new CReplacePasteEditFactory(gPasteChannelsDialog),menu,"",FOXIcons->edit_paste);
		new CActionMenuCommand(new COverwritePasteEditFactory(gPasteChannelsDialog),menu,"",FOXIcons->edit_paste);
		new CActionMenuCommand(new CLimitedOverwritePasteEditFactory(gPasteChannelsDialog),menu,"",FOXIcons->edit_paste);
		new CActionMenuCommand(new CMixPasteEditFactory(gPasteChannelsDialog),menu,"",FOXIcons->edit_paste);
		new CActionMenuCommand(new CLimitedMixPasteEditFactory(gPasteChannelsDialog),menu,"",FOXIcons->edit_paste);
		new CActionMenuCommand(new CFitMixPasteEditFactory(gPasteChannelsDialog),menu,"",FOXIcons->edit_paste);

		new FXMenuSeparator(menu);
		new CActionMenuCommand(new CInsertSilenceEditFactory(gChannelSelectDialog,new CInsertSilenceDialog(this)),menu,"");
		new CActionMenuCommand(new CMuteEditFactory(gChannelSelectDialog),menu,"Ctrl+M");

		new FXMenuSeparator(menu);
		new CActionMenuCommand(new CAddChannelsEditFactory(new CAddChannelsDialog(this)),menu,"");
		new CActionMenuCommand(new CRemoveChannelsEditFactory(gChannelSelectDialog),menu,"");
		new CActionMenuCommand(new CSwapChannelsEditFactory(new CSwapChannelsDialog(this)),menu,"");

		new FXMenuSeparator(menu);
		new CActionMenuCommand(new CRotateLeftEditFactory(gChannelSelectDialog,new CRotateDialog(this)),menu,"");
		new CActionMenuCommand(new CRotateRightEditFactory(gChannelSelectDialog,new CRotateDialog(this)),menu,"");

		new FXMenuSeparator(menu);
		new CActionMenuCommand(new CSelectionEditFactory(sSelectAll),menu,"Ctrl+A");
		new CActionMenuCommand(new CSelectionEditFactory(sSelectToBeginning),menu,"");
		new CActionMenuCommand(new CSelectionEditFactory(sSelectToEnd),menu,"");
		new CActionMenuCommand(new CSelectionEditFactory(sFlopToBeginning),menu,"");
		new CActionMenuCommand(new CSelectionEditFactory(sFlopToEnd),menu,"");
		new CActionMenuCommand(new CSelectionEditFactory(sSelectToSelectStart),menu,"");
		new CActionMenuCommand(new CSelectionEditFactory(sSelectToSelectStop),menu,"");


	menu=new FXMenuPane(this);
	new FXMenuTitle(menubar,"Effec&ts",NULL,menu);
		new CActionMenuCommand(new CReverseEffectFactory(gChannelSelectDialog),menu,"");
		new CActionMenuCommand(new CChangeVolumeEffectFactory(gChannelSelectDialog,new CNormalVolumeChangeDialog(this)),menu,"");
		new CActionMenuCommand(new CGainEffectFactory(gChannelSelectDialog,new CNormalGainDialog(this),new CAdvancedGainDialog(this)),menu,"");
		new CActionMenuCommand(new CChangeRateEffectFactory(gChannelSelectDialog,new CNormalRateChangeDialog(this),new CAdvancedRateChangeDialog(this)),menu,"");
		new CActionMenuCommand(new CFlangeEffectFactory(gChannelSelectDialog,new CFlangeDialog(this)),menu,"");
		new CActionMenuCommand(new CSimpleDelayEffectFactory(gChannelSelectDialog,new CSimpleDelayDialog(this)),menu,"");
		new CActionMenuCommand(new CQuantizeEffectFactory(gChannelSelectDialog,new CQuantizeDialog(this)),menu,"");
		new CActionMenuCommand(new CDistortionEffectFactory(gChannelSelectDialog,new CDistortionDialog(this)),menu,"");
		new CActionMenuCommand(new CVariedRepeatEffectFactory(gChannelSelectDialog,new CVariedRepeatDialog(this)),menu,"");

		new CActionMenuCommand(new CTestEffectFactory(gChannelSelectDialog),menu,"");

	menu=new FXMenuPane(this);
	new FXMenuTitle(menubar,"F&ilters",NULL,menu);
		new CActionMenuCommand(new CConvolutionFilterFactory(gChannelSelectDialog,new CConvolutionFilterDialog(this)),menu,"");

		new FXMenuSeparator(menu);
		new CActionMenuCommand(new CSinglePoleLowpassFilterFactory(gChannelSelectDialog,new CSinglePoleLowpassFilterDialog(this)),menu,"");
		new CActionMenuCommand(new CSinglePoleHighpassFilterFactory(gChannelSelectDialog,new CSinglePoleHighpassFilterDialog(this)),menu,"");
		new CActionMenuCommand(new CBandpassFilterFactory(gChannelSelectDialog,new CBandpassFilterDialog(this)),menu,"");
		new CActionMenuCommand(new CNotchFilterFactory(gChannelSelectDialog,new CNotchFilterDialog(this)),menu,"");

		new FXMenuSeparator(menu);
		new CActionMenuCommand(new CBiquadResLowpassFilterFactory(gChannelSelectDialog,new CBiquadResLowpassFilterDialog(this)),menu,"");
		new CActionMenuCommand(new CBiquadResHighpassFilterFactory(gChannelSelectDialog,new CBiquadResHighpassFilterDialog(this)),menu,"");
		new CActionMenuCommand(new CBiquadResBandpassFilterFactory(gChannelSelectDialog,new CBiquadResBandpassFilterDialog(this)),menu,"");

	menu=new FXMenuPane(this);
	new FXMenuTitle(menubar,"&Looping",NULL,menu);
		new CActionMenuCommand(new CMakeSymetricActionFactory(gChannelSelectDialog),menu,"");
		new CActionMenuCommand(new CAddCuesActionFactory(new CAddCuesDialog(this)),menu,"");

	menu=new FXMenuPane(this);
	new FXMenuTitle(menubar,"&Remaster",NULL,menu);
		new CActionMenuCommand(new CRemoveDCActionFactory(gChannelSelectDialog),menu,"");
		new CActionMenuCommand(new CNoiseGateActionFactory(gChannelSelectDialog,new CNoiseGateDialog(this)),menu,"");
		new CActionMenuCommand(new CCompressorActionFactory(gChannelSelectDialog,new CCompressorDialog(this)),menu,"");
		new CActionMenuCommand(new CNormalizeActionFactory(gChannelSelectDialog,new CNormalizeDialog(this)),menu,"");
		new CActionMenuCommand(new CResampleActionFactory(gChannelSelectDialog,new CResampleDialog(this)),menu,"");

		new CActionMenuCommand(new CUnclipActionFactory(gChannelSelectDialog),menu,"");


	create(); // re-call create for this window which will call it for all new child windows
}


long CMainWindow::onQuit(FXObject *sender,FXSelector sel,void *ptr)
{
	if(getApp()->getModality()==MODAL_FOR_WINDOW)
	{ // don't allow a quit if there is a modal window showing
		gStatusComm->beep();
		return(1);
	}

	if(exitReZound(gSoundFileManager))
	{
		hide();
		getApp()->exit(0);
	}
	return(1);
}

long CMainWindow::onFollowPlayPositionButton(FXObject *sender,FXSelector sel,void *ptr)
{
	gFollowPlayPosition=followPlayPositionButton->getCheck();
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
	case ID_FILE_NEW_MENUITEM:
		newSound(gSoundFileManager);
		break;
	
	case ID_FILE_OPEN_MENUITEM:
		openSound(gSoundFileManager);
		break;

	case ID_FILE_REOPEN_MENUITEM:
		openSound(gSoundFileManager,dynamic_cast<FXMenuCommand *>(sender)->getText().text());
		break;
	
	case ID_FILE_SAVE_MENUITEM:
		saveSound(gSoundFileManager);
		break;

	case ID_FILE_SAVE_AS_MENUITEM:
		saveAsSound(gSoundFileManager);
		break;

	case ID_FILE_CLOSE_MENUITEM:
		closeSound(gSoundFileManager);
		break;

	case ID_FILE_REVERT_MENUITEM:
		revertSound(gSoundFileManager);
		break;

	case ID_FILE_RECORD_MENUITEM:
		recordSound(gSoundFileManager);
		break;

	case ID_ABOUT_MENUITEM:
		gAboutDialog->execute(PLACEMENT_SCREEN);
		break;

	default:
		throw(runtime_error(string(__func__)+" -- unhandled file button selector"));
	}
	return 1;
}

// play control events
long CMainWindow::onPlayControlButton(FXObject *sender,FXSelector sel,void *ptr)
{
	switch(FXSELID(sel))
	{
	case ID_PLAY_ALL_ONCE_BUTTON:
		metersWindow->resetGrandMaxPeakLevels();
		play(gSoundFileManager,false,false);
		break;

	case ID_PLAY_ALL_LOOPED_BUTTON:
		metersWindow->resetGrandMaxPeakLevels();
		play(gSoundFileManager,true,false);
		break;

	case ID_PLAY_SELECTION_ONCE_BUTTON:
		metersWindow->resetGrandMaxPeakLevels();
		play(gSoundFileManager,false,true);
		break;

	case ID_PLAY_SELECTION_LOOPED_BUTTON:
		metersWindow->resetGrandMaxPeakLevels();
		play(gSoundFileManager,true,true);
		break;

	case ID_STOP_BUTTON:
		stop(gSoundFileManager);
		break;

	case ID_PAUSE_BUTTON:
		pause(gSoundFileManager);
		break;

	case ID_JUMP_TO_BEGINNING_BUTTON:
		jumpToBeginning(gSoundFileManager);
		break;

	case ID_JUMP_TO_START_POSITION_BUTTON:
		jumpToStartPosition(gSoundFileManager);
		break;

	case ID_JUMP_TO_PREV_CUE_BUTTON:
		jumpToPreviousCue(gSoundFileManager);
		break;

	case ID_JUMP_TO_NEXT_CUE_BUTTON:
		jumpToNextCue(gSoundFileManager);
		break;

	default:
		throw(runtime_error(string(__func__)+" -- unhandled play button selector"));
	}
	return 1;
}

long CMainWindow::onUserNotesButton(FXObject *sender,FXSelector sel,void *ptr)
{
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
	return(1);
}

long CMainWindow::onUndoButton(FXObject *sender,FXSelector sel,void *ptr)
{
	undo(gSoundFileManager);
	return(1);
}

long CMainWindow::onClearUndoHistoryButton(FXObject *sender,FXSelector sel,void *ptr)
{
	clearUndoHistory(gSoundFileManager);
	return(1);
}

long CMainWindow::onShuttleReturn(FXObject *sender,FXSelector sel,void *ptr)
{
	if(((FXEvent *)ptr)->code==LEFTBUTTON && !shuttleDialSpringButton->getState())
		return 1; // this wasn't a left click release and where we're in spring-back mode

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

		FXint minValue,maxValue;
		shuttleDial->getRange(minValue,maxValue);

		const FXint shuttlePos=shuttleDial->getValue();
		float seekSpeed;


		if(shuttlePos==0)
			seekSpeed=1.0;
		else
		{
			const string text=shuttleDialScaleButton->getText().text();
			if(text=="1x")
			{ // 1x +/- (0..1]
				if(shuttlePos>0)
					seekSpeed=(double)shuttlePos/(double)maxValue;
				else //if(shuttlePos<0)
					seekSpeed=(double)-shuttlePos/(double)minValue;
			}
			else if(text=="2x")
			{ // 2x +/- [1..2]
				if(shuttlePos>0)
					seekSpeed=(double)shuttlePos/(double)maxValue+1.0;
				else //if(shuttlePos<0)
					seekSpeed=(double)-shuttlePos/(double)minValue-1.0;
			}
			else if(text=="100x")
			{ // 100x +/- [1..100]
						// I square the value to give a more useful range
				if(shuttlePos>0)
					seekSpeed=(pow((double)shuttlePos/(double)maxValue,2.0)*100.0)+1.0;
				else //if(shuttlePos<0)
					seekSpeed=(pow((double)shuttlePos/(double)minValue,2.0)*-100.0)-1.0;
			}
			else
				throw(runtime_error(string(__func__)+" -- internal error -- unhandled text for shuttleDialScaleButton: '"+text+"'"));
		}

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
		else
			throw(runtime_error(string(__func__)+" -- internal error -- unhandled text for shuttleDialScaleButton: '"+text+"'"));
		
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
	if(text=="1x")
		shuttleDialScaleButton->setText("2x");
	else if(text=="2x")
		shuttleDialScaleButton->setText("100x");
	else if(text=="100x")
		shuttleDialScaleButton->setText("1x");
	else
		throw(runtime_error(string(__func__)+" -- internal error -- unhandled text for shuttleDialScaleButton: '"+text+"'"));

	// return the shuttle control to the middle
	shuttleDial->setValue(0);
	onShuttleChange(NULL,0,NULL);

	return 1;
}

long CMainWindow::onKeyboardSeek(FXObject *sender,FXSelector sel,void *ptr)
{
	FXint lo,hi;
	shuttleDial->getRange(lo,hi);

	FXint inc= (hi-lo)/14; // 7 positions surrounding 0 

	FXint pos=shuttleDial->getValue();

	if(pos==0 && FXSELID(sel)==ID_SEEK_LEFT)
	{
		shuttleDial->setValue(pos-inc);
		onShuttleChange(sender,sel,ptr);
	}
	else if(pos==0 && FXSELID(sel)==ID_SEEK_RIGHT)
	{
		shuttleDial->setValue(pos+inc);
		onShuttleChange(sender,sel,ptr);
	}
	else if(pos!=0 && FXSELID(sel)==ID_SEEK_MODIFY)
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

long CMainWindow::onViewKey(FXObject *sender,FXSelector sel,void *ptr)
{
	switch(FXSELID(sel))
	{
	case ID_CENTER_START_POS:
		if(gSoundFileManager->getActiveWindow())
			gSoundFileManager->getActiveWindow()->centerStartPos();
		break;

	case ID_CENTER_STOP_POS:
		if(gSoundFileManager->getActiveWindow())
			gSoundFileManager->getActiveWindow()->centerStopPos();
		break;
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
	
	return(1);
}

