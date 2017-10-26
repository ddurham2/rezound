#ifndef __RecentActionsMenu_H__
#define __RecentActionsMenu_H__

#include <QMenu>
#include <QVariant>

#include <string>
using namespace std;

#include "MainWindow.h"
#include "RezAction.h"

extern const string escapeAmpersand(const string i); // defined in StatusComm.cpp
class RecentActionsMenu : public QMenu
{
	Q_OBJECT
public:
    RecentActionsMenu(MainWindow *mainWindow) :
		m_mainWindow(mainWindow)
	{
		connect(this,SIGNAL(aboutToShow()),this,SLOT(onAboutToShow()));
	}

    virtual ~RecentActionsMenu()
	{
	}

private Q_SLOTS:

	void onAboutToShow()
	{
		clear();

		// create menu items
        const vector<RezAction *> &recentActions=m_mainWindow->m_recentActions;
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

        RezAction *actionToDo=(RezAction *)  (item->data().value<void *>());
		actionToDo->activate(QAction::Trigger);
	}

private:
	MainWindow *m_mainWindow;

};

#endif
