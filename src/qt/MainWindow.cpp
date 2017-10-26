#include "MainWindow.h"

#include "TransportWindow.h"
#include "MiscControlsWindow.h"
#include "SoundListWindow.h"
#include "LevelMeters.h"
#include "StereoPhaseMeters.h"
#include "FrequencyAnalyzer.h"

#include "SoundWindow.h"

#include "UserNotesDialog.h"
#include "KeyBindingsDialog.h"

#include "../backend/main_controls.h"
#include "../backend/CActionParameters.h"
#include "../backend/AAction.h"

#include "../backend/CRunMacroAction.h"

#include "FileActionDialogs.h"
#include "../backend/File/FileActions.h"

#include "FilterActionDialogs.h"
#include "../backend/Filters/FilterActions.h"

#include "RemasterActionDialogs.h"
#include "../backend/Remaster/RemasterActions.h"

#include "GenerateActionDialogs.h"
#include "../backend/Generate/GenerateActions.h"

#include "../backend/Looping/LoopingActions.h"
#include "LoopingActionDialogs.h"

#include "ActionParam/ChannelSelectDialog.h"
#include "ActionParam/PasteChannelsDialog.h"
#include "EditActionDialogs.h"
#include "../backend/Edits/EditActions.h"
#include "../backend/Edits/CCueAction.h"

#include "EffectActionDialogs.h"
#include "../backend/Effects/EffectActions.h"

#include "../backend/LADSPA/LADSPAActions.h"


#include "../misc/urlencode.h"
#include "../misc/CPath.h"

#include "shortcutKey.h"

#include "SoundFileManager.h"

#include <stdio.h>

#include <QVBoxLayout>
#include <QToolBar>
#include <QDockWidget>
#include <QMenu>
#include <QMenuBar>

#define DOCK_STATE_INFO_VERSION 0

MainWindow::MainWindow(QWidget * parent, Qt::WindowFlags flags) :
	QMainWindow(parent,flags),

	m_lastToolbarsMenuAction(NULL)
{
	setupUi(this);
	setStyleSheet("QAbstractButton { qproperty-iconSize: 28px 28px; } ");
}

void MainWindow::backendInitialized()
{
	m_transportWindow->setMainWindow(this);
	m_miscControlsWindow->setMainWindow(this);

	m_recordMacroTimerID=startTimer(1000);

	/* this would need to be done programatically .. after menus are built.. 
	// add toolbars
	{
		QToolBar *tb;
		tb=addToolBar("File");
		tb->setObjectName("File");
		tb->addAction(action_Open);
		tb->addAction(action_Quit);

		tb=addToolBar("File2");
		tb->setObjectName("File2");
		tb->addAction(action_Open);
		tb->addAction(action_Quit);

		tb=addToolBar("File3");
		tb->setObjectName("File3");
		tb->addAction(action_Open);
		tb->addAction(action_Quit);
	}
	*/

	// add dock widgets
	{
		QDockWidget *dw;
		QWidget *w;
		QSize s;

#if 0
		dw=new QDockWidget(_("Transport"));
		dw->setObjectName("TransportWindow");
		w=m_transportWindow=new TransportWindow(this);
		//s=w->size(); // 212x146
		//s.rheight()+=15;
		//s.rwidth()+=4;
		//dw->setFixedSize(s);
		dw->setWidget(w);
		addDockWidget(Qt::TopDockWidgetArea, dw);

		dw=new QDockWidget(_("Misc Controls"));
		dw->setObjectName("MiscControlsWindow");
		w=m_miscControlsWindow=new MiscControlsWindow(this);
		//s=w->size(); // 212x146
		//s.rheight()+=15;
		//s.rwidth()+=4;
		//dw->setFixedSize(s);
		dw->setWidget(w);
		addDockWidget(Qt::TopDockWidgetArea, dw);

		dw=new QDockWidget(_("Loaded Sound List"));
		dw->setObjectName("SoundListWindow");
		dw->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
		w=m_soundListWindow=new SoundListWindow();
		dw->setWidget(w);
		addDockWidget(Qt::TopDockWidgetArea, dw);

		dw=new QDockWidget(_("Level"));
		dw->setObjectName("LevelMeters");
		dw->setFeatures(dw->features()|QDockWidget::DockWidgetVerticalTitleBar);
		dw->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
		w=m_levelMeters=new PlaybackLevelMeters(0);
		dw->setWidget(w);
		addDockWidget(Qt::BottomDockWidgetArea, dw);

		dw=new QDockWidget(_("Stereo Phase"));
		dw->setObjectName("StereoPhaseMeters");
		dw->setFeatures(dw->features()|QDockWidget::DockWidgetVerticalTitleBar);
		w=m_stereoPhaseMeters=new StereoPhaseMeters(0);
		dw->setWidget(w);
		addDockWidget(Qt::BottomDockWidgetArea, dw);

		dw=new QDockWidget(_("Frequency Analyzer"));
		dw->setObjectName("FrequencyAnalyzer");
		dw->setFeatures(dw->features()|QDockWidget::DockWidgetVerticalTitleBar);
		w=m_frequencyAnalyzer=new FrequencyAnalyzer(0);
		dw->setWidget(w);
		addDockWidget(Qt::BottomDockWidgetArea, dw);
#endif

	}


	// add the layout to the middle frame
	contentFrame->setLayout(new QVBoxLayout);
	contentFrame->layout()->setContentsMargins(0,0,0,0);
		// ??? if I'm using a stacked layout, that may be the problem with the resizing while two files are loaded

/* done in designer (but can we translate???)
	// add the different types of crossfading to the combobox
	{
		crossfadeEdgesComboBox->addItem(_("No Crossfade"));
		crossfadeEdgesComboBox->addItem(_("Crossfade Inner Edges"));
		crossfadeEdgesComboBox->addItem(_("Crossfade Outer Edges"));
	}
*/

/*
	// add the column headers for the loaded sounds list
	{
		QStringList headers;
		headers.append("#");
		headers.append("M");
		headers.append("Name");
		headers.append("Path");
		loadedSoundsList->setHeaderLabels(headers);

		loadedSoundsList->resizeColumnToContents(0);
		loadedSoundsList->resizeColumnToContents(1);
	}
*/

#if 0
	// add the meters
	{
		QLayout *layout;

		m_levelMeters=new PlaybackLevelMeters(0);
		layout=new QHBoxLayout;
		layout->setMargin(0);
		layout->addWidget(m_levelMeters);
		levelMetersFrame->setLayout(layout);

		m_stereoPhaseMeters=new StereoPhaseMeters(0);
		stereoPhaseFrame->layout()->addWidget(m_stereoPhaseMeters);

		m_frequencyAnalyzer=new FrequencyAnalyzer(0);
		frequencyAnalyzerFrame->layout()->addWidget(m_frequencyAnalyzer);
	}
#endif

	// or just name the method right: connect(pushButton, SIGNAL(clicked()), this, SLOT(button1Clicked()));

	//openAudioFileFactory=new COpenAudioFileActionFactory(new COpenAudioFileActionDialog(this));
}

