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

#include "CSound.h"

#include "AStatusComm.h"
#include "settings.h"

#include <stdexcept>
#include <cc++/path.h>

#include <CNestedDataFile/CNestedDataFile.h>

#include "ASoundPlayer.h"
#include "CSoundPlayerChannel.h"
#include "ASoundRecorder.h"
#include "ASoundTranslator.h"

#define MAX_REOPEN_HISTORY 16 // needs to be a preference ???

ASoundFileManager *gSoundFileManager=NULL;


ASoundFileManager::ASoundFileManager(ASoundPlayer *_soundPlayer,CNestedDataFile *_loadedRegistryFile) :
	soundPlayer(_soundPlayer),
	loadedRegistryFile(_loadedRegistryFile)
{
}

void ASoundFileManager::createNew()
{
	prvCreateNew(true);
}

CLoadedSound *ASoundFileManager::prvCreateNew(bool askForLength)
{
	CSound *sound=NULL;
	CSoundPlayerChannel *channel=NULL;
	CLoadedSound *loaded=NULL;

	string filename;
	unsigned channelCount;
	unsigned sampleRate;
	sample_pos_t length=1;  // 1 if askForLength is false
	if(
		(askForLength && promptForNewSoundParameters(filename,channelCount,sampleRate,length)) ||
		(!askForLength && promptForNewSoundParameters(filename,channelCount,sampleRate))
	)
	{
		if(isFilenameRegistered(filename))
			throw(runtime_error(string(__func__)+" -- a file named '"+filename+"' is already opened"));

		// should get based on extension
		const ASoundTranslator *translator=getTranslator(filename,false/*isRaw*/);

		try
		{
				sound=new CSound(filename,sampleRate,channelCount,length);
				channel=soundPlayer->newSoundPlayerChannel(sound);
				loaded=new CLoadedSound(filename,channel,false,translator);

				createWindow(loaded);
		}
		catch(...)
		{
			if(loaded!=NULL)
				destroyWindow(loaded);
			delete loaded;
			delete channel;
			delete sound;
			throw;
		}

		registerFilename(filename);

		return(loaded);
	}

	return(NULL);
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

/*
 * I believe the only time we wouldn't want a file to be registered, doRegisterFilename==false is 
 * when we are loading the registered files from a previous sessions, so they would already be
 * in the registry
 */
void ASoundFileManager::prvOpen(const string &filename,bool readOnly,bool doRegisterFilename,const ASoundTranslator *translatorToUse)
{
	if(doRegisterFilename && isFilenameRegistered(filename))
		throw(runtime_error(string(__func__)+" -- file already opened"));

	CSound *sound=NULL;
	CSoundPlayerChannel *channel=NULL;
	CLoadedSound *loaded=NULL;

	try
	{
		if(translatorToUse==NULL)
			translatorToUse=getTranslator(filename,/*isRaw*/false);
		sound=new CSound;
		translatorToUse->loadSound(filename,sound);
		channel=soundPlayer->newSoundPlayerChannel(sound);
		loaded=new CLoadedSound(filename,channel,readOnly,translatorToUse);

		createWindow(loaded);
	}
	catch(...)
	{
		if(loaded!=NULL)
			destroyWindow(loaded);
		delete loaded;
		delete channel;
		delete sound;
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
		string filename=loaded->getFilename();
		if(filename=="" || loaded->translator==NULL)
			throw(runtime_error(string(__func__)+" -- filename is not set or translator is NULL -- how did this happen? -- I shouldn't have this problem since even a new sound has to be given a filename"));
		
		loaded->translator->saveSound(filename,loaded->getSound());
		loaded->getSound()->setIsModified(false);
		updateAfterEdit();
		updateReopenHistory(filename);
	}
}

void ASoundFileManager::saveAs()
{
	CLoadedSound *loaded=getActive();
	if(loaded)
	{
		string filename=loaded->getFilename();
		if(!promptForSave(filename,ost::Path(loaded->getFilename()).Extension()))
			return;

		if(loaded->getFilename()==filename)
		{ // the user chose the same name
			save();
			return;
		}

		if(isFilenameRegistered(filename))
			throw(runtime_error(string(__func__)+" -- file is currently opened: '"+filename+"'"));

		if(ost::Path(filename).Exists())
		{
			if(Question("Overwrite Existing File:\n"+filename,yesnoQues)!=yesAns)
				return;
		}

		const ASoundTranslator *translator=getTranslator(filename,/*isRaw*/false);

		translator->saveSound(filename,loaded->getSound());
		loaded->translator=translator; // make save use this translator next time

		unregisterFilename(loaded->getFilename());
		loaded->changeFilename(filename);
		registerFilename(filename);

		loaded->getSound()->setIsModified(false);
		updateAfterEdit();
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
				VAnswer a=Question("Save Modified Sound:\n"+loaded->getFilename(),cancelQues);
				if(a==cancelAns)
					throw EStopClosing();
				doSave=(a==yesAns);
			}
			else if(closeType==ctSaveYesNoCancel)
			{
				VAnswer a=Question("Save Modified Sound:\n"+loaded->getFilename(),cancelQues);
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

		// save before we start deconstructing everything so that if there
		// is an error saving, the user hasn't lost all when it closes below
		if(doSave)
			save();

		unregisterFilename(loaded->getFilename());

		destroyWindow(loaded);

		loaded->getSound()->closeSound();
		delete loaded; // also deletes channel
	}
}

void ASoundFileManager::revert()
{
	CLoadedSound *loaded=getActive();
	// NOTE: can't revert if the sound has never been saved 
	if(loaded)
	{
		const ASoundTranslator *translatorToUse=loaded->translator;
		const bool readOnly=loaded->isReadOnly();
		string filename=loaded->getFilename();

		if(!ost::Path(filename).Exists())
		{
			gStatusComm->beep();
			return;
		}

		// ??? could check isMofied(), but if it isn't, then there's no need to revert...
		if(Question("Are you sure you want to revert to the last saved copy of '"+filename+"'",yesnoQues)!=yesAns)
			return;

		// could be more effecient by not destroying then creating the sound window
		close(ctSaveNone); // ??? I need a way to know not to defrag since we're not attempting to save any results... altho.. I may never want to defrag.. perhaps on open... 
		prvOpen(filename,readOnly,true,translatorToUse);
		// should I remember the selection positions and use them if they're valid? ???
	}
}

#include <COSSSoundRecorder.h> // ??? when porting we need to somehow choose which implementation to instantiate and use

void ASoundFileManager::recordToNew()
{
	CLoadedSound *loaded=prvCreateNew(false);
	if(loaded==NULL)
		return; // cancelled

	// need to somehow choose an implementation ???
	COSSSoundRecorder recorder;
	try
	{
		recorder.initialize(loaded->getSound());
	}
	catch(...)
	{
		close(ctSaveNone,loaded);
		recorder.deinitialize();
		throw;
	}

	bool ret=true;
	try
	{
		ret=promptForRecord(&recorder);
		recorder.deinitialize();
	}
	catch(...)
	{
		recorder.deinitialize();
		throw;
	}

	if(!ret)
		close(ctSaveNone,loaded); // record dialog was cancelled, don't ask to save when closing
	else
	{
		// set some kind of initial selection
		loaded->channel->setStopPosition(loaded->getSound()->getLength()/2+loaded->getSound()->getLength()/4);
		loaded->channel->setStartPosition(loaded->getSound()->getLength()/2-loaded->getSound()->getLength()/4);

		updateAfterEdit();
	}
}


const string ASoundFileManager::getUntitledFilename(const string directory,const string extension)
{
	// return "/dir/untitled#.ext" where # is 0-99, then 'newfile#', then 'default#'
	static const char *prefixes[]={"untitled","newfile","default"};
	for(size_t i=0;i<(sizeof(prefixes)/sizeof(*prefixes));i++)
	{
		const string filename=directory+prefixes[i];
		for(size_t t=0;t<100;t++)
		{
			const string temp=filename+istring(t)+"."+extension;
			if(!ost::Path(temp).Exists() && !isFilenameRegistered(temp))
				return(filename+istring(t)+"."+extension);
		}
	}
	return("");
}


#define LOADED_REG_KEY "loaded"

const vector<string> ASoundFileManager::loadFilesInRegistry()
{
	vector<string> errors;
	for(size_t t=0;t<loadedRegistryFile->getArraySize(LOADED_REG_KEY);t++)
	{
		string filename;
		try
		{
			filename=loadedRegistryFile->getArrayValue(LOADED_REG_KEY,t);
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

	loadedRegistryFile->createArrayKey(LOADED_REG_KEY,loadedRegistryFile->getArraySize(LOADED_REG_KEY),filename);
}

void ASoundFileManager::unregisterFilename(const string filename)
{
	size_t l=loadedRegistryFile->getArraySize(LOADED_REG_KEY);
	for(size_t t=0;t<l;t++)
	{
		if(loadedRegistryFile->getArrayValue(LOADED_REG_KEY,t)==filename)
		{
			loadedRegistryFile->removeArrayKey(LOADED_REG_KEY,t);
			break;
		}
	}
}

bool ASoundFileManager::isFilenameRegistered(const string filename)
{
	size_t l=loadedRegistryFile->getArraySize(LOADED_REG_KEY);
	for(size_t t=0;t<l;t++)
	{
		if(loadedRegistryFile->getArrayValue(LOADED_REG_KEY,t)==filename)
			return(true);
	}
	return(false);
}

void ASoundFileManager::updateReopenHistory(const string &filename)
{
	// rewrite the reopen history to the gSettingsRegistry
	vector<string> reopenFilenames;
	
	for(size_t t=0;gSettingsRegistry->keyExists(("ReopenHistory.item"+istring(t)).c_str());t++)
	{
		const string h=gSettingsRegistry->getValue(("ReopenHistory.item"+istring(t)).c_str());
		if(h!=filename)
			reopenFilenames.push_back(h);
	}

	if(reopenFilenames.size()>=MAX_REOPEN_HISTORY)
		reopenFilenames.erase(reopenFilenames.begin()+reopenFilenames.size()-1);
		
	reopenFilenames.insert(reopenFilenames.begin(),filename);

	for(size_t t=0;t<reopenFilenames.size();t++)
		gSettingsRegistry->createKey(("ReopenHistory.item"+istring(t)).c_str(),reopenFilenames[t]);
}

const size_t ASoundFileManager::getReopenHistorySize() const
{
	size_t t;
	for(t=0;gSettingsRegistry->keyExists(("ReopenHistory.item"+istring(t)).c_str());t++);

	return(t);
}

const string ASoundFileManager::getReopenHistoryItem(const size_t index) const
{
	const string key="ReopenHistory.item"+istring(index);
	if(gSettingsRegistry->keyExists(key.c_str()))
		return(gSettingsRegistry->getValue(key.c_str()));
	else
		return("");
}


/*
	This method could be given some abstract stream class pointer instead 
	of a filename which could access a file or a network URL.   Then the 
	translators would also have to be changed to read from that stream 
	instead of the file, and libaudiofile would at this point in time have 
	trouble doing that.
*/
const ASoundTranslator *ASoundFileManager::getTranslator(const string filename,bool isRaw)
{
	if(isRaw)
	{
		for(size_t t=0;t<ASoundTranslator::registeredTranslators.size();t++)
		{
			if(ASoundTranslator::registeredTranslators[t]->handlesRaw())
				return(ASoundTranslator::registeredTranslators[t]);
		}
	}

	if(ost::Path(filename).Exists())
	{ // try to determine from the contents of the file
		for(size_t t=0;t<ASoundTranslator::registeredTranslators.size();t++)
		{
			if(ASoundTranslator::registeredTranslators[t]->supportsFormat(filename))
				return(ASoundTranslator::registeredTranslators[t]);
		}
	}

	// file doesn't exist or no supported signature, so attempt to determine the translater based on the file extension
	const string extension=istring(ost::Path(filename).Extension()).lower();
	if(extension=="")
		throw(runtime_error(string(__func__)+" -- cannot determine the extension on the filename: "+filename));
	else
	{
		for(size_t t=0;t<ASoundTranslator::registeredTranslators.size();t++)
		{
			if(ASoundTranslator::registeredTranslators[t]->handlesExtension(extension))
				return(ASoundTranslator::registeredTranslators[t]);
		}
	}

	throw(runtime_error(string(__func__)+" -- unhandled format/extension for the filename '"+filename+"'"));
}


