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

#ifndef __CNewSoundDialog_H__
#define __CNewSoundDialog_H__

#include "../../config/common.h"
#include "fox_compat.h"

#include <string>

class CNewSoundDialog;

#include "FXModalDialogBox.h"

#include "../backend/CSound_defs.h"

class CNewSoundDialog : public FXModalDialogBox
{
	FXDECLARE(CNewSoundDialog);
public:
	CNewSoundDialog(FXWindow *mainWindow);
	virtual ~CNewSoundDialog();

	void show(FXuint placement);

	long onBrowseButton(FXObject *sender,FXSelector sel,void *ptr);

	enum	
	{
		ID_BROWSE_BUTTON=FXModalDialogBox::ID_LAST,
		ID_LAST,
	};

	const string getFilename() const { return filename; }
	const bool getRawFormat() const { return rawFormat; }
	void setFilename(const string f,const bool _rawFormat=false) { filename=f; rawFormat=_rawFormat; }
	const unsigned getChannelCount() const { return channelCount; }
	const unsigned getSampleRate() const { return sampleRate; }
	const sample_pos_t getLength() const { return length; }

	void hideFilename(bool hide);
	void hideSampleRate(bool hide);
	void hideLength(bool hide);

protected:
	CNewSoundDialog() {}

	bool validateOnOkay();

private:
	FXHorizontalFrame *filenameFrame;
		FXLabel *filenameLabel;
		FXTextField *filenameTextBox;
		FXButton *browseButton;
	FXHorizontalFrame *rawFormatFrame;
		FXCheckButton *rawFormatCheckButton;
	FXMatrix *matrix;
		FXLabel *channelsLabel;
		FXComboBox *channelsComboBox;

		FXLabel *sampleRateLabel;
		FXComboBox *sampleRateComboBox;

		FXLabel *lengthLabel;
		FXHorizontalFrame *lengthFrame;
			FXComboBox *lengthComboBox;
			FXLabel *lengthUnitsLabel;

	string filename;
	bool rawFormat;
	unsigned channelCount;
	unsigned sampleRate;
	sample_pos_t length;
};

#endif