MainWindow::~MainWindow()
{
}

void MainWindow::setSoundPlayerForMeters(ASoundPlayer *player)
{
	m_levelMeters->setSoundPlayer(player);
	m_stereoPhaseMeters->setSoundPlayer(player);
	m_frequencyAnalyzer->setSoundPlayer(player);
}

void MainWindow::restoreDockState() // called by FrontendHooks
{
	// restore how toolbars were positioned
	const string state=urldecode(gSettingsRegistry->getValue<string>("Qt4" DOT "mainWindowDockState"));
	restoreState(QByteArray(state.data(),state.length()),DOCK_STATE_INFO_VERSION);
}

void MainWindow::showEvent(QShowEvent *e)
{
	m_miscControlsWindow->init();
}

void MainWindow::closeEvent(QCloseEvent *e)
{
	if(QApplication::activeWindow()!=this)
	{ // don't allow a quit if there is a modal window showing
		gStatusComm->beep();
		e->ignore();
		return;
	}

	if(exitReZound(gSoundFileManager))
	{
		e->accept();
		//enableAutoRepeat(getApp()->getDisplay(),true); // QQQ (didn't work just putting this after application->run() in main.cpp)

		QByteArray stateInfo=saveState(DOCK_STATE_INFO_VERSION);
		gSettingsRegistry->setValue<string>("Qt4" DOT "mainWindowDockState",urlencode(string(stateInfo.data(),stateInfo.size())));

		QApplication::exit(0);
		return;
	}
	else
		e->ignore();
}

void MainWindow::positionShuttleGivenSpeed(double seekSpeed,const string shuttleControlScalar,bool springBack)
{
	/*
	int minValue,maxValue;
	shuttleDial->getRange(minValue,maxValue);

	FXint shuttlePos;
	if(seekSpeed==1.0)
		shuttlePos=0;
	else
	{
		const string &text=shuttleControlScalar;
		if(text=="1x")
		{
			if(seekSpeed>0.0)
				shuttlePos=(FXint)(seekSpeed*maxValue);
			else //if(seekSpeed<0.0)
				shuttlePos=(FXint)(-seekSpeed*minValue);
		}
		else if(text=="2x")
		{
			if(seekSpeed>0.0)
				shuttlePos=(FXint)((seekSpeed-1.0)*maxValue);
			else //if(seekSpeed<0.0)
				shuttlePos=(FXint)(-(seekSpeed+1.0)*minValue);
		}
		else if(text=="100x")
		{
			if(seekSpeed>0.0)
				shuttlePos=(FXint)(maxValue*sqrt((seekSpeed-1.0)/100.0));
			else //if(seekSpeed<0.0)
				shuttlePos=(FXint)(minValue*sqrt((seekSpeed+1.0)/-100.0));
		}
		else if(text==_("semitones"))
		{
			if(seekSpeed>0.0)
				shuttlePos=(FXint)(log(seekSpeed)/log(2.0)*maxValue);
			else //if(seekSpeed<0.0)
				shuttlePos=(FXint)(log(seekSpeed)/log(2.0)*minValue);
		}
		else
			throw runtime_error(string(__func__)+" -- internal error -- unhandled text for shuttleDialScaleButton: '"+text+"'");
		
	}

	shuttleDialScaleButton->setText(shuttleControlScalar.c_str());
	shuttleDialSpringButton->setState(springBack);
	shuttleDial->setValue(shuttlePos);
	*/
}


#if 0
void MainWindow::on_action_Open_triggered()
{
	/*
	SoundWindow *wv=new SoundWindow;
	contentFrame->layout()->addWidget(wv);
	*/
	CActionParameters ap(gSoundFileManager);
	openAudioFileFactory->performAction(NULL,&ap,false);
}
#endif

/*
void MainWindow::on_action_Quit_triggered()
{
	close();
}
*/

void MainWindow::onViewMenu_aboutToShow()
{
	QMenu *viewMenu=(QMenu *)QObject::sender();
	// since the View menu is about to show, we want to populate the toolsbars submenu with all the toolbars
	delete m_lastToolbarsMenuAction; // delete the last one we showed (which also removes it from the 'View' menu)
	QMenu *m=createPopupMenu();
	m->setTitle(_("Toolbars And Windows"));
	m_lastToolbarsMenuAction=viewMenu->insertMenu(viewMenu->actions()[0],m);
}

void MainWindow::switchToWindow(int index)
{
	m_soundListWindow->switchToWindow(index);
}

