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

#include "CAboutDialog.h"

#include "images.h"
#include "../backend/license.h"

CAboutDialog *gAboutDialog=NULL;


FXDEFMAP(CAboutDialog) CAboutDialogMap[]=
{
//	Message_Type			ID					Message_Handler
	//FXMAPFUNC(SEL_COMMAND,	CAboutDialog::ID_OKAY_BUTTON,	CAboutDialog::onOkayButton),
};
		

FXIMPLEMENT(CAboutDialog,FXDialogBox,CAboutDialogMap,ARRAYNUMBER(CAboutDialogMap))



// ----------------------------------------

CAboutDialog::CAboutDialog(FXWindow *mainWindow) :
	FXDialogBox(mainWindow,"About ReZound",DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE,0,0,620,450, 0,0,0,0, 0,0)
{
	FXPacker *contents=new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 2,2,2,2, 0,0);

	FXPacker *upperFrame=new FXHorizontalFrame(contents,LAYOUT_FILL_X | FRAME_RAISED|FRAME_THICK);
		new FXLabel(upperFrame,"",new FXGIFIcon(getApp(),logo_gif,0,IMAGE_ALPHACOLOR),LABEL_NORMAL | LAYOUT_SIDE_LEFT | FRAME_RAISED|FRAME_THICK, 0,0,0,0, 0,0,0,0);
		new FXLabel(upperFrame,FXString(REZOUND_PACKAGE)+" v"+FXString(REZOUND_VERSION),NULL,LABEL_NORMAL|LAYOUT_CENTER_X);
		
	FXTabBook *tabs=new FXTabBook(contents,NULL,0,TABBOOK_NORMAL | LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0);
	FXTabItem *tab;

	#define MAKE_TEXT(parent,name,value) \
		new FXLabel(parent,name); \
		new FXLabel(parent,value,NULL,LABEL_NORMAL|LAYOUT_FILL_COLUMN);


	#define MAKE_LINK(parent,name,link) \
		{ \
		new FXLabel(parent,name); \
		FXTextField *tf=new FXTextField(parent,0,NULL,0,TEXTFIELD_READONLY | LAYOUT_FILL_COLUMN|LAYOUT_FILL_X); \
		tf->setText(link); \
		tf->setTextColor(FXRGB(0,0,255)); \
		tf->setBackColor(t->getBackColor()); \
		}

	// ??? temporary for the alpha stage
	tab=new FXTabItem(tabs,"Please Read",NULL,TAB_TOP_NORMAL);
	{
		FXPacker *t=new FXPacker(tabs, LAYOUT_FILL_X|LAYOUT_FILL_Y | FRAME_RAISED|FRAME_THICK);

		(new FXText(t,NULL,0,TEXT_READONLY|TEXT_WORDWRAP|LAYOUT_FILL_X|LAYOUT_FILL_Y))->setText("*** PLEASE READ ***

   Welcome to this alpha stage release of ReZound.  The primary goal for this alpha stage is to get feedback on what problems ReZound has compiling on different linux distros and such.  Please report any problems you had with the configure/compiling process to me through the bug report link on the next tab in this about dialog.
   There aren't a huge number of features yet, but they are growing.  I plan to write a brief document on how to implement new actions/effects/edits.  I specifically designed ReZound so that it would be easy to create new actions.
   If you have any suggestions about the UI or other functionality then please report them also to me.  Thank you for giving ReZound a try.  I hope your experience is a good one and that you will find this application useful when you need to edit audio as I find it useful.

   -- Davy
		");								   

	}

	// ??? I would probably eventually like all this info to come from the share/doc files or something
	// but right now I'll just code it here.. The advantage of getting it from those files is that I 
	// could create a class in the backend that has methods like: getAuthors (which would return a vector
	// of strings" and getLicense() which would return a string.. etc.. I really probably won't worry about
	// this until there are actually two frontends.. of course I really would hate to maintain two separate
	// frontends.

	tab=new FXTabItem(tabs,"About",NULL,TAB_TOP_NORMAL);
	{
		FXMatrix *t=new FXMatrix(tabs,2,MATRIX_BY_COLUMNS | LAYOUT_FILL_X | FRAME_RAISED|FRAME_THICK);

		MAKE_TEXT(t,"Author","Davy Durham, ddurham@users.sourceforge.net");
		MAKE_TEXT(t,"Autoconfiscator","Anthony Ventimiglia");
		MAKE_TEXT(t,"Artistic Consultant","Will Jayroe");
		MAKE_LINK(t,"Home Page","http://rezound.sourceforge.net");
		MAKE_LINK(t,"Bug Reports","http://sourceforge.net/tracker/?group_id=5056&atid=105056");
		MAKE_LINK(t,"Feature Requests","http://sourceforge.net/tracker/?atid=355056&group_id=5056&func=browse");
		MAKE_LINK(t,"","");
		MAKE_LINK(t,"Also Please Read","http://sourceforge.net/docman/display_doc.php?docid=9886&group_id=5056");
	}

	tab=new FXTabItem(tabs,"Thanks To",NULL,TAB_TOP_NORMAL);
	{
		FXMatrix *t=new FXMatrix(tabs,2,MATRIX_BY_COLUMNS | LAYOUT_FILL_X | FRAME_RAISED|FRAME_THICK);

		MAKE_TEXT(t,"John Doe","Beta Testing");
	}

	tab=new FXTabItem(tabs,"License",NULL,TAB_TOP_NORMAL);
	{
		FXPacker *t=new FXPacker(tabs,LAYOUT_FILL_X | FRAME_RAISED|FRAME_THICK);

		(new FXText(t,NULL,0,TEXT_READONLY | FRAME_SUNKEN|FRAME_THICK | LAYOUT_FILL_X|LAYOUT_FILL_Y))->setText(gLicense);
	}

	FXPacker *lowerFrame=new FXHorizontalFrame(contents,FRAME_RAISED|FRAME_THICK | LAYOUT_FILL_X|LAYOUT_FIX_HEIGHT, 0,0,0,55);
		FXButton *okayButton=new FXButton(lowerFrame,"&Close",new FXGIFIcon(getApp(),GreenCheck1_gif),this,ID_ACCEPT,FRAME_RAISED|FRAME_THICK | JUSTIFY_NORMAL | ICON_BEFORE_TEXT | LAYOUT_CENTER_X|LAYOUT_CENTER_Y, 0,0,0,0, 10,10,5,5);
}


