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
#include "CMainWindow.h"

#include "CNewSoundDialog.h"
#include "CRecordDialog.h"
#include "CRecordMacroDialog.h"
#include "CMacroActionParamsDialog.h"
#include "CJACKPortChoiceDialog.h"
#include "CRezSaveParametersDialog.h"
#include "CRawDialog.h"
#include "COggDialog.h"
#include "CMp3Dialog.h"
#include "CVoxDialog.h"
#include "CMIDIDumpSampleIdDialog.h"
#include "ClibaudiofileSaveParametersDialog.h"

#include "../backend/ASoundTranslator.h"
#include "../backend/AStatusComm.h"

CFrontendHooks::CFrontendHooks(FXWindow *_mainWindow) :
	mainWindow(_mainWindow),

	openDialog(NULL),
	saveDialog(NULL),
	dirDialog(NULL),

	newSoundDialog(NULL),
	recordDialog(NULL),
	recordMacroDialog(NULL),
	JACKPortChoiceDialog(NULL),
	rezSaveParametersDialog(NULL),
	rawDialog(NULL),
	oggDialog(NULL),
	mp3Dialog(NULL),
	voxDialog(NULL),
	MIDIDumpSampleIdDialog(NULL),
	libaudiofileSaveParametersDialog(NULL)
{
	dirDialog=new FXDirDialog(mainWindow,_("Select Directory"));

	JACKPortChoiceDialog=new CJACKPortChoiceDialog(mainWindow);
}

CFrontendHooks::~CFrontendHooks()
{
	delete openDialog;
	delete saveDialog;

	delete newSoundDialog;
	delete recordDialog;
	delete recordMacroDialog;
	delete JACKPortChoiceDialog;
	delete rezSaveParametersDialog;
	delete rawDialog;
	delete oggDialog;
	delete mp3Dialog;
	delete voxDialog;
	delete MIDIDumpSampleIdDialog;
	delete libaudiofileSaveParametersDialog;
}

void CFrontendHooks::doSetupAfterBackendIsSetup()
{
	openDialog=new FXFileDialog(mainWindow,_("Open File"));
	openDialog->setPatternList(getFOXFileTypes().c_str());
	openDialog->setCurrentPattern(0);
	openDialog->showReadOnly(false); // would be true if I supported it
	openDialog->setReadOnly(false);
	{ // add the "Open as Raw" check button
		FXVerticalFrame *f=new FXVerticalFrame(openDialog,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0, 0,0,0,0, 0,0);
		openDialog->childAtIndex(0)->reparent(f);
		openAsRawCheckButton=new FXCheckButton(new FXPacker(f,0, 0,0,0,0, DEFAULT_SPACING*2,0,0),_("Open as Raw"),NULL,0,CHECKBUTTON_NORMAL);

		if(!ASoundTranslator::findRawTranslator()) // hide if we can't handle it
			f->hide();
	}
	if(openDialog->getDirectory()!=gPromptDialogDirectory.c_str())
		openDialog->setDirectory(gPromptDialogDirectory.c_str());

	saveDialog=new FXFileDialog(mainWindow,_("Save File"));
	saveDialog->setSelectMode(SELECTFILE_ANY);
	saveDialog->setPatternList(getFOXFileTypes().c_str());
	saveDialog->setCurrentPattern(0);
	saveDialog->setDirectory(gPromptDialogDirectory.c_str());
	{ // add the "Save as Raw" check button
		FXVerticalFrame *f=new FXVerticalFrame(saveDialog,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0, 0,0,0,0, 0,0);
		saveDialog->childAtIndex(0)->reparent(f);
		saveAsRawCheckButton=new FXCheckButton(new FXPacker(f,0, 0,0,0,0, DEFAULT_SPACING*2,0,0),_("Save as Raw"),NULL,0,CHECKBUTTON_NORMAL);

		if(!ASoundTranslator::findRawTranslator()) // hide if we can't handle it
			f->hide();
	}

	newSoundDialog=new CNewSoundDialog(mainWindow);
	recordDialog=new CRecordDialog(mainWindow);
	recordMacroDialog=new CRecordMacroDialog(mainWindow);
	macroActionParamsDialog=new CMacroActionParamsDialog(mainWindow);
	rezSaveParametersDialog=new CRezSaveParametersDialog(mainWindow);
	rawDialog=new CRawDialog(mainWindow);
	oggDialog=new COggDialog(mainWindow);
	mp3Dialog=new CMp3Dialog(mainWindow);
	voxDialog=new CVoxDialog(mainWindow);
	MIDIDumpSampleIdDialog=new CMIDIDumpSampleIdDialog(mainWindow);
	libaudiofileSaveParametersDialog=new ClibaudiofileSaveParametersDialog(mainWindow);
}

void CFrontendHooks::setWhichClipboard(size_t whichClipboard)
{
	((CMainWindow *)mainWindow)->setWhichClipboard(whichClipboard);
}