void MainWindow::onRezAction_triggered()
{
	RezAction *action=dynamic_cast<RezAction *>(QObject::sender());
	if(!action)
		return;

	// save as a recent action (but only if the action has an action factory)
	if(action->getActionFactory())
	{
		for(vector<RezAction *>::iterator i=m_recentActions.begin();i!=m_recentActions.end();i++)
		{
			if((*i)->text()==action->text())
			{
				RezAction *t=*i;
				m_recentActions.erase(i);
				m_recentActions.insert(m_recentActions.begin(),t);
				return;
			}
		}

		if(m_recentActions.size()>=10)
			m_recentActions.pop_back();
		m_recentActions.insert(m_recentActions.begin(),action);
	}

	if(!action->getActionFactory())
	{ // if it doesn't have an action factory, then main window handles its task

		// this slot is called for the few actions that do not have an actionFactory, but are action specifically handled by the main window
		const string strippedActionName=action->getStrippedUntranslatedText();

		// ====================
		// FILE ACTIONS
		// ====================
		if(strippedActionName=="Quit")
		{
			close();
		}
		else if(strippedActionName=="Close")
		{
			if(AActionFactory::macroRecorder.isRecording())
			{
				Message(_("Cannot close a file while recording a macro"));
				return;
			}
			closeSound(gSoundFileManager);
		}
		else if(strippedActionName=="Revert")
		{
			if(AActionFactory::macroRecorder.isRecording())
			{
				Message(_("Cannot revert a file while recording a macro"));
				return;
			}
			revertSound(gSoundFileManager);
		}
		else if(strippedActionName=="Record Macro")
		{
			recordMacro();
			if(AActionFactory::macroRecorder.isRecording())
			{
				m_miscControlsWindow->recordMacroButton->setCheckable(true);
				m_miscControlsWindow->recordMacroButton->setChecked(true);
				m_miscControlsWindow->recordMacroButton->setIcon(loadIcon("RedLED1.png"));
			}
			else
			{
				m_miscControlsWindow->recordMacroButton->setIcon(loadIcon("OffLED1.png"));
				m_miscControlsWindow->recordMacroButton->setChecked(false);
				m_miscControlsWindow->recordMacroButton->setCheckable(false);
			}
		}
		else if(strippedActionName=="About ReZound")
		{
			//(new AboutDialog(this))->exec();
		}
		else if(strippedActionName=="User Notes")
		{
			try
			{
				CLoadedSound *s=gSoundFileManager->getActive();
				if(s!=NULL)
					gUserNotesDialog->exec(s);
				else
					QApplication::beep();
			}
			catch(exception &e)
			{
				Error(e.what());
			}
		}
		else if(strippedActionName=="Setup Hotkeys")
		{
			try
			{
				if(gKeyBindingsDialog->exec(actionRegistry))
					setupKeyBindings();
			}
			catch(exception &e)
			{
				Error(e.what());
			}
		}
		else if(strippedActionName=="Defrag")
		{
			if(gSoundFileManager->getActive())
				gSoundFileManager->getActive()->sound->defragPoolFile();
			gSoundFileManager->updateAfterEdit();
		}
		else if(strippedActionName=="PrintSAT")
		{
			if(gSoundFileManager->getActive())
				gSoundFileManager->getActive()->sound->printSAT();
		}
		else if(strippedActionName=="VerifySAT")
		{
			if(gSoundFileManager->getActive())
				gSoundFileManager->getActive()->sound->verifySAT();
		}

		// ====================
		// VIEW ACTIONS
		// ====================
		else if(strippedActionName=="Zoom Out Full")
		{
			if(gSoundFileManager->getActiveWindow())
				gSoundFileManager->getActiveWindow()->horzZoomOutFull();
		}
		else if(strippedActionName=="Zoom Out")
		{
			if(gSoundFileManager->getActiveWindow())
				gSoundFileManager->getActiveWindow()->horzZoomOutSome();
		}
		else if(strippedActionName=="Zoom In")
		{
			if(gSoundFileManager->getActiveWindow())
				gSoundFileManager->getActiveWindow()->horzZoomInSome();
		}
		else if(strippedActionName=="Zoom Fit Selection")
		{
			if(gSoundFileManager->getActiveWindow())
				gSoundFileManager->getActiveWindow()->horzZoomSelectionFit();
		}
		else if(strippedActionName=="Find Start Position")
		{
			if(gSoundFileManager->getActiveWindow())
				gSoundFileManager->getActiveWindow()->centerStartPos();
		}
		else if(strippedActionName=="Find Stop Position")
		{
			if(gSoundFileManager->getActiveWindow())
				gSoundFileManager->getActiveWindow()->centerStopPos();
		}
		else if(strippedActionName=="Redraw")
		{
			if(gSoundFileManager->getActiveWindow())
				gSoundFileManager->getActiveWindow()->redraw();
		}

		else if(strippedActionName=="Switch to Previously Viewed File")
			m_soundListWindow->switchToWindow(gSoundFileManager->previousActiveWindow);
		else if(strippedActionName=="Switch to Loaded File 1")
			switchToWindow(0);
		else if(strippedActionName=="Switch to Loaded File 2")
			switchToWindow(1);
		else if(strippedActionName=="Switch to Loaded File 3")
			switchToWindow(2);
		else if(strippedActionName=="Switch to Loaded File 4")
			switchToWindow(3);
		else if(strippedActionName=="Switch to Loaded File 5")
			switchToWindow(4);
		else if(strippedActionName=="Switch to Loaded File 6")
			switchToWindow(5);
		else if(strippedActionName=="Switch to Loaded File 7")
			switchToWindow(6);
		else if(strippedActionName=="Switch to Loaded File 8")
			switchToWindow(7);
		else if(strippedActionName=="Switch to Loaded File 9")
			switchToWindow(8);
		else if(strippedActionName=="Switch to Loaded File 10")
			switchToWindow(9);

		// ====================
		// EDIT ACTIONS
		// ====================
		else if(strippedActionName=="Undo")
			undo(gSoundFileManager);
		else if(strippedActionName=="Clear Undo History")
			clearUndoHistory(gSoundFileManager);

	}
}


