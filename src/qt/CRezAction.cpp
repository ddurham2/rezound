/* 
 * Copyright (C) 2007 - David W. Durham
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


#include <QApplication>
#include <QAbstractButton>

#include "CRezAction.h"

#include "CSoundFileManager.h"

#include "../backend/CActionParameters.h"
#include "../backend/AAction.h"

/*
FXDEFMAP(CRezAction) CRezActionMap[]=
{
	//Message_Type				ID				Message_Handler

	FXMAPFUNC(SEL_LEFTBUTTONRELEASE,	0,				CRezAction::onMouseClick),
	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,	0,				CRezAction::onMouseClick),

	FXMAPFUNC(SEL_KEYRELEASE,		0,				CRezAction::onKeyClick),

	FXMAPFUNC(SEL_COMMAND,			CRezAction::ID_HOTKEY,	CRezAction::onCommand),

#if REZ_FOX_VERSION<10304
	FXMAPFUNC(SEL_UPDATE,			FXWindow::ID_QUERY_TIP,		CRezAction::onQueryTip)
#else
	FXMAPFUNC(SEL_QUERY_TIP,		0,				CRezAction::onQueryTip)
#endif
};

FXIMPLEMENT(CRezAction,FXMenuCommand,CRezActionMap,ARRAYNUMBER(CRezActionMap))
*/

CRezAction::CRezAction(AActionFactory *_actionFactory,const QIcon &ic) :
	QAction(NULL),
	/*
	FXMenuCommand(
		p,
		// i18n/translate the action's name, append '...' if it has a dialog
		(string(gettext(_actionFactory->getName().c_str()))+(_actionFactory->hasDialog() ? "..." : "")).c_str(),
		(ic==NULL ? FOXIcons->normal_action_buff : ic),
		this,
		ID_HOTKEY,
		opts
		),
	*/
	m_actionFactory(_actionFactory),
	m_abstractButton(NULL),
	m_untranslatedText(_actionFactory->getName()+(_actionFactory->hasDialog() ? "..." : ""))
{
	setText((string(gettext(m_actionFactory->getName().c_str()))+(m_actionFactory->hasDialog() ? "..." : "")).c_str());
	setIcon(ic);
	setToolTip(m_actionFactory->getDescription().c_str());
	// ??? could look up hot-key in key_bindings.dat here

	connect(this,SIGNAL(triggered(bool)),this,SLOT(onTriggered()));
}

CRezAction::CRezAction(const string untranslatedText,const QIcon &ic) :
	QAction(NULL),
	m_actionFactory(NULL),
	m_abstractButton(NULL),
	m_untranslatedText(untranslatedText)
{
	setText(gettext(untranslatedText.c_str()));
	setIcon(ic);

	// ??? could look up hot-key in key_bindings.dat here
}

CRezAction::CRezAction(QAbstractButton *abstractButton) :
	QAction(NULL),
	m_actionFactory(NULL),
	m_abstractButton(abstractButton),
	m_untranslatedText(abstractButton->toolTip().toStdString())
{
	setText(abstractButton->toolTip());
	setIcon(abstractButton->icon());

	connect(this,SIGNAL(triggered(bool)),this,SLOT(onTriggered()));
}

CRezAction::~CRezAction()
{
}

void CRezAction::onTriggered()
{
	if(m_actionFactory)
	{
		CLoadedSound *activeSound=gSoundFileManager->getActive();
		if(activeSound!=NULL || !m_actionFactory->requiresALoadedSound)
		{
				// ??? let action parameters contain actionSound and the two bool parameters
				// they should have some flag which says that they would not be streamed to disk (if that were ever something I do with action parameters)
			CActionParameters actionParameters(gSoundFileManager);
					// ??? I should not use this modifier + action-trigger thing anymore.. just put the advanced controls on the dialog itself
			m_actionFactory->performAction(activeSound,&actionParameters,QApplication::keyboardModifiers()&Qt::ShiftModifier);

			// necessary if say a new file was opened, and it needs to be update (because AAction::performAction() didn't know about the new one that exists now)
			if(gSoundFileManager->getActive()!=activeSound)
				gSoundFileManager->updateAfterEdit();

			// done in AActionFactory::performAction() now
			/*
			// now, in case a newly created window has taken the place of the previously active window, it still may be necessary to update the previous active sound window 
			if(gSoundFileManager->getActive()!=activeSound && gSoundFileManager->isValidLoadedSound(activeSound))
				gSoundFileManager->updateAfterEdit(activeSound);
			*/
		}
		else
			QApplication::beep();
	}
	
	if(m_abstractButton)
	{
		m_abstractButton->click();
	}
}

static const string stripAmpersand(const string str)
{
	return istring(str).maximalSearchAndReplace("&","");
/*
	string stripped;
	for(size_t t=0;t<str.length();t++)
		if(str[t]!='&') stripped+=str[t];
	return stripped;
*/
}

static const string stripElipsis(const string str)
{
	if(str.rfind("...")==str.length()-3 && str.length()>3/*don't strip if that's the whole string*/)
		return str.substr(0,str.length()-3);
	return str;
}

const string CRezAction::getStrippedUntranslatedText() const
{
	return stripElipsis(stripAmpersand(getUntranslatedText()));
}

const string CRezAction::getStrippedText() const
{
	return stripElipsis(stripAmpersand(text().toStdString()));
}

