#include "TransportWindow.h"

#include <QKeyEvent>

#include "MainWindow.h"
#include "LevelMeters.h"
#include "SoundFileManager.h"
#include "SoundWindow.h"

#include "../backend/main_controls.h"
#include "../backend/CLoadedSound.h"
#include "../backend/AAction.h"

TransportWindow::TransportWindow(QWidget *parent) :
	QWidget(parent),
	m_mainWindow(NULL)
{
	setupUi(this);

}

TransportWindow::~TransportWindow()
{
	if(m_mainWindow)
		m_mainWindow->removeEventFilter(this);
}

void TransportWindow::setMainWindow(MainWindow *mainWindow)
{
	m_mainWindow=mainWindow;
	m_mainWindow->installEventFilter(this);

/*	stopButton->setIcon(loadIcon("stop.png"));
	playAllButton->setIcon(loadIcon("play_all_once.png"));
	playSelectionOnceButton->setIcon(loadIcon("play_selection_once.png"));
	playSelectionLoopedGapBeforeRepeatButton->setIcon(loadIcon("play_selection_looped_gap_before_repeat.png"));
	playAllLoopedButton->setIcon(loadIcon("play_all_looped.png"));
	playSelectionLoopedButton->setIcon(loadIcon("play_selection_looped.png"));
	playSelectionLoopedSkipMostButton->setIcon(loadIcon("play_selection_looped_skip_most.png"));
	playFromStartPositionButton->setIcon(loadIcon("play_selection_start_to_end.png"));
	playFromLeftEdgeOfScreenToEndButton->setIcon(loadIcon("play_screen.png"));
	pauseButton->setIcon(loadIcon("pause.png"));
	jumpToBeginningButton->setIcon(loadIcon("jump_to_beginning.png"));
	jumpToStartPositionButton->setIcon(loadIcon("jump_to_selection.png"));
	jumpToPrevCueButton->setIcon(loadIcon("jump_to_previous_q.png"));
	jumpToNextCueButton->setIcon(loadIcon("jump_to_next_q.png"));
	recordButton->setIcon(loadIcon("record.png"));
	*/
}

//void positionShuttleGivenSpeed(double seekSpeed,const string shuttleControlScalar,bool springBack); ??? I think I'm needed when switching between windows

bool TransportWindow::eventFilter(QObject *watched, QEvent *event)
{
	if(event->type()==QEvent::KeyPress || event->type()==QEvent::KeyRelease)
	{
		QKeyEvent *ke=(QKeyEvent *)event;
		if(!ke->isAutoRepeat())
		{
			QKeySequence seq(ke->key()); // ??? modifiers .. test me
			if(m_shuttleBackwardAction->shortcut()==seq)
			{
				if(ke->type()==QEvent::KeyPress)
				{
					const int inc=(shuttleDial->maximum()-shuttleDial->minimum())/14; // 7 positions surrounding 0 
					shuttleDial->setValue(shuttleDial->value()-inc);
				}
				else
				{
					shuttleDial->setValue(0);
				}
				
				return true;
			}
			else if(m_shuttleAmountAction->shortcut()==seq)
			{
				if(ke->type()==QEvent::KeyPress)
				{
					const int inc=(shuttleDial->maximum()-shuttleDial->minimum())/14; // 7 positions surrounding 0 
					int pos=shuttleDial->value();
					if(pos<0)
					{ // go more leftward
						shuttleDial->setValue(pos-inc);
					}
					else if(pos>0)
					{ // go more rightward
						shuttleDial->setValue(pos+inc);
					}
				}

				return true;
			}
			else if(m_shuttleForwardAction->shortcut()==seq)
			{
				if(ke->type()==QEvent::KeyPress)
				{
					const int inc=(shuttleDial->maximum()-shuttleDial->minimum())/14; // 7 positions surrounding 0 
					shuttleDial->setValue(shuttleDial->value()+inc);
				}
				else
				{
					shuttleDial->setValue(0);
				}
				
				return true;
			}
		}
	}
	return false;
}