void MainWindow::timerEvent(QTimerEvent *ev)
{
	if(ev->timerId()==m_recordMacroTimerID)
	{
		static int counter=0;
		if(AActionFactory::macroRecorder.isRecording())
		{
			m_miscControlsWindow->recordMacroButton->setIcon( (counter%2) ? loadIcon("RedLED1.png") : loadIcon("OffLED1") );
			counter++;
		}
		else 
			counter=0; 
	}
}


// --- dynamic/rearrangable actions stuff ------------------------------------------------

#include "ReopenMenu.h"
#include "RecentActionsMenu.h"

/* the usual case -- adding a RezAction to the action registry */
void MainWindow::registerAction(RezAction *action)
{
	// initially parent all actions to the main window and reparent when it's added to 
	// the menu, which is how we later know if it ended up being handled by menu.dat
	action->setParent(this);

	const string strippedActionName=action->getStrippedUntranslatedText();

	// something a developer would want to know
	if(actionRegistry.find(strippedActionName)!=actionRegistry.end())
		printf("NOTE: duplicate action name in action registry '%s'\n",strippedActionName.c_str());

	// we want notification when the action is triggered (for recent actions, and possibly to handle the action's task)
	connect(action,SIGNAL(triggered()),this,SLOT(onRezAction_triggered()));

	actionRegistry[strippedActionName]=action;
}

