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

#ifndef __FXDiskEntityParamValue_H__
#define __FXDiskEntityParamValue_H__

#include "../../config/common.h"
#include "fox_compat.h"

#include <string>

#include <fox/fx.h>

class CNestedDataFile;

/*
	This widget is added to action dialogs and can be used to select various types 
	of entities which would reside on disk (i.e. selection of an audio filename, 
	directory, or just a general filename)
*/

class FXDiskEntityParamValue : public FXVerticalFrame
{
	FXDECLARE(FXDiskEntityParamValue);

public:
	enum DiskEntityTypes
	{
		detAudioFilename,
		detGeneralFilename,
		detDirectory
	};

	FXDiskEntityParamValue(FXComposite *p,int opts,const char *title,const string initialEntityName,DiskEntityTypes entityType);
	virtual ~FXDiskEntityParamValue();

	long onEntityNameTextBoxChange(FXObject *sender,FXSelector sel,void *ptr);
	long onBrowseButton(FXObject *sender,FXSelector sel,void *ptr);

	const string getEntityName() const;
	const bool getOpenAsRaw() const; // the notion of 'raw' is really only applicable for a type of detAudioFilename and can be ignored for other types
	void setEntityName(const string entityName,bool openAsRaw=false);

	const string getTitle() const;

	void setTipText(const FXString &text);
	FXString getTipText() const;

	const DiskEntityTypes getEntityType() const;

	void readFromFile(const string &prefix,CNestedDataFile *f);
	void writeToFile(const string &prefix,CNestedDataFile *f);

	enum
	{
		ID_ENTITYNAME_TEXTBOX=FXVerticalFrame::ID_LAST,
		ID_BROWSE_BUTTON,
		ID_LAST
	};


protected:
	FXDiskEntityParamValue() {}

private:
	const DiskEntityTypes entityType;

	FXHorizontalFrame *hFrame;
		FXLabel *titleLabel;
		FXTextField *entityNameTextBox;
		FXButton *browseButton;
	FXCheckButton *openAsRawCheckButton;
};

#endif
