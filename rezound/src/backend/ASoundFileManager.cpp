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
#include <CPath.h>

#include <CNestedDataFile/CNestedDataFile.h>

#include "CLoadedSound.h"
#include "ASoundPlayer.h"
#include "CSoundPlayerChannel.h"
#include "ASoundTranslator.h"
#include "AFrontendHooks.h"

#include "ASoundRecorder.h"

ASoundFileManager::ASoundFileManager(ASoundPlayer *_soundPlayer,CNestedDataFile *_loadedRegistryFile) :
	soundPlayer(_soundPlayer),
	loadedRegistryFile(_loadedRegistryFile)
{
}

void ASoundFileManager::createNew()
{
	prvCreateNew(0,true,0,true);
}

CLoadedSound *ASoundFileManager::prvCreateNew(sample_pos_t _length,bool askForLength,unsigned _sampleRate,bool askForSampleRate)
{
	string filename=getUntitledFilename(gPromptDialogDirectory,"rez");
	bool rawFormat=false;
	unsigned channelCount;
	unsigned sampleRate=_sampleRate;
	sample_pos_t length=_length;
	if(gFrontendHooks->promptForNewSoundParameters(filename,rawFormat,false,channelCount,false,sampleRate,!askForSampleRate,length,!askForLength))
		return createNew(filename,channelCount,sampleRate,length,rawFormat);

	return NULL;
}

CLoadedSound *ASoundFileManager::createNew(const string filename,unsigned channelCount,unsigned sampleRate,unsigned length,bool rawFormat)
{
	CSound *sound=NULL;
	CSoundPlayerChannel *channel=NULL;
	CLoadedSound *loaded=NULL;

	if(isFilenameRegistered(filename))
		throw runtime_error(string(__func__)+" -- a file named '"+filename+"' is already opened");

	// should get based on extension
	const ASoundTranslator *translator=ASoundTranslator::findTranslator(filename,true,rawFormat);

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

	return loaded;
}

bool ASoundFileManager::open(const string _filename,bool openAsRaw)
{
	vector<string> filenames;
	string filename=_filename;
	bool readOnly=false;
	if(filename=="")
	{
		if(!gFrontendHooks->promptForOpenSoundFilenames(filenames,readOnly,openAsRaw))
			return false;
	}
	else
		filenames.push_back(filename);

	for(size_t t=0;t<filenames.size();t++)
	{
		try
		{
			prvOpen(filenames[t],readOnly,true,openAsRaw);
			updateReopenHistory(filenames[t]);
		}
		catch(runtime_error &e)
		{
			Error(e.what());
			return false;
		}
	}
	return true;
}

/*
 * I believe the only time we wouldn't want a file to be registered, doRegisterFilename==false is 
 * when we are loading the registered files from a previous sessions, so they would already be
 * in the registry
 */