void MainWindow::buildActionMap() 
{
	// This initializes the menu action map, creating the collection of all available menu actions.
	// This allows menus to be dynamically laid out using a configuration file.
	
	RezAction *action;
	
	// File 
	registerAction(		new RezAction(new CNewAudioFileActionFactory(new NewAudioFileActionDialog(this)),		loadIcon("file_new.png")));
	registerAction(		new RezAction(new COpenAudioFileActionFactory(new OpenAudioFileActionDialog(this)),		loadIcon("file_open.png")));
	registerAction(action=	new RezAction(N_("&Reopen"),									loadIcon("file_open.png")));
		action->setMenu(new ReopenMenu);
	registerAction(		new RezAction(new CSaveAudioFileActionFactory,							loadIcon("file_save.png")));
	registerAction(		new RezAction(new CSaveAsAudioFileActionFactory(new SaveAsAudioFileActionDialog(this)),	loadIcon("file_save_as.png")));
	registerAction(		new RezAction(new CSaveSelectionAsActionFactory,						loadIcon("file_save_as.png")));
	registerAction(		new RezAction(new CSaveAsMultipleFilesActionFactory(new SaveAsMultipleFilesDialog(this)),	loadIcon("file_save_as.png")));
	registerAction(		new RezAction(new CBurnToCDActionFactory(new BurnToCDDialog(this)),				loadIcon("file_burn.png")));
		// not converting close or revert to AAction classes because they pull the sound object out from under the calling code in AAction
	registerAction(		new RezAction(N_("&Close"),									loadIcon("file_close.png")));
	registerAction(		new RezAction(N_("Re&vert"),									loadIcon("file_revert.png")));
	// -
	registerAction(		new RezAction(new CRunMacroActionFactory(new RunMacroDialog(this))				));
	registerAction(		new RezAction(N_("Record Macro...")								));
	// -
	registerAction(		new RezAction(N_("User No&tes..."),								loadIcon("notes.png")));
	// -
	registerAction(		new RezAction(N_("&About ReZound"),								QIcon()	));
	registerAction(		new RezAction(N_("&Setup Hotkeys..."),								QIcon() ));
	// -
	registerAction(		new RezAction("- Just for testing",								QIcon() ));
	registerAction(		new RezAction("Defrag",									QIcon() ));
	registerAction(		new RezAction("PrintSAT",									QIcon() ));
	registerAction(		new RezAction("VerifySAT",									QIcon() ));
	// -
	registerAction(		new RezAction(N_("&Quit"),									loadIcon("exit.png")));

	// Control
	registerAction(		new RezAction(m_transportWindow->recordButton));
	registerAction(		new RezAction(m_transportWindow->playAllButton));
	registerAction(		new RezAction(m_transportWindow->playAllLoopedButton));
	registerAction(		new RezAction(m_transportWindow->playSelectionOnceButton));
	registerAction(		new RezAction(m_transportWindow->playFromLeftEdgeOfScreenToEndButton));
	registerAction(		new RezAction(m_transportWindow->playFromStartPositionButton));
	registerAction(		new RezAction(m_transportWindow->playSelectionLoopedButton));
	registerAction(		new RezAction(m_transportWindow->playSelectionLoopedSkipMostButton));
	registerAction(		new RezAction(m_transportWindow->playSelectionLoopedGapBeforeRepeatButton));
	registerAction(		new RezAction(m_transportWindow->stopButton));
	registerAction(		new RezAction(m_transportWindow->pauseButton));
	// -
	registerAction(		new RezAction(m_transportWindow->jumpToBeginningButton));
	registerAction(		new RezAction(m_transportWindow->jumpToStartPositionButton));
	registerAction(		new RezAction(m_transportWindow->jumpToPrevCueButton));
	registerAction(		new RezAction(m_transportWindow->jumpToNextCueButton));

	registerAction(		m_transportWindow->m_shuttleBackwardAction=new RezAction(N_("Shuttle Rewind"),			loadIcon("shuttle_backward.png")));
	registerAction(		m_transportWindow->m_shuttleAmountAction=new RezAction(N_("Shuttle Amount"),			loadIcon("shuttle_normal.png")));
	registerAction(		m_transportWindow->m_shuttleForwardAction=new RezAction(N_("Shuttle Forward"),			loadIcon("shuttle_forward.png")));


	// View Controls
	registerAction(		new RezAction(N_("Zoom Out F&ull"),								loadIcon("zoom_out_full.png")));
	registerAction(		new RezAction(N_("Zoom &Out"),									loadIcon("zoom_out.png")));
	registerAction(		new RezAction(N_("Zoom &In"),									loadIcon("zoom_in.png")));
	registerAction(		new RezAction(N_("Zoom &Fit Selection"),							loadIcon("zoom_fit.png")));
	// -
	registerAction(		new RezAction(N_("Find &Start Position"),							loadIcon("normal_action_buff.png")));
	registerAction(		new RezAction(N_("Find Sto&p Position"),							loadIcon("normal_action_buff.png")));
	// -
	registerAction(		new RezAction(N_("&Redraw"),									loadIcon("normal_action_buff.png")));
	// -
	registerAction(		new RezAction(N_("Switch to Previously Viewed File")						));
	registerAction(		new RezAction(N_("Switch to Loaded File 1")							));
	registerAction(		new RezAction(N_("Switch to Loaded File 2")							));
	registerAction(		new RezAction(N_("Switch to Loaded File 3")							));
	registerAction(		new RezAction(N_("Switch to Loaded File 4")							));
	registerAction(		new RezAction(N_("Switch to Loaded File 5")							));
	registerAction(		new RezAction(N_("Switch to Loaded File 6")							));
	registerAction(		new RezAction(N_("Switch to Loaded File 7")							));
	registerAction(		new RezAction(N_("Switch to Loaded File 8")							));
	registerAction(		new RezAction(N_("Switch to Loaded File 9")							));
	registerAction(		new RezAction(N_("Switch to Loaded File 10")							));


	// Edit
	registerAction(		new RezAction(N_("Undo"),									loadIcon("edit_undo.png") ));
	registerAction(		new RezAction(N_("Clear Undo History")								));
	// -
	registerAction(action=	new RezAction(N_("&Recent Actions")								));
		action->setMenu(new RecentActionsMenu(this));
	// -
	registerAction(		new RezAction(new CCopyEditFactory(gChannelSelectDialog),					loadIcon("edit_copy.png") ));
	registerAction(		new RezAction(new CCopyToNewEditFactory(gChannelSelectDialog),					loadIcon("edit_copy.png") ));
	registerAction(		new RezAction(new CCutEditFactory(gChannelSelectDialog),					loadIcon("edit_cut.png") ));
	registerAction(		new RezAction(new CCutToNewEditFactory(gChannelSelectDialog),					loadIcon("edit_cut.png")  ));
	registerAction(		new RezAction(new CDeleteEditFactory(gChannelSelectDialog),					loadIcon("edit_delete.png") ));
	registerAction(		new RezAction(new CCropEditFactory(gChannelSelectDialog),					loadIcon("edit_crop.png") ));
	// -
	registerAction(		new RezAction(new CInsertPasteEditFactory(gPasteChannelsDialog),				loadIcon("edit_paste.png") ));
	registerAction(		new RezAction(new CReplacePasteEditFactory(gPasteChannelsDialog),				loadIcon("edit_paste.png") ));
	registerAction(		new RezAction(new COverwritePasteEditFactory(gPasteChannelsDialog),				loadIcon("edit_paste.png") ));
	registerAction(		new RezAction(new CLimitedOverwritePasteEditFactory(gPasteChannelsDialog),			loadIcon("edit_paste.png") ));
	registerAction(		new RezAction(new CMixPasteEditFactory(gPasteChannelsDialog),					loadIcon("edit_paste.png") ));
	registerAction(		new RezAction(new CLimitedMixPasteEditFactory(gPasteChannelsDialog),				loadIcon("edit_paste.png") ));
	registerAction(		new RezAction(new CFitMixPasteEditFactory(gPasteChannelsDialog),				loadIcon("edit_paste.png") ));
	registerAction(		new RezAction(new CPasteAsNewEditFactory,							loadIcon("edit_paste.png") ));
	// -
	registerAction(		new RezAction(new CInsertSilenceEditFactory(gChannelSelectDialog,new InsertSilenceDialog(this))	));
	registerAction(		new RezAction(new CMuteEditFactory(gChannelSelectDialog)						));
	// -
	registerAction(		new RezAction(new CAddChannelsEditFactory(new AddChannelsDialog(this))				));
	registerAction(		new RezAction(new CDuplicateChannelEditFactory(new DuplicateChannelDialog(this))			));
	registerAction(		new RezAction(new CRemoveChannelsEditFactory(gChannelSelectDialog)					));
	registerAction(		new RezAction(new CSwapChannelsEditFactory(new SwapChannelsDialog(this))				));
	// -
	registerAction(		new RezAction(new CRotateLeftEditFactory(gChannelSelectDialog,new RotateDialog(this))			));
	registerAction(		new RezAction(new CRotateRightEditFactory(gChannelSelectDialog,new RotateDialog(this))		));
	// -
	registerAction(		new RezAction(new CSelectionEditFactory(sSelectAll)							));
	registerAction(		new RezAction(new CGrowOrSlideSelectionEditFactory(new GrowOrSlideSelectionDialog(this))		));
	registerAction(		new RezAction(new CSelectionEditFactory(sSelectToBeginning)						));
	registerAction(		new RezAction(new CSelectionEditFactory(sSelectToEnd)							));
	registerAction(		new RezAction(new CSelectionEditFactory(sFlopToBeginning)						));
	registerAction(		new RezAction(new CSelectionEditFactory(sFlopToEnd)							));
	registerAction(		new RezAction(new CSelectionEditFactory(sSelectToSelectStart)						));
	registerAction(		new RezAction(new CSelectionEditFactory(sSelectToSelectStop)						));
	// -
	registerAction(		new RezAction(new CAddCueWhilePlayingActionFactory							));


	// Effects
	registerAction(		new RezAction(new CReverseEffectFactory(gChannelSelectDialog) 							));
	// -
	registerAction(		new RezAction(new CChangeVolumeEffectFactory(gChannelSelectDialog,new NormalVolumeChangeDialog(this))		));
	registerAction(		new RezAction(new CSimpleGainEffectFactory(gChannelSelectDialog,new NormalGainDialog(this)) 			));
	registerAction(		new RezAction(new CCurvedGainEffectFactory(gChannelSelectDialog,new AdvancedGainDialog(this)) 		));
	// -
	registerAction(		new RezAction(new CSimpleChangeRateEffectFactory(gChannelSelectDialog,new NormalRateChangeDialog(this))	));
	registerAction(		new RezAction(new CCurvedChangeRateEffectFactory(gChannelSelectDialog,new AdvancedRateChangeDialog(this))	));
	// -
	registerAction(		new RezAction(new CFlangeEffectFactory(gChannelSelectDialog,new FlangeDialog(this)) 				));
	registerAction(		new RezAction(new CSimpleDelayEffectFactory(gChannelSelectDialog,new SimpleDelayDialog(this)) 		));
	registerAction(		new RezAction(new CQuantizeEffectFactory(gChannelSelectDialog,new QuantizeDialog(this)) 			));
	registerAction(		new RezAction(new CDistortionEffectFactory(gChannelSelectDialog,new DistortionDialog(this)) 			));
	registerAction(		new RezAction(new CVariedRepeatEffectFactory(gChannelSelectDialog,new VariedRepeatDialog(this)) 		));

	registerAction(		new RezAction(new CTestEffectFactory(gChannelSelectDialog)							));


	// Filter
	registerAction(		new RezAction(new CConvolutionFilterFactory(gChannelSelectDialog,new ConvolutionFilterDialog(this))			));
	registerAction(		new RezAction(new CArbitraryFIRFilterFactory(gChannelSelectDialog,new ArbitraryFIRFilterDialog(this)),		loadIcon("filter_custom.png") ));
	registerAction(		new RezAction(new CMorphingArbitraryFIRFilterFactory(gChannelSelectDialog,new MorphingArbitraryFIRFilterDialog(this)),loadIcon("filter_custom.png") ));
	// -
	registerAction(		new RezAction(new CSinglePoleLowpassFilterFactory(gChannelSelectDialog,new SinglePoleLowpassFilterDialog(this)),	loadIcon("filter_lowpass.png") ));
	registerAction(		new RezAction(new CSinglePoleHighpassFilterFactory(gChannelSelectDialog,new SinglePoleHighpassFilterDialog(this)),	loadIcon("filter_highpass.png") ));
	registerAction(		new RezAction(new CBandpassFilterFactory(gChannelSelectDialog,new BandpassFilterDialog(this)),			loadIcon("filter_bandpass.png") ));
	registerAction(		new RezAction(new CNotchFilterFactory(gChannelSelectDialog,new NotchFilterDialog(this)),				loadIcon("filter_notch.png") ));
	// -
	registerAction(		new RezAction(new CBiquadResLowpassFilterFactory(gChannelSelectDialog,new BiquadResLowpassFilterDialog(this)),	loadIcon("filter_lowpass.png") ));
	registerAction(		new RezAction(new CBiquadResHighpassFilterFactory(gChannelSelectDialog,new BiquadResHighpassFilterDialog(this)),	loadIcon("filter_highpass.png") ));
	registerAction(		new RezAction(new CBiquadResBandpassFilterFactory(gChannelSelectDialog,new BiquadResBandpassFilterDialog(this)),	loadIcon("filter_bandpass.png") ));


	// Looping
	registerAction(		new RezAction(new CMakeSymetricActionFactory(gChannelSelectDialog) 						));
	// -
	registerAction(		new RezAction(new CAddNCuesActionFactory(new AddNCuesDialog(this)) 						));
	registerAction(		new RezAction(new CAddTimedCuesActionFactory(new AddTimedCuesDialog(this)) 					));


	// Remaster
	registerAction(		new RezAction(new CSimpleBalanceActionFactory(NULL,new SimpleBalanceActionDialog(this)) 			));
	registerAction(		new RezAction(new CCurvedBalanceActionFactory(NULL,new CurvedBalanceActionDialog(this)) 			));
	// -
	registerAction(		new RezAction(new CMonoizeActionFactory(NULL,new MonoizeActionDialog(this)) 					));
	// -
	registerAction(		new RezAction(new CNoiseGateActionFactory(gChannelSelectDialog,new NoiseGateDialog(this)) 			));
	registerAction(		new RezAction(new CCompressorActionFactory(gChannelSelectDialog,new CompressorDialog(this)) 			));
	registerAction(		new RezAction(new CNormalizeActionFactory(gChannelSelectDialog,new NormalizeDialog(this)) 			));
	registerAction(		new RezAction(new CAdaptiveNormalizeActionFactory(gChannelSelectDialog,new AdaptiveNormalizeDialog(this)) 	));
	registerAction(		new RezAction(new CMarkQuietAreasActionFactory(new MarkQuietAreasDialog(this)) 				));
	registerAction(		new RezAction(new CShortenQuietAreasActionFactory(new ShortenQuietAreasDialog(this)) 				));
	// -
	registerAction(		new RezAction(new CResampleActionFactory(gChannelSelectDialog,new ResampleDialog(this)) 			));
	registerAction(		new RezAction(new CChangePitchActionFactory(gChannelSelectDialog,new ChangePitchDialog(this)) 		));
	registerAction(		new RezAction(new CChangeTempoActionFactory(gChannelSelectDialog,new ChangeTempoDialog(this)) 		));
	// -
	registerAction(		new RezAction(new CRemoveDCActionFactory(gChannelSelectDialog) 						));
	registerAction(		new RezAction(new CInvertPhaseActionFactory(gChannelSelectDialog) 						));
	// -
	registerAction(		new RezAction(new CUnclipActionFactory(gChannelSelectDialog) 							));

	// Generate
	registerAction(		new RezAction(new CGenerateNoiseActionFactory(gChannelSelectDialog,new GenerateNoiseDialog(this)) 		));
	registerAction(		new RezAction(new CGenerateToneActionFactory(gChannelSelectDialog,new GenerateToneDialog(this)) 		));



	// These are here simply so that xgettext will add entries for these in the rezound.pot file
	N_("&File");
	N_("&Edit");
	N_("Selection");
	N_("&View");
	N_("&Control");
	N_("Effec&ts");
	N_("&Looping");
	N_("F&ilters");
	N_("&Remaster");
	N_("&Generate");

	// create registry of all actions (for macros)
	gRegisteredActionFactories.clear();
	for(map<const string,RezAction *>::iterator i=actionRegistry.begin();i!=actionRegistry.end();i++)
	{
		if(dynamic_cast<RezAction *>(i->second)!=NULL)
			gRegisteredActionFactories[i->first]=((RezAction *)i->second)->getActionFactory();
	}

	setupKeyBindings();
}

