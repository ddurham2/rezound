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

#ifndef __CRecordDialog_H__
#define __CRecordDialog_H__

#include "../../config/common.h"
#include "fox_compat.h"

class CRecordDialog;

#include "FXModalDialogBox.h"
#include <vector>

#include "../backend/CSound_defs.h"

class ASoundRecorder;

class CRecordDialog : public FXModalDialogBox
{
	FXDECLARE(CRecordDialog);
public:
	CRecordDialog(FXWindow *mainWindow);
	virtual ~CRecordDialog();

	bool show(ASoundRecorder *recorder);

	enum
	{
		ID_START_BUTTON=FXModalDialogBox::ID_LAST,
		ID_STOP_BUTTON,
		ID_REDO_BUTTON,

		ID_ADD_CUE_BUTTON,
		ID_ADD_ANCHORED_CUE_BUTTON,

		ID_CLEAR_CLIP_COUNT_BUTTON,

		ID_DURATION_SPINNER,
		ID_START_THRESHOLD_SPINNER,

		ID_STATUS_UPDATE,

		ID_DC_OFFSET_COMPENSATE_BUTTON,

		ID_LAST
	};

	long onStartButton(FXObject *sender,FXSelector sel,void *ptr);
	long onStopButton(FXObject *sender,FXSelector sel,void *ptr);
	long onRedoButton(FXObject *sender,FXSelector sel,void *ptr);

	long onAddCueButton(FXObject *sender,FXSelector sel,void *ptr);

	long onStatusUpdate(FXObject *sender,FXSelector sel,void *ptr);

	long onClearClipCountButton(FXObject *sender,FXSelector sel,void *ptr);

	long onDurationSpinner(FXObject *sender,FXSelector sel,void *ptr);

	long onStartThresholdSpinner(FXObject *sender,FXSelector sel,void *ptr);

	long onDCOffsetCompensateButton(FXObject *sender,FXSelector sel,void *ptr);

protected:
	CRecordDialog() {}

private:

	ASoundRecorder *recorder;
	bool showing;

	FXHorizontalFrame *meterFrame;

	FXLabel *recordedLengthStatusLabel;
	FXLabel *recordedSizeStatusLabel;
	FXLabel *recordLimitLabel;

	FXLabel *DCOffsetLabel;

	FXPacker *recordingLED;
	FXPacker *waitingForThresholdLED;

	FXLabel *clipCountLabel;

	FXCheckButton *setDurationButton;
	FXTextField *durationEdit;
	FXSpinner *durationSpinner;

	FXCheckButton *startThresholdButton;
	FXTextField *startThresholdEdit;
	FXSpinner *startThresholdSpinner;

	FXTextField *cueNamePrefix;
	FXSpinner *cueNameNumber;

	void cleanupMeters();
	void setMeterValue(unsigned channel,float value); // value is 0 to 1
	vector<FXProgressBar *> meters;

	void clearClipCount();

	const sample_pos_t getMaxDuration();
	const double getStartThreshold();

};

#endif
