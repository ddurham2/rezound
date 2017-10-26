/* 
 * Copyright (C) 2007 - David W. Durham
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

#ifndef __DiskEntityParamValue_H__
#define __DiskEntityParamValue_H__

#include "../../../config/common.h"
#include "../qt_compat.h"

#include "ui_DiskEntityParamValue.h"

#include <string>

class CNestedDataFile;

/*
	This widget is added to action dialogs and can be used to select various types 
	of entities which would reside on disk (i.e. selection of an audio filename, 
	directory, or just a general filename)
*/

class DiskEntityParamValue : public QWidget, public Ui::DiskEntityParamValue
{
	Q_OBJECT
public:
	enum DiskEntityTypes
	{
		detAudioFilename,
		detGeneralFilename,
		detDirectory
	};

    DiskEntityParamValue(const char *name,const string initialEntityName,DiskEntityTypes entityType);
    virtual ~DiskEntityParamValue();

	const string getEntityName() const;
	const bool getOpenAsRaw() const; // the notion of 'raw' is really only applicable for a type of detAudioFilename and can be ignored for other types
	void setEntityName(const string entityName,bool openAsRaw=false);

	const string getName() const;

	void setTipText(const string &text);
	string getTipText() const;

	const DiskEntityTypes getEntityType() const;

	void readFromFile(const string &prefix,CNestedDataFile *f);
	void writeToFile(const string &prefix,CNestedDataFile *f);

private Q_SLOTS:
	void on_browseButton_clicked();

private:
	const string name;

	const DiskEntityTypes entityType;
};

#endif