static const string stripAmpersand(const string str)
{
	return istring(str).maximalSearchAndReplace("&","");
}

void MainWindow::buildMenu(QMenu *menu,const CNestedDataFile *menuLayoutFile,const string menuKey,const string actionName)
{
	if(actionName=="-") 
	{
		if(menu) menu->addSeparator();
		return;
	}

	// if the item is a submenu item, recur for each item in it; otherwise, add as normal menu item
	if(menuLayoutFile->keyExists(menuKey)==CNestedDataFile::ktScope)
	{	// add as a submenu
		QMenu *submenu=NULL;

		const size_t DOTCount=istring(menuKey).count(CNestedDataFile::delim);
		if(DOTCount==0)
		{	// we've been passed just the layout name
			// this is the theorical place to create menubar, but in Qt, it's already part of the mainWindow
		}
		else if(DOTCount==1)
		{	// we've been passed a top-level pull-down menu name
			menuBar()->addMenu(submenu=new QMenu(gettext(actionName.c_str()),menuBar()));

			if(stripAmpersand(actionName)=="View")
			{
				submenu->addSeparator(); // add this to visually separate it as well as to make certain there is at least one action in the menu widget
				connect(submenu,SIGNAL(aboutToShow()),this,SLOT(onViewMenu_aboutToShow()));
			}
		}
		else if(DOTCount>1)
		{	// submenu of menu
			menu->addMenu(submenu=new QMenu(gettext(actionName.c_str()),menu));
		}

		const string menuItemsKey=menuKey DOT "menuitems";
		if(menuLayoutFile->keyExists(menuItemsKey)==CNestedDataFile::ktValue)
		{
			const vector<string> menuItems=menuLayoutFile->getValue<vector<string> >(menuItemsKey);
			for(size_t t=0;t<menuItems.size();t++) 
			{
				const string name=menuItems[t];
				buildMenu(submenu,menuLayoutFile,menuKey DOT stripAmpersand(name),name);
			}
		} // else (if it's not an array) something may be screwed up in the layout definition
	}
	else
	{	// add as normal menu item
		const string strippedItemName=stripAmpersand(actionName);
		if(actionRegistry.find(strippedItemName)!=actionRegistry.end())
		{
			// if the parent is not the main window, then it's already been reparented
			if(actionRegistry[strippedItemName]->parent()!=this) // just a check
				printf("NOTE: registered menu item '%s' was mapped more than once in layout; only the last one will be effective\n",strippedItemName.c_str());

			// reparent the action under the menubar (just somewhere else than the main window), so that we'll know it has already been mapped
			// also, my making the shuttle actions context be the widget instead of the window, we can grab those keys specially, before they're actually handled by the shortcut handler
			// (they have to be handled specially because they're not one-off actions)
			actionRegistry[strippedItemName]->setParent(this->menubar);

			RezAction *action=actionRegistry[strippedItemName];

			// change menu name's caption if there is an alias defined: 'origMenuName="new menu name";' in the same scope as where menuitems is defined
			if(menuLayoutFile->keyExists(menuKey)==CNestedDataFile::ktValue) 
			{
				const string alias=menuLayoutFile->getValue<string>(menuKey);
/*
				if(alias!=action->text())
				{
					// clone action and let the continue to below
				}
*/
				action->setText(gettext(alias.c_str()));
			}

			menu->addAction(action);
		}
		else
		{
			if(menu)
				menu->addAction((actionName+" (unregistered)").c_str());
			else
				// since menu is NULL, this is a top-level menu without a defined body
				menuBar()->addAction((actionName+" (no subitems defined)").c_str());
		}
	}
}

