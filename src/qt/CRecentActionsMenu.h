#ifndef __CRecentActionsMenu_H__
#define __CRecentActionsMenu_H__

#include <QMenu>
#include <QVariant>

#include <string>
using namespace std;

#include "CMainWindow.h"
#include "CRezAction.h"

extern const string escapeAmpersand(const string i); // defined in CStatusComm.cpp
class CRecentActionsMenu : public QMenu
{
	Q_OBJECT
public:
	CRecentActionsMenu(CMainWindow *mainWindow) :
		m_mainWindow(mainWindow)
	{
		connect(this,SIGNAL(aboutToShow()),this,SLOT(onAboutToShow()));
	}

	virtual ~CRecentActionsMenu()
	{
	}

private Q_SLOTS:

	void onAboutToShow()
	{
		clear();

		// create menu items
		const vector<CRezAction *> &recentActions=m_mainWindow->m_recentActions;
		if(recentActions.size()<=0)
			return;
		for(size_t t=0;t<recentActions.size();t++)
		{
			QAction *action=addAction(recentActions[t]->text());
			action->setData(qVariantFromValue( (void *)(recentActions[t]) ));
			connect(action,SIGNAL(triggered()),this,SLOT(onItem_triggered()));
		}
	}

	void onItem_triggered()
	{
		QAction *item=dynamic_cast<QAction *>(QObject::sender());
		if(!item)
			return;

		CRezAction *actionToDo=(CRezAction *)  (item->data().value<void *>());
		actionToDo->activate(QAction::Trigger);
	}

private:
	CMainWindow *m_mainWindow;

};

#endif
