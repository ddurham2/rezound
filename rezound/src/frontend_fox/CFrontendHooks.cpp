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

#include <fox/fx.h>


CFrontendHooks::CFrontendHooks(FXWindow *_mainWindow) :
	mainWindow(_mainWindow)
{
}

CFrontendHooks::~CFrontendHooks()
{
}

bool CFrontendHooks::promptForOpen(string &filename,bool &readOnly)
{
	// I can do getOpenFilenames to be able to open multiple files ??? then the parameter would need to be a vector<string>
																// this list could be build programatically from all the registered ASoundTranslator derivations
	FXString _filename=FXFileDialog::getOpenFilename(mainWindow,"Open file",gPromptDialogDirectory.c_str(),"Supported Files (*.wav,*.WAV,*.rez,*.aiff,*.AIFF,*.au,*.AU,*.snd,*.SND,*.sf,*.SF,*.raw)\nAll Files(*)",0);
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

	string _filename=FXFileDialog::getSaveFilename(mainWindow,"Save file",filename=="" ? gPromptDialogDirectory.c_str() : filename.c_str(),("Save Type (*."+istring(extension).lower()+",*."+istring(extension).upper()+")\nAll Files(*)").c_str(),0).text();
	if(_filename!="")
	{
		// add the extension if the user didn't type it
		if(ost::Path(_filename).Extension()=="")
			_filename.append("."+extension);

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

