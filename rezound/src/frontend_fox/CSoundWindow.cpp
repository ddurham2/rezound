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

static void playTrigger(void *Pthis);
#warning using the "fit" button doesnt refresh the ruler (cues stick in their position) 

/*
 * TODO
 * 	- change the clicking on the + and - signs go between 0, 25, 50, 100 
 *
 * 	- maybe replace the +/- signs with little magnifying glass icons
 */

#include "CSoundWindow.h"
#include "CSoundFileManager.h"
#include "CMainWindow.h"

#include <algorithm>

#include <istring>

#include "../backend/CLoadedSound.h"
#include "../backend/CSoundPlayerChannel.h"
#include "../backend/CActionParameters.h"

#include "../backend/Edits/CCueAction.h"
#include "CCueDialog.h"
#include "CCueListDialog.h"

#include "CFOXIcons.h"

#include "settings.h"

#define DRAW_PLAY_POSITION_TIME 75	// 75 ms

#define ZOOM_MUL 7

static double zoomDialToZoomFactor(FXint zoomDial)
{
	return pow(zoomDial/(100.0*ZOOM_MUL),1.0/4.0);
}

static FXint zoomFactorToZoomDial(double zoomFactor)
{
	return (FXint)(pow(zoomFactor,4.0)*(100.0*ZOOM_MUL));
}


FXDEFMAP(CSoundWindow) CSoundWindowMap[]=
{
	//	  Message_Type			ID						Message_Handler
	
		// invoked when one of the check buttons down the left side is changed
	FXMAPFUNC(SEL_COMMAND,			CSoundWindow::ID_MUTE_BUTTON,			CSoundWindow::onMuteButton),
	FXMAPFUNC(SEL_COMMAND,			CSoundWindow::ID_INVERT_MUTE_BUTTON,		CSoundWindow::onInvertMuteButton),
	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,	CSoundWindow::ID_INVERT_MUTE_BUTTON,		CSoundWindow::onInvertMuteButton),
	FXMAPFUNC(SEL_CONFIGURE,		0,						CSoundWindow::onResize),

		// invoked when horz zoom dial is changed
	FXMAPFUNC(SEL_CHANGED,			CSoundWindow::ID_HORZ_ZOOM_DIAL,		CSoundWindow::onHorzZoomDialChange),
		// events to quickly zoom all the way in or out
	FXMAPFUNC(SEL_COMMAND,			CSoundWindow::ID_HORZ_ZOOM_DIAL_PLUS,		CSoundWindow::onHorzZoomDialPlusIndClick),
	FXMAPFUNC(SEL_COMMAND,			CSoundWindow::ID_HORZ_ZOOM_DIAL_MINUS,		CSoundWindow::onHorzZoomDialMinusIndClick),
	FXMAPFUNC(SEL_COMMAND,			CSoundWindow::ID_HORZ_ZOOM_FIT,			CSoundWindow::onHorzZoomFitClick),

		// invoked when vert zoom dial is changed
	FXMAPFUNC(SEL_CHANGED,			CSoundWindow::ID_VERT_ZOOM_DIAL,		CSoundWindow::onVertZoomDialChange),
		// events to quickly zoom all the way in or out
	FXMAPFUNC(SEL_COMMAND,			CSoundWindow::ID_VERT_ZOOM_DIAL_PLUS,		CSoundWindow::onVertZoomDialPlusIndClick),
	FXMAPFUNC(SEL_COMMAND,			CSoundWindow::ID_VERT_ZOOM_DIAL_MINUS,		CSoundWindow::onVertZoomDialMinusIndClick),

	FXMAPFUNC(SEL_COMMAND,			CSoundWindow::ID_BOTH_ZOOM_DIAL_MINUS,		CSoundWindow::onBothZoomDialMinusIndClick),

		// timer event to draw the play status position
	FXMAPFUNC(SEL_TIMEOUT,			CSoundWindow::ID_DRAW_PLAY_POSITION,		CSoundWindow::onDrawPlayPosition),


	FXMAPFUNC(SEL_COMMAND,			CSoundWindow::ID_SELECT_START_SPINNER,		CSoundWindow::onSelectStartSpinnerChange),
	FXMAPFUNC(SEL_COMMAND,			CSoundWindow::ID_SELECT_STOP_SPINNER,		CSoundWindow::onSelectStopSpinnerChange),


	FXMAPFUNC(FXRezWaveView::SEL_SELECTION_CHANGED,CSoundWindow::ID_WAVEVIEW,		CSoundWindow::onSelectionChange),

		// cue handlers
	FXMAPFUNC(FXRezWaveView::SEL_ADD_CUE,CSoundWindow::ID_WAVEVIEW,				CSoundWindow::onAddCue),
	FXMAPFUNC(FXRezWaveView::SEL_ADD_CUE_AT_STOP_POSITION,CSoundWindow::ID_WAVEVIEW,	CSoundWindow::onAddCue),
	FXMAPFUNC(FXRezWaveView::SEL_ADD_CUE_AT_START_POSITION,CSoundWindow::ID_WAVEVIEW,	CSoundWindow::onAddCue),
	FXMAPFUNC(FXRezWaveView::SEL_REMOVE_CUE,CSoundWindow::ID_WAVEVIEW,			CSoundWindow::onRemoveCue),
	FXMAPFUNC(FXRezWaveView::SEL_EDIT_CUE,CSoundWindow::ID_WAVEVIEW,			CSoundWindow::onEditCue),
	FXMAPFUNC(FXRezWaveView::SEL_SHOW_CUE_LIST,CSoundWindow::ID_WAVEVIEW,			CSoundWindow::onShowCueList),


	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,	CSoundWindow::ID_TIME_UNITS_SETTING,		CSoundWindow::onTimeUnitsSetting),


	FXMAPFUNC(SEL_CLOSE,			0,						CSoundWindow::onCloseWindow),
};

