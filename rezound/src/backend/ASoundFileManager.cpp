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

#include "ASoundFileManager.h"

#include "AStatusComm.h"

#include <stdexcept>

#include <cc++/path.h>

#include <CNestedDataFile/CNestedDataFile.h>

ASoundFileManager::ASoundFileManager(CSoundManager &_soundManager,ASoundPlayer &_soundPlayer,CNestedDataFile &_loadedRegistryFile) :
	soundManager(_soundManager),
	soundPlayer(_soundPlayer),
	loadedRegistryFile(_loadedRegistryFile)
{
}

void ASoundFileManager::createNew()
{
	CSoundManagerClient *client=NULL;
	CSoundPlayerChannel *channel=NULL;
	CLoadedSound *loaded=NULL;

	string filename;
	unsigned channelCount;
	unsigned sampleRate;
	sample_pos_t length;
	if(promptForNewSoundParameters(filename,channelCount,sampleRate,length))
	{
		try
		{
				client=new CSoundManagerClient(soundManager.newSound(filename,sampleRate,channelCount,length));
				channel=soundPlayer.newSoundPlayerChannel(client->sound);
				loaded=new CLoadedSound(client,channel);

				createWindow(loaded);
		}
		catch(...)
		{
			delete loaded;
			delete channel;
			delete client;
			if(loaded!=NULL)
				destroyWindow(loaded);
			throw;
		}

		registerFilename(filename);
	}
}

void ASoundFileManager::open(const string _filename)
{
	string filename=_filename;
	bool readOnly=false;
	if(filename=="")
	{
		if(!promptForOpen(filename,readOnly))
			return;
	}

	prvOpen(filename,readOnly,true);
	updateReopenHistory(filename);
}

void ASoundFileManager::prvOpen(const string &filename,bool readOnly,bool doRegisterFilename)
{
	if(doRegisterFilename && isFilenameRegistered(filename))
		throw(runtime_error(string(__func__)+" -- file already opened"));

	CSoundManagerClient *client=NULL;
	CSoundPlayerChannel *channel=NULL;
	CLoadedSound *loaded=NULL;

	try
	{
		client=new CSoundManagerClient(soundManager.openSound(filename,readOnly));
		channel=soundPlayer.newSoundPlayerChannel(client->sound);
		loaded=new CLoadedSound(client,channel);

		createWindow(loaded);

	}
	catch(...)
	{
		delete loaded;
		delete channel;
		delete client;
		if(loaded!=NULL)
			destroyWindow(loaded);
		throw;
	}

	if(doRegisterFilename)
		registerFilename(filename);

}

void ASoundFileManager::save()
{
	// get active sound
	// if it has no filename, then call save As and be done
	// else save it

	CLoadedSound *loaded=getActive();
	if(loaded)
	{
		string filename=loaded->getSound()->getFilename();
		if(filename=="")
			throw(runtime_error(string(__func__)+" -- filename is not set -- how did this happen? -- I shouldn't have this problem since even a new sound has to be given a filename"));
/*
		{
			if(!promptForSave(filename,filename.substr(filename.rfind(".")+1)))
				return;
			
			if(ost::Path(filename).Exists())
			{
				if(Question("Overwrite Existing File:\n"+filename,yesnoQues)!=yesAns)
					return;
			}
			
		}
*/
		
		loaded->getSound()->saveSound(filename);
		loaded->getSound()->setIsModified(false);
		redrawActive();
		updateReopenHistory(filename);
	}
}

void ASoundFileManager::saveAs()
{
	CLoadedSound *loaded=getActive();
	if(loaded)
	{
		string filename=loaded->getSound()->getFilename();
		//if(!promptForSave(filename,loaded->getSound()->getFilename().substr(loaded->getSound()->getFilename().rfind(".")+1)))
		if(!promptForSave(filename,ost::Path(loaded->getSound()->getFilename()).Extension()))
			return;

		if(ost::Path(filename).Exists())
		{
			if(Question("Overwrite Existing File:\n"+filename,yesnoQues)!=yesAns)
				return;
		}
		
		loaded->getSound()->saveSound(filename);

		unregisterFilename(loaded->client->sound->getFilename());
		loaded->getSound()->changeFilename(filename);
		registerFilename(loaded->client->sound->getFilename());

		loaded->getSound()->setIsModified(false);
		redrawActive();
		updateReopenHistory(filename);
	}
}

