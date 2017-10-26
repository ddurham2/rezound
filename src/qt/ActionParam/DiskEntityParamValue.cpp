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

#include "DiskEntityParamValue.h"
#include <QFileDialog>

#include <stdlib.h>

#include <stdexcept>

#include <istring>

#include "../FrontendHooks.h"
#include "../settings.h"

#include <CNestedDataFile/CNestedDataFile.h>
#include "../../backend/ASoundTranslator.h"

//#include "utils.h"

/*
	- This is the text entry widget used over and over by ReZound on action dialogs
	- Its purpose is to select a constant value for a parameter to an action
*/

DiskEntityParamValue::DiskEntityParamValue(const char *_name,const string initialEntityName,DiskEntityTypes _entityType) :
	name(_name),
	
	entityType(_entityType)
{
	setupUi(this);

	if(entityType!=detAudioFilename || !ASoundTranslator::findRawTranslator())
	{ // if not an audio file chooser or there is no raw translator, then remoe the "Open As Raw" checkbox
		delete openAsRawCheckBox;
		openAsRawCheckBox=NULL;
	}

	label->setText(gettext(name.c_str()));
	entityNameTextBox->setText(initialEntityName.c_str());
}

DiskEntityParamValue::~DiskEntityParamValue()
{
}

void DiskEntityParamValue::on_browseButton_clicked()
{
	switch(entityType)
	{
	case detAudioFilename:
		{
			string filename;
			bool dummy=false;
			bool openAsRaw=false;
			if(gFrontendHooks->promptForOpenSoundFilename(filename,dummy,openAsRaw))
				setEntityName(filename,openAsRaw);
		}
		break;

	case detGeneralFilename:
		{
			QString ret=QFileDialog::getOpenFileName(this,_("Select File"));
			if(ret!="")
				setEntityName(ret.toStdString());
		}
		break;

	case detDirectory:
		{
			QString ret=QFileDialog::getExistingDirectory(this,_("Select Directory"), getEntityName().c_str());
			if(ret!="")
				setEntityName(ret.toStdString());
		}
		break;

	default:
		throw runtime_error(string(__func__)+" -- internal error -- unhandled entityType: "+istring(entityType));
	}
}

const string DiskEntityParamValue::getEntityName() const
{
	return decodeFilenamePresetParameter(entityNameTextBox->text().toStdString());
}

const bool DiskEntityParamValue::getOpenAsRaw() const
{
	if(openAsRawCheckBox)
		return openAsRawCheckBox->checkState()==Qt::Checked;
	else
		return false;
}

void DiskEntityParamValue::setEntityName(const string entityName,bool openAsRaw)
{
	entityNameTextBox->setText(entityName.c_str());
	if(openAsRawCheckBox)
		openAsRawCheckBox->setCheckState(openAsRaw ? Qt::Checked : Qt::Unchecked);
}

const string DiskEntityParamValue::getName() const
{
	return name;
}

void DiskEntityParamValue::setTipText(const string &text)
{
	label->setToolTip(text.c_str());	
	entityNameTextBox->setToolTip(text.c_str());
}

string DiskEntityParamValue::getTipText() const
{
	return label->toolTip().toStdString();
}

const DiskEntityParamValue::DiskEntityTypes DiskEntityParamValue::getEntityType() const
{
	return entityType;
}

void DiskEntityParamValue::readFromFile(const string &prefix,CNestedDataFile *f)
{
	const string key=prefix DOT getName();
	const string v=f->getValue<string>(key);
	setEntityName(v,f->getValue<bool>(key+" AsRaw"));
}

void DiskEntityParamValue::writeToFile(const string &prefix,CNestedDataFile *f)
{
	const string key=prefix DOT getName();
	f->setValue<string>(key,encodeFilenamePresetParameter(entityNameTextBox->text().toStdString()));

	if(openAsRawCheckBox)
		f->setValue<bool>(key+" AsRaw",openAsRawCheckBox->checkState()==Qt::Checked);
}