FXIMPLEMENT(CSoundWindow,FXPacker,CSoundWindowMap,ARRAYNUMBER(CSoundWindowMap))


void playTrigger(void *Pthis)
{
	CSoundWindow *that=(CSoundWindow *)Pthis;
	// ??? this is called from another thread.. I don't know if it will cause a problem in FOX
	if(that->timerHandle==NULL)
		that->timerHandle=that->getApp()->addTimeout(that,CSoundWindow::ID_DRAW_PLAY_POSITION,DRAW_PLAY_POSITION_TIME);
}

// ----------------------------------------------------------

CSoundWindow::CSoundWindow(FXComposite *parent,CLoadedSound *_loadedSound) :
	FXPacker(parent,FRAME_NONE|LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0, 0,0,0,0, 0,0),

	shuttleControlScalar("100x"),
	shuttleControlSpringBack(true),

	loadedSound(_loadedSound),

	timerHandle(NULL),
	firstTimeShowing(true),
	closing(false),

	statusPanel(new FXHorizontalFrame(this,FRAME_NONE | LAYOUT_SIDE_BOTTOM | LAYOUT_FILL_X, 0,0,0,0, 4,4,2,2, 2,0)),

	waveViewPanel(new FXPacker(this,FRAME_NONE | LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 0,0)),
		horzZoomPanel(new FXPacker(waveViewPanel,LAYOUT_SIDE_BOTTOM | FRAME_NONE | LAYOUT_FILL_X | LAYOUT_FIX_HEIGHT, 0,0,0,22, 4,4,2,2, 2,2)),
			horzZoomFitButton(new FXButton(horzZoomPanel,_("Fit\tZoom to Fit Selection"),NULL,this,ID_HORZ_ZOOM_FIT,FRAME_RAISED | LAYOUT_SIDE_LEFT | LAYOUT_FILL_Y)),
			horzZoomMinusInd(new FXButton(horzZoomPanel,FXString(" - \t")+_("Zoom Out Full"),NULL,this,ID_HORZ_ZOOM_DIAL_MINUS,FRAME_RAISED | LAYOUT_SIDE_LEFT | LAYOUT_FILL_Y)),
			horzZoomDial(new FXDial(horzZoomPanel,this,ID_HORZ_ZOOM_DIAL,LAYOUT_SIDE_LEFT | DIAL_HORIZONTAL|DIAL_HAS_NOTCH | LAYOUT_FILL_Y | LAYOUT_FIX_WIDTH, 0,0,200,0, 0,0,0,0)),
			horzZoomPlusInd(new FXButton(horzZoomPanel,FXString(" + \t")+_("Zoom In Full"),NULL,this,ID_HORZ_ZOOM_DIAL_PLUS,FRAME_RAISED | LAYOUT_SIDE_LEFT | LAYOUT_FILL_Y)),
			horzZoomValueLabel(new FXLabel(horzZoomPanel,"",NULL,LAYOUT_SIDE_LEFT | LAYOUT_FILL_Y)),
			bothZoomMinusButton(new FXButton(horzZoomPanel,FXString(" -- \t")+_("Horizontally and Vertically Zoom Out Full"),NULL,this,ID_BOTH_ZOOM_DIAL_MINUS,FRAME_RAISED | LAYOUT_SIDE_RIGHT | LAYOUT_FILL_Y)),
		vertZoomPanel(new FXPacker(waveViewPanel,LAYOUT_SIDE_RIGHT | FRAME_NONE | LAYOUT_FILL_Y | LAYOUT_FIX_WIDTH, 0,0,17,0, 2,2,2,2, 2,2)),
			vertZoomPlusInd(new FXButton(vertZoomPanel,FXString("+\t")+_("Zoom In Full"),NULL,this,ID_VERT_ZOOM_DIAL_PLUS,FRAME_RAISED | LAYOUT_SIDE_TOP | LAYOUT_FILL_X)),
			vertZoomDial(new FXDial(vertZoomPanel,this,LAYOUT_SIDE_TOP | ID_VERT_ZOOM_DIAL,DIAL_VERTICAL|DIAL_HAS_NOTCH | LAYOUT_FILL_X | LAYOUT_FIX_HEIGHT, 0,0,0,100, 0,0,0,0)),
			vertZoomMinusInd(new FXButton(vertZoomPanel,FXString("-\t")+_("Zoom Out Full"),NULL,this,ID_VERT_ZOOM_DIAL_MINUS,FRAME_RAISED | LAYOUT_SIDE_TOP | LAYOUT_FILL_X)),
		mutePanel(new FXPacker(waveViewPanel,LAYOUT_SIDE_LEFT | FRAME_NONE | LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 0,0)),
			upperMuteLabel(new FXLabel(mutePanel,_("M\tMute Channels"),NULL,LABEL_NORMAL|LAYOUT_SIDE_TOP|LAYOUT_FIX_HEIGHT)),
			invertMuteButton(new FXButton(mutePanel,_("!\tInvert the Muted State of Each Channel or (right click) Unmute All Channels"),NULL,this,ID_INVERT_MUTE_BUTTON,LAYOUT_FILL_X | FRAME_RAISED|FRAME_THICK | LAYOUT_SIDE_BOTTOM|LAYOUT_FIX_HEIGHT)),
			muteContents(new FXVerticalFrame(mutePanel,LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 0,0)),
		waveView(new FXRezWaveView(waveViewPanel,_loadedSound)),

	statusFont(getApp()->getNormalFont()),

	muteButtonCount(0),

	addCueActionFactory(NULL),
	removeCueActionFactory(NULL),
	replaceCueActionFactory(NULL),

	playingLEDOn(false),
	pausedLEDOn(false)
{
	waveView->setTarget(this);
	waveView->setSelector(ID_WAVEVIEW);

	recreateMuteButtons(false);
	
	// create the font to use for status information
        FXFontDesc d;
        statusFont->getFontDesc(d);
        d.size-=10;
        statusFont=new FXFont(getApp(),d);


	// fill the status panel
	// ??? If I really wanted to make sure there was a min width to each section (and I may want to) I could put an FXFrame with zero height or 1 height in each on with a fixed width.. then things can get bigger as needed... but there would be a minimum size
	FXComposite *t;
	FXLabel *l;

	statusPanel->setTarget(this);
	statusPanel->setSelector(ID_TIME_UNITS_SETTING);

	t=new FXVerticalFrame(statusPanel,FRAME_NONE|LAYOUT_FILL_Y, 0,0,0,0, 2,0,0,0, 0,0);
		// add the actual LEDs to the packer for turning the LED on and off, done by raising or lowering the first child of the packer
		playingLED=new FXPacker(t,LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0);
			(new FXLabel(playingLED,_("Playing:"),FOXIcons->GreenLED1,JUSTIFY_NORMAL | ICON_AFTER_TEXT | LAYOUT_FIX_X|LAYOUT_FIX_Y, 0,0))->setFont(statusFont);
			(new FXLabel(playingLED,_("Playing:"),FOXIcons->OffLED1,JUSTIFY_NORMAL | ICON_AFTER_TEXT | LAYOUT_FIX_X|LAYOUT_FIX_Y, 0,0))->setFont(statusFont);
		pausedLED=new FXPacker(t,LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0);
			(new FXLabel(pausedLED,_("Paused:"),FOXIcons->YellowLED1,JUSTIFY_NORMAL | ICON_AFTER_TEXT | LAYOUT_FIX_X|LAYOUT_FIX_Y, 0,0))->setFont(statusFont);
			(new FXLabel(pausedLED,_("Paused:"),FOXIcons->OffLED1,JUSTIFY_NORMAL | ICON_AFTER_TEXT | LAYOUT_FIX_X|LAYOUT_FIX_Y, 0,0))->setFont(statusFont);

	new FXVerticalSeparator(statusPanel);
	t=new FXVerticalFrame(statusPanel,FRAME_NONE|LAYOUT_FILL_Y, 0,0,0,0, 2,0,0,0, 0,0);
		sampleRateLabel=new FXLabel(t,_("Sample Rate; "),NULL,LAYOUT_LEFT|LAYOUT_FILL_Y);
		sampleRateLabel->setFont(statusFont);
		channelCountLabel=new FXLabel(t,_("Channel Count: "),NULL,LAYOUT_LEFT|LAYOUT_FILL_Y);
		channelCountLabel->setFont(statusFont);

	new FXVerticalSeparator(statusPanel);
	t=new FXVerticalFrame(statusPanel,FRAME_NONE|LAYOUT_FILL_Y, 0,0,0,0, 2,0,0,0, 0,0);
		audioDataSizeLabel=new FXLabel(t,_("Audio Size; "),NULL,LAYOUT_RIGHT|LAYOUT_FILL_Y);
		audioDataSizeLabel->setFont(statusFont);
		poolFileSizeLabel=new FXLabel(t,_("Working File: "),NULL,LAYOUT_RIGHT|LAYOUT_FILL_Y);
		poolFileSizeLabel->setFont(statusFont);

	new FXVerticalSeparator(statusPanel);
	t=new FXMatrix(statusPanel,TIME_UNITS_COUNT+1,FRAME_NONE|MATRIX_BY_COLUMNS|LAYOUT_FILL_Y,0,0,0,0, 2,0,0,0, 2,0);
		// total
		(l=new FXLabel(t,_("Total: "),NULL,LAYOUT_RIGHT|LAYOUT_FILL_ROW|LAYOUT_CENTER_Y,0,0,0,0, 0,0,0,0))->setFont(statusFont);
		l->setTarget(this);
		l->setSelector(ID_TIME_UNITS_SETTING);
		for(size_t i=0;i<TIME_UNITS_COUNT;i++)
		{
			(totalLengthLabels[i]=new FXLabel(t,"",NULL,LAYOUT_RIGHT|LAYOUT_FILL_ROW|LAYOUT_CENTER_Y,0,0,0,0, 0,0,0,0))->setFont(statusFont);
			totalLengthLabels[i]->setTarget(this);
			totalLengthLabels[i]->setSelector(ID_TIME_UNITS_SETTING);
		}

		// selection
		(l=new FXLabel(t,_("Selection: "),NULL,LAYOUT_RIGHT|LAYOUT_FILL_ROW|LAYOUT_CENTER_Y,0,0,0,0, 0,0,0,0))->setFont(statusFont);
		l->setTarget(this);
		l->setSelector(ID_TIME_UNITS_SETTING);
		for(size_t i=0;i<TIME_UNITS_COUNT;i++)
		{
			(selectionLengthLabels[i]=new FXLabel(t,"",NULL,LAYOUT_RIGHT|LAYOUT_FILL_ROW|LAYOUT_CENTER_Y,0,0,0,0, 0,0,0,0))->setFont(statusFont);
			selectionLengthLabels[i]->setTarget(this);
			selectionLengthLabels[i]->setSelector(ID_TIME_UNITS_SETTING);
		}
		
	new FXVerticalSeparator(statusPanel);
	t=new FXMatrix(statusPanel,TIME_UNITS_COUNT+2,FRAME_NONE|MATRIX_BY_COLUMNS|LAYOUT_FILL_Y,0,0,0,0, 2,0,0,0, 2,0);

		selectStartSpinner=new FXSpinner(t,0,this,ID_SELECT_START_SPINNER, SPIN_NOTEXT|LAYOUT_FILL_ROW|LAYOUT_CENTER_Y);
		selectStartSpinner->setRange(-10,10);
		selectStartSpinner->setTipText(_("Increase/Decrease Start Position by One Sample"));
		(l=new FXLabel(t,_("Start: "),NULL,LAYOUT_RIGHT|LAYOUT_FILL_ROW|LAYOUT_CENTER_Y,0,0,0,0, 0,0,0,0))->setFont(statusFont);
		l->setTarget(this);
		l->setSelector(ID_TIME_UNITS_SETTING);
		for(size_t i=0;i<TIME_UNITS_COUNT;i++)
		{
			(selectStartLabels[i]=new FXLabel(t,"",NULL,LAYOUT_RIGHT|LAYOUT_FILL_ROW|LAYOUT_CENTER_Y,0,0,0,0, 0,0,0,0))->setFont(statusFont);
			selectStartLabels[i]->setTarget(this);
			selectStartLabels[i]->setSelector(ID_TIME_UNITS_SETTING);
		}

		selectStopSpinner=new FXSpinner(t,0,this,ID_SELECT_STOP_SPINNER, SPIN_NOTEXT|LAYOUT_FILL_ROW|LAYOUT_CENTER_Y);
		selectStopSpinner->setRange(-10,10);
		selectStopSpinner->setTipText(_("Increase/Decrease Stop Position by One Sample"));
		(l=new FXLabel(t,_("Stop: "),NULL,LAYOUT_RIGHT|LAYOUT_FILL_ROW|LAYOUT_CENTER_Y,0,0,0,0, 0,0,0,0))->setFont(statusFont);
		l->setTarget(this);
		l->setSelector(ID_TIME_UNITS_SETTING);
		for(size_t i=0;i<TIME_UNITS_COUNT;i++)
		{
			(selectStopLabels[i]=new FXLabel(t,"",NULL,LAYOUT_RIGHT|LAYOUT_FILL_ROW|LAYOUT_CENTER_Y,0,0,0,0, 0,0,0,0))->setFont(statusFont);
			selectStopLabels[i]->setTarget(this);
			selectStopLabels[i]->setSelector(ID_TIME_UNITS_SETTING);
		}

	new FXVerticalSeparator(statusPanel);
	playPositionLabel=new FXFrame(statusPanel,FRAME_NONE|LAYOUT_FILL_X|LAYOUT_FILL_Y); // used to be a label, but setText was too CPU intensive
	prevPlayPositionLabelWidth=0;

	// register to know when the sound start/stops so we can know when it is necessary
	// to draw the play position and when to change the state of the playing/paused LEDs
	loadedSound->channel->addOnPlayTrigger(playTrigger,this);


	// set the ranges and initial values of the vertical and horiztonal zoom dials
	horzZoomDial->setRange(0,100*ZOOM_MUL);
	horzZoomDial->setRevolutionIncrement(100*ZOOM_MUL);
	horzZoomDial->setValue(0);

	vertZoomDial->setRange(0,100*ZOOM_MUL);
	vertZoomDial->setRevolutionIncrement(100*ZOOM_MUL);
	vertZoomDial->setValue(0);


	addCueActionFactory=new CAddCueActionFactory(gCueDialog);
	removeCueActionFactory=new CRemoveCueActionFactory;
	replaceCueActionFactory=new CReplaceCueActionFactory(gCueDialog);
}