void ASoundFileManager::close(CloseTypes closeType,CLoadedSound *closeWhichSound)
{
	CLoadedSound *loaded=closeWhichSound==NULL ? getActive() : closeWhichSound;
	if(loaded)
	{
		bool doSave=false;
		if(loaded->getSound()->isModified())
		{
			if(closeType==ctSaveYesNoStop)
			{
				VAnswer a=Question("Save Modified Sound:\n"+loaded->getSound()->getFilename(),cancelQues);
				if(a==cancelAns)
					throw EStopClosing();
				doSave=(a==yesAns);
			}
			else if(closeType==ctSaveYesNoCancel)
			{
				VAnswer a=Question("Save Modified Sound:\n"+loaded->getSound()->getFilename(),cancelQues);
				if(a==cancelAns)
					return;
				doSave=(a==yesAns);
			}
			// else closeType==ctSaveNone  (no question to ask; just don't save)
		}

		loaded->getSound()->lockForResize();
		try
		{
			loaded->channel->kill(); // stop any playing
			loaded->getSound()->unlockForResize();
		}
		catch(...)
		{
			loaded->getSound()->unlockForResize();
			// perhaps don't worry about it???
		}
		unregisterFilename(loaded->client->sound->getFilename());

		if(doSave)
		{
			try
			{
				save();
			}
			catch(exception &e)
			{
				Error(e.what());
			}
		}

		destroyWindow(loaded);
		soundManager.closeSound(*(loaded->client));
		delete loaded;
	}
}

void ASoundFileManager::revert()
{
	CLoadedSound *loaded=getActive();
	// NOTE: can't revert if the sound has never been saved 
	if(loaded)
	{
		const bool readOnly=loaded->client->isReadOnly();
		string filename=loaded->getSound()->getFilename();

		// ??? could check isMofied(), but if it isn't, then there's no need to revert...
		if(Question("Are you sure you want to revert to the last saved copy of '"+filename+"'",yesnoQues)!=yesAns)
			return;

		// could be more effecient by not destroying then creating the sound window
		close(ctSaveNone); // ??? I need a way to know not to defrag since we're not attempting to save any results... altho.. I may never want to defrag.. perhaps on open... 
		prvOpen(filename,readOnly,true);
		// should I remember the selection positions and use them if they're valid? ???
	}
}





#define LOADED_REG_KEY "loaded"

const vector<string> ASoundFileManager::loadFilesInRegistry()
{
	vector<string> errors;
	for(size_t t=0;t<loadedRegistryFile.getArraySize(LOADED_REG_KEY);t++)
	{
		string filename;
		try
		{
			filename=loadedRegistryFile.getArrayValue(LOADED_REG_KEY,t);
			printf("--- reopening file: %s\n",filename.c_str()); // ??? should be dprintf

			if(Question("Load sound from previous session?\n   "+filename,yesnoQues)==yesAns)
			{
						// ??? readOnly (really needs to be whatever the last value was, when it was originally loaded)
				prvOpen(filename,false,false);
			}
			else
			{
				unregisterFilename(filename);
				t--;
			}

		}
		catch(exception &e)
		{	
			if(filename!="")
			{
				unregisterFilename(filename);
				t--;
			}
			errors.push_back(e.what());
		}
	}
	return(errors);
}

void ASoundFileManager::registerFilename(const string filename)
{
	if(isFilenameRegistered(filename))
		throw(runtime_error(string(__func__)+" -- file already registered"));

	loadedRegistryFile.createArrayKey(LOADED_REG_KEY,loadedRegistryFile.getArraySize(LOADED_REG_KEY),filename);
}

void ASoundFileManager::unregisterFilename(const string filename)
{
	size_t l=loadedRegistryFile.getArraySize(LOADED_REG_KEY);
	for(size_t t=0;t<l;t++)
	{
		if(loadedRegistryFile.getArrayValue(LOADED_REG_KEY,t)==filename);
		{
			loadedRegistryFile.removeArrayKey(LOADED_REG_KEY,t);
			break;
		}
	}
}

bool ASoundFileManager::isFilenameRegistered(const string filename)
{
	size_t l=loadedRegistryFile.getArraySize(LOADED_REG_KEY);
	for(size_t t=0;t<l;t++)
	{
		if(loadedRegistryFile.getArrayValue(LOADED_REG_KEY,t)==filename)
			return(true);
	}
	return(false);
}