void TransportWindow::on_playAllButton_clicked()
{
printf("mainWindow: 0x%p\n",m_mainWindow);
	m_mainWindow->levelMeters()->resetGrandMaxPeakLevels();
	play(gSoundFileManager,CSoundPlayerChannel::ltLoopNone,false);
}

void TransportWindow::on_playAllLoopedButton_clicked()
{
	m_mainWindow->levelMeters()->resetGrandMaxPeakLevels();
	play(gSoundFileManager,CSoundPlayerChannel::ltLoopNormal,false);
}

void TransportWindow::on_playSelectionOnceButton_clicked()
{
	m_mainWindow->levelMeters()->resetGrandMaxPeakLevels();
	play(gSoundFileManager,CSoundPlayerChannel::ltLoopNone,true);
}

void TransportWindow::on_playFromStartPositionButton_clicked()
{
	m_mainWindow->levelMeters()->resetGrandMaxPeakLevels();
	if(gSoundFileManager->getActive())
		play(gSoundFileManager,gSoundFileManager->getActive()->channel->getStartPosition());
}

void TransportWindow::on_playSelectionLoopedButton_clicked()
{
	m_mainWindow->levelMeters()->resetGrandMaxPeakLevels();
	play(gSoundFileManager,CSoundPlayerChannel::ltLoopNormal,true);
}

void TransportWindow::on_playSelectionLoopedSkipMostButton_clicked()
{
	m_mainWindow->levelMeters()->resetGrandMaxPeakLevels();
	play(gSoundFileManager,CSoundPlayerChannel::ltLoopSkipMost,true);
}

void TransportWindow::on_playSelectionLoopedGapBeforeRepeatButton_clicked()
{
	m_mainWindow->levelMeters()->resetGrandMaxPeakLevels();
	play(gSoundFileManager,CSoundPlayerChannel::ltLoopGapBeforeRepeat,true);
}

void TransportWindow::on_playFromLeftEdgeOfScreenToEndButton_clicked()
{
	if(gSoundFileManager->getActiveWindow())
	{
		m_mainWindow->levelMeters()->resetGrandMaxPeakLevels();
		play(gSoundFileManager,gSoundFileManager->getActiveWindow()->getLeftEdgePosition());
	}
}

void TransportWindow::on_stopButton_clicked()
{
	stop(gSoundFileManager);
}

void TransportWindow::on_pauseButton_clicked()
{
	pause(gSoundFileManager);
}

void TransportWindow::on_recordButton_clicked()
{
	if(AActionFactory::macroRecorder.isRecording())
		Message(_("Cannot record audio while recording a macro"));
	else
		recordSound(gSoundFileManager);
}

void TransportWindow::on_jumpToBeginningButton_clicked()
{
	jumpToBeginning(gSoundFileManager);
}

void TransportWindow::on_jumpToStartPositionButton_clicked()
{
	jumpToStartPosition(gSoundFileManager);
}

void TransportWindow::on_jumpToPrevCueButton_clicked()
{
	jumpToPreviousCue(gSoundFileManager);
}

void TransportWindow::on_jumpToNextCueButton_clicked()
{
	jumpToNextCue(gSoundFileManager);
}

void TransportWindow::on_shuttleDial_sliderReleased()
{
	if(/*((FXEvent *)ptr)->code==LEFTBUTTON &&*/ !shuttleSpringButton->isChecked())
		return; // this wasn't a left click release and where we're in spring-back mode

	// return shuttle control to the middle
	shuttleDial->setValue(0);
	on_shuttleDial_valueChanged(0); // necessary???
}

