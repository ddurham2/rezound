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

#include "CKeyBindingsDialog.h"

#include "settings.h"
#include "CStatusComm.h"

#include <ctype.h>
#include <stdio.h>

#include <CNestedDataFile/CNestedDataFile.h>

CKeyBindingsDialog *gKeyBindingsDialog=NULL;


FXString fxunparseAccel(FXHotKey key);

void parseItemText(const string itemText,string &name,string &key)
{
	size_t tab_pos=itemText.find("\t");
	name=itemText.substr(0,tab_pos);
	key=itemText.substr(tab_pos+1);
}

// -----------------------------------------

class CAssignPopup : public FXDialogBox
{
	FXDECLARE(CAssignPopup);
public:
	CAssignPopup(FXWindow *owner) :
		FXDialogBox(owner,_("Assign Hotkey"),DECOR_TITLE|DECOR_BORDER, 200,50)
	{

		FXVerticalFrame *t=new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0, 5,5,5,5, 5,5);

		new FXLabel(t,_("Press a Key Combination"),NULL, LABEL_NORMAL|LAYOUT_CENTER_X);
		new FXButton(t,_("Assign No Key"),NULL,this,ID_NOKEY_BUTTON, BUTTON_NORMAL|LAYOUT_CENTER_X);
		new FXButton(t,_("Cancel"),NULL,this,ID_CANCEL, BUTTON_NORMAL|LAYOUT_CENTER_X);
	}

	virtual ~CAssignPopup()
	{
	}

	FXuint key;
	FXuint showIt()
	{
		if(execute())
			return key;
		return 0;
	}

	long onNoKeyButton(FXObject *sender,FXSelector sel,void *ptr)
	{
		key=1;
		FXDialogBox::handle(this,MKUINT(ID_ACCEPT,SEL_COMMAND),ptr);
		return 1;
	}

	long onKeyRelease(FXObject *sender,FXSelector sel,void *ptr)
	{
		FXEvent *ev=(FXEvent *)ptr;
		if(ev->code>=KEY_Shift_L && ev->code<=KEY_Hyper_R)
		{ // in fox these are contiguously defined, and we won't get key release events for these keys.. they're just modifiers
			return 1;
		}

		key=MKUINT(tolower(ev->code),ev->state);
		FXDialogBox::handle(this,MKUINT(ID_ACCEPT,SEL_COMMAND),ptr);
		return 1;
	}

	enum	
	{
		ID_NOKEY_BUTTON=FXDialogBox::ID_LAST,
		ID_LAST
	};

	CAssignPopup() {}

};

FXDEFMAP(CAssignPopup) CAssignPopupMap[]=
{
//	Message_Type			ID					Message_Handler
	FXMAPFUNC(SEL_COMMAND,		CAssignPopup::ID_NOKEY_BUTTON,		CAssignPopup::onNoKeyButton),
	FXMAPFUNC(SEL_KEYRELEASE,	0,					CAssignPopup::onKeyRelease),
};
		
FXIMPLEMENT(CAssignPopup,FXDialogBox,CAssignPopupMap,ARRAYNUMBER(CAssignPopupMap))

// -----------------------------------------









FXDEFMAP(CKeyBindingsDialog) CKeyBindingsDialogMap[]=
{
//	Message_Type			ID					Message_Handler
	FXMAPFUNC(SEL_DOUBLECLICKED,	CKeyBindingsDialog::ID_HOTKEY_LIST,	CKeyBindingsDialog::onDefineKeyBinding),
	FXMAPFUNC(SEL_COMMAND,		CKeyBindingsDialog::ID_ASSIGN_BUTTON,	CKeyBindingsDialog::onDefineKeyBinding)
};
		

FXIMPLEMENT(CKeyBindingsDialog,FXModalDialogBox,CKeyBindingsDialogMap,ARRAYNUMBER(CKeyBindingsDialogMap))


FXint sortfunc(const FXIconItem *a,const FXIconItem *b)
{
	return strcasecmp(a->getText().text(), b->getText().text());
}


// ----------------------------------------

CKeyBindingsDialog::CKeyBindingsDialog(FXWindow *mainWindow) :
	FXModalDialogBox(mainWindow,N_("Hot Keys"),600,550,FXModalDialogBox::ftVertical)
{

	assignPopup=new CAssignPopup(this);

	new FXLabel(getFrame(),_("Assign Hot Keys"));
	//new FXHorizontalSeparator(getFrame(),SEPARATOR_GROOVE|LAYOUT_FILL_X|LAYOUT_BOTTOM);

	FXPacker *t;

	// build hot keys list 
	t=new FXPacker(getFrame(),LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0, 4,4,2,3, 0,0);
		t=new FXPacker(t,LAYOUT_FILL_X|LAYOUT_FILL_Y | FRAME_SUNKEN|FRAME_THICK, 0,0,0,0, 0,0,0,0, 0,0);
			hotkeyList=new FXIconList(t,this,ID_HOTKEY_LIST,HSCROLLER_NEVER|ICONLIST_BROWSESELECT|LAYOUT_FILL_X|LAYOUT_FILL_Y);

			/*
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
			*/

				//soundList->getHeader()->setFont(soundListHeaderFont);
				hotkeyList->getHeader()->setPadLeft(2);
				hotkeyList->getHeader()->setPadRight(2);
				hotkeyList->getHeader()->setPadTop(0);
				hotkeyList->getHeader()->setPadBottom(0);

				hotkeyList->appendHeader(_("Action"),NULL,400);
				hotkeyList->appendHeader(_("Hot Key"),NULL,9999);

				hotkeyList->setSortFunc(sortfunc);

	new FXButton(getFrame(),_("Assign\tOr Double-Click An Item"),NULL,this,ID_ASSIGN_BUTTON);

}

