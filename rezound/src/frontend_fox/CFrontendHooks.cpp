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
#include "CRawDialog.h"
#include "COggDialog.h"
#include "CMp3Dialog.h"
#include "CVoxDialog.h"

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
	delete mp3Dialog;
	delete voxDialog;
}

void CFrontendHooks::doSetupAfterBackendIsSetup()
{
	openDialog=new FXFileDialog(mainWindow,"Open File");
	openDialog->setPatternList(getFOXFileTypes().c_str());
	openDialog->setCurrentPattern(0);
	openDialog->showReadOnly(false); // would be true if I supported it
	openDialog->setReadOnly(false);
	{ // add the "Open as Raw" check button
		FXVerticalFrame *f=new FXVerticalFrame(openDialog,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0, 0,0,0,0, 0,0);
		openDialog->childAtIndex(0)->reparent(f);
		openAsRawCheckButton=new FXCheckButton(new FXPacker(f,0, 0,0,0,0, DEFAULT_SPACING*2,0,0),"Open as Raw",NULL,0,CHECKBUTTON_NORMAL);
	}
	if(openDialog->getDirectory()!=gPromptDialogDirectory.c_str())
		openDialog->setDirectory(gPromptDialogDirectory.c_str());

	saveDialog=new FXFileDialog(mainWindow,"Save File");
	saveDialog->setSelectMode(SELECTFILE_ANY);
	saveDialog->setPatternList(getFOXFileTypes().c_str());
	saveDialog->setCurrentPattern(0);
	saveDialog->setDirectory(gPromptDialogDirectory.c_str());
	{ // add the "Save as Raw" check button
		FXVerticalFrame *f=new FXVerticalFrame(saveDialog,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0, 0,0,0,0, 0,0);
		saveDialog->childAtIndex(0)->reparent(f);
		saveAsRawCheckButton=new FXCheckButton(new FXPacker(f,0, 0,0,0,0, DEFAULT_SPACING*2,0,0),"Save as Raw",NULL,0,CHECKBUTTON_NORMAL);
	}

	newSoundDialog=new CNewSoundDialog(mainWindow);
	recordDialog=new CRecordDialog(mainWindow);
	rawDialog=new CRawDialog(mainWindow);
	oggDialog=new COggDialog(mainWindow);
	mp3Dialog=new CMp3Dialog(mainWindow);
	voxDialog=new CVoxDialog(mainWindow);
	
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

bool CFrontendHooks::promptForOpenSoundFilename(string &filename,bool &readOnly,bool &openAsRaw)
{
	openDialog->setFilename("");
	if(openDialog->getDirectory()!=gPromptDialogDirectory.c_str())
		openDialog->setDirectory(gPromptDialogDirectory.c_str());
	openDialog->setSelectMode(SELECTFILE_EXISTING);
	openAsRawCheckButton->setCheck(openAsRaw);
	if(openDialog->execute())
	{
		// save directory to open the opendialog to next time
		gPromptDialogDirectory=openDialog->getDirectory().text();

		filename=openDialog->getFilename().text();
		readOnly=openDialog->getReadOnly();
		openAsRaw=openAsRawCheckButton->getCheck();

		return(true);
	}
	return(false);
}

bool CFrontendHooks::promptForOpenSoundFilenames(vector<string> &filenames,bool &readOnly,bool &openAsRaw)
{
	openDialog->setFilename("");
	if(openDialog->getDirectory()!=gPromptDialogDirectory.c_str())
		openDialog->setDirectory(gPromptDialogDirectory.c_str());
	openDialog->setSelectMode(SELECTFILE_MULTIPLE);
	openAsRawCheckButton->setCheck(openAsRaw);
	if(openDialog->execute())
	{
		// save directory to open the opendialog to next time
		gPromptDialogDirectory=openDialog->getDirectory().text();

		// add the selected filenames to the filenames vector
		FXString *_filenames=openDialog->getFilenames();
		while(_filenames!=NULL && (*_filenames)!="")
		{
			filenames.push_back(_filenames->text());
			_filenames++;
		}
		readOnly=openDialog->getReadOnly();
		openAsRaw=openAsRawCheckButton->getCheck();

		return(true);
	}
	return(false);
}

bool CFrontendHooks::promptForSaveSoundFilename(string &filename,bool &saveAsRaw)
{
	if(filename!="")
	{
		saveDialog->setFilename(CPath(filename).baseName().c_str());
		saveDialog->setDirectory(gPromptDialogDirectory.c_str());
	}
	saveAsRawCheckButton->setCheck(saveAsRaw);

	if(saveDialog->execute())
	{
		gPromptDialogDirectory=saveDialog->getDirectory().text();

		filename=saveDialog->getFilename().text();
		saveAsRaw=saveAsRawCheckButton->getCheck();

		return(true);
	}
	return(false);
}

bool CFrontendHooks::promptForNewSoundParameters(string &filename,bool &rawFormat,unsigned &channelCount,unsigned &sampleRate,sample_pos_t &length)
{
	newSoundDialog->hideFilename(false);
	newSoundDialog->hideLength(false);
	newSoundDialog->setFilename(filename,rawFormat);
	if(newSoundDialog->execute(PLACEMENT_CURSOR))	
	{
		filename=newSoundDialog->getFilename();
		rawFormat=newSoundDialog->getRawFormat();
		channelCount=newSoundDialog->getChannelCount();
		sampleRate=newSoundDialog->getSampleRate();
		length=newSoundDialog->getLength();
		return(true);
	}
	return(false);
}

bool CFrontendHooks::promptForNewSoundParameters(string &filename,bool &rawFormat,unsigned &channelCount,unsigned &sampleRate)
{
	newSoundDialog->hideFilename(false);
	newSoundDialog->hideLength(true);
	newSoundDialog->setFilename(filename,rawFormat);
	if(newSoundDialog->execute(PLACEMENT_CURSOR))	
	{
		filename=newSoundDialog->getFilename();
		rawFormat=newSoundDialog->getRawFormat();
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

bool CFrontendHooks::promptForRawParameters(RawParameters &parameters)
{
	return(rawDialog->show(parameters));
}

bool CFrontendHooks::promptForOggCompressionParameters(OggCompressionParameters &parameters)
{
	return(oggDialog->show(parameters));
}

bool CFrontendHooks::promptForMp3CompressionParameters(Mp3CompressionParameters &parameters)
{
	return(mp3Dialog->show(parameters));
}

bool CFrontendHooks::promptForVoxParameters(VoxParameters &parameters)
{
	return(voxDialog->show(parameters));
}