void ASoundFileManager::prvOpen(const string filename,bool readOnly,bool doRegisterFilename,bool asRaw,const ASoundTranslator *translatorToUse)
{
	if(doRegisterFilename && isFilenameRegistered(filename))
		throw(runtime_error(string(__func__)+_(_(" -- file already opened"))));

	if(readOnly)
		throw(runtime_error(string(__func__)+" -- readOnly is true -- read only loading is not implemented yet"));

	CSound *sound=NULL;
	CSoundPlayerChannel *channel=NULL;
	CLoadedSound *loaded=NULL;

	try
	{
		if(!CPath(filename).exists())
			throw runtime_error(string(__func__)+" -- file does not exist: "+filename);

		if(translatorToUse==NULL)
			translatorToUse=ASoundTranslator::findTranslator(filename,false,asRaw);
		sound=new CSound;

		if(!translatorToUse->loadSound(filename,sound))
		{ // cancelled
			delete sound;
			return;
		}

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

/*
 * This needs to be called after saving a file.. it sets false the saved state 
 * on all previous actions on the undo stack which might because the current 
 * state if the user undoes.
 */
#include "AAction.h"
static void iterateUndoStackAndUnsetSavedState(CLoadedSound *loaded)
{
	stack<AAction *> temp;
	
	while(loaded->actions.empty())
	{
		loaded->actions.top()->setOrigIsModified();
		temp.push(loaded->actions.top());
		loaded->actions.pop();
	}

	while(temp.empty())
	{
		loaded->actions.push(temp.top());
		temp.pop();
	}
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
		
		if(loaded->translator->saveSound(filename,loaded->sound))
		{
			loaded->sound->setIsModified(false);
			updateAfterEdit();
			updateReopenHistory(filename);
		}
	}
}


void ASoundFileManager::saveAs()
{
	CLoadedSound *loaded=getActive();
	if(loaded)
	{
		string filename=loaded->getFilename();
askAgain:
		bool saveAsRaw=false;
		if(!gFrontendHooks->promptForSaveSoundFilename(filename,saveAsRaw))
			return;

		bool reregisterFilenameOnError=false;
		if(loaded->getFilename()==filename && compareBool(loaded->translator->handlesRaw(),saveAsRaw))
		{ // the user chose the same name (and didn't change whether to save as raw or not)
			save();
			return;
		}
		else if(loaded->getFilename()==filename && !compareBool(loaded->translator->handlesRaw(),saveAsRaw))
		{ // same name, but now saving as raw
			unregisterFilename(loaded->getFilename());
			reregisterFilenameOnError=true;
		}

		if(isFilenameRegistered(filename))
			throw(runtime_error(string(__func__)+_(" -- file is currently opened")+": '"+filename+"'"));

		try
		{
			if(CPath(filename).exists() && !CPath(filename).isDevice())
			{
				if(Question(_("Overwrite Existing File")+string(":\n")+filename,yesnoQues)!=yesAns)
				{
					if(reregisterFilenameOnError)
						registerFilename(filename);
					goto askAgain;
				}
			}

			const ASoundTranslator *translator=ASoundTranslator::findTranslator(filename,true,saveAsRaw);

			if(translator->saveSound(filename,loaded->sound))
			{
				loaded->translator=translator; // make save use this translator next time

				unregisterFilename(loaded->getFilename());
				loaded->changeFilename(filename);
				registerFilename(filename);

				loaded->sound->setIsModified(false);
				updateAfterEdit();
				updateReopenHistory(filename);
			}
		}
		catch(...)
		{
			if(reregisterFilenameOnError)
				registerFilename(filename);
			throw;
		}
	}
}

bool ASoundFileManager::savePartial(const CSound *sound,const string _filename,const sample_pos_t saveStart,const sample_pos_t saveLength,bool useLastUserPrefs)
{
	string filename=_filename;
	bool first=true;

askAgain:
	bool saveAsRaw=false;
	if(filename=="" || !first)
	{
		if(!gFrontendHooks->promptForSaveSoundFilename(filename,saveAsRaw))
			return false;
	}

	first=false;

	if(isFilenameRegistered(filename))
		throw(runtime_error(string(__func__)+_(" -- file is currently opened")+": '"+filename+"'"));

	if(CPath(filename).exists() && !CPath(filename).isDevice())
	{
		if(Question(_("Overwrite Existing File")+string(":\n")+filename,yesnoQues)!=yesAns)
			goto askAgain;
	}

	const ASoundTranslator *translator=ASoundTranslator::findTranslator(filename,true,saveAsRaw);

	if(translator->saveSound(filename,sound,saveStart,saveLength,useLastUserPrefs))
	{
		updateReopenHistory(filename);
		return true;
	}
	return false;
}

void ASoundFileManager::close(CloseTypes closeType,CLoadedSound *closeWhichSound)
{
	CLoadedSound *loaded=closeWhichSound==NULL ? getActive() : closeWhichSound;
	if(loaded)
	{
		bool doSave=false;
		if(loaded->sound->isModified())
		{
			if(closeType==ctSaveYesNoStop)
			{
				VAnswer a=Question(_("Save Modified Sound")+string(":\n")+loaded->getFilename(),cancelQues);
				if(a==cancelAns)
					throw EStopClosing();
				doSave=(a==yesAns);
			}
			else if(closeType==ctSaveYesNoCancel)
			{
				VAnswer a=Question(_("Save Modified Sound")+string(":\n")+loaded->getFilename(),cancelQues);
				if(a==cancelAns)
					return;
				doSave=(a==yesAns);
			}
			// else closeType==ctSaveNone  (no question to ask; just don't save)
		}

		loaded->sound->lockForResize();
		try
		{
			loaded->channel->stop();
			loaded->sound->unlockForResize();
		}
		catch(...)
		{
			loaded->sound->unlockForResize();
			// perhaps don't worry about it???
		}

		// save before we start deconstructing everything so that if there
		// is an error saving, the user hasn't lost all when it closes below
		if(doSave)
			save();

		unregisterFilename(loaded->getFilename());

		destroyWindow(loaded);

		loaded->sound->closeSound();
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

		if(!CPath(filename).exists())
		{
			gStatusComm->beep();
			return;
		}

		// ??? could check isMofied(), but if it isn't, then there's no need to revert...
		if(Question(_("Are you sure you want to revert to the last saved copy of")+string(" '")+filename+"'",yesnoQues)!=yesAns)
			return;

		// could be more effecient by not destroying then creating the sound window
		close(ctSaveNone); // ??? I need a way to know not to defrag since we're not attempting to save any results... altho.. I may never want to defrag.. perhaps on open... 
		prvOpen(filename,readOnly,true,translatorToUse->handlesRaw(),translatorToUse);
		// should I remember the selection positions and use them if they're valid? ???
	}
}


void ASoundFileManager::recordToNew()
{
#ifdef ENABLE_JACK /* this really needs to be a run-time check because we might not use a CJACKSoundRecorder after all it fails to initialize */
	// since we can't set the sample rate for the JACK audio system, then I don't ask for a sample rate
								// ??? device zero is the only one right now
	CLoadedSound *loaded=prvCreateNew(1,false,soundPlayer->devices[0].sampleRate,false);
#else
	CLoadedSound *loaded=prvCreateNew(1,false,0,true);
#endif
	if(loaded==NULL)
		return; // cancelled

	soundPlayer->aboutToRecord();
	try
	{

		ASoundRecorder *recorder=NULL;
		try
		{
			recorder=ASoundRecorder::createInitializedSoundRecorder(loaded->sound);
		}
		catch(...)
		{
			close(ctSaveNone,loaded);
			throw;
		}

		bool ret=true;
		try
		{
			ret=gFrontendHooks->promptForRecord(recorder);
			delete recorder;
		}
		catch(...)
		{
			delete recorder;

			if(loaded->sound->getLength()<=1) // don't close if something did seem to get recorded
				close(ctSaveNone,loaded);

			throw;
		}

		if(!ret)
			close(ctSaveNone,loaded); // record dialog was cancelled, don't ask to save when closing
		else
		{
			// ??? temporary until CSound can have zero length
			loaded->sound->lockForResize(); try { loaded->sound->removeSpace(0,1); loaded->sound->unlockForResize(); } catch(...) { loaded->sound->unlockForResize(); throw; }

			// set some kind of initial selection
			loaded->channel->setStopPosition(loaded->sound->getLength()/2+loaded->sound->getLength()/4);
			loaded->channel->setStartPosition(loaded->sound->getLength()/2-loaded->sound->getLength()/4);

			updateAfterEdit();
		}

		soundPlayer->doneRecording();
	}
	catch(...)
	{
		soundPlayer->doneRecording();
		throw;
	}
}


const string ASoundFileManager::getUntitledFilename(const string directory,const string extension)
{
	// return "/dir/untitled#.ext" where # is 0-99, then 'newfile#', then 'default#'
	static const char *prefixes[]={"untitled","newfile","default"};
	for(size_t i=0;i<(sizeof(prefixes)/sizeof(*prefixes));i++)
	{
		const string filename=directory+CPath::dirDelim+prefixes[i];
		for(size_t t=0;t<100;t++)
		{
			const string temp=filename+istring(t)+"."+extension;
			if(!CPath(temp).exists() && !isFilenameRegistered(temp))
				return(filename+istring(t)+"."+extension);
		}
	}
	return("");
}


#define LOADED_REG_KEY "loaded"

const vector<string> ASoundFileManager::loadFilesInRegistry()
{
	vector<string> errors;
	const vector<string> reg=loadedRegistryFile->getValue<vector<string> >(LOADED_REG_KEY);
	for(size_t t=0;t<reg.size();t++)
	{
		const string filename=reg[t];
		try
		{
			if(Question(_("Load sound from previous session?")+string("\n   ")+filename,yesnoQues)==yesAns)
			{
						// ??? readOnly and asRaw really need to be whatever the last value was, when it was originally loaded
				prvOpen(filename,false,false,false);
			}
			else
				unregisterFilename(filename);

		}
		catch(exception &e)
		{	
			if(filename!="")
				unregisterFilename(filename);
			errors.push_back(e.what());
		}
	}
	return(errors);
}

void ASoundFileManager::registerFilename(const string filename)
{
	if(isFilenameRegistered(filename))
		throw(runtime_error(string(__func__)+" -- file already registered"));

	vector<string> reg=loadedRegistryFile->getValue<vector<string> >(LOADED_REG_KEY);
	reg.push_back(filename);
	loadedRegistryFile->createValue<vector<string> >(LOADED_REG_KEY,reg);
}

void ASoundFileManager::unregisterFilename(const string filename)
{
	vector<string> reg=loadedRegistryFile->getValue<vector<string> >(LOADED_REG_KEY);
	for(size_t t=0;t<reg.size();t++)
	{
		if(reg[t]==filename)
		{
			reg.erase(reg.begin()+t);
			loadedRegistryFile->createValue<vector<string> >(LOADED_REG_KEY,reg);
			break;
		}
	}
}

bool ASoundFileManager::isFilenameRegistered(const string filename)
{
	vector<string> reg=loadedRegistryFile->getValue<vector<string> >(LOADED_REG_KEY);
	for(size_t t=0;t<reg.size();t++)
	{
		if(reg[t]==filename)
			return true;
	}
	return false;
}

void ASoundFileManager::updateReopenHistory(const string filename)
{
// ??? I think it would really make sense just to write this as a vector value since it's that way in memory
	// rewrite the reopen history to the gSettingsRegistry
	vector<string> reopenFilenames;
	for(size_t t=0;gSettingsRegistry->keyExists("ReopenHistory" DOT "item"+istring(t));t++)
	{
		const string h=gSettingsRegistry->getValue<string>("ReopenHistory" DOT "item"+istring(t));
		if(h!=filename)
			reopenFilenames.push_back(h);
	}

	if(reopenFilenames.size()>=gMaxReopenHistory)
		reopenFilenames.erase(reopenFilenames.begin()+reopenFilenames.size()-1);
		
	reopenFilenames.insert(reopenFilenames.begin(),filename);

	for(size_t t=0;t<reopenFilenames.size();t++)
		gSettingsRegistry->createValue<string>("ReopenHistory" DOT "item"+istring(t),reopenFilenames[t]);
}

const size_t ASoundFileManager::getReopenHistorySize() const
{
	size_t t;
	for(t=0;t<gMaxReopenHistory && gSettingsRegistry->keyExists("ReopenHistory" DOT "item"+istring(t));t++);

	return t;
}

const string ASoundFileManager::getReopenHistoryItem(const size_t index) const
{
	const string key="ReopenHistory" DOT "item"+istring(index);
	if(gSettingsRegistry->keyExists(key))
		return gSettingsRegistry->getValue<string>(key);
	else
		return "";
}

