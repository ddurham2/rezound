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
#include <CPath.h>

#include "settings.h"

#include "CNewSoundDialog.h"
#include "CRecordDialog.h"
#include "COggDialog.h"

#include "../backend/ASoundTranslator.h"

#include <fox/fx.h>


CFrontendHooks::CFrontendHooks(FXWindow *_mainWindow) :
	mainWindow(_mainWindow),

	openDialog(NULL),
	saveDialog(NULL),

	newSoundDialog(NULL),
	recordDialog(NULL),
	oggDialog(NULL)
{
	dirDialog=new FXDirDialog(mainWindow,"Select Directory");
}

CFrontendHooks::~CFrontendHooks()
{
	delete openDialog;
	delete saveDialog;

	delete newSoundDialog;
	delete recordDialog;
	delete oggDialog;
}

void CFrontendHooks::doSetupAfterBackendIsSetup()
{
	openDialog=new FXFileDialog(mainWindow,"Open File");
	openDialog->setSelectMode(SELECTFILE_EXISTING);
	openDialog->setPatternList(getFOXFileTypes().c_str());
	openDialog->setCurrentPattern(0);
	openDialog->setDirectory(gPromptDialogDirectory.c_str());

	saveDialog=new FXFileDialog(mainWindow,"Save File");
	saveDialog->setSelectMode(SELECTFILE_ANY);
	saveDialog->setPatternList(getFOXFileTypes().c_str());
	saveDialog->setCurrentPattern(0);
	saveDialog->setDirectory(gPromptDialogDirectory.c_str());

	newSoundDialog=new CNewSoundDialog(mainWindow);
	recordDialog=new CRecordDialog(mainWindow);
	oggDialog=new COggDialog(mainWindow);
	
}

const string CFrontendHooks::getFOXFileTypes() const
{
	string types;
	string allTypes;

	/*
		From all the registered translators build a string to use as the file type 
		drop-down in the FOX file dialogs:
		   Format Name (*.ext,*.EXT)\nFormat Name (*.ext,*.EXT)\n...
		And built a list for "All Types" with the *.ext,*.ext,...,*.EXT,*.EXT
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
				allTypes+="*."+extensions[i][k];
			}
			types+=")\n";
		}
	}

	for(size_t t=0;t<ASoundTranslator::registeredTranslators.size();t++)
	{
		const vector<string> names=ASoundTranslator::registeredTranslators[t]->getFormatNames();
		const vector<vector<string> > extensions=ASoundTranslator::registeredTranslators[t]->getFormatExtensions();
	
		for(size_t i=0;i<names.size();i++)
		{
			for(size_t k=0;k<extensions[i].size();k++)
			{
				if(allTypes!="")
					allTypes+=",";
				allTypes+="*."+istring(extensions[i][k]).upper();
			}
		}
	}
	
	types="All Supported Types ("+allTypes+")\n"+types+"All Files(*)\n";

	return(types);
}

bool CFrontendHooks::promptForOpenSoundFilename(string &filename,bool &readOnly)
{
	openDialog->setDirectory(gPromptDialogDirectory.c_str());
	if(openDialog->execute())
	{
		// save directory to open the opendialog to next time
		gPromptDialogDirectory=openDialog->getDirectory().text();

		filename=openDialog->getFilename().text();
		readOnly=false;

		return(true);
	}
	return(false);
}

bool CFrontendHooks::promptForSaveSoundFilename(string &filename)
{
	if(filename!="")
	{
		saveDialog->setFilename(CPath(filename).baseName().c_str());
		saveDialog->setDirectory(gPromptDialogDirectory.c_str());
	}

	if(saveDialog->execute())
	{
		gPromptDialogDirectory=saveDialog->getDirectory().text();

		filename=saveDialog->getFilename().text();
		return(true);
	}
	return(false);
}

bool CFrontendHooks::promptForNewSoundParameters(string &filename,unsigned &channelCount,unsigned &sampleRate,sample_pos_t &length)
{
	newSoundDialog->hideFilename(false);
	newSoundDialog->hideLength(false);
	newSoundDialog->setFilename(filename);
	if(newSoundDialog->execute(PLACEMENT_CURSOR))	
	{
		filename=newSoundDialog->getFilename();
		channelCount=newSoundDialog->getChannelCount();
		sampleRate=newSoundDialog->getSampleRate();
		length=newSoundDialog->getLength();
		return(true);
	}
	return(false);
}

bool CFrontendHooks::promptForNewSoundParameters(string &filename,unsigned &channelCount,unsigned &sampleRate)
{
	newSoundDialog->hideFilename(false);
	newSoundDialog->hideLength(true);
	newSoundDialog->setFilename(filename);
	if(newSoundDialog->execute(PLACEMENT_CURSOR))	
	{
		filename=newSoundDialog->getFilename();
		channelCount=newSoundDialog->getChannelCount();
		sampleRate=newSoundDialog->getSampleRate();
		return(true);
	}
	return(false);
}

bool CFrontendHooks::promptForNewSoundParameters(unsigned &channelCount,unsigned &sampleRate)
{
	newSoundDialog->hideFilename(true);
	newSoundDialog->hideLength(true);
	if(newSoundDialog->execute(PLACEMENT_CURSOR))	
	{
		channelCount=newSoundDialog->getChannelCount();
		sampleRate=newSoundDialog->getSampleRate();
		return(true);
	}
	return(false);
}

bool CFrontendHooks::promptForDirectory(string &dirname,const string title)
{
	dirDialog->setTitle(title.c_str());
	if(dirDialog->execute())
	{
		dirname=dirDialog->getDirectory().text();
		return(true);
	}
	return(false);
}

bool CFrontendHooks::promptForRecord(ASoundRecorder *recorder)
{
	if(recordDialog->show(recorder))
		return(true);
	return(false);
}

bool CFrontendHooks::promptForOggCompressionParameters(OggCompressionParameters &parameters)
{
	return(oggDialog->show(parameters));
}

