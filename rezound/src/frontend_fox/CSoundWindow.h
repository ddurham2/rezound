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

#ifndef __CSoundWindow_H__
#define __CSoundWindow_H__

#include "../../config/common.h"
#include "fox_compat.h"

#include <string>
#include <map>

#include "FXRezWaveView.h"
#include "settings.h"

class CLoadedSound;

class CAddCueActionFactory;
class CRemoveCueActionFactory;
class CReplaceCueActionFactory;

#include "../backend/CSound_defs.h"


/*
 * This is the window created for each opened sound file
 */

class CSoundWindow : public FXPacker
{
	FXDECLARE(CSoundWindow)
public:
	CSoundWindow(FXComposite *parent,CLoadedSound *_loadedSound);
	virtual ~CSoundWindow();

	void setActiveState(bool isActive);
	bool getActiveState() const;

	virtual void show();

	void updateFromEdit(bool undoing=false);

	void centerStartPos();
	void centerStopPos();

	int getZoomDecimalPlaces() const; // return number of decimal places that should be appropriated on a time value depending on the zoom factor

	sample_pos_t getLeftEdgePosition() const;

	enum
	{
		ID_HORZ_ZOOM_DIAL=FXTopWindow::ID_LAST,
		ID_HORZ_ZOOM_DIAL_PLUS,
		ID_HORZ_ZOOM_DIAL_MINUS,
		ID_HORZ_ZOOM_FIT,

		ID_VERT_ZOOM_DIAL,
		ID_VERT_ZOOM_DIAL_PLUS,
		ID_VERT_ZOOM_DIAL_MINUS,

		ID_BOTH_ZOOM_DIAL_MINUS,

		ID_DRAW_PLAY_POSITION,

		ID_SELECT_START_SPINNER,
		ID_SELECT_STOP_SPINNER,

		ID_WAVEVIEW,

		ID_MUTE_BUTTON,
		ID_INVERT_MUTE_BUTTON,

		ID_REDRAW_BUTTON,

		ID_TIME_UNITS_SETTING,

		ID_SAMPLE_RATE_LABEL,

		ID_LAST
	};

	// events I get from overriding a method
	
	virtual void create();



	// --- event handlers I setup  --------------------------------------------

	// little red play position bar draw event ... this method is called on a timer every fraction of a sec
	long onDrawPlayPosition(FXObject *sender,FXSelector,void*);
	
	// mute button
	long onMuteButton(FXObject *sender,FXSelector sel,void *ptr);
	long onInvertMuteButton(FXObject *sender,FXSelector sel,void *ptr);
	long onResize(FXObject *sender,FXSelector sel,void *ptr);

	// horz zoom handlers
	long onHorzZoomDialChange(FXObject *sender,FXSelector,void*);
	long onHorzZoomDialPlusIndClick(FXObject *sender,FXSelector sel,void *ptr);
	long onHorzZoomDialMinusIndClick(FXObject *sender,FXSelector sel,void *ptr);
	long onHorzZoomFitClick(FXObject *sender,FXSelector sel,void *ptr);

	void horzZoomInSome();
	void horzZoomOutSome();
	void horzZoomInFull();
	void horzZoomOutFull();
	void horzZoomSelectionFit();

	void redraw();


	// vert zoom handlers
	long onVertZoomDialChange(FXObject *sender,FXSelector,void*);
	long onVertZoomDialPlusIndClick(FXObject *sender,FXSelector sel,void *ptr);
	long onVertZoomDialMinusIndClick(FXObject *sender,FXSelector sel,void *ptr);

	long onBothZoomDialMinusIndClick(FXObject *sender,FXSelector sel,void *ptr);


	// selection position spinner handlers
	long onSelectStartSpinnerChange(FXObject *sender,FXSelector sel,void *ptr);
	long onSelectStopSpinnerChange(FXObject *sender,FXSelector sel,void *ptr);


	// position change events in the FXRezWaveView
	long onSelectionChange(FXObject *sender,FXSelector sel,void *ptr);

	// cue handlers
	long onAddCue(FXObject *sender,FXSelector sel,void *ptr);
	long onRemoveCue(FXObject *sender,FXSelector sel,void *ptr);
	long onEditCue(FXObject *sender,FXSelector sel,void *ptr);
	long onShowCueList(FXObject *sender,FXSelector sel,void *ptr);

