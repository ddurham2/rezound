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
#include "CActionButton.h"

#include <stdexcept>
#include <string>

#include "CSoundFileManager.h"

#include "settings.h"

#include "../backend/file.h"
#include "../backend/playcontrols.h"

#include "../backend/Effects/EffectActions.h"
#include "../backend/Remaster/RemasterActions.h"

#include "../backend/CLoadedSound.h"
#include "../backend/AAction.h"
#include "../backend/ASoundClipboard.h"

#include "../backend/CSoundPlayerChannel.h"

#include "CEditToolbar.h"

#include "CSoundWindow.h"

#include "CUserNotesDialog.h"
#include "CCrossfadeEdgesDialog.h"

#include "rememberShow.h"

/* TODO:
 * 	- make a button that brings back the editing toolbar incase they closed it
 *
 * 	- put gap after repeating and other toggles
 * 
 *	- remove all the data members for controls that don't need to have their value saved for any reason
 *
 */

FXDEFMAP(CMainWindow) CMainWindowMap[]=
{
	//Message_Type				ID						Message_Handler

	FXMAPFUNC(SEL_CLOSE,			0,						CMainWindow::onQuit),
	
		// file actions
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_FILE_NEW_BUTTON,		CMainWindow::onFileButton),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_FILE_OPEN_BUTTON,		CMainWindow::onFileButton),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_FILE_SAVE_BUTTON,		CMainWindow::onFileButton),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_FILE_SAVE_AS_BUTTON,		CMainWindow::onFileButton),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_FILE_CLOSE_BUTTON,		CMainWindow::onFileButton),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_FILE_REVERT_BUTTON,		CMainWindow::onFileButton),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_FILE_RECORD_BUTTON,		CMainWindow::onFileButton),

	FXMAPFUNC(SEL_RIGHTBUTTONPRESS,		CMainWindow::ID_FILE_OPEN_BUTTON,		CMainWindow::onReopenMenuPopup),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_REOPEN_MENU_SELECT,		CMainWindow::onReopenMenuSelect),


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
	FXMAPFUNC(SEL_CHANGED,			CMainWindow::ID_SHUTTLE_DIAL,			CMainWindow::onShuttleChange),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SEEK_NORMAL,			CMainWindow::onShuttleReturn),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SEEK_LEFT,			CMainWindow::onKeyboardSeek),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SEEK_MODIFY,			CMainWindow::onKeyboardSeek),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_SEEK_RIGHT,			CMainWindow::onKeyboardSeek),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_CENTER_START_POS,		CMainWindow::onViewKey),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_CENTER_STOP_POS,		CMainWindow::onViewKey),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_REDRAW_BUTTON,			CMainWindow::onRedrawButton),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_UNDO_BUTTON,			CMainWindow::onUndoButton),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_CLEAR_UNDO_HISTORY_BUTTON,	CMainWindow::onClearUndoHistoryButton),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_NOTES_BUTTON,			CMainWindow::onUserNotesButton),
	
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_DEFRAG_BUTTON,			CMainWindow::onDefragButton),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_PRINT_SAT_BUTTON,		CMainWindow::onPrintSATButton),

	FXMAPFUNC(SEL_MOTION,			CMainWindow::ID_ACTIONCONTROL_TAB,		CMainWindow::onActionControlTabMouseMove),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_FOLLOW_PLAY_POSITION_BUTTON,	CMainWindow::onFollowPlayPositionButton),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_CROSSFADE_EDGES_COMBOBOX,	CMainWindow::onCrossfadeEdgesComboBox),
	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_CROSSFADE_EDGES_SETTINGS,	CMainWindow::onCrossfadeEdgesSettings),

	FXMAPFUNC(SEL_COMMAND,			CMainWindow::ID_CLIPBOARD_COMBOBOX,		CMainWindow::onClipboardComboBox),
};

FXIMPLEMENT(CMainWindow,FXMainWindow,CMainWindowMap,ARRAYNUMBER(CMainWindowMap))