CSoundWindow::~CSoundWindow()
{
	if(timerHandle!=NULL)
		getApp()->removeTimeout(timerHandle);

	loadedSound->channel->removeOnPlayTrigger(playTrigger,this);

	closing=true;
/*
	while(timerHandle!=NULL)
	{
		printf("waiting on timerHandle to fire\n");
		fxsleep(1000); // sleep for 1 millisecond
	}
*/

	delete addCueActionFactory;
	delete removeCueActionFactory;
	delete replaceCueActionFactory;

	// ??? do I need to delete the stuff I allocated

	delete statusFont;
}

void CSoundWindow::recreateMuteButtons(bool callCreate)
{
	FXbool state[MAX_CHANNELS];
	for(unsigned t=0;t<MAX_CHANNELS;t++)
		state[t]=false;

	for(unsigned t=0;t<muteButtonCount;t++)
	{
		state[t]=muteButtons[t]->getCheck();
		delete muteButtons[t];
	}

	muteButtonCount=loadedSound->sound->getChannelCount();
	for(unsigned t=0;t<muteButtonCount;t++)
	{
		muteButtons[t]=new FXCheckButton(muteContents,"",this,ID_MUTE_BUTTON,CHECKBUTTON_NORMAL|LAYOUT_CENTER_Y);
		if(callCreate)
			muteButtons[t]->create();
		muteButtons[t]->setCheck(state[t]); // restore the checked state
	}
}