CKeyBindingsDialog::~CKeyBindingsDialog()
{
	delete assignPopup;
}

long CKeyBindingsDialog::onDefineKeyBinding(FXObject *sender,FXSelector sel,void *ptr)
{
	FXint index=hotkeyList->getCurrentItem();
	if(index>=0)
	{
		string name,key;
		parseItemText(hotkeyList->getItemText(index).text(),name,key);

		FXuint keycode=assignPopup->showIt();
		if(keycode>0)
		{
			if(keycode==1)
			{ // special return value meaning to unassign
				key="";
			}
			else
			{
				string tmp_key=fxunparseAccel(keycode).text();
				if(tmp_key=="")
					Error(_("Unhandled Key Combination"));
				else
				{
					// verify if it's already been assigned
					for(FXint t=0;t<hotkeyList->getNumItems();t++)
					{
						if(t==index)
							continue; // skip the one we're assigning to

						string name,key;
						parseItemText(hotkeyList->getItemText(t).text(),name,key);
						
						if(key==tmp_key)
						{
							if(Question("'"+tmp_key+"'"+_(" is already assigned to: ")+name+"\n\n"+_("Do you want to override the existing key assignment?"),yesnoQues)==yesAns)
							{
								hotkeyList->setItemText(t,name.c_str());
								break;
							}
							else
								return 1; // cancel the assignment
						}
					}
					
					// ok make the assignment
					key=tmp_key;
				}
			}

			hotkeyList->setItemText(index,(name+"\t"+key).c_str());
		}

	}

	return 1;
}

bool CKeyBindingsDialog::showIt(const map<string,FXMenuCommand *> &keyBindingRegistry)
{
	hotkeyList->clearItems();

	for(map<string,FXMenuCommand *>::const_iterator i=keyBindingRegistry.begin(); i!=keyBindingRegistry.end(); i++)
	{
		const string name=i->first;

		string key="";
		if(gKeyBindingsStore->keyExists(name))
			key=gKeyBindingsStore->getValue<string>(name);

		hotkeyList->appendItem( (i->first+"\t"+key).c_str() );
	}

	hotkeyList->sortItems();

	if(execute())
	{
		for(FXint t=0;t<hotkeyList->getNumItems();t++)
		{
			string name,key;
			parseItemText(hotkeyList->getItemText(t).text(),name,key);

			gKeyBindingsStore->setValue<string>(name,key);
		}
		gKeyBindingsStore->save();

		
		return true;
	}
	return false;
}

FXString fxunparseAccel(FXHotKey key)
{
	FXString s;

	FXuint code=key&0xffff;
	FXuint mods=(key&0xffff0000)>>16;

	// handle modifier keys
	if(mods&CONTROLMASK)
		s+="ctrl+";
	if(mods&ALTMASK)
		s+="alt+";
	if(mods&SHIFTMASK)
		s+="shift+";
	if(mods&METAMASK)
		s+="meta+";


	// handle some special keys
	switch(code)
	{
		case KEY_Home:
			s+="Home";
			break;

		case KEY_End:
			s+="End";
			break;

		case KEY_Page_Up:
			s+="PgUp";
			break;

		case KEY_Page_Down:
			s+="PgDn";
			break;

		case KEY_Left:
			s+="Left";
			break;

		case KEY_Right:
			s+="Right";
			break;

		case KEY_Up:
			s+="Up";
			break;

		case KEY_Down:
			s+="Down";
			break;

		case KEY_Insert:
			s+="Ins";
			break;

		case KEY_Delete:
			s+="Del";
			break;

		case KEY_Escape:
			s+="Esc";
			break;

		case KEY_Tab:
			s+="Tab";
			break;

		case KEY_Return:
			s+="Return";
			break;

		case KEY_BackSpace:
			s+="Back";
			break;

		case KEY_space:
			s+="Space";
			break;

		case KEY_F1:
		case KEY_F2:
		case KEY_F3:
		case KEY_F4:
		case KEY_F5:
		case KEY_F6:
		case KEY_F7:
		case KEY_F8:
		case KEY_F9:
		case KEY_F10:
		case KEY_F11:
		case KEY_F12:
		case KEY_F13:
		case KEY_F14:
		case KEY_F15:
		case KEY_F16:
		case KEY_F17:
		case KEY_F18:
		case KEY_F19:
		case KEY_F20:
		case KEY_F21:
		case KEY_F22:
		case KEY_F23:
		case KEY_F24:
		case KEY_F25:
		case KEY_F26:
		case KEY_F27:
		case KEY_F28:
		case KEY_F29:
		case KEY_F30:
		case KEY_F31:
		case KEY_F32:
		case KEY_F33:
		case KEY_F34:
		case KEY_F35:
		{
			char buffer[64];
			sprintf(buffer,"F%d",code-KEY_F1+1);
			s+=buffer;
		}
			break;

		default:
			if(isprint(code))
			{
				if(mods&SHIFTMASK)
					s+=(char)toupper(code);
				else
					s+=(char)tolower(code);
			}
			else
			{
				return ""; // return that it's unknown
				/* would like to do this, but fxparseAccel wouldn't handle it inversly .. can I get this changed?
				char buffer[64];
				sprintf(buffer,"#%d",code);
				s+=buffer;
				*/
			}
			break;
	}

	return s;
}