#include <fox/fxkeys.h>

CMainWindow::CMainWindow(FXApp* a) :
	FXMainWindow(a,"ReZound",NULL,NULL,DECOR_ALL,0,0,800,0),

	mouseMoveLastTab(NULL)
{
	contents=new FXHorizontalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 1,1,1,1, 1,1);

	#define BUTTON_STYLE FRAME_RAISED|LAYOUT_EXPLICIT

	FXPacker *t;

	playControlsFrame=new FXPacker(new FXPacker(contents,FRAME_RIDGE|LAYOUT_FILL_Y,0,0,0,0, 6,6,6,6),LAYOUT_FILL_Y|LAYOUT_FILL_X, 0,0,0,0, 0,0,0,0, 0,0);
		#define PLAY_CONTROLS_BUTTON_STYLE BUTTON_STYLE
		new FXButton(playControlsFrame,"PAO\tPlay All Once",NULL,this,ID_PLAY_ALL_ONCE_BUTTON,PLAY_CONTROLS_BUTTON_STYLE, 0,0,32,32);
		new FXButton(playControlsFrame,"PSO\tPlay Selection Once",NULL,this,ID_PLAY_SELECTION_ONCE_BUTTON,PLAY_CONTROLS_BUTTON_STYLE, 32,0,32,32);
		new FXButton(playControlsFrame,"PAL\tPlay All Looped",NULL,this,ID_PLAY_ALL_LOOPED_BUTTON,PLAY_CONTROLS_BUTTON_STYLE, 0,32,32,32);
		new FXButton(playControlsFrame,"PSL\tPlay Selection Looped",NULL,this,ID_PLAY_SELECTION_LOOPED_BUTTON,PLAY_CONTROLS_BUTTON_STYLE, 32,32,32,32);

		new FXButton(playControlsFrame,"Stop\tStop",NULL,this,ID_STOP_BUTTON,PLAY_CONTROLS_BUTTON_STYLE, 32+32,0,32,32),
		new FXButton(playControlsFrame,"Pause\tPause",NULL,this,ID_PAUSE_BUTTON,PLAY_CONTROLS_BUTTON_STYLE, 32+32,32,32,32),

		new FXButton(playControlsFrame,"||<<\tJump to Beginning",NULL,this,ID_JUMP_TO_BEGINNING_BUTTON,PLAY_CONTROLS_BUTTON_STYLE, 0,32+32,32+32,16);
		new FXButton(playControlsFrame,"|<<\tJump to Start Position",NULL,this,ID_JUMP_TO_START_POSITION_BUTTON,PLAY_CONTROLS_BUTTON_STYLE, 32+32,32+32,32+32,16);

		new FXButton(playControlsFrame,"Q<<\tJump to Previous Cue",NULL,this,ID_JUMP_TO_PREV_CUE_BUTTON,PLAY_CONTROLS_BUTTON_STYLE, 0,32+32+16,32+32,16);
		new FXButton(playControlsFrame,">>Q\tJump to Next Cue",NULL,this,ID_JUMP_TO_NEXT_CUE_BUTTON,PLAY_CONTROLS_BUTTON_STYLE, 32+32,32+32+16,32+32,16);

		shuttleDial=new FXDial(playControlsFrame,this,ID_SHUTTLE_DIAL,DIAL_HORIZONTAL|DIAL_HAS_NOTCH|LAYOUT_EXPLICIT, 0,32+32+16+16,32+32+32+32,20);
		shuttleDial->setRange(-((shuttleDial->getWidth())/2),(shuttleDial->getWidth())/2);
		shuttleDial->setRevolutionIncrement(shuttleDial->getWidth()*2-1);
		shuttleDial->setTipText("Shuttle Seek While Playing\n(Hint: try the mouse wheel as well as dragging)");

	miscControlsFrame=new FXPacker(new FXPacker(contents,FRAME_RIDGE|LAYOUT_FILL_Y,0,0,0,0, 6,6,6,6),LAYOUT_FILL_Y|LAYOUT_FILL_X, 0,0,0,0, 0,0,0,0, 3,2);
		t=new FXHorizontalFrame(miscControlsFrame,0, 0,0,0,0, 0,0,0,0);
				// ??? this and redo should probably be the top two buttons on the edit panel
			new FXButton(t,"&Undo",NULL,this,ID_UNDO_BUTTON,FRAME_RAISED);
			new FXButton(t,"&ClrUndo",NULL,this,ID_CLEAR_UNDO_HISTORY_BUTTON,FRAME_RAISED);
		new FXButton(miscControlsFrame,"&Redraw",NULL,this,ID_REDRAW_BUTTON,FRAME_RAISED);
		followPlayPositionButton=new FXCheckButton(miscControlsFrame,"Follow Play Position",this,ID_FOLLOW_PLAY_POSITION_BUTTON);
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

	/* ??? it is not necessary to have all these data members for all the buttons */

	int actionControlTabOrder=0;
	actionControlsFrame=new FXTabBook(contents,NULL,0,TABBOOK_NORMAL|LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_NONE, 0,0,0,0, 1,1,1,1);
		fileTab=new FXTabItem(actionControlsFrame,"&File",NULL,TAB_TOP_NORMAL);
		fileTab->setTarget(this);
		fileTab->setSelector(ID_ACTIONCONTROL_TAB);
		actionControlTabOrdering[fileTab]=actionControlTabOrder++;
			fileTabFrame=new FXHorizontalFrame(actionControlsFrame,LAYOUT_SIDE_TOP|LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_THICK|FRAME_RAISED, 0,0,0,0, 4,4,4,4, 3,3);
				fileNewButton=new FXButton(fileTabFrame,"&New",NULL,this,ID_FILE_NEW_BUTTON,FRAME_RAISED);
				fileOpenButton=new FXButton(fileTabFrame,"&Open/\nReopen\t(Right Click for Reopen History)",NULL,this,ID_FILE_OPEN_BUTTON,FRAME_RAISED);
				fileSaveButton=new FXButton(fileTabFrame,"&Save",NULL,this,ID_FILE_SAVE_BUTTON,FRAME_RAISED);
				fileSaveAsButton=new FXButton(fileTabFrame,"Save &As",NULL,this,ID_FILE_SAVE_AS_BUTTON,FRAME_RAISED);
				fileCloseButton=new FXButton(fileTabFrame,"&Close",NULL,this,ID_FILE_CLOSE_BUTTON,FRAME_RAISED);
				fileRevertButton=new FXButton(fileTabFrame,"Re&vert",NULL,this,ID_FILE_REVERT_BUTTON,FRAME_RAISED);
				fileRecordButton=new FXButton(fileTabFrame,"&Record",NULL,this,ID_FILE_RECORD_BUTTON,FRAME_RAISED);
				notesButton=new FXButton(fileTabFrame,"Notes\tUser notes about the sound (and preserved in the file if the format supports it)",NULL,this,ID_NOTES_BUTTON,FRAME_RAISED);

				// just for testing
				new FXFrame(fileTabFrame,FRAME_NONE);
				new FXLabel(fileTabFrame,"debug:");
				new FXButton(fileTabFrame,"&Defrag",NULL,this,ID_DEFRAG_BUTTON,FRAME_RAISED);
				new FXButton(fileTabFrame,"&PrintSAT",NULL,this,ID_PRINT_SAT_BUTTON,FRAME_RAISED);

		effectsTab=new FXTabItem(actionControlsFrame,"&Effects",NULL,TAB_TOP_NORMAL);
		effectsTab->setTarget(this);
		effectsTab->setSelector(ID_ACTIONCONTROL_TAB);
		actionControlTabOrdering[effectsTab]=actionControlTabOrder++;
			effectsTabFrame=new FXHorizontalFrame(actionControlsFrame,LAYOUT_SIDE_TOP|LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_THICK|FRAME_RAISED, 0,0,0,0, 4,4,4,4, 3,3);

		loopingTab=new FXTabItem(actionControlsFrame,"&Looping",NULL,TAB_TOP_NORMAL);
		loopingTab->setTarget(this);
		loopingTab->setSelector(ID_ACTIONCONTROL_TAB);
		actionControlTabOrdering[loopingTab]=actionControlTabOrder++;
			loopingTabFrame=new FXHorizontalFrame(actionControlsFrame,LAYOUT_SIDE_TOP|LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_THICK|FRAME_RAISED, 0,0,0,0, 4,4,4,4, 3,3);

		remasterTab=new FXTabItem(actionControlsFrame,"&Remaster",NULL,TAB_TOP_NORMAL);
		remasterTab->setTarget(this);
		remasterTab->setSelector(ID_ACTIONCONTROL_TAB);
		actionControlTabOrdering[remasterTab]=actionControlTabOrder++;
			remasterTabFrame=new FXHorizontalFrame(actionControlsFrame,LAYOUT_SIDE_TOP|LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_THICK|FRAME_RAISED, 0,0,0,0, 4,4,4,4, 3,3);
}