void CSoundWindow::setActiveState(bool isActive)
{
	if(isActive)
	{
		static_cast<CMainWindow *>(getOwner()->getOwner()->getOwner())->positionShuttleGivenSpeed(loadedSound->channel->getSeekSpeed(),shuttleControlScalar,shuttleControlSpringBack);
		recalc();
		gSoundFileManager->untoggleActiveForAllSoundWindows(this);
		show();
	}
	else
		hide();
}

bool CSoundWindow::getActiveState() const
{
	return(shown());
}

void CSoundWindow::show()
{
	FXPacker::show();
	if(firstTimeShowing)
	{
			// approximately show 10 (setting) seconds of sound (apx because we haven't done layout yet)
			// can't do this at construction because we don't know the max until the window's are laid out
		waveView->showAmount(gInitialLengthToShow,0);
		horzZoomDial->setValue(zoomFactorToZoomDial(waveView->getHorzZoom()));
		onHorzZoomDialChange(NULL,0,NULL);

		firstTimeShowing=false;
	}
}

template <class T> bool between(const T &v, const T &v1, const T &v2)
{
	return(v>=min(v1,v2) && v<=max(v1,v2));
}

void CSoundWindow::drawPlayPosition(bool justErasing)
{
	if(!justErasing && !loadedSound->channel->isPlaying())
		return;

	const sample_pos_t position=loadedSound->channel->getPosition();
	const sample_pos_t start=loadedSound->channel->getStartPosition();
	const sample_pos_t stop=loadedSound->channel->getStopPosition();

	// length to test for to see if we should draw the play position
	const sample_pos_t length=
		(loadedSound->channel->isPlayingSelectionOnly() && between(position,start,stop) )
			? 
			(stop-start) 
			: 
			(loadedSound->channel->sound->getLength())
		;

	const size_t sampleRate=loadedSound->channel->sound->getSampleRate();

	// quarter second minimum to draw the play status
	if(length>=sampleRate || length*4/sampleRate>=1 || loadedSound->channel->isPaused())
		waveView->drawPlayPosition(position,justErasing,gFollowPlayPosition);
	else
		waveView->drawPlayPosition(position,true,false);
}

