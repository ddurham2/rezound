/* 
 * Copyright (C) 2006 - David W. Durham
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

#include "FrontendHooks.h"

#include <QFileDialog>
#include <QDir>

#include <string>

#include <istring>
#include <CPath.h>

#include "settings.h"
#include "MainWindow.h"
#include "MiscControlsWindow.h"

#include "NewSoundDialog.h"
#include "RecordDialog.h"
#include "RezSaveParametersDialog.h"
//#include "RecordMacroDialog.h"
#include "MacroActionParamsDialog.h"
//#include "JACKPortChoiceDialog.h"
#include "RawParametersDialog.h"
#include "OggSaveParametersDialog.h"
#include "Mp3SaveParametersDialog.h"
#include "VoxOpenParametersDialog.h"
#include "MIDISampleDumpParametersDialog.h"
#include "LibaudiofileSaveParametersDialog.h"

#include "../backend/ASoundTranslator.h"
#include "../backend/AStatusComm.h"

FrontendHooks::FrontendHooks(MainWindow *_mainWindow) :
	mainWindow(_mainWindow),

	openDialog(NULL),
	saveDialog(NULL),
	dirDialog(NULL),

	newSoundDialog(NULL),
	recordDialog(NULL),
	macroActionParamsDialog(NULL),
	rezSaveParametersDialog(NULL),
	//JACKPortChoiceDialog(NULL),
	rawParametersDialog(NULL),
	oggSaveParametersDialog(NULL),
	mp3Dialog(NULL),
	voxDialog(NULL),
    midiSampleDumpParametersDialog(NULL),
    libaudiofileSaveParametersDialog(NULL)
{
	/*
	JACKPortChoiceDialog=new CJACKPortChoiceDialog(mainWindow);
	*/
}

FrontendHooks::~FrontendHooks()
{
	delete openDialog;
	delete saveDialog;
	delete dirDialog;
	delete newSoundDialog;
	delete recordDialog;
	delete macroActionParamsDialog;
	delete rezSaveParametersDialog;
	//delete JACKPortChoiceDialog;
	delete rawParametersDialog;
	delete oggSaveParametersDialog;
	delete mp3Dialog;
	delete voxDialog;
    delete midiSampleDumpParametersDialog;
    delete libaudiofileSaveParametersDialog;
}

void FrontendHooks::doSetupAfterBackendIsSetup()
{
	mainWindow->restoreDockState();

	openDialog=new QFileDialog(mainWindow);
	openDialog->setWindowTitle(_("Open File"));
	openDialog->setAcceptMode(QFileDialog::AcceptOpen);
	openDialog->setNameFilters(getQtFileTypes());
	//openDialog->setReadOnly(true); // would be true if I supported it
	openDialog->setOption(QFileDialog::ReadOnly, "on");
	if(openDialog->directory().absolutePath()!=gPromptDialogDirectory.c_str())
		openDialog->setDirectory(gPromptDialogDirectory.c_str());
/*
	{ // add the "Open as Raw" check button
		FXVerticalFrame *f=new FXVerticalFrame(openDialog,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0, 0,0,0,0, 0,0);
		openDialog->childAtIndex(0)->reparent(f);
		openAsRawCheckButton=new FXCheckButton(new FXPacker(f,0, 0,0,0,0, DEFAULT_SPACING*2,0,0),_("Open as Raw"),NULL,0,CHECKBUTTON_NORMAL);

		if(!ASoundTranslator::findRawTranslator()) // hide if we can't handle it
			f->hide();
	}
*/


	saveDialog=new QFileDialog(mainWindow);
	saveDialog->setWindowTitle(_("Save File"));
	//we do this ourself saveDialog->setConfirmOverwrite(true);
	saveDialog->setAcceptMode(QFileDialog::AcceptSave);
	saveDialog->setNameFilters(getQtFileTypes());
	if(saveDialog->directory().absolutePath()!=gPromptDialogDirectory.c_str())
		saveDialog->setDirectory(gPromptDialogDirectory.c_str());
	/*
	{ // add the "Save as Raw" check button
		FXVerticalFrame *f=new FXVerticalFrame(saveDialog,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0, 0,0,0,0, 0,0);
		saveDialog->childAtIndex(0)->reparent(f);
		saveAsRawCheckButton=new FXCheckButton(new FXPacker(f,0, 0,0,0,0, DEFAULT_SPACING*2,0,0),_("Save as Raw"),NULL,0,CHECKBUTTON_NORMAL);

		if(!ASoundTranslator::findRawTranslator()) // hide if we can't handle it
			f->hide();
	}
	*/

	dirDialog=new QFileDialog(mainWindow);
	dirDialog->setWindowTitle(_("Select Directory"));
	dirDialog->setFileMode(QFileDialog::Directory);
	if(dirDialog->directory().absolutePath()!=gPromptDialogDirectory.c_str())
		dirDialog->setDirectory(gPromptDialogDirectory.c_str());

	newSoundDialog=new NewSoundDialog(mainWindow);
	recordDialog=new RecordDialog(mainWindow);
	rezSaveParametersDialog=new RezSaveParametersDialog(mainWindow);
	macroActionParamsDialog=new MacroActionParamsDialog(mainWindow);
	rawParametersDialog=new RawParametersDialog(mainWindow);
	oggSaveParametersDialog=new OggSaveParametersDialog(mainWindow);
	mp3Dialog=new Mp3SaveParametersDialog(mainWindow);
	voxDialog=new VoxOpenParametersDialog(mainWindow);
    midiSampleDumpParametersDialog=new MIDISampleDumpParametersDialog(mainWindow);
    libaudiofileSaveParametersDialog=new LibaudiofileSaveParametersDialog(mainWindow);
}

