/* 
 * Copyright (C) 2002 - David W. Durham
 * 
 * This file is part of ReZound, an audio editing application.
 * 
 * ReZound is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 * 
 * ReZound is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */

#include "CRecordDialog.h"

#include "../backend/ASoundRecorder.h"
#include "../backend/AStatusComm.h"
#include "CFOXIcons.h"

#include <math.h>

#include <istring>

#include "../backend/unit_conv.h"

#define STATUS_UPDATE_TIME 100

FXDEFMAP(CRecordDialog) CRecordDialogMap[]=
{
//	Message_Type			ID						Message_Handler
	FXMAPFUNC(SEL_COMMAND,		CRecordDialog::ID_START_BUTTON,			CRecordDialog::onStartButton),
	FXMAPFUNC(SEL_COMMAND,		CRecordDialog::ID_STOP_BUTTON,			CRecordDialog::onStopButton),
	FXMAPFUNC(SEL_COMMAND,		CRecordDialog::ID_REDO_BUTTON,			CRecordDialog::onRedoButton),

	FXMAPFUNC(SEL_COMMAND,		CRecordDialog::ID_ADD_CUE_BUTTON,		CRecordDialog::onAddCueButton),
	FXMAPFUNC(SEL_COMMAND,		CRecordDialog::ID_ADD_ANCHORED_CUE_BUTTON,	CRecordDialog::onAddCueButton),

	FXMAPFUNC(SEL_COMMAND,		CRecordDialog::ID_DURATION_SPINNER,		CRecordDialog::onDurationSpinner),

	FXMAPFUNC(SEL_COMMAND,		CRecordDialog::ID_START_THRESHOLD_SPINNER,	CRecordDialog::onStartThresholdSpinner),

	FXMAPFUNC(SEL_TIMEOUT,		CRecordDialog::ID_STATUS_UPDATE,		CRecordDialog::onStatusUpdate),

	FXMAPFUNC(SEL_COMMAND,		CRecordDialog::ID_CLEAR_CLIP_COUNT_BUTTON,	CRecordDialog::onClearClipCountButton),

	FXMAPFUNC(SEL_COMMAND,		CRecordDialog::ID_DC_OFFSET_COMPENSATE_BUTTON,	CRecordDialog::onDCOffsetCompensateButton),
};
		

FXIMPLEMENT(CRecordDialog,FXModalDialogBox,CRecordDialogMap,ARRAYNUMBER(CRecordDialogMap))



// ----------------------------------------

CRecordDialog::CRecordDialog(FXWindow *mainWindow) :
	FXModalDialogBox(mainWindow,N_("Record"),350,360,FXModalDialogBox::ftVertical),

	recorder(NULL),
	showing(false)