int CSoundWindow::getZoomDecimalPlaces() const
{
	// ??? this should depend on the FXRezWaveView's actual horzZoomFactor value because the FXDial doesn't represent how many samples a pixel represents
	int places;
	if(horzZoomDial->getValue()>75*ZOOM_MUL)
		places=5;
	else if(horzZoomDial->getValue()>60*ZOOM_MUL)
		places=4;
	else if(horzZoomDial->getValue()>40*ZOOM_MUL)
		places=3;
	else
		places=2;

	return places;
}

void CSoundWindow::updateFromEdit(bool undoing)
{
	// see if the number of channels changed
	if(loadedSound->sound->getChannelCount()!=muteButtonCount)
		recreateMuteButtons(true);

	waveView->updateFromEdit(undoing);
	updateAllStatusInfo();
}

static const string commifyNumber(const string s)
{
	string ret;
	ret.reserve(s.length());
	for(size_t t=0;t<s.length();t++)
	{
		if(t>0 && (t%3)==0)
			ret=","+ret;
		ret=s[s.length()-t-1]+ret;
	}

	return ret;
}

const string CSoundWindow::getLengthString(sample_pos_t length,TimeUnits units)
{
	if(units==tuSeconds)
		return gShowTimeUnits[units] ? loadedSound->sound->getTimePosition(length,getZoomDecimalPlaces()) : "";
	else if(units==tuFrames)
		return gShowTimeUnits[units] ? commifyNumber(istring(length))+"f" : "";
	else if(units==tuBytes)
		return gShowTimeUnits[units] ? commifyNumber(istring(length*sizeof(sample_t)*loadedSound->sound->getChannelCount()))+"b" : "";
	throw runtime_error(string(__func__)+" -- unhandled units enum value: "+istring(units));
}

void CSoundWindow::updateAllStatusInfo()
{
	sampleRateLabel->setText((_("Rate: ")+istring(loadedSound->sound->getSampleRate())).c_str());
	channelCountLabel->setText((_("Channels: ")+istring(loadedSound->sound->getChannelCount())).c_str());

	audioDataSizeLabel->setText((_("Audio Size: ")+loadedSound->sound->getAudioDataSize()).c_str());
	poolFileSizeLabel->setText((_("Working File: ")+loadedSound->sound->getPoolFileSize()).c_str());

	for(size_t t=0;t<TIME_UNITS_COUNT;t++)
		totalLengthLabels[t]->setText(getLengthString(loadedSound->sound->getLength(),(TimeUnits)t).c_str());

	updateSelectionStatusInfo();
	updatePlayPositionStatusInfo();
}