void FrontendHooks::setWhichClipboard(size_t whichClipboard)
{
	mainWindow->miscControlsWindow()->setWhichClipboard(whichClipboard);
}

const QStringList FrontendHooks::getQtFileTypes() const
{
	const vector<const ASoundTranslator *> translators=ASoundTranslator::getTranslators();
	QStringList types;
	string allTypes;

	/*
		From all the registered translators build a string list to use as the file type 
		drop-down in the FOX file dialogs:
		   [0] Format Name (*.ext,*.EXT)
		   [1] Format Name (*.ext,*.EXT)
		   ...
		And built a list for "All Types" with the *.ext,*.ext,...,*.EXT,*.EXT
	*/
	for(size_t t=0;t<translators.size();t++)
	{
		const vector<string> names=translators[t]->getFormatNames();
		const vector<vector<string> > fileMasks=translators[t]->getFormatFileMasks();
	
		for(size_t i=0;i<names.size();i++)
		{
			string type;
			
			type=names[i];
			type+=" (";
			for(size_t k=0;k<fileMasks[i].size();k++)
			{
// TODO see if we can just display lower case and not have to uppercase it.. and that it will accept both
				type+=fileMasks[i][k]+" "+istring(fileMasks[i][k]).upper()+" ";
				allTypes+=fileMasks[i][k]+" ";
			}
			type+=")";

			types.push_back(type.c_str());
		}
	}

	// go back through and add uppercase of all types' extensions
	for(size_t t=0;t<translators.size();t++)
	{
		const vector<string> names=translators[t]->getFormatNames();
		const vector<vector<string> > fileMasks=translators[t]->getFormatFileMasks();
	
		for(size_t i=0;i<names.size();i++)
		{
			for(size_t k=0;k<fileMasks[i].size();k++)
			{
				allTypes+=istring(fileMasks[i][k]).upper()+" ";
			}
		}
	}
	
	types.push_front((string(_("All Supported Types"))+" ("+allTypes+")").c_str());

	return types;
}

bool FrontendHooks::promptForOpenSoundFilename(string &filename,bool &readOnly,bool &openAsRaw)
{
	openDialog->selectFile("");
	if(openDialog->directory().absolutePath()!=gPromptDialogDirectory.c_str())
		openDialog->setDirectory(gPromptDialogDirectory.c_str());
	openDialog->setFileMode(QFileDialog::ExistingFile);
	//openAsRawCheckButton->setCheck(openAsRaw);
	if(openDialog->exec())
	{
		// save directory to open the opendialog to next time
		gPromptDialogDirectory=openDialog->directory().absolutePath().toStdString();

		filename=openDialog->selectedFiles()[0].toStdString();
		readOnly=openDialog->testOption(QFileDialog::ReadOnly);
		openAsRaw=false;
		//openAsRaw=openAsRawCheckButton->getCheck();

		return true;
	}
	return false;
/*
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
*/
}

bool FrontendHooks::promptForOpenSoundFilenames(vector<string> &filenames,bool &readOnly,bool &openAsRaw)
{
	openDialog->selectFile("");
	if(openDialog->directory().absolutePath()!=gPromptDialogDirectory.c_str())
		openDialog->setDirectory(gPromptDialogDirectory.c_str());
	openDialog->setFileMode(QFileDialog::ExistingFiles);
	//openAsRawCheckButton->setCheck(openAsRaw);
	if(openDialog->exec())
	{
		// save directory to open the opendialog to next time
		gPromptDialogDirectory=openDialog->directory().absolutePath().toStdString();

		QStringList files=openDialog->selectedFiles();
		for(int t=0;t<files.size();t++)
			filenames.push_back(files[t].toStdString());

		readOnly=openDialog->testOption(QFileDialog::ReadOnly);
		openAsRaw=false;
		//openAsRaw=openAsRawCheckButton->getCheck();

		return true;
	}
	return false;
}