void TransportWindow::on_shuttleDial_valueChanged(int value)
{
	SoundWindow *w=gSoundFileManager->getActiveWindow();
	if(w!=NULL)
	{
		CLoadedSound *s=w->loadedSound;

		string labelString;

		int minValue=shuttleDial->minimum();
		int maxValue=shuttleDial->maximum();

		const int shuttlePos=value;
		float seekSpeed;

		const QString scale=shuttleScaleButton->property("scale").toString();

		if(shuttlePos==0)
		{
			if(scale=="semitones")
				labelString="+ 0";
			else
				labelString="1x";

			seekSpeed=1.0;
		}
		else
		{
			if(scale=="1x")
			{ // 1x +/- (0..1]
				if(shuttlePos>0)
					seekSpeed=(double)shuttlePos/(double)maxValue;
				else //if(shuttlePos<0)
					seekSpeed=(double)-shuttlePos/(double)minValue;
				labelString=istring(seekSpeed,4,3)+"x";
			}
			else if(scale=="2x")
			{ // 2x +/- [1..2]
				if(shuttlePos>0)
					seekSpeed=(double)shuttlePos/(double)maxValue+1.0;
				else //if(shuttlePos<0)
					seekSpeed=(double)-shuttlePos/(double)minValue-1.0;
				labelString=istring(seekSpeed,3,2)+"x";
			}
			else if(scale=="100x" || scale=="")
			{ // 100x +/- [1..100]
						// I square the value to give a more useful range
				if(shuttlePos>0)
					seekSpeed=(pow((double)shuttlePos/(double)maxValue,2.0)*99.0)+1.0;
				else //if(shuttlePos<0)
					seekSpeed=(pow((double)shuttlePos/(double)minValue,2.0)*-99.0)-1.0;
				labelString=istring(seekSpeed,4,2)+"x";
			}
			else if(scale=="semitones")
			{ // semitone + [0.5..2]
				float semitones = round((double)shuttlePos/(double)maxValue*12); // +/- 12 semitones
				if(shuttlePos>0) 
				{
					semitones = round((double)shuttlePos/(double)maxValue*12);
					seekSpeed=pow(2.0,semitones/12.0);
				}
				else //if(shuttlePos<0)
				{
					semitones = round((double)shuttlePos/(double)minValue*12);
					seekSpeed=pow(0.5,semitones/12.0);
				}
				labelString = (semitones>=0 ? "+" : "") + istring((int)semitones,2,false) + " ("+istring(seekSpeed,3,2)+"x)";
			}
			else
				throw runtime_error(string(__func__)+" -- internal error -- unhandled scale for shuttleScaleButton: '"+scale.toStdString()+"'");
		}

		shuttleLabel->setText(labelString.c_str());
		w->shuttleControlScalar=scale.toStdString();
		w->shuttleControlSpringBack=shuttleSpringButton->isChecked();
		s->channel->setSeekSpeed(seekSpeed);
	}
}

void TransportWindow::on_shuttleSpringButton_toggled(bool checked)
{
	shuttleSpringButton->clearFocus(); // ??? necessary?
	if(checked)
	{
		// return the shuttle to the middle
		shuttleDial->setValue(0);
		on_shuttleDial_valueChanged(0); // verify that this is necessary???
	}

	shuttleSpringButton->setText(checked ? _("spring") : _("free"));
}

void TransportWindow::on_shuttleScaleButton_clicked()
{
	shuttleScaleButton->clearFocus(); // ??? necessary?
	const QString scale=shuttleScaleButton->property("scale").toString();
	if(scale=="100x" || scale=="")
	{
		shuttleScaleButton->setText(_("1x"));
		shuttleScaleButton->setProperty("scale","1x");
	}
	else if(scale=="1x")
	{
		shuttleScaleButton->setText(_("2x"));
		shuttleScaleButton->setProperty("scale","2x");
	}
	else if(scale=="2x")
	{
		shuttleScaleButton->setText(_("semitones"));
		shuttleScaleButton->setProperty("scale","semitones");
	}
	else if(scale=="semitones")
	{
		shuttleScaleButton->setText(_("100x"));
		shuttleScaleButton->setProperty("scale","100x");
	}

	// return the shuttle to the middle
	shuttleDial->setValue(0);
	on_shuttleDial_valueChanged(0); // verify that this is necessary???
}

