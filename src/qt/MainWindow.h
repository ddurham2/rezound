#ifndef MainWindow_H__
#define MainWindow_H__

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
//class TransportWindow;
//class MiscControlsWindow;
//class CSoundListWindow;
//class CPlaybackLevelMeters;
//class CStereoPhaseMeters;
//class CFrequencyAnalyzer;

#include "RezAction.h"

#include "ui_MainWindow.h"
class MainWindow : public QMainWindow, private Ui::MainWindow
{
	Q_OBJECT

public:
    MainWindow(QWidget * parent = 0);
    virtual ~MainWindow();

	void backendInitialized();
	void createMenus();

	QLayout *getSoundWindowLayout() { return contentFrame->layout(); }
    SoundListWindow *soundListWindow() { return m_soundListWindow; }
    MiscControlsWindow *miscControlsWindow() { return m_miscControlsWindow; }
    PlaybackLevelMeters *levelMeters() { return m_levelMeters; }
    StereoPhaseMeters *stereoPhaseMeters() { return m_stereoPhaseMeters; }
    FrequencyAnalyzer *frequencyAnalyzer() { return m_frequencyAnalyzer; }

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
	//TransportWindow *m_transportWindow;
	//MiscControlsWindow *m_miscControlsWindow;
	//CSoundListWindow *m_soundListWindow;
	//CPlaybackLevelMeters  *m_levelMeters;
	//CStereoPhaseMeters *m_stereoPhaseMeters;
	//CFrequencyAnalyzer *m_frequencyAnalyzer;

	//AActionFactory *openAudioFileFactory;
	QAction *m_lastToolbarsMenuAction;

	friend class RecentActionsMenu;
    vector<RezAction *> m_recentActions;


    friend class MiscControlsWindow;
    map<const string,RezAction *> actionRegistry;
    void registerAction(RezAction *action);
	void buildActionMap();
	void buildMenu(QMenu *menu,const CNestedDataFile *menuLayoutFile,const string menuKey,const string actionName);
	void buildLADSPAMenus();


	void setupKeyBindings();

};

#endif
