#include "CModalDialog.h"

#include <QProgressBar>

#include <vector>
using namespace std;

#include "settings.h"
#include "../backend/CSound_defs.h"
#include "../backend/ASoundRecorder.h"
#include "../backend/AStatusComm.h"

#include <math.h>

#include <istring>

#include "../backend/unit_conv.h"

#include "CLevelMeters.h"

#define STATUS_UPDATE_TIME 100 /* if you change this, then consider changing the blink frequency in the timer event */

#include "ui_CRecordDialogContent.h"
class CRecordDialog : public CModalDialog, private Ui::CRecordDialogContent
{
	Q_OBJECT
public:
	CRecordDialog(QWidget *parent) :
		CModalDialog(parent,_("Record"))
	{
		Ui::CRecordDialogContent::setupUi(getFrame());

		levelMeters=new CRecordLevelMeters(metersFrame);
		metersFrame->setLayout(new QVBoxLayout);
		metersFrame->layout()->addWidget(levelMeters);

		QMetaObject::connectSlotsByName(this);
		connect(addAnchoredCueButton,SIGNAL(clicked()),this,SLOT(on_addCueButton_clicked()));
	}

	virtual ~CRecordDialog()
	{
	}


	int exec(ASoundRecorder *_recorder)
	{
		recorder=_recorder;
		clearClipCount();

		// ??? should find name (using a method on CSound) that finds the first available number or 1 more than the max actually
		cueNumberSpinBox->setValue(1);

		levelMeters->setSoundRecorder(recorder);

		on_stopButton_clicked(); // to clear status labels

		try
		{
			statusTimerID=startTimer(STATUS_UPDATE_TIME);

			// ??? might wanna use rememberShow or some base method
			if(QDialog::exec() && recorder->getRecordedLength()>0)
			{
				killTimer(statusTimerID);
				levelMeters->setSoundRecorder(NULL);
				return true;
			}
			killTimer(statusTimerID);
			levelMeters->setSoundRecorder(NULL);
			return false;
		}
		catch(...)
		{	
			killTimer(statusTimerID);
			levelMeters->setSoundRecorder(NULL);
			throw;
		}
	}

protected:

	/*override*/ bool validateOnOkay()
	{
		return true;
	}

	void timerEvent(QTimerEvent *e)
	{
		if(e->timerId()!=statusTimerID)
		{
			CModalDialog::timerEvent(e);
			return;
		}

		// see about setting fixed sized labels to avoid layout every timer tick ???

		clipCountLabel->setText((_("Clip Count: ")+istring(recorder->clipCount)).c_str());

		recordedLengthStatusLabel->setText(recorder->getRecordedLengthS().c_str());
		recordedSizeStatusLabel->setText(recorder->getRecordedSizeS().c_str());

		if(recorder->isStarted())
			recordLimitStatusLabel->setText(recorder->getRecordLimitS().c_str());
		else
			recordLimitStatusLabel->setText("");


		if(blinkCounter%10==0)
		{ // do something every ten frames
			bool on=((blinkCounter/10)%2)==0;

			// handle the red recording LED
			recordingLED->setPixmap(loadPixmap((recorder->isStarted() && !recorder->isWaitingForThreshold() && on) ? "RedLED1.png" : "OffLED1.png"));

			// handle the yellow waiting LED
			waitingForThresholdLED->setPixmap(loadPixmap((recorder->isWaitingForThreshold() && on) ? "YellowLED1.png" : "OffLED1.png"));
		}

		blinkCounter++;

		// display the current DC Offset
		const unsigned channelCount=recorder->getChannelCount();
			// get greatest abs DCOffset
		sample_t DCOffset=recorder->getDCOffset(0);
		for(unsigned i=1;i<channelCount;i++)
		{
			if(fabsf(DCOffset)>fabsf(recorder->getDCOffset(i)))
				DCOffset=recorder->getDCOffset(i);
		}
		DCOffsetLabel->setText((_("DC Offset: ")+istring(amp_to_dBFS(recorder->getDCOffset(1)))+"dBFS").c_str());
	}

private Q_SLOTS:
	void on_startButton_clicked()
	{
		blinkCounter=0;
		clearClipCount();
		recorder->start(getStartThreshold(),getMaxDuration());
	}

	void on_stopButton_clicked()
	{
		blinkCounter=0;
		recorder->stop();
	}

	void on_redoButton_clicked()
	{
		recorder->redo(getMaxDuration());
		clearClipCount();
	}

	void on_addCueButton_clicked()
	{
		try
		{
			if(cueNumberSpinBox->value()==-1)
				recorder->addCueNow(cuePrefixEdit->text().toStdString(),sender()==addAnchoredCueButton);
			else
			{
				recorder->addCueNow((cuePrefixEdit->text().toStdString()+istring(cueNumberSpinBox->value())),sender()==addAnchoredCueButton);
				cueNumberSpinBox->setValue(cueNumberSpinBox->value()+1);
			}
		}
		catch(exception &e)
		{
			Error(e.what());
		}
	}

	void on_resetClipCountButton_clicked()
	{
		clearClipCount();
	}

	void on_durationSpinBox_valueChanged()
	{
#if 0 // ??? do me
		bool wasInvalid;
		sample_fpos_t d=recorder->getSound()->getPositionFromTime(durationSpinBox->getText().text(),wasInvalid);
		if(wasInvalid)
			d=0;

		d+=((sample_fpos_t)durationSpinner->getValue()*(sample_fpos_t)recorder->getSound()->getSampleRate());
		if(d<0.0)
			d=0;

		durationSpinner->setValue(0);

		durationEdit->setText(recorder->getSound()->getTimePosition((sample_pos_t)d,3,false).c_str());
		if(d!=0)
			setDurationButton->setCheck(true);
#endif
	}

	void on_DCOffsetCompensateButton_clicked()
	{
		recorder->compensateForDCOffset();
	}

private:
	ASoundRecorder *recorder;
	CRecordLevelMeters *levelMeters;
	int statusTimerID;
	int blinkCounter;
	int blinkTimerID;
	vector<QProgressBar *> meters;

	const sample_pos_t getMaxDuration()
	{
#if 0 // ??? do me
		if(limitDurationCheckBox->isChecked())
		{
			bool wasInvalid;
			const sample_pos_t d=recorder->getSound()->getPositionFromTime(durationSpinBox->getText().text(),wasInvalid);
			if(wasInvalid)
			{
				Error(_("Invalid record time limit -- Should be in the form of HH:MM:SS.sss, MM:SS.sss or SS.sss"));
				return 1;
			}

			if(d==0)
			{
				limitDuratioinCheckBox->setChecked(false);
				return NIL_SAMPLE_POS;
			}
			else
				return d;
		}
#endif
		return NIL_SAMPLE_POS;
	}

	const double getStartThreshold()
	{
		if(beginOnThresholdCheckBox->isChecked())
		{
			const double startThreshold=thresholdSpinBox->value(); // ??? need to change to a QDoubleSpinBo
			return startThreshold;
		}
		return 1.0; // no threshold (>0 is no threshold)
	}

	void clearClipCount()
	{
		recorder->clipCount=0;
		clipCountLabel->setText((istring(_("Clip Count: "))+"0").c_str());
	}

};