void CMainWindow::show()
{
	rememberShow(this);
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

void setupButton(CActionButton *b)
{
	b->create();
	b->recalc();
}

// ??? perhaps this could be a function placed else where that wouldn't require a recompile of CMainWindow everytime we change a dialog
#include "CChannelSelectDialog.h"
#include "EffectActionDialogs.h"
#include "RemasterActionDialogs.h"
void CMainWindow::createToolbars()
{
	// ??? when/if I have loadable plugins that I get AAction derivations from, I could have the plugin return the name of the panel it wants to be on, then all actions work this way where some belong to "Effects" and some to "Looping", etc

	// effects
	setupButton(new CActionButton(new CReverseEffectFactory(gChannelSelectDialog),effectsTabFrame,"rev",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32));
	setupButton(new CActionButton(new CChangeAmplitudeEffectFactory(gChannelSelectDialog,new CNormalAmplitudeChangeDialog(this),new CAdvancedAmplitudeChangeDialog(this)),effectsTabFrame,"vol",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32));
	setupButton(new CActionButton(new CChangeRateEffectFactory(gChannelSelectDialog,new CNormalRateChangeDialog(this),new CAdvancedRateChangeDialog(this)),effectsTabFrame,"rate",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32));
	setupButton(new CActionButton(new CFlangeEffectFactory(gChannelSelectDialog,new CFlangeDialog(this)),effectsTabFrame,"flng",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32));
	setupButton(new CActionButton(new CSimpleDelayEffectFactory(gChannelSelectDialog,new CSimpleDelayDialog(this)),effectsTabFrame,"dly",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32));
	setupButton(new CActionButton(new CStaticReverbEffectFactory(gChannelSelectDialog),effectsTabFrame,"rvrb",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32));
	setupButton(new CActionButton(new CVariedRepeatEffectFactory(gChannelSelectDialog,new CVariedRepeatDialog(this)),effectsTabFrame,"vrep",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32));

	setupButton(new CActionButton(new CTestEffectFactory(gChannelSelectDialog),effectsTabFrame,"test",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32));

	// remaster
	setupButton(new CActionButton(new CUnclipActionFactory(gChannelSelectDialog),remasterTabFrame,"unclp",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32));
	setupButton(new CActionButton(new CRemoveDCActionFactory(gChannelSelectDialog),remasterTabFrame,"-DC",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32));
	setupButton(new CActionButton(new CNoiseGateActionFactory(gChannelSelectDialog,new CNoiseGateDialog(this)),remasterTabFrame,"ng",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32));
	setupButton(new CActionButton(new CCompressorActionFactory(gChannelSelectDialog,new CCompressorDialog(this)),remasterTabFrame,"comp",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32));
}


long CMainWindow::onQuit(FXObject *sender,FXSelector sel,void *ptr)
{
	if(getApp()->modalModality()==MODAL_FOR_WINDOW)
	{ // don't allow a quit if there is a modal window showing
		gStatusComm->beep();
		return(1);
	}

	if(exitReZound(gSoundFileManager))
	{
		gEditToolbar->hide();
		hide();
		getApp()->exit(0);
	}
	return(1);
}

long CMainWindow::onActionControlTabMouseMove(FXObject *sender,FXSelector sel,void *ptr)
{
	// This even is invoked when the mouse is moved over one of the tabs containing
	// action buttons.  It is used to auto raise the tab when the mouse is moved over
	// it to save the user from having to click on it.

	if(dynamic_cast<FXTabItem *>(sender)==NULL)
		return(0);

	if(mouseMoveLastTab!=sender)
	{
		mouseMoveLastTab=sender;
		actionControlsFrame->setCurrent(actionControlTabOrdering[sender]);
	}

	return 1;
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
long CMainWindow::onFileButton(FXObject *sender,FXSelector sel,void *ptr)
{
	switch(SELID(sel))
	{
	case ID_FILE_NEW_BUTTON:
		newSound(gSoundFileManager);
		break;
	
	case ID_FILE_OPEN_BUTTON:
		openSound(gSoundFileManager);
		break;
	
	case ID_FILE_SAVE_BUTTON:
		saveSound(gSoundFileManager);
		break;

	case ID_FILE_SAVE_AS_BUTTON:
		saveAsSound(gSoundFileManager);
		break;

	case ID_FILE_CLOSE_BUTTON:
		closeSound(gSoundFileManager);
		break;

	case ID_FILE_REVERT_BUTTON:
		revertSound(gSoundFileManager);
		break;

	case ID_FILE_RECORD_BUTTON:
		recordSound(gSoundFileManager);
		break;

	default:
		throw(runtime_error(string(__func__)+" -- unhandled file button selector"));
	}
	return 1;
}

long CMainWindow::onReopenMenuPopup(FXObject *sender,FXSelector sel,void *ptr)
{
	if(!fileOpenButton->underCursor())
		return(0);

	FXEvent *event=(FXEvent*)ptr;

	size_t reopenSize=gSoundFileManager->getReopenHistorySize();
	if(reopenSize<=0)
		return(0);

	FXMenuPane reopenMenu(this);

	for(size_t t=0;t<reopenSize;t++)
		new FXMenuCommand(&reopenMenu,gSoundFileManager->getReopenHistoryItem(t).c_str(),NULL,this,ID_REOPEN_MENU_SELECT);

	reopenMenu.create();
	reopenMenu.popup(NULL,event->root_x,event->root_y);
	getApp()->runModalWhileShown(&reopenMenu);
	return(1);
}

long CMainWindow::onReopenMenuSelect(FXObject *sender,FXSelector sel,void *ptr)
{
	openSound(gSoundFileManager,((FXMenuCommand *)sender)->getText().text());

	return(1);
}

// play control events
long CMainWindow::onPlayControlButton(FXObject *sender,FXSelector sel,void *ptr)
{
	switch(SELID(sel))
	{
	case ID_PLAY_ALL_ONCE_BUTTON:
		play(gSoundFileManager,false,false);
		break;

	case ID_PLAY_ALL_LOOPED_BUTTON:
		play(gSoundFileManager,true,false);
		break;

	case ID_PLAY_SELECTION_ONCE_BUTTON:
		play(gSoundFileManager,false,true);
		break;

	case ID_PLAY_SELECTION_LOOPED_BUTTON:
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

long CMainWindow::onRedrawButton(FXObject *sender,FXSelector sel,void *ptr)
{
	CLoadedSound *s=gSoundFileManager->getActive();
	if(s!=NULL)
	{
		for(unsigned t=0;t<s->getSound()->getChannelCount();t++)
			s->getSound()->invalidateAllPeakData();
		gSoundFileManager->updateAfterEdit();
	}
	else
		getApp()->beep();
	
	return(1);
}

long CMainWindow::onUndoButton(FXObject *sender,FXSelector sel,void *ptr)
{
	try
	{
		// ??? this code should actually be put in the backend if it isn't already there and become just a call to undo() in file.h
		CLoadedSound *s=gSoundFileManager->getActive();
		if(s!=NULL)
		{
			if(!s->actions.empty())
			{
				AAction *a=s->actions.top();
				s->actions.pop();

				a->undoAction(s->channel);
				delete a; // ??? what ever final logic is implemented for undo, it should probably push it onto a redo stack
				gSoundFileManager->updateAfterEdit();
			}
			else
				gStatusComm->beep();
		}
		else
			getApp()->beep();
	}
	catch(exception &e)
	{
		Error(e.what());
	}
	
	return(1);
}

long CMainWindow::onClearUndoHistoryButton(FXObject *sender,FXSelector sel,void *ptr)
{
	try
	{
		CLoadedSound *s=gSoundFileManager->getActive();
		if(s!=NULL)
		{
			s->clearUndoHistory();
			gSoundFileManager->updateAfterEdit();
		}
		else
			getApp()->beep();
	}
	catch(exception &e)
	{
		Error(e.what());
	}
	return(1);
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

long CMainWindow::onShuttleReturn(FXObject *sender,FXSelector sel,void *ptr)
{
	shuttleDial->setValue(0);
	onShuttleChange(sender,sel,ptr); // I think I need to call this ???
	return 1;
}

long CMainWindow::onShuttleChange(FXObject *sender,FXSelector sel,void *ptr)
{
	// ??? if the active window changes.... I need to set the channel's speak back to 1x
	CLoadedSound *s=gSoundFileManager->getActive();
	if(s!=NULL)
	{
		FXint minValue,maxValue;
		shuttleDial->getRange(minValue,maxValue);
		const FXint shuttlePos=shuttleDial->getValue();
		float playSpeed;

		if(shuttlePos==0)
			playSpeed=1.0;
		else if(shuttlePos>0)
			playSpeed=(pow((float)shuttlePos/(float)maxValue,2.0)*100.0)+1.0;
		else if(shuttlePos<0)
			playSpeed=(pow((float)shuttlePos/(float)minValue,2.0)*-100.0)-1.0;

		s->channel->setPlaySpeed(playSpeed);	
	}

	return 1;
}

long CMainWindow::onKeyboardSeek(FXObject *sender,FXSelector sel,void *ptr)
{
	FXint lo,hi;
	shuttleDial->getRange(lo,hi);

	FXint inc= (hi-lo)/14; // 7 positions surrounding 0 

	FXint pos=shuttleDial->getValue();

	if(pos==0 && SELID(sel)==ID_SEEK_LEFT)
	{
		shuttleDial->setValue(pos-inc);
		onShuttleChange(sender,sel,ptr);
	}
	else if(pos==0 && SELID(sel)==ID_SEEK_RIGHT)
	{
		shuttleDial->setValue(pos+inc);
		onShuttleChange(sender,sel,ptr);
	}
	else if(pos!=0 && SELID(sel)==ID_SEEK_MODIFY)
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
	switch(SELID(sel))
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

long CMainWindow::onDefragButton(FXObject *sender,FXSelector sel,void *ptr)
{
	CLoadedSound *s=gSoundFileManager->getActive();
	if(s!=NULL)
	{
		s->getSound()->defragPoolFile();
		gSoundFileManager->updateAfterEdit();
	}
	else
		getApp()->beep();
	
	return(1);
}

long CMainWindow::onPrintSATButton(FXObject *sender,FXSelector sel,void *ptr)
{
	CLoadedSound *s=gSoundFileManager->getActive();
	if(s!=NULL)
		s->getSound()->printSAT();
	else
		getApp()->beep();
	
	return(1);
}


