#ifndef __CSoundListWindow_H__
#define __CSoundListWindow_H__

#include "../../config/common.h"

#include "ui_CSoundListWindow.h"

class CSoundWindow;

class CSoundListWindow : public QWidget, private Ui::SoundListWindow
{
	Q_OBJECT

public:
	CSoundListWindow(QWidget * parent = 0, Qt::WindowFlags flags = 0);
	virtual ~CSoundListWindow();

	void rebuildSoundWindowList();

	void switchToWindow(int index);

	void switchToWindow(CSoundWindow *window);

protected:
	// this makes the initial size of the top doc area sane
	QSize sizeHint() const;


private Q_SLOTS:
	void on_loadedSoundsList_currentItemChanged(QTreeWidgetItem *currentItem,QTreeWidgetItem *previous);

private:
};

#endif