	long onCloseWindow(FXObject *sender,FXSelector sel,void *ptr);

	long onEditSampleRate(FXObject *sender,FXSelector sel,void *ptr);

	long onTimeUnitsSetting(FXObject *sender,FXSelector sel,void *ptr);

	// called by CSoundFileManager::same_method_name()
	const map<string,string> getPositionalInfo() const;
	void setPositionalInfo(const map<string,string> info);

	string shuttleControlScalar;
	bool shuttleControlSpringBack;
	CLoadedSound * const loadedSound;


protected:

	CSoundWindow() : loadedSound(NULL) {}

private:

	friend void playTrigger(void *Pthis);

#if REZ_FOX_VERSION<10322
	FXTimer *timerHandle; // used to draw play position
#endif
	bool firstTimeShowing;
	bool closing;

	/* 
	                  +-------------------------------------+
	                  | -  -----------------------------  - |
	                  || ||                             || || <---- waveViewPanel
	                  || ||                             || ||
	                  || ||           waveView          || ||
	     mutePanel --->| ||                             || |<---- vertZoomPanel
	                  || ||                             || ||
	                  || ||                             || ||
	                  | -  -----------------------------  - |
	                  | ----------------------------------- |
	                  ||           horzZoomPanel           ||
	                  | ----------------------------------- |
	                  +-------------------------------------+
	                   -------------------------------------
	                  |                                     |<---- statusPanel
	                  |                                     |
	                   -------------------------------------
	*/

	FXHorizontalFrame *statusPanel;
		FXPacker *playingLED; // there are actually two FXLabels in this packer occupying the same positions that I just use raise() on them to turn the LED on or off
		FXPacker *pausedLED;

		FXLabel *sampleRateLabel;
		FXLabel *channelCountLabel;

		FXLabel *audioDataSizeLabel;
		FXLabel *poolFileSizeLabel;

		FXLabel *totalLengthLabels[TIME_UNITS_COUNT];
		FXLabel *selectionLengthLabels[TIME_UNITS_COUNT];

		FXSpinner *selectStartSpinner;
		FXSpinner *selectStopSpinner;

		FXLabel *selectStartLabels[TIME_UNITS_COUNT];
		FXLabel *selectStopLabels[TIME_UNITS_COUNT];

		//FXLabel *playPositionLabel;    used to be a simple label, but calling setText was too CPU intensive
		FXFrame *playPositionLabel;
		FXint prevPlayPositionLabelWidth;

	FXPacker *waveViewPanel;
		FXPacker *horzZoomPanel;
			FXButton *horzZoomFitButton;
			FXButton *horzZoomMinusInd;
			FXDial *horzZoomDial;
			FXButton *horzZoomPlusInd;
			FXLabel *horzZoomValueLabel;
			FXButton *bothZoomMinusButton;
		FXPacker *vertZoomPanel;
			FXButton *vertZoomPlusInd;
			FXDial *vertZoomDial;
			FXButton *vertZoomMinusInd;
		FXPacker *mutePanel;
			FXLabel *upperMuteLabel;
			FXButton *invertMuteButton;
			FXVerticalFrame *muteContents;
		FXRezWaveView *waveView;

	FXFont *statusFont;

	FXCheckButton *muteButtons[MAX_CHANNELS];
	unsigned muteButtonCount;
	void recreateMuteButtons(bool callCreate);

	CAddCueActionFactory * addCueActionFactory;
	CRemoveCueActionFactory * removeCueActionFactory;
	CReplaceCueActionFactory * replaceCueActionFactory;

	bool playingLEDOn;
	bool pausedLEDOn;

	void drawPlayPosition(bool justErasing);

	void updateAllStatusInfo();
	void updateSelectionStatusInfo();
	void updatePlayPositionStatusInfo();

	void changeHorzZoom(double horzZoom,FXWaveCanvas::HorzRecenterTypes horzRecenterType=FXWaveCanvas::hrtNone); // 0 -> 1

	void updateVertZoomDial();
	void updateHorzZoomDial();

	const string getLengthString(sample_pos_t length,TimeUnits units);
};


#endif
