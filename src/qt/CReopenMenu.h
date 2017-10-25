#ifndef __CReopenMenu_H__
#define __CReopenMenu_H__

#include <QMenu>

#include <string>
using namespace std;

#include "CSoundFileManager.h"

#include "../backend/CActionParameters.h"
#include "../backend/File/FileActions.h"

extern const string escapeAmpersand(const string i); // defined in CStatusComm.cpp
class CReopenMenu : public QMenu
{
	Q_OBJECT
public:
	CReopenMenu()
	{
		connect(this,SIGNAL(aboutToShow()),this,SLOT(onAboutToShow()));

		reopenActionFactory=new COpenAudioFileActionFactory(NULL);
	}

	virtual ~CReopenMenu()
	{
	}

private Q_SLOTS:

	void onAboutToShow()
	{
		clear();

		// create menu items
		size_t reopenSize=gSoundFileManager->getReopenHistorySize();
		if(reopenSize<=0)
			return;
		for(size_t t=0;t<reopenSize;t++)
		{
			QAction *action=addAction(escapeAmpersand(gSoundFileManager->getReopenHistoryItem(t)).c_str());
			connect(action,SIGNAL(triggered()),this,SLOT(onItem_triggered()));
		}
	}

	void onItem_triggered()
	{
		QAction *item=dynamic_cast<QAction *>(QObject::sender());
		if(!item)
			return;

		vector<string> filenames;
		filenames.push_back(item->text().toStdString());

		CActionParameters actionParameters(gSoundFileManager);
		actionParameters.setValue<vector<string> >("filenames",filenames);

		reopenActionFactory->performAction(NULL,&actionParameters,false);
	}

private:
	AActionFactory *reopenActionFactory;
};

#endif
