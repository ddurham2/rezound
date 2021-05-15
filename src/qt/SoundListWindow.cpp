#include "SoundListWindow.h"

#include <QScrollBar>
#include <QStringList>
#include "../misc/CPath.h"

#include "SoundFileManager.h"
#include "SoundWindow.h"
#include "../backend/CLoadedSound.h"
#include "../backend/CSound.h"

Q_DECLARE_METATYPE(SoundWindow*)

SoundListWindow::SoundListWindow(QWidget * parent)
{
	setupUi(this);

	loadedSoundsList->setColumnWidth(0,40);
	loadedSoundsList->setColumnWidth(1,40);

	QStringList headers;
		headers.push_back(_("#"));
		headers.push_back(_("M"));
		headers.push_back(_("Name"));
		headers.push_back(_("Path"));
	loadedSoundsList->setHeaderLabels(headers);
}

SoundListWindow::~SoundListWindow()
{
}

void SoundListWindow::rebuildSoundWindowList()
{
	int ypos=loadedSoundsList->verticalScrollBar()->value();

	loadedSoundsList->clear();
	for(size_t t=0;t<gSoundFileManager->getOpenedCount();t++)
	{
		SoundWindow *win=gSoundFileManager->getSoundWindow(t);

		// add to sound window list
		CPath p(win->loadedSound->getFilename());

		QStringList fields;
		fields.push_back(QString::number(t+1));
		fields.push_back(win->loadedSound->sound->isModified() ? "*" : "");
		fields.push_back(p.baseName().c_str());
		fields.push_back(CPath(p.realPath()).dirName().c_str());

		QTreeWidgetItem *twi=new QTreeWidgetItem(fields);
		twi->setData(0,Qt::UserRole,qVariantFromValue(win));

		loadedSoundsList->addTopLevelItem(twi);
	}


	loadedSoundsList->verticalScrollBar()->setValue(ypos); // scroll to the same position we were at before

	// if it isn't visible make the active sound visible in the sound list
	SoundWindow *active=gSoundFileManager->getActiveWindow();
	if(active!=NULL)
	{
		for(int t=0;t<loadedSoundsList->topLevelItemCount();t++)
		{
			if(loadedSoundsList->topLevelItem(t)->data(0,Qt::UserRole).value<SoundWindow *>()==active)
			{
				loadedSoundsList->setCurrentItem(loadedSoundsList->topLevelItem(t));
				loadedSoundsList->scrollToItem(loadedSoundsList->topLevelItem(t));
				break;
			}
		}
	}
}

void SoundListWindow::switchToWindow(int index)
{
	if(index>=0 && index<loadedSoundsList->topLevelItemCount())
	{
		loadedSoundsList->setCurrentItem(loadedSoundsList->topLevelItem(index));
		loadedSoundsList->scrollToItem(loadedSoundsList->topLevelItem(index));
	}
}

void SoundListWindow::switchToWindow(SoundWindow *window)
{
	if(!window)
		return;

	for(int index=0;index<loadedSoundsList->topLevelItemCount();index++)
	{ // find the index in the sound list of the window and set it to be the current item
		if(loadedSoundsList->topLevelItem(index)->data(0,Qt::UserRole).value<SoundWindow *>()==window)
		{
			switchToWindow(index);
			break;
		}
	}
}

// this makes the initial size of the top doc area sane
QSize SoundListWindow::sizeHint() const
{
	return QSize(400,100);
}


void SoundListWindow::on_loadedSoundsList_currentItemChanged(QTreeWidgetItem *currentItem,QTreeWidgetItem *previous)
{
	if(currentItem)
		gSoundFileManager->setActiveSoundWindow(currentItem->data(0,Qt::UserRole).value<SoundWindow *>());
}