{
	setIcon(FOXIcons->small_record);

	getFrame()->setHSpacing(1);
	getFrame()->setVSpacing(1);

	FXPacker *frame1;
	FXPacker *frame2;
	FXPacker *frame3;
	FXPacker *frame4;

	frame1=new FXHorizontalFrame(getFrame(),LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 1,1);
		frame2=new FXVerticalFrame(frame1,LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 1,1);
			frame3=new FXVerticalFrame(frame2,FRAME_RAISED | LAYOUT_FILL_X|LAYOUT_FILL_Y);
				frame4=new FXHorizontalFrame(frame3,LAYOUT_FILL_X, 0,0,0,0, 0,0,0,0, 0,0);
					new FXLabel(frame4,_("   Cue Prefix:"));
					cueNamePrefix=new FXTextField(frame4,10);
					cueNamePrefix->setText("cue");
				frame4=new FXHorizontalFrame(frame3,LAYOUT_FILL_X, 0,0,0,0, 0,0,0,0, 0,0);
					new FXLabel(frame4,_("Cue Number:"));
					cueNameNumber=new FXSpinner(frame4,3,NULL,0,SPIN_NORMAL|FRAME_NORMAL);
					cueNameNumber->setRange(-1,1000);
				new FXButton(frame3,_("Add Cue"),NULL,this,ID_ADD_CUE_BUTTON);
				new FXButton(frame3,_("Add Anchored Cue"),NULL,this,ID_ADD_ANCHORED_CUE_BUTTON);
					// but what name shall I give the cues
					// the checkbox should indicate that cues will be placed at each time the sound is stopped and started
				//surroundWithCuesButton=new FXCheckButton(frame3,"Surround With Cues");
			frame3=new FXVerticalFrame(frame2,FRAME_RAISED | LAYOUT_FILL_X, 0,0,0,0, 0,45,0,0, 0,0);
				recordingLED=new FXPacker(frame3,LAYOUT_RIGHT|LAYOUT_CENTER_Y, 0,0,0,0, 0,0,0,0, 0,0);
					new FXLabel(recordingLED,_("Recording:"),FOXIcons->RedLED1,JUSTIFY_NORMAL | ICON_AFTER_TEXT | LAYOUT_FIX_X|LAYOUT_FIX_Y, 0,0);
					new FXLabel(recordingLED,_("Recording:"),FOXIcons->OffLED1,JUSTIFY_NORMAL | ICON_AFTER_TEXT | LAYOUT_FIX_X|LAYOUT_FIX_Y, 0,0);
				waitingForThresholdLED=new FXPacker(frame3,LAYOUT_RIGHT|LAYOUT_CENTER_Y, 0,0,0,0, 0,0,0,0, 0,0);
					new FXLabel(waitingForThresholdLED,_("Waiting for Threshold:"),FOXIcons->YellowLED1,JUSTIFY_NORMAL | ICON_AFTER_TEXT | LAYOUT_FIX_X|LAYOUT_FIX_Y, 0,0);
					new FXLabel(waitingForThresholdLED,_("Waiting for Threshold:"),FOXIcons->OffLED1,JUSTIFY_NORMAL | ICON_AFTER_TEXT | LAYOUT_FIX_X|LAYOUT_FIX_Y, 0,0);
			frame3=new FXHorizontalFrame(frame2,FRAME_RAISED | LAYOUT_FILL_X|LAYOUT_FILL_Y);
				frame3->setHSpacing(0);
				frame3->setVSpacing(0);
				frame4=new FXVerticalFrame(frame3,LAYOUT_FILL_X, 0,0,0,0, 0,0,0,0, 0,0);
					new FXLabel(frame4,_("Record Length: "),NULL,LAYOUT_FILL_X|JUSTIFY_RIGHT);
					new FXLabel(frame4,_("Record Size: "),NULL,LAYOUT_FILL_X|JUSTIFY_RIGHT);
					new FXLabel(frame4,_("Record Limit: "),NULL,LAYOUT_FILL_X|JUSTIFY_RIGHT);
				frame4=new FXVerticalFrame(frame3,LAYOUT_FILL_X, 0,0,0,0, 0,0,0,0, 0,0);
					recordedLengthStatusLabel=new FXLabel(frame4,"00:00.000",NULL,LAYOUT_FILL_X|JUSTIFY_LEFT);
					recordedSizeStatusLabel=new FXLabel(frame4,"0",NULL,LAYOUT_FILL_X|JUSTIFY_LEFT);
					recordLimitLabel=new FXLabel(frame4,"",NULL,LAYOUT_FILL_X|JUSTIFY_LEFT);
			frame3=new FXVerticalFrame(frame2,FRAME_RAISED | LAYOUT_FILL_X);
				DCOffsetLabel=new FXLabel(frame3,_("DCOffset: "),NULL,LAYOUT_CENTER_Y);
				DCOffsetLabel->setTipText(_("Reported Every 5 Seconds"));
				new FXButton(frame3,_("Compensate\tSubtract out the currently reported DC Offset from any further recorded data"),NULL,this,ID_DC_OFFSET_COMPENSATE_BUTTON,BUTTON_NORMAL|LAYOUT_CENTER_Y);
		frame2=new FXVerticalFrame(frame1,FRAME_RAISED | LAYOUT_FILL_X|LAYOUT_FILL_Y);
			frame3=new FXHorizontalFrame(frame2,0,0,0,0, 0,0,0,0, 0,0);
				new FXButton(frame3,_("Reset"),NULL,this,ID_CLEAR_CLIP_COUNT_BUTTON);
				clipCountLabel=new FXLabel(frame3,FXString(_("Clip Count: "))+"0",NULL);
			meterFrame=new FXHorizontalFrame(frame2,LAYOUT_FILL_X|LAYOUT_FILL_Y);
	frame1=new FXVerticalFrame(getFrame(),FRAME_RAISED | LAYOUT_FILL_X, 0,0,0,0, 0,0,0,0, 1,1);
		frame2=new FXHorizontalFrame(frame1,LAYOUT_CENTER_X);
			new FXButton(frame2,_("Record/\nResume"),NULL,this,ID_START_BUTTON);
			new FXButton(frame2,_("Stop/\nPause"),NULL,this,ID_STOP_BUTTON);
			new FXButton(frame2,_("Redo/\nReset"),NULL,this,ID_REDO_BUTTON);

		frame2=new FXHorizontalFrame(frame1,LAYOUT_CENTER_X);
			setDurationButton=new FXCheckButton(frame2,_("Limit Duration to "));
			durationEdit=new FXTextField(frame2,12);
			durationEdit->setText("MM:SS.sss");
			durationSpinner=new FXSpinner(frame2,0,this,ID_DURATION_SPINNER, SPIN_NOTEXT);
			durationSpinner->setRange(-10,10);

		frame2=new FXHorizontalFrame(frame1,LAYOUT_CENTER_X);
			startThresholdButton=new FXCheckButton(frame2,_("Effective on Threshold "));
			startThresholdEdit=new FXTextField(frame2,12);
			startThresholdEdit->setText("-25");
			startThresholdSpinner=new FXSpinner(frame2,0,this,ID_START_THRESHOLD_SPINNER, SPIN_NOTEXT);
			startThresholdSpinner->setRange(-10,10);
			new FXLabel(frame2,"dBFS");
	
}