const string CFrontendHooks::getFOXFileTypes() const
{
	const vector<const ASoundTranslator *> translators=ASoundTranslator::getTranslators();
	string types;
	string allTypes;

	/*
		From all the registered translators build a string to use as the file type 
		drop-down in the FOX file dialogs:
		   Format Name (*.ext,*.EXT)\nFormat Name (*.ext,*.EXT)\n...
		And built a list for "All Types" with the *.ext,*.ext,...,*.EXT,*.EXT
	*/
	for(size_t t=0;t<translators.size();t++)
	{
		const vector<string> names=translators[t]->getFormatNames();
		const vector<vector<string> > fileMasks=translators[t]->getFormatFileMasks();
	
		for(size_t i=0;i<names.size();i++)
		{
			types+=names[i];
			types+=" (";
			for(size_t k=0;k<fileMasks[i].size();k++)
			{
				if(k>0)
					types+=",";
				types+=fileMasks[i][k]+","+istring(fileMasks[i][k]).upper();

				if(allTypes!="")
					allTypes+=",";
				allTypes+=fileMasks[i][k];
			}
			types+=")\n";
		}
	}

	for(size_t t=0;t<translators.size();t++)
	{
		const vector<string> names=translators[t]->getFormatNames();
		const vector<vector<string> > fileMasks=translators[t]->getFormatFileMasks();
	
		for(size_t i=0;i<names.size();i++)
		{
			for(size_t k=0;k<fileMasks[i].size();k++)
			{
				if(allTypes!="")
					allTypes+=",";
				allTypes+=istring(fileMasks[i][k]).upper();
			}
		}
	}
	
	types=string(_("All Supported Types"))+" ("+allTypes+")\n"+types+_("All Files")+"(*)\n";

	return types;
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

		return true;
	}
	return false;
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

		return true;
	}
	return false;
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

		return true;
	}
	return false;
}

bool CFrontendHooks::promptForNewSoundParameters(string &filename,bool &rawFormat,bool hideFilename,unsigned &channelCount,bool hideChannelCount,unsigned &sampleRate,bool hideSampleRate,sample_pos_t &length,bool hideLength)
{
	newSoundDialog->hideFilename(hideFilename);
	if(hideChannelCount)
		throw runtime_error(string(__func__)+" -- unimplemented: hideChannelCount");
	newSoundDialog->hideSampleRate(hideSampleRate);
	newSoundDialog->hideLength(hideLength);

	if(!hideFilename)
		newSoundDialog->setFilename(filename,rawFormat);

	if(newSoundDialog->execute(PLACEMENT_CURSOR))	
	{
		if(!hideFilename)
		{
			filename=newSoundDialog->getFilename();
			rawFormat=newSoundDialog->getRawFormat();
		}

		if(!hideChannelCount)
			channelCount=newSoundDialog->getChannelCount();

		if(!hideSampleRate)
			sampleRate=newSoundDialog->getSampleRate();

		if(!hideLength)
			length=newSoundDialog->getLength();

		return true;
	}
	return false;
}

bool CFrontendHooks::promptForDirectory(string &dirname,const string title)
{
	dirDialog->setTitle(title.c_str());
	if(dirDialog->execute())
	{
		dirname=dirDialog->getDirectory().text();
		return true;
	}
	return false;
}

bool CFrontendHooks::promptForRecord(ASoundRecorder *recorder)
{
	if(recordDialog->show(recorder))
		return true;
	return false;
}

bool CFrontendHooks::showRecordMacroDialog(string &macroName)
{
	recordMacroDialog->setMacroName(macroName);
	if(recordMacroDialog->showIt())
	{
		macroName=recordMacroDialog->getMacroName();
		return true;
	}
	return false;

}

bool CFrontendHooks::showMacroActionParamsDialog(const AActionFactory *actionFactory,MacroActionParameters &macroActionParameters,CLoadedSound *loadedSound)
{
	return macroActionParamsDialog->showIt(actionFactory,macroActionParameters,loadedSound);
}

const string CFrontendHooks::promptForJACKPort(const string message,const vector<string> portNames)
{
	if(JACKPortChoiceDialog->show(message,portNames))
		return JACKPortChoiceDialog->getPortName();
	else
		throw runtime_error(string(__func__)+" -- choice aborted");
}

bool CFrontendHooks::promptForRezSaveParameters(RezSaveParameters &parameters)
{
	return rezSaveParametersDialog->show(parameters);
}

bool CFrontendHooks::promptForRawParameters(RawParameters &parameters,bool showLoadRawParameters)
{
	return rawDialog->show(parameters,showLoadRawParameters);
}

bool CFrontendHooks::promptForOggCompressionParameters(OggCompressionParameters &parameters)
{
	return oggDialog->show(parameters);
}

bool CFrontendHooks::promptForMp3CompressionParameters(Mp3CompressionParameters &parameters)
{
	return mp3Dialog->show(parameters);
}

bool CFrontendHooks::promptForVoxParameters(VoxParameters &parameters)
{
	return voxDialog->show(parameters);
}

#ifdef ENABLE_LADSPA

#include "CChannelSelectDialog.h"
#include "CLADSPAActionDialog.h"
AActionDialog *CFrontendHooks::getChannelSelectDialog()
{
	return gChannelSelectDialog;
}

AActionDialog *CFrontendHooks::getLADSPAActionDialog(const LADSPA_Descriptor *desc)
{
	return new CLADSPAActionDialog(mainWindow,desc);
#if 0 // nah
	// only return a dialog if there is at least one input control ports
	for(unsigned t=0;t<desc->PortCount;t++)
	{
		const LADSPA_PortDescriptor portDesc=desc->PortDescriptors[t];
		if(LADSPA_IS_PORT_CONTROL(portDesc) && LADSPA_IS_PORT_INPUT(portDesc))
			return new CLADSPAActionDialog(mainWindow,desc);
	}
	return NULL;
#endif
}

#endif


bool CFrontendHooks::promptForOpenMIDISampleDump(int &sysExChannel,int &waveformId)
{
	return MIDIDumpSampleIdDialog->showForOpen(sysExChannel,waveformId);
}

bool CFrontendHooks::promptForSaveMIDISampleDump(int &sysExChannel,int &waveformId,int &loopType)
{
	return MIDIDumpSampleIdDialog->showForSave(sysExChannel,waveformId,loopType);
}

bool CFrontendHooks::promptForlibaudiofileSaveParameters(libaudiofileSaveParameters &parameters,const string formatName)
{
	return libaudiofileSaveParametersDialog->show(parameters,formatName);
}