void CSoundWindow::updateSelectionStatusInfo()
{
	for(size_t t=(TimeUnits)0;t<TIME_UNITS_COUNT;t++)
	{
		selectionLengthLabels[t]->setText(getLengthString(loadedSound->channel->getStopPosition()-loadedSound->channel->getStartPosition()+1,(TimeUnits)t).c_str());
		selectStartLabels[t]->setText(getLengthString(loadedSound->channel->getStartPosition(),(TimeUnits)t).c_str());
		selectStopLabels[t]->setText(getLengthString(loadedSound->channel->getStopPosition(),(TimeUnits)t).c_str());
	}
}

void CSoundWindow::updatePlayPositionStatusInfo()
{
	FXDCWindow dc(playPositionLabel);

	if(prevPlayPositionLabelWidth>0)
	{ // erase previous text
		dc.setForeground(playPositionLabel->getBackColor());
		dc.fillRectangle(2,0,prevPlayPositionLabelWidth,playPositionLabel->getHeight());
		prevPlayPositionLabelWidth=0;
	}

	if(loadedSound->channel->isPlaying())
	{ // draw new play position
		const string label=_("Playing: ")+getLengthString(loadedSound->channel->getPosition(),tuSeconds);

#if REZ_FOX_VERSION<10117	
		dc.setTextFont(statusFont);
#else
		dc.setFont(statusFont);
#endif
		dc.setForeground(FXRGB(0,0,0));
		dc.drawText(2,(playPositionLabel->getHeight()/2)+(statusFont->getFontHeight()/2),label.c_str(),label.size());

		prevPlayPositionLabelWidth=statusFont->getTextWidth(label.c_str(),label.size())+1;
	}
}

void CSoundWindow::centerStartPos()
{
	waveView->centerStartPos();
}

void CSoundWindow::centerStopPos()
{
	waveView->centerStopPos();
}


// events I get from overriding a method

void CSoundWindow::create()
{
	FXPacker::create();

	updateAllStatusInfo();
}

// --- event handlers I setup  --------------------------------------------

// mute button
long CSoundWindow::onMuteButton(FXObject *sender,FXSelector,void*)
{
	for(unsigned t=0;t<loadedSound->sound->getChannelCount();t++)
		loadedSound->channel->setMute(t,muteButtons[t]->getCheck());

	return 1;
}

long CSoundWindow::onInvertMuteButton(FXObject *sender,FXSelector sel,void *ptr)
{
	if(FXSELTYPE(sel)==SEL_COMMAND)
	{
		for(unsigned t=0;t<loadedSound->sound->getChannelCount();t++)
			muteButtons[t]->setCheck(!muteButtons[t]->getCheck());
	}
	else
	{
		if(!invertMuteButton->underCursor())
			return(0);

		for(unsigned t=0;t<loadedSound->sound->getChannelCount();t++)
			muteButtons[t]->setCheck(false);
	}

	onMuteButton(sender,sel,ptr);

	return 1;
}

long CSoundWindow::onResize(FXObject *sender,FXSelector sel,void *ptr)
{
	// set the proper height of the labels above and below the panel with the mute checkboxes in it
	int top,height;
	waveView->getWaveSize(top,height);
	upperMuteLabel->setHeight(top);
	invertMuteButton->setHeight(waveView->getHeight()-(height+top));

	return 1;
}


// little red play position bar draw event ... this method is called on a timer every fraction of a sec
long CSoundWindow::onDrawPlayPosition(FXObject *sender,FXSelector,void*)
{
	// timer has fired
	timerHandle=NULL;

	if(closing) // destructor is waiting on this event
		return 1;

	updatePlayPositionStatusInfo();

	if(loadedSound->channel->isPlaying() && !playingLEDOn)
	{
		playingLED->getFirst()->raise();
		playingLEDOn=true;
	}
	else if(!loadedSound->channel->isPlaying() && playingLEDOn)
	{
		playingLED->getFirst()->lower();
		playingLEDOn=false;
	}

	if(loadedSound->channel->isPaused() && !pausedLEDOn)
	{
		pausedLED->getFirst()->raise();
		pausedLEDOn=true;
	}
	else if(!loadedSound->channel->isPaused() && pausedLEDOn)
	{
		pausedLED->getFirst()->lower();
		pausedLEDOn=false;
	}


	if(loadedSound->channel->isPlaying())
	{
		drawPlayPosition(false);

		// ??? I could make the calculation of the next event more intelligent
		// 	- if the onscreen data is smaller I could register to get the timer faster 
		// reregister to get this event again
		timerHandle=getApp()->addTimeout(this,ID_DRAW_PLAY_POSITION,DRAW_PLAY_POSITION_TIME);
	}
	else
	{
		// erase the play position line and don't register to get the timer again
		drawPlayPosition(true);
	}

	return 0; // returning 0 because 1 makes it use a ton of CPU (because returning 1 causes FXApp::refresh() to be called)
}

// horz zoom handlers

