#ifndef __shortcutKey_H__
#define __shortcutKey_H__

#include <QKeySequence>
#include "../misc/istring"

/*
 * This funtion parses strings like "ctrl+d" or "ctrl-d" or "ctrl+alt+g", etc.. into a QKeySequence
 */
QKeySequence parseKeySequence(string orig_str)
{
	if(orig_str.empty())
		return QKeySequence();

	int mods=0;
	int mainKey=-1;
	istring seq=istring(orig_str).upper();
	while(!seq.empty())
	{
		if(seq=="+")
		{ // special case
			mainKey=Qt::Key_Plus;
			break;
		}
		else if(seq=="-")
		{ // special case
			mainKey=Qt::Key_Minus;
			break;
		}

		string key;
		if(seq.find("+")!=istring::npos || seq.find("-")!=istring::npos)
		{
			if(seq.find("+")!=istring::npos)
				key=seq.eatField("+");
			else if(seq.find("-")!=istring::npos)
				key=seq.eatField("-");

			if(key=="CTRL" || key=="CTL")
				mods|=Qt::CTRL;
			else if(key=="ALT")
				mods|=Qt::ALT;
			else if(key=="SHIFT")
				mods|=Qt::SHIFT;
			else if(key=="META")
				mods|=Qt::META;
			else
				fprintf(stderr,"%s: unknown modifier key string: '%s' in sequence: '%s'\n",__func__,key.c_str(),orig_str.c_str());
		}
		else
		{
			key=seq;
			seq.clear();

			// fox supported beginning the sequence with # for a hex key code ???

			if(key.length()==1)
			{
				switch(key[0])
				{
				case ' ':
					mainKey=Qt::Key_Space; break;
				case 'A':
					mainKey=Qt::Key_A; break;
				case 'B':
					mainKey=Qt::Key_B; break;
				case 'C':
					mainKey=Qt::Key_C; break;
				case 'D':
					mainKey=Qt::Key_D; break;
				case 'E':
					mainKey=Qt::Key_E; break;
				case 'F':
					mainKey=Qt::Key_F; break;
				case 'G':
					mainKey=Qt::Key_G; break;
				case 'H':
					mainKey=Qt::Key_H; break;
				case 'I':
					mainKey=Qt::Key_I; break;
				case 'J':
					mainKey=Qt::Key_J; break;
				case 'K':
					mainKey=Qt::Key_K; break;
				case 'L':
					mainKey=Qt::Key_L; break;
				case 'M':
					mainKey=Qt::Key_M; break;
				case 'N':
					mainKey=Qt::Key_N; break;
				case 'O':
					mainKey=Qt::Key_O; break;
				case 'P':
					mainKey=Qt::Key_P; break;
				case 'Q':
					mainKey=Qt::Key_Q; break;
				case 'R':
					mainKey=Qt::Key_R; break;
				case 'S':
					mainKey=Qt::Key_S; break;
				case 'T':
					mainKey=Qt::Key_T; break;
				case 'U':
					mainKey=Qt::Key_U; break;
				case 'V':
					mainKey=Qt::Key_V; break;
				case 'W':
					mainKey=Qt::Key_W; break;
				case 'X':
					mainKey=Qt::Key_X; break;
				case 'Y':
					mainKey=Qt::Key_Y; break;
				case 'Z':
					mainKey=Qt::Key_Z; break;
				case '`': case '~':
					mainKey=Qt::Key_QuoteLeft; break;
				case '1': case '!':
					mainKey=Qt::Key_1; break;
				case '2': case '@':
					mainKey=Qt::Key_2; break;
				case '3': case '#':
					mainKey=Qt::Key_3; break;
				case '4': case '$':
					mainKey=Qt::Key_4; break;
				case '5': case '%':
					mainKey=Qt::Key_5; break;
				case '6': case '^':
					mainKey=Qt::Key_6; break;
				case '7': case '&':
					mainKey=Qt::Key_7; break;
				case '8': case '*':
					mainKey=Qt::Key_8; break;
				case '9': case '(':
					mainKey=Qt::Key_9; break;
				case '0': case ')':
					mainKey=Qt::Key_0; break;
				case '_':
					mainKey=Qt::Key_Underscore; break;
				case '=': case '+':
					mainKey=Qt::Key_Equal; break;
				case '[': case '{':
					mainKey=Qt::Key_BracketLeft; break;
				case ']': case '}':
					mainKey=Qt::Key_BracketRight; break;
				case '\\': case '|':
					mainKey=Qt::Key_Backslash; break;
				case ';': case ':':
					mainKey=Qt::Key_Semicolon; break;
				case '\'': case '"':
					mainKey=Qt::Key_Apostrophe; break;
				case ',': case '<':
					mainKey=Qt::Key_Comma; break;
				case '.': case '>':
					mainKey=Qt::Key_Period; break;
				case '/': case '?':
					mainKey=Qt::Key_Slash; break;
				}
			}
			else if(key.length()==2)
			{
				if(key=="UP")
					mainKey=Qt::Key_Up;
				else if(key=="F1")
					mainKey=Qt::Key_F1;
				else if(key=="F2")
					mainKey=Qt::Key_F2;
				else if(key=="F3")
					mainKey=Qt::Key_F3;
				else if(key=="F4")
					mainKey=Qt::Key_F4;
				else if(key=="F5")
					mainKey=Qt::Key_F5;
				else if(key=="F6")
					mainKey=Qt::Key_F6;
				else if(key=="F7")
					mainKey=Qt::Key_F7;
				else if(key=="F8")
					mainKey=Qt::Key_F8;
				else if(key=="F9")
					mainKey=Qt::Key_F9;
			}
			else if(key.length()==3)
			{
				if(key=="TAB")
					mainKey=Qt::Key_Tab;
				else if(key=="END")
					mainKey=Qt::Key_End;
				else if(key=="INS")
					mainKey=Qt::Key_Insert;
				else if(key=="DEL")
					mainKey=Qt::Key_Delete;
				else if(key=="ESC")
					mainKey=Qt::Key_Escape;
				else if(key=="SPC")
					mainKey=Qt::Key_Space;
				else if(key=="F10")
					mainKey=Qt::Key_F10;
				else if(key=="F11")
					mainKey=Qt::Key_F11;
				else if(key=="F12")
					mainKey=Qt::Key_F12;
				else if(key=="F13")
					mainKey=Qt::Key_F13;
				else if(key=="F14")
					mainKey=Qt::Key_F14;
				else if(key=="F15")
					mainKey=Qt::Key_F15;
				else if(key=="F16")
					mainKey=Qt::Key_F16;
				else if(key=="F17")
					mainKey=Qt::Key_F17;
				else if(key=="F18")
					mainKey=Qt::Key_F18;
				else if(key=="F19")
					mainKey=Qt::Key_F19;
				else if(key=="F20")
					mainKey=Qt::Key_F20;
				else if(key=="F21")
					mainKey=Qt::Key_F21;
				else if(key=="F22")
					mainKey=Qt::Key_F22;
				else if(key=="F23")
					mainKey=Qt::Key_F23;
				else if(key=="F24")
					mainKey=Qt::Key_F24;
				else if(key=="F25")
					mainKey=Qt::Key_F25;
				else if(key=="F26")
					mainKey=Qt::Key_F26;
				else if(key=="F27")
					mainKey=Qt::Key_F27;
				else if(key=="F28")
					mainKey=Qt::Key_F28;
				else if(key=="F29")
					mainKey=Qt::Key_F29;
				else if(key=="F30")
					mainKey=Qt::Key_F30;
				else if(key=="F31")
					mainKey=Qt::Key_F31;
				else if(key=="F32")
					mainKey=Qt::Key_F32;
				else if(key=="F33")
					mainKey=Qt::Key_F33;
				else if(key=="F34")
					mainKey=Qt::Key_F34;
				else if(key=="F35")
					mainKey=Qt::Key_F29;
			}
			else
			{
				if(key=="ENTER")
					mainKey=Qt::Key_Enter;
				else if(key=="RETURN")
					mainKey=Qt::Key_Return;
				else if(key=="ESCAPE")
					mainKey=Qt::Key_Escape;
				else if(key=="SPACE")
					mainKey=Qt::Key_Space;
				else if(key=="BACK" || key=="BACKSPACE")
					mainKey=Qt::Key_Backspace;
				else if(key=="DOWN")
					mainKey=Qt::Key_Down;
				else if(key=="LEFT")
					mainKey=Qt::Key_Left;
				else if(key=="RIGHT")
					mainKey=Qt::Key_Right;
				else if(key=="INSERT")
					mainKey=Qt::Key_Insert;
				else if(key=="DELETE")
					mainKey=Qt::Key_Delete;
				else if(key=="PAUSE")
					mainKey=Qt::Key_Pause;
				else if(key=="PRINT")
					mainKey=Qt::Key_Print;
				else if(key=="SYSREQ")
					mainKey=Qt::Key_SysReq;
				else if(key=="CLEAR")
					mainKey=Qt::Key_Clear;
				else if(key=="HOME")
					mainKey=Qt::Key_Home;
				else if(key=="PGUP" || key=="PAGEUP")
					mainKey=Qt::Key_PageUp;
				else if(key=="PGDN" || key=="PAGEDOWN")
					mainKey=Qt::Key_PageDown;
				else if(key=="CAPSLOCK")
					mainKey=Qt::Key_CapsLock;
				else if(key=="NUMLOCK")
					mainKey=Qt::Key_NumLock;
				else if(key=="SCROLLLOCK")
					mainKey=Qt::Key_ScrollLock;
				else if(key=="MENU")
					mainKey=Qt::Key_Menu;
			}
		}
	}

	if(mainKey<0)
	{
		fprintf(stderr,"%s: no main key found in key sequence: %s\n",__func__,orig_str.c_str());
		return QKeySequence();
	}

	return QKeySequence(mainKey|mods);
}

const string unparseKeySequence(const QKeySequence &key)
{
return "";
}

#endif
