#ifndef __CMainWindow_H__
#define __CMainWindow_H__

#include "../../config/common.h"

#include <QMainWindow>
#include <QVBoxLayout>
#include <QStackedLayout>

#include <string>
#include <map>
#include <vector>
using namespace std;

class AActionFactory;
class CNestedDataFile;
class ASoundPlayer;

// docked
//class CTransportWindow;
//class CMiscControlsWindow;
//class CSoundListWindow;
//class CPlaybackLevelMeters;
//class CStereoPhaseMeters;
//class CFrequencyAnalyzer;

#include "CRezAction.h"

#include "ui_CMainWindow.h"
class CMainWindow : public QMainWindow, private Ui::MainWindow
{
	Q_OBJECT

public:
	CMainWindow(QWidget * parent = 0, Qt::WindowFlags flags = 0);
	virtual ~CMainWindow();

	void backendInitialized();
	void createMenus();

	QLayout *getSoundWindowLayout() { return contentFrame->layout(); }
	CSoundListWindow *soundListWindow() { return m_soundListWindow; }
	CMiscControlsWindow *miscControlsWindow() { return m_miscControlsWindow; }
	CPlaybackLevelMeters *levelMeters() { return m_levelMeters; }
	CStereoPhaseMeters *stereoPhaseMeters() { return m_stereoPhaseMeters; }
	CFrequencyAnalyzer *frequencyAnalyzer() { return m_frequencyAnalyzer; }

	void positionShuttleGivenSpeed(double seekSpeed,const string shuttleControlScalar,bool springBack);

	void restoreDockState();

	void switchToWindow(int index);

	void setSoundPlayerForMeters(ASoundPlayer *player);

protected:
	void showEvent(QShowEvent *e);
	void closeEvent(QCloseEvent *e);
	void timerEvent(QTimerEvent *ev);

private Q_SLOTS:

	/*
	//void button1Clicked()
	void on_pushButton_clicked()
	{
		//lineEdit->insert("hello world");
	}
	*/

	//void on_action_Open_triggered();
	//void on_action_Quit_triggered();

	void onViewMenu_aboutToShow();

	void onRezAction_triggered();

private:
	int m_recordMacroTimerID;

	// docked windows
	//CTransportWindow *m_transportWindow;
	//CMiscControlsWindow *m_miscControlsWindow;
	//CSoundListWindow *m_soundListWindow;
	//CPlaybackLevelMeters  *m_levelMeters;
	//CStereoPhaseMeters *m_stereoPhaseMeters;
	//CFrequencyAnalyzer *m_frequencyAnalyzer;

	//AActionFactory *openAudioFileFactory;
	QAction *m_lastToolbarsMenuAction;

	friend class CRecentActionsMenu;
	vector<CRezAction *> m_recentActions;


	friend class CMiscControlsWindow;
	map<const string,CRezAction *> actionRegistry;
	void registerAction(CRezAction *action);
	void buildActionMap();
	void buildMenu(QMenu *menu,const CNestedDataFile *menuLayoutFile,const string menuKey,const string actionName);
	void buildLADSPAMenus();


	void setupKeyBindings();

};

#endif
