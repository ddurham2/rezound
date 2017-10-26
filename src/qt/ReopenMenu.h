#ifndef __ReopenMenu_H__
#define __ReopenMenu_H__

#include <QMenu>

#include <string>
using namespace std;

#include "SoundFileManager.h"

#include "../backend/CActionParameters.h"
#include "../backend/File/FileActions.h"

extern const string escapeAmpersand(const string i); // defined in StatusComm.cpp
class ReopenMenu : public QMenu
{
	Q_OBJECT
public:
    ReopenMenu()
	{
		connect(this,SIGNAL(aboutToShow()),this,SLOT(onAboutToShow()));

		reopenActionFactory=new COpenAudioFileActionFactory(NULL);
	}

    virtual ~ReopenMenu()
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