long CSoundWindow::onHorzZoomDialChange(FXObject *sender ,FXSelector sel,void *ptr)
{
	FXWaveCanvas::HorzRecenterTypes horzRecenterType=FXWaveCanvas::hrtAuto;

	FXint dummy;
	FXuint keyboardModifierState;
	horzZoomDial->getCursorPosition(dummy,dummy,keyboardModifierState);

	// only regard the keyboard modifier keys if the mouse is bing used to drag the dial
	if(keyboardModifierState&LEFTBUTTONMASK)
	{
		// depend on keyboard modifier change the way it recenters while zooming
		if(keyboardModifierState&SHIFTMASK)
			horzRecenterType=FXWaveCanvas::hrtStart;
		else if(keyboardModifierState&CONTROLMASK)
			horzRecenterType=FXWaveCanvas::hrtStop;
	}
		

	// ??? for some reason, this does not behave quite like the original (before rewrite), but it is acceptable for now... perhaps look at it later
	waveView->setHorzZoom(zoomDialToZoomFactor(horzZoomDial->getValue()),horzRecenterType);
	horzZoomValueLabel->setText(("  "+istring(horzZoomDial->getValue()/(double)ZOOM_MUL,3,1,true)+"%").c_str());
	horzZoomValueLabel->repaint(); // make it update now

	updateAllStatusInfo(); // in case the number of drawn decimal places needs to change

	return 1;
}

long CSoundWindow::onHorzZoomDialPlusIndClick(FXObject *sender,FXSelector sel,void *ptr)
{
	horzZoomInFull();
	return 1;
}

long CSoundWindow::onHorzZoomDialMinusIndClick(FXObject *sender,FXSelector sel,void *ptr)
{
	horzZoomOutFull();
	return 1;
}

long CSoundWindow::onHorzZoomFitClick(FXObject *sender,FXSelector sel,void *ptr)
{
	horzZoomSelectionFit();
	return 1;
}

void CSoundWindow::horzZoomInSome()
{
	FXint hi,lo;
	horzZoomDial->getRange(hi,lo);
	horzZoomDial->setValue((FXint)(horzZoomDial->getValue()-((hi-lo)/25.0))); // zoom in 1/25th of the full range
	onHorzZoomDialChange(NULL,0,NULL);
}

void CSoundWindow::horzZoomOutSome()
{
	FXint hi,lo;
	horzZoomDial->getRange(hi,lo);
	horzZoomDial->setValue((FXint)(horzZoomDial->getValue()+((hi-lo)/25.0))); // zoom out 1/25th of the full range
	onHorzZoomDialChange(NULL,0,NULL);
}

void CSoundWindow::horzZoomInFull()
{
	horzZoomDial->setValue(100*ZOOM_MUL);
	onHorzZoomDialChange(NULL,0,NULL);
}

void CSoundWindow::horzZoomOutFull()
{
	horzZoomDial->setValue(0);
	onHorzZoomDialChange(NULL,0,NULL);
}

void CSoundWindow::updateHorzZoomDial()
{
	horzZoomDial->setValue(zoomFactorToZoomDial(waveView->getHorzZoom()));
	horzZoomValueLabel->setText(("  "+istring(horzZoomDial->getValue()/(double)ZOOM_MUL,3,1,true)+"%").c_str());
}

void CSoundWindow::horzZoomSelectionFit()
{
	waveView->showAmount((sample_fpos_t)(loadedSound->channel->getStopPosition()-loadedSound->channel->getStartPosition()+1)/(sample_fpos_t)loadedSound->sound->getSampleRate(),loadedSound->channel->getStartPosition(),10);
	updateHorzZoomDial();
}

void CSoundWindow::redraw()
{
	loadedSound->sound->invalidateAllPeakData();
	updateFromEdit(); // to cause everything to redraw even if not necessary
}



// vert zoom handlers
long CSoundWindow::onVertZoomDialChange(FXObject *sender,FXSelector,void*)
{
	waveView->setVertZoom(pow(vertZoomDial->getValue()/(100.0*ZOOM_MUL),1.0/2.0));
	return 1;
}

long CSoundWindow::onVertZoomDialPlusIndClick(FXObject *sender,FXSelector sel,void *ptr)
{
	vertZoomDial->setValue(100*ZOOM_MUL);
	onVertZoomDialChange(NULL,0,NULL);
	return 1;
}

long CSoundWindow::onVertZoomDialMinusIndClick(FXObject *sender,FXSelector sel,void *ptr)
{
	vertZoomDial->setValue(0);
	onVertZoomDialChange(NULL,0,NULL);
	return 1;
}


long CSoundWindow::onBothZoomDialMinusIndClick(FXObject *sender,FXSelector sel,void *ptr)
{
	horzZoomDial->setValue(0);
	onHorzZoomDialChange(NULL,0,NULL);

	vertZoomDial->setValue(0);
	onVertZoomDialChange(NULL,0,NULL);

	return 1;
}

void CSoundWindow::updateVertZoomDial()
{
				// inverse of expr in onVertZoomDialChange
	vertZoomDial->setValue((FXint)(100*ZOOM_MUL*pow((double)waveView->getVertZoom(),2.0)));
}



