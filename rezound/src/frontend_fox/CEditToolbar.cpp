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

#include "CEditToolbar.h"

#include "CChannelSelectDialog.h"
#include "CPasteChannelsDialog.h"

#include "rememberShow.h"

#include "../backend/Edits/EditActions.h"

#include "EditActionDialogs.h"

/* TODO:
 * - make a better way of deciding on the position of the tool bar.. it would be nice if I knew I could dock it with the main window
 *
 * - detect when the FXScrollWindow uses its scrollbar and widen the toolbar window by that much
 *
 */


CEditToolbar *gEditToolbar=NULL;

FXDEFMAP(CEditToolbar) CEditToolbarMap[]=
{
	//	  Message_Type			ID					Message_Handler
};

FXIMPLEMENT(CEditToolbar,FXTopWindow,CEditToolbarMap,ARRAYNUMBER(CEditToolbarMap))

	
CEditToolbar::CEditToolbar(FXWindow *mainWindow) :
	FXTopWindow(mainWindow,"Edit",NULL,NULL,DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE,mainWindow->getX(),mainWindow->getY()+mainWindow->getDefaultHeight()+30,75,450, 0,0,0,0, 0,0),

	scrollWindow(new FXScrollWindow(this,LAYOUT_FILL_X|LAYOUT_FILL_Y)),
	contents(new FXMatrix(scrollWindow,2,MATRIX_BY_COLUMNS|FRAME_RAISED, 0,0,0,0, 4,4,4,4, 1,1))
{

	#define MAKE_FILLER new FXFrame(contents,LAYOUT_FIX_HEIGHT, 0,0,0,8);

	/*
	 * If I wanted to put "Selection:" ... "Editing:" I could group the sets of buttons in separate FXMatrix's and put the label above
	 */

	// selection functions
	new CActionButton(new CSelectionEditFactory(sSelectAll),contents,"sa",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32);
	MAKE_FILLER;
	new CActionButton(new CSelectionEditFactory(sSelectToBeginning),contents,"stb",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32);
	new CActionButton(new CSelectionEditFactory(sSelectToEnd),contents,"ste",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32);
	new CActionButton(new CSelectionEditFactory(sFlopToBeginning),contents,"ftb",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32);
	new CActionButton(new CSelectionEditFactory(sFlopToEnd),contents,"fte",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32);
	new CActionButton(new CSelectionEditFactory(sSelectToSelectStart),contents,"sp2st",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32);
	new CActionButton(new CSelectionEditFactory(sSelectToSelectStop),contents,"st2sp",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32);

	MAKE_FILLER;
	MAKE_FILLER;

	// edit functions that remove/copy
	new CActionButton(new CCopyEditFactory(gChannelSelectDialog),contents,"copy",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32);
	new CActionButton(new CCutEditFactory(gChannelSelectDialog),contents,"cut",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32);
	new CActionButton(new CDeleteEditFactory(gChannelSelectDialog),contents,"del",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32);
	new CActionButton(new CCropEditFactory(gChannelSelectDialog),contents,"crop",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32);
	new CActionButton(new CRotateLeftEditFactory(gChannelSelectDialog,new CRotateDialog(mainWindow)),contents,"<<",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32);
	new CActionButton(new CRotateRightEditFactory(gChannelSelectDialog,new CRotateDialog(mainWindow)),contents,">>",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32);

	MAKE_FILLER; 
	MAKE_FILLER;

	// edit functions that paste/mute
	new CActionButton(new CInsertPasteEditFactory(gPasteChannelsDialog),contents,"insrt",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32);
	new CActionButton(new CReplacePasteEditFactory(gPasteChannelsDialog),contents,"replc",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32);
	new CActionButton(new COverwritePasteEditFactory(gPasteChannelsDialog),contents,"ovwr",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32);
	new CActionButton(new CLimitedOverwritePasteEditFactory(gPasteChannelsDialog),contents,"lovr",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32);
	new CActionButton(new CMixPasteEditFactory(gPasteChannelsDialog),contents,"mix",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32);
	new CActionButton(new CLimitedMixPasteEditFactory(gPasteChannelsDialog),contents,"lmix",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32);
	new CActionButton(new CInsertSilenceEditFactory(gChannelSelectDialog,new CInsertSilenceDialog(mainWindow)),contents,"slnc",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32);
	new CActionButton(new CMuteEditFactory(gChannelSelectDialog),contents,"mute",NULL,FRAME_RAISED | LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,32,32);
}

/*
void CEditToolbar::create()
{
	FXTopWindow::create();

	show();
}
*/

void CEditToolbar::show()
{
	rememberShow(this);
	FXTopWindow::show();
}

void CEditToolbar::hide()
{
	rememberHide(this);
	FXTopWindow::hide();
}


