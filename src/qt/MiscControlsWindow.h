#ifndef __MiscControlsWindow_H__
#define __MiscControlsWindow_H__

#include "../../config/common.h"

#include <QWidget>
#include "ui_MiscControlsWindow.h"

class MainWindow;

class MiscControlsWindow : public QWidget, public Ui::MiscControlsWindow
{
	Q_OBJECT

public:
    MiscControlsWindow(QWidget *parent, Qt::WindowFlags flags = 0);
    virtual ~MiscControlsWindow();

	void setMainWindow(MainWindow *mainWindow);

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
	MainWindow *m_mainWindow;
};

#endif