// selection position spinners
long CSoundWindow::onSelectStartSpinnerChange(FXObject *sender,FXSelector sel,void *ptr)
{
	sample_fpos_t newSelectStart=(sample_fpos_t)loadedSound->channel->getStartPosition()+selectStartSpinner->getValue();


	if(newSelectStart>=loadedSound->sound->getLength()-1)
		newSelectStart=loadedSound->sound->getLength()-1;
	if(newSelectStart<0)
		newSelectStart=0;

	loadedSound->channel->setStartPosition((sample_pos_t)newSelectStart);
	selectStartSpinner->setValue(0);
	waveView->updateFromSelectionChange(FXWaveCanvas::lcpStart);
	return 1;
}

long CSoundWindow::onSelectStopSpinnerChange(FXObject *sender,FXSelector sel,void *ptr)
{
	sample_fpos_t newSelectStop=(sample_fpos_t)loadedSound->channel->getStopPosition()+selectStopSpinner->getValue();

	if(newSelectStop>=loadedSound->sound->getLength()-1)
		newSelectStop=loadedSound->sound->getLength()-1;
	if(newSelectStop<0)
		newSelectStop=0;

	loadedSound->channel->setStopPosition((sample_pos_t)newSelectStop);
	selectStopSpinner->setValue(0);
	waveView->updateFromSelectionChange(FXWaveCanvas::lcpStop);
	return 1;
}

long CSoundWindow::onSelectionChange(FXObject *sender,FXSelector sel,void *ptr)
{
	updateSelectionStatusInfo();
	return 1;
}

long CSoundWindow::onAddCue(FXObject *sender,FXSelector sel,void *ptr)
{
	try
	{
		CActionParameters actionParameters(NULL);

		// add the parameters for the dialog to display initially
		actionParameters.addStringParameter("name","Cue1");

		switch(FXSELTYPE(sel))
		{
		case FXRezWaveView::SEL_ADD_CUE:
			actionParameters.addSamplePosParameter("position",*((sample_pos_t *)ptr));
			break;

		case FXRezWaveView::SEL_ADD_CUE_AT_START_POSITION:
			actionParameters.addSamplePosParameter("position",loadedSound->channel->getStartPosition());
			break;

		case FXRezWaveView::SEL_ADD_CUE_AT_STOP_POSITION:
			actionParameters.addSamplePosParameter("position",loadedSound->channel->getStopPosition());
			break;
		}

		actionParameters.addBoolParameter("isAnchored",false);

		addCueActionFactory->performAction(loadedSound,&actionParameters,false);
		updateFromEdit();
	}
	catch(exception &e)
	{
		Error(e.what());
	}

	return 1;
}

long CSoundWindow::onRemoveCue(FXObject *sender,FXSelector sel,void *ptr)
{
	CActionParameters actionParameters(NULL);
	actionParameters.addUnsignedParameter("index",*((size_t *)ptr));
	removeCueActionFactory->performAction(loadedSound,&actionParameters,false);
	updateFromEdit();
	return 1;
}

long CSoundWindow::onEditCue(FXObject *sender,FXSelector sel,void *ptr)
{
	CActionParameters actionParameters(NULL);
	size_t cueIndex=*((size_t *)ptr);

	// add the parameters for the dialog to display initially
	actionParameters.addUnsignedParameter("index",cueIndex);
	actionParameters.addStringParameter("name",loadedSound->sound->getCueName(cueIndex));
	actionParameters.addSamplePosParameter("position",loadedSound->sound->getCueTime(cueIndex));
	actionParameters.addBoolParameter("isAnchored",loadedSound->sound->isCueAnchored(cueIndex));

	replaceCueActionFactory->performAction(loadedSound,&actionParameters,false);
	updateFromEdit();
	return 1;
}

long CSoundWindow::onShowCueList(FXObject *sender,FXSelector sel,void *ptr)
{
	gCueListDialog->setLoadedSound(loadedSound);
	gCueListDialog->execute(PLACEMENT_CURSOR);
	return 1;
}

long CSoundWindow::onCloseWindow(FXObject *sender,FXSelector sel,void *ptr)
{
	gSoundFileManager->close(ASoundFileManager::ctSaveYesNoCancel,loadedSound);
	return 1;
}

long CSoundWindow::onTimeUnitsSetting(FXObject *sender,FXSelector sel,void *ptr)
{
	popupTimeUnitsSelectionMenu((FXWindow *)sender,(FXEvent *)ptr);
	updateAllStatusInfo();
}

const map<string,string> CSoundWindow::getPositionalInfo() const
{
	map<string,string> info;
	info["horzZoom"]=istring(waveView->getHorzZoom());
	info["vertZoom"]=istring(waveView->getVertZoom());
	info["horzOffset"]=istring(waveView->getHorzOffset());
	info["vertOffset"]=istring(waveView->getVertOffset());
	return info;
}

void CSoundWindow::setPositionalInfo(const map<string,string> _info)
{
	map<string,string> info=_info; /* making copy so ["asdf"] can be called on the map */
	if(!info.empty())
	{
		waveView->setHorzZoom(atof(info["horzZoom"].c_str()),FXWaveCanvas::hrtNone);
		waveView->setVertZoom(atof(info["vertZoom"].c_str()));
		waveView->setHorzOffset(atoll(info["horzOffset"].c_str()));
		waveView->setVertOffset(atoi(info["vertOffset"].c_str()));

		updateVertZoomDial();
		updateHorzZoomDial();
		updateAllStatusInfo();
	}
}