CRecordDialog::~CRecordDialog()
{
}

void CRecordDialog::cleanupMeters()
{
	while(!meters.empty())
	{
		delete meters[0];
		meters.erase(meters.begin());
	}
}

//??? include this to get colors.. but they should be in the settings registry actually
#include "drawPortion.h"
void CRecordDialog::setMeterValue(unsigned channel,float value)
{
	FXProgressBar *meter=meters[channel];
	if(value>=1.0)
		meter->setBarColor(clippedWaveformColor);
	else
		// need to make this more usefully be green for comfortable values, to yellow, to red at close to clipping
		meter->setBarColor(FXRGB((int)(value*255),(int)((1.0-value)*255),0));

	meter->setProgress((FXint)(value*1000));
}


static int blinkCounter=0;
long CRecordDialog::onStatusUpdate(FXObject *sender,FXSelector sel,void *ptr)
{
	if(!showing)
		return 0;

	for(unsigned i=0;i<meters.size();i++)
		setMeterValue(i,recorder->getAndResetLastPeakValue(i));

	clipCountLabel->setText((_("Clip Count: ")+istring(recorder->clipCount)).c_str());

	recordedLengthStatusLabel->setText(recorder->getRecordedLengthS().c_str());
	recordedSizeStatusLabel->setText(recorder->getRecordedSizeS().c_str());

	if(recorder->isStarted())
		recordLimitLabel->setText(recorder->getRecordLimitS().c_str());
	else
		recordLimitLabel->setText("");


	blinkCounter++;
	blinkCounter%=6;

	// handle the red recording LED
	if(recorder->isStarted())
	{
		if(recorder->isWaitingForThreshold())
			recordingLED->getFirst()->raise(); // solid while waiting
		else
		{ // blinking while actually recording
			if(blinkCounter>=2)
				recordingLED->getFirst()->raise();
			else
				recordingLED->getFirst()->lower();
		}
	}
	else
		recordingLED->getFirst()->lower();

	// handle the yellow waiting LED
	if(recorder->isWaitingForThreshold())
	{
		//blink
		if(blinkCounter>=2)
			waitingForThresholdLED->getFirst()->raise();
		else
			waitingForThresholdLED->getFirst()->lower();
	}
	else
		waitingForThresholdLED->getFirst()->lower();

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

	// schedule for the next status update
	getApp()->addTimeout(this,CRecordDialog::ID_STATUS_UPDATE,STATUS_UPDATE_TIME);
	return 0;
}

bool CRecordDialog::show(ASoundRecorder *_recorder)
{
	recorder=_recorder;
	clearClipCount();

	// ??? should find name (using a method on CSound) that finds the first available number or 1 more than the max actually
	cueNameNumber->setValue(1);

	for(unsigned i=0;i<recorder->getChannelCount();i++)
	{
		FXProgressBar *meter=new FXProgressBar(meterFrame,NULL,0,PROGRESSBAR_NORMAL|PROGRESSBAR_VERTICAL | LAYOUT_FILL_Y);
		meter->setBarBGColor(FXRGB(0,0,0));
		meter->setTotal(1000);
		meters.push_back(meter);
	}
	meterFrame->recalc();

	onStopButton(NULL,0,NULL); // to clear status labels
	try
	{
		showing=true;
		getApp()->addTimeout(this,CRecordDialog::ID_STATUS_UPDATE,STATUS_UPDATE_TIME);

		// ??? might wanna use rememberShow or some base method
		if(execute(PLACEMENT_CURSOR) && recorder->getRecordedLength()>0)
		{
			showing=false;
			cleanupMeters();
			return true;
		}
		showing=false;
		cleanupMeters();
		return false;
	}
	catch(...)
	{
		cleanupMeters();
		throw;
	}
}

