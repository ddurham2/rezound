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

#ifndef __CRezAction_H__
#define __CRezAction_H__

#include <QAction>
#include "../../config/common.h"

#include <string>

#include "settings.h"

class AActionFactory;
class QAbstractButton;

class CRezAction : public QAction
{
	Q_OBJECT
public:
	// when the action is triggered, the action factory will perform the action
	CRezAction(AActionFactory *_actionFactory, const QIcon &ic=loadIcon("normal_action_buff.png"));

	// the action's task should be handled by something else
	CRezAction(const string untranslatedText, const QIcon &ic=loadIcon("normal_action_buff.png"));

	// when the action is triggered, the button will be "click()-ed", also the button's icon will be this action's icon, and it's toolTip will be this action's text
	// the next of the button should aready be translated
	CRezAction(QAbstractButton *abstractButton);

	virtual ~CRezAction();

	const string getStrippedText() const;
	const string getUntranslatedText() const { return m_untranslatedText; }
	const string getStrippedUntranslatedText() const;

	AActionFactory *getActionFactory() { return m_actionFactory; }

private Q_SLOTS:
	void onTriggered();

private:
	AActionFactory * const m_actionFactory;
	QAbstractButton *m_abstractButton;
	string m_untranslatedText;
};

#endif