bool FrontendHooks::promptForSaveSoundFilename(string &filename,bool &saveAsRaw)
{
	if(filename!="")
	{
		if(saveDialog->directory().absolutePath()!=gPromptDialogDirectory.c_str())
			saveDialog->setDirectory(gPromptDialogDirectory.c_str());
		saveDialog->selectFile(CPath(filename).baseName().c_str());
	}
	//saveAsRawCheckButton->setCheck(saveAsRaw);

	if(saveDialog->exec())
	{
		gPromptDialogDirectory=saveDialog->directory().absolutePath().toStdString();

		filename=saveDialog->selectedFiles()[0].toStdString();
		saveAsRaw=false;
		//saveAsRaw=saveAsRawCheckButton->getCheck();

		return true;
	}
	return false;
}

bool FrontendHooks::promptForNewSoundParameters(string &filename,bool &rawFormat,bool hideFilename,unsigned &channelCount,bool hideChannelCount,unsigned &sampleRate,bool hideSampleRate,sample_pos_t &length,bool hideLength)
{
	newSoundDialog->hideFilename(hideFilename);
	newSoundDialog->hideChannelCount(hideChannelCount);
	newSoundDialog->hideSampleRate(hideSampleRate);
	newSoundDialog->hideLength(hideLength);

	if(!hideFilename)
		newSoundDialog->setFilename(filename,rawFormat);

	if(newSoundDialog->exec())
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

bool FrontendHooks::promptForDirectory(string &dirname,const string title)
{
	dirDialog->setWindowTitle(title.c_str());
	if(dirDialog->exec())
	{
		// ??? verifiy if this is correct when selecting a directory
		dirname=dirDialog->directory().absolutePath().toStdString();
		return true;
	}
	return false;
}

bool FrontendHooks::promptForRecord(ASoundRecorder *recorder)
{
	return recordDialog->exec(recorder);
}

bool FrontendHooks::showRecordMacroDialog(string &macroName)
{
	QString t=macroName.c_str();
	t=QInputDialog::getText(NULL,_("Record Macro"),_("Macro Name:"),QLineEdit::Normal,t).trimmed();
	if(t=="")
		return false;
	macroName=t.toStdString();
	return true;
}

bool FrontendHooks::showMacroActionParamsDialog(const AActionFactory *actionFactory,MacroActionParameters &macroActionParameters,CLoadedSound *loadedSound)
{
	return macroActionParamsDialog->showIt(actionFactory,macroActionParameters,loadedSound);
}

const string FrontendHooks::promptForJACKPort(const string message,const vector<string> portNames)
{
	return "";
	/*
	if(JACKPortChoiceDialog->show(message,portNames))
		return JACKPortChoiceDialog->getPortName();
	else
		throw runtime_error(string(__func__)+" -- choice aborted");
		*/
}

bool FrontendHooks::promptForRezSaveParameters(RezSaveParameters &parameters)
{
	return rezSaveParametersDialog->show(parameters);
}

bool FrontendHooks::promptForRawParameters(RawParameters &parameters,bool showLoadRawParameters)
{
	return rawParametersDialog->show(parameters,showLoadRawParameters);
}

bool FrontendHooks::promptForOggCompressionParameters(OggCompressionParameters &parameters)
{
	return oggSaveParametersDialog->show(parameters);
}

bool FrontendHooks::promptForMp3CompressionParameters(Mp3CompressionParameters &parameters)
{
	return mp3Dialog->show(parameters);
}

bool FrontendHooks::promptForVoxParameters(VoxParameters &parameters)
{
	return voxDialog->show(parameters);
}

#if defined(ENABLE_LADSPA)
#include "ActionParam/ChannelSelectDialog.h"
#include "LADSPAActionDialog.h"
AActionDialog *FrontendHooks::getChannelSelectDialog()
{
	return gChannelSelectDialog;
}


AActionDialog *FrontendHooks::getLADSPAActionDialog(const LADSPA_Descriptor *desc)
{
	return new LADSPAActionDialog(mainWindow,desc);
#if 0 // nah
	// only return a dialog if there is at least one input control ports
	for(unsigned t=0;t<desc->PortCount;t++)
	{
		const LADSPA_PortDescriptor portDesc=desc->PortDescriptors[t];
		if(LADSPA_IS_PORT_CONTROL(portDesc) && LADSPA_IS_PORT_INPUT(portDesc))
			return new LADSPAActionDialog(mainWindow,desc);
	}
	return NULL;
#endif
}
#endif



bool FrontendHooks::promptForOpenMIDISampleDump(int &sysExChannel,int &waveformId)
{
	return midiSampleDumpParametersDialog->showForOpen(sysExChannel,waveformId);
}

bool FrontendHooks::promptForSaveMIDISampleDump(int &sysExChannel,int &waveformId,int &loopType)
{
	return midiSampleDumpParametersDialog->showForSave(sysExChannel,waveformId,loopType);
}

bool FrontendHooks::promptForlibaudiofileSaveParameters(libaudiofileSaveParameters &parameters,const string formatName)
{
	return libaudiofileSaveParametersDialog->show(parameters,formatName);
}