#include <CNestedDataFile/CNestedDataFile.h>
void MainWindow::createMenus()
{
	buildActionMap();	
       
	// Try to find out which set of menu information we should load and which file contains the menu information.
	// The default menu layout is 'default' in .../share/.../menu.dat

	// ??? make this a global setting instead of a lookup here 
	//	NOTE: this would be the first frontend specific global setting (but this isn't the only lookup (showAbout() also does it))
	string menuLayout;
        if(gSettingsRegistry->keyExists("MenuLayout"))
		menuLayout=gSettingsRegistry->getValue<string>("MenuLayout");
	else
		menuLayout="default";

	tryAgain:

	string menuLayoutFilename;
	if(menuLayout=="default")
		menuLayoutFilename=gSysDataDirectory+CPath::dirDelim+"menu.dat";
	else
		menuLayoutFilename=gUserDataDirectory+CPath::dirDelim+"menu.dat";
	
	const CNestedDataFile *menuLayoutFile=new CNestedDataFile(menuLayoutFilename);
	try
	{
		if(	menuLayoutFile->keyExists(menuLayout)!=CNestedDataFile::ktScope || 
			menuLayoutFile->keyExists(menuLayout DOT "menuitems")!=CNestedDataFile::ktValue)
		{
			Warning(menuLayout+".menuitems array does not exist in requested menu layout '"+ menuLayout+"' in '"+menuLayoutFilename+"'");
			menuLayout="default"; // go around again and use the default menu layout
			goto tryAgain;
		}

		buildMenu(NULL,menuLayoutFile,menuLayout,menuLayout);
		delete menuLayoutFile;
	}
	catch(...)
	{
		delete menuLayoutFile;
		throw;
	}

	// give feedback about actions that didn't get mapped to a menu on screen
	for(map<const string,RezAction *>::iterator i=actionRegistry.begin();i!=actionRegistry.end();i++)
	{
		if(i->second->parent()==this) // parent is still MainWindow.. so buildMenu() didn't find anywhere to put it
			printf("NOTE: registered menu item '%s' was not mapped anywhere in '%s' in layout '%s'\n",i->first.c_str(),menuLayoutFilename.c_str(),menuLayout.c_str());
	}


	//??? add me back! (but make me faster) 
	//buildLADSPAMenus();

	//setupKeyBindings();

	//create(); // it is necessary to call create again which will call it for all new child windows
}

