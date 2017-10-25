#ifndef __CTransportWindow_H__
#define __CTransportWindow_H__

#include "../../config/common.h"

#include "ui_CTransportWindow.h"

class CMainWindow;
class CRezAction;

class CTransportWindow : public QWidget, public Ui::TransportWindow
{
	Q_OBJECT
public:
	CRezAction *m_shuttleBackwardAction;
	CRezAction *m_shuttleAmountAction;
	CRezAction *m_shuttleForwardAction;

	CTransportWindow(QWidget *parent, Qt::WindowFlags flags = 0);
	virtual ~CTransportWindow();

	void setMainWindow(CMainWindow *mainWindow);

	//void positionShuttleGivenSpeed(double seekSpeed,const string shuttleControlScalar,bool springBack); ??? I think I'm needed when switching between windows

	bool eventFilter(QObject *watched, QEvent *event);
	
private Q_SLOTS:
	void on_playAllButton_clicked();
	void on_playAllLoopedButton_clicked();
	void on_playSelectionOnceButton_clicked();
	void on_playFromStartPositionButton_clicked();
	void on_playSelectionLoopedButton_clicked();
	void on_playSelectionLoopedSkipMostButton_clicked();
	void on_playSelectionLoopedGapBeforeRepeatButton_clicked();
	void on_playFromLeftEdgeOfScreenToEndButton_clicked();
	void on_stopButton_clicked();
	void on_pauseButton_clicked();
	void on_recordButton_clicked();
	void on_jumpToBeginningButton_clicked();
	void on_jumpToStartPositionButton_clicked();
	void on_jumpToPrevCueButton_clicked();
	void on_jumpToNextCueButton_clicked();
	void on_shuttleDial_sliderReleased();
	void on_shuttleDial_valueChanged(int value);
	void on_shuttleSpringButton_toggled(bool checked);
	void on_shuttleScaleButton_clicked();

private:
	CMainWindow *m_mainWindow;
};

#endif
