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

#ifndef __FXFilenameParamValue_H__
#define __FXFilenameParamValue_H__

#include "../../config/common.h"
#include "fox_compat.h"

#include <string>

#include <fox/fx.h>

class CNestedDataFile;

class FXFilenameParamValue : public FXVerticalFrame
{
	FXDECLARE(FXFilenameParamValue);
public:
	FXFilenameParamValue(FXComposite *p,int opts,const char *title,const string filename);
	virtual ~FXFilenameParamValue();

	long onFilenameTextBoxChange(FXObject *sender,FXSelector sel,void *ptr);
	long onBrowseButton(FXObject *sender,FXSelector sel,void *ptr);

	const string getFilename();
	const bool getOpenAsRaw();
	void setFilename(const string filename,bool openAsRaw);

	const string getTitle() const;

	void setTipText(const FXString &text);
	FXString getTipText() const;

	void readFromFile(const string &prefix,CNestedDataFile *f);
	void writeToFile(const string &prefix,CNestedDataFile *f);

	enum
	{
		ID_FILENAME_TEXTBOX=FXVerticalFrame::ID_LAST,
		ID_BROWSE_BUTTON,
		ID_LAST
	};


protected:
	FXFilenameParamValue() {}

private:
	FXHorizontalFrame *hFrame;
		FXLabel *titleLabel;
		FXTextField *filenameTextBox;
		FXButton *browseButton;
	FXCheckButton *openAsRawCheckButton;
};

#endif