void MainWindow::buildLADSPAMenus()
{
#ifdef ENABLE_LADSPA
	// now stick the LADSPA menu in there if it needs to be
	// ??? (with dynamic menus, maybe let the layout define WHERE the ladspa submenu goes, (except it might not always be compiled for ladspa, so we wouldn't want to add it to the map if ENABLE_LADSPA wasn't defined)
	QMenu *menu=new QMenu("L&ADSPA",menuBar());
	menuBar()->addMenu(menu);

	const vector<CLADSPAActionFactory *> LADSPAActionFactories=getLADSPAActionFactories();
	if(LADSPAActionFactories.size()<=0)
	{
		menu->addAction(_("No LADSPA Plugins Found"));
		menu->addSeparator();
		menu->addAction(_("Like PATH, set LADSPA_PATH to point"));
		menu->addAction(_("to a directory(s) containing LADSPA"));
		menu->addAction(_("plugin .so file at least once.  Or"));
		menu->addAction(_("edit the value in ~/.rezound/registry.dat"));
	}
	else
	{
		// register all the LADSPA action so they can be used in macros
		for(size_t t=0;t<LADSPAActionFactories.size();t++)
			gRegisteredActionFactories[LADSPAActionFactories[t]->getName()]=LADSPAActionFactories[t];
		

		// determine number of FXMenuCaption will fit on the screen (msh -> menu screen height)
		//const int msh=(getApp()->getRootWindow()->getHeight()/(7+getApp()->getNormalFont()->getFontHeight()))-1;

		if(LADSPAActionFactories.size()>20)
		{
			// group action factories by the maker name
			map<const string,map<const string,CLADSPAActionFactory *> > makerGrouped;
			for(size_t t=0;t<LADSPAActionFactories.size();t++)
			{
				const string maker= LADSPAActionFactories[t]->getDescriptor()->Maker;
				makerGrouped[maker][LADSPAActionFactories[t]->getName()]=LADSPAActionFactories[t];
			}

			QMenu *makerMenu=NULL;
			if(makerGrouped.size()>1)
			{ // more than one maker 
				makerMenu=new QMenu(_("By Maker"),menu);
				menu->addMenu(makerMenu);
			}

			menu->addSeparator();

			// group action factories by the first letter
			map<const char,multimap<const string,CLADSPAActionFactory *> > nameGrouped;
			for(size_t t=0;t<LADSPAActionFactories.size();t++)
			{
				const char letter= *(istring(LADSPAActionFactories[t]->getName()).upper()).begin();
				nameGrouped[letter].insert(make_pair(LADSPAActionFactories[t]->getName(),LADSPAActionFactories[t]));
			}

			map<CLADSPAActionFactory *,RezAction *> actionByFactory;
			for(map<const char,multimap<const string,CLADSPAActionFactory *> >::iterator i=nameGrouped.begin();i!=nameGrouped.end();i++)
			{
				QMenu *submenu=new QMenu(string(&(i->first),1).c_str(),menu);
				menu->addMenu(submenu);

				for(multimap<const string,CLADSPAActionFactory *>::iterator t=i->second.begin();t!=i->second.end();t++)
				{
					RezAction *a=new RezAction(t->second);
					a->setParent(this);
					submenu->addAction(a);
					registerAction(a);
					actionByFactory[t->second]=a;
				}
			}

			if(makerMenu)
			{ // add same actions above but to the "By Maker" menu
				for(map<const string,map<const string,CLADSPAActionFactory *> >::iterator i=makerGrouped.begin();i!=makerGrouped.end();i++)
				{
					QMenu *submenu=new QMenu(i->first.c_str(),makerMenu);
					makerMenu->addMenu(submenu);

					for(map<const string,CLADSPAActionFactory *>::iterator t=i->second.begin();t!=i->second.end();t++)
						submenu->addAction(actionByFactory[t->second]);
				}
			}

		}
		else
		{
			for(size_t t=0;t<LADSPAActionFactories.size();t++)
			{
				RezAction *a=new RezAction(LADSPAActionFactories[t]);
				a->setParent(this);
				menu->addAction(a);
				registerAction(a);
			}
		}
	}
#endif
}

// rebuilds all shortcuts
void MainWindow::setupKeyBindings()
{
	// set key bindings
	for(map<const string,RezAction *>::iterator i=actionRegistry.begin();i!=actionRegistry.end();i++)
	{
		const string &name=i->first;
		RezAction *a=i->second;
		a->setShortcutContext(Qt::WindowShortcut);


		if(gKeyBindingsStore->keyExists(name)==CNestedDataFile::ktValue)
		{
			const string value=gKeyBindingsStore->getValue<string>(name);
			if(value=="")
			{
				a->setShortcut(QKeySequence());
				continue; // no key bound to this action
			}

				// ??? will QKeySequence::fromString() not work?
			a->setShortcut(parseKeySequence(value));
		}
		else
			// no shortcut
			a->setShortcut(QKeySequence());
	}

	// since we'll be handling these key event's specially in TransportWindow, make them have a shortcut context of their parent widget (menubar in this case)
	// so that our eventFilter on the main window will catch the events (because window level shortcuts are handled before event filters on that window)
	m_transportWindow->m_shuttleBackwardAction->setShortcutContext(Qt::WidgetShortcut);
	m_transportWindow->m_shuttleAmountAction->setShortcutContext(Qt::WidgetShortcut);
	m_transportWindow->m_shuttleForwardAction->setShortcutContext(Qt::WidgetShortcut);
}