const sample_pos_t CRecordDialog::getMaxDuration()
{
	if(setDurationButton->getCheck())
	{
		bool wasInvalid;
		const sample_pos_t d=recorder->getSound()->getPositionFromTime(durationEdit->getText().text(),wasInvalid);
		if(wasInvalid)
		{
			Error(_("Invalid record time limit -- Should be in the form of HH:MM:SS.sss, MM:SS.sss or SS.sss"));
			return 1;
		}

		if(d==0)
		{
			setDurationButton->setCheck(false);
			return NIL_SAMPLE_POS;
		}
		else
			return d;
	}
	return NIL_SAMPLE_POS;
}

const double CRecordDialog::getStartThreshold()
{
	if(startThresholdButton->getCheck())
	{
		const double startThreshold=atof(startThresholdEdit->getText().text());
		startThresholdEdit->setText(istring(startThreshold,3,2,false).c_str());
		return startThreshold;
	}
	return 1.0; // no threshold
}

long CRecordDialog::onStartButton(FXObject *sender,FXSelector sel,void *ptr)
{
	blinkCounter=2;
	clearClipCount();
	recorder->start(getStartThreshold(),getMaxDuration());
	return 1;
}

long CRecordDialog::onStopButton(FXObject *sender,FXSelector sel,void *ptr)
{
	recorder->stop();
	return 1;
}

long CRecordDialog::onRedoButton(FXObject *sender,FXSelector sel,void *ptr)
{
	recorder->redo(getMaxDuration());
	clearClipCount();
	return 1;
}

long CRecordDialog::onAddCueButton(FXObject *sender,FXSelector sel,void *ptr)
{
	try
	{
		if(cueNameNumber->getValue()==-1)
			recorder->addCueNow(cueNamePrefix->getText().text(),FXSELID(sel)==ID_ADD_ANCHORED_CUE_BUTTON);
		else
		{
			recorder->addCueNow((cueNamePrefix->getText().text()+istring(cueNameNumber->getValue())).c_str(),FXSELID(sel)==ID_ADD_ANCHORED_CUE_BUTTON);
			cueNameNumber->increment();
		}
	}
	catch(exception &e)
	{
		Error(e.what());
	}

	return 1;
}

void CRecordDialog::clearClipCount()
{
	recorder->clipCount=0;
	clipCountLabel->setText(FXString(_("Clip Count: "))+"0");
}

long CRecordDialog::onClearClipCountButton(FXObject *sender,FXSelector sel,void *ptr)
{
	clearClipCount();
	return 1;
}

long CRecordDialog::onDurationSpinner(FXObject *sender,FXSelector sel,void *ptr)
{
	bool wasInvalid;
	sample_fpos_t d=recorder->getSound()->getPositionFromTime(durationEdit->getText().text(),wasInvalid);
	if(wasInvalid)
		d=0;

	d+=((sample_fpos_t)durationSpinner->getValue()*(sample_fpos_t)recorder->getSound()->getSampleRate());
	if(d<0.0)
		d=0;

	durationSpinner->setValue(0);

	durationEdit->setText(recorder->getSound()->getTimePosition((sample_pos_t)d,3,false).c_str());
	if(d!=0)
		setDurationButton->setCheck(true);

	return 1;
}

long CRecordDialog::onStartThresholdSpinner(FXObject *sender,FXSelector sel,void *ptr)
{
	double startThreshold=atof(startThresholdEdit->getText().text());
	startThreshold+=startThresholdSpinner->getValue();
	startThresholdSpinner->setValue(0);

	if(startThreshold<-100)
		startThreshold=-100;
	else if(startThreshold>0)
		startThreshold=0;

	startThresholdEdit->setText(istring(startThreshold,3,2,false).c_str());
	
	startThresholdButton->setCheck(true);
	return 1;
}

long CRecordDialog::onDCOffsetCompensateButton(FXObject *sender,FXSelector sel,void *ptr)
{
	recorder->compensateForDCOffset();
	return 1;
}
