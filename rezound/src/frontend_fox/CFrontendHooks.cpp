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

#include "CFrontendHooks.h"

#include <string>

#include <istring>

#include <cc++/path.h>

#include "settings.h"

#include "CNewSoundDialog.h"
#include "CRecordDialog.h"

#include "../backend/ASoundTranslator.h"

#include <fox/fx.h>


CFrontendHooks::CFrontendHooks(FXWindow *_mainWindow) :
	mainWindow(_mainWindow)
{
}

CFrontendHooks::~CFrontendHooks()
{
}

const string CFrontendHooks::getFOXFileTypes() const
{
	string types;
	string allTypes;

	/*
		From all the registered translators build a string to use as the file type 
		drop-down in the FOX file dialogs:
		   Format Name (*.ext,*.EXT)\nFormat Name (*.ext,*.EXT)\n...
	*/
	for(size_t t=0;t<ASoundTranslator::registeredTranslators.size();t++)
	{
		const vector<string> names=ASoundTranslator::registeredTranslators[t]->getFormatNames();
		const vector<vector<string> > extensions=ASoundTranslator::registeredTranslators[t]->getFormatExtensions();
	
		for(size_t i=0;i<names.size();i++)
		{
			types+=names[i];
			types+=" (";
			for(size_t k=0;k<extensions[i].size();k++)
			{
				if(k>0)
					types+=",";
				types+="*."+extensions[i][k]+",";
				types+="*."+istring(extensions[i][k]).upper();

				if(allTypes!="")
					allTypes+=",";
				allTypes+="*."+extensions[i][k]+",";
				allTypes+="*."+istring(extensions[i][k]).upper();
			}
			types+=")\n";
		}
	}
	
	types="All Supported Types ("+allTypes+")\nAll Files(*)"+types;

	return(types);
}

bool CFrontendHooks::promptForOpen(string &filename,bool &readOnly)
{
	// I can do getOpenFilenames to be able to open multiple files ??? then the parameter would need to be a vector<string>
	FXString _filename=FXFileDialog::getOpenFilename(mainWindow,"Open file",gPromptDialogDirectory.c_str(),getFOXFileTypes().c_str(),0);
	if(_filename!="")
	{
		// save directory to open the opendialog to next time
		gPromptDialogDirectory=ost::Path(_filename.text()).DirName();
		gPromptDialogDirectory.append(&ost::Path::dirDelim,1);

		filename=_filename.text();
		readOnly=false; // can do it.. but I need to invoke the open dialog differently
		return(true);
	}
	return(false);
}

bool CFrontendHooks::promptForSave(string &filename,const string _extension)
{
	istring extension=_extension;
	extension.lower();

	string _filename=FXFileDialog::getSaveFilename(mainWindow,"Save file",filename=="" ? gPromptDialogDirectory.c_str() : filename.c_str(),("Save Type (*."+istring(extension).lower()+",*."+istring(extension).upper()+")\n"+getFOXFileTypes()).c_str(),0).text();
	if(_filename!="")
	{
/*
		// add the extension if the user didn't type it
		if(ost::Path(_filename).Extension()=="")
			_filename.append("."+extension);
*/

		// save directory to open the opendialog to next time
		gPromptDialogDirectory=ost::Path(_filename).DirName();
		gPromptDialogDirectory.append(&ost::Path::dirDelim,1);


		filename=_filename;
		return(true);
	}
	return(false);
}

bool CFrontendHooks::promptForNewSoundParameters(string &filename,unsigned &channelCount,unsigned &sampleRate,sample_pos_t &length)
{
	gNewSoundDialog->hideFilename(false);
	gNewSoundDialog->hideLength(false);
	gNewSoundDialog->setFilename(filename);
	if(gNewSoundDialog->execute(PLACEMENT_CURSOR))	
	{
		filename=gNewSoundDialog->getFilename();
		channelCount=gNewSoundDialog->getChannelCount();
		sampleRate=gNewSoundDialog->getSampleRate();
		length=gNewSoundDialog->getLength();
		return(true);
	}
	return(false);
}

bool CFrontendHooks::promptForNewSoundParameters(string &filename,unsigned &channelCount,unsigned &sampleRate)
{
	gNewSoundDialog->hideFilename(false);
	gNewSoundDialog->hideLength(true);
	gNewSoundDialog->setFilename(filename);
	if(gNewSoundDialog->execute(PLACEMENT_CURSOR))	
	{
		filename=gNewSoundDialog->getFilename();
		channelCount=gNewSoundDialog->getChannelCount();
		sampleRate=gNewSoundDialog->getSampleRate();
		return(true);
	}
	return(false);
}

bool CFrontendHooks::promptForNewSoundParameters(unsigned &channelCount,unsigned &sampleRate)
{
	gNewSoundDialog->hideFilename(true);
	gNewSoundDialog->hideLength(true);
	if(gNewSoundDialog->execute(PLACEMENT_CURSOR))	
	{
		channelCount=gNewSoundDialog->getChannelCount();
		sampleRate=gNewSoundDialog->getSampleRate();
		return(true);
	}
	return(false);
}

bool CFrontendHooks::promptForRecord(ASoundRecorder *recorder)
{
	if(gRecordDialog->show(recorder))
		return(true);
	return(false);
}

