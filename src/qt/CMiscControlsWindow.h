#ifndef __CMiscControlsWindow_H__
#define __CMiscControlsWindow_H__

#include "../../config/common.h"

#include <QWidget>
#include "ui_CMiscControlsWindow.h"

class CMainWindow;

class CMiscControlsWindow : public QWidget, public Ui::MiscControlsWindow
{
	Q_OBJECT

public:
	CMiscControlsWindow(QWidget *parent, Qt::WindowFlags flags = 0);
	virtual ~CMiscControlsWindow();

	void setMainWindow(CMainWindow *mainWindow);

	void init();

	void setWhichClipboard(size_t whichClipboard);

private Q_SLOTS:
	void on_followPlayPositionCheckBox_toggled();
	void on_clippingWarningCheckBox_toggled();
	void on_drawVerticalCuePositionsCheckBox_toggled();
	void on_recordMacroButton_clicked();
	void on_crossfadeEdgesComboBox_activated(int index);
	void on_crossfadeEdgesSettingsButton_clicked();
	void on_clipboardComboBox_activated(int index);

private:
	CMainWindow *m_mainWindow;
};

#endif
