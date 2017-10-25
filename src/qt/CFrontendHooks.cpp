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

#include "CFrontendHooks.h"

#include <QFileDialog>
#include <QDir>

#include <string>

#include <istring>
#include <CPath.h>

#include "settings.h"
#include "CMainWindow.h"
#include "CMiscControlsWindow.h"

#include "CNewSoundDialog.h"
#include "CRecordDialog.h"
#include "CRezSaveParametersDialog.h"
//#include "CRecordMacroDialog.h"
#include "CMacroActionParamsDialog.h"
//#include "CJACKPortChoiceDialog.h"
#include "CRawParametersDialog.h"
#include "COggSaveParametersDialog.h"
#include "CMp3SaveParametersDialog.h"
#include "CVoxOpenParametersDialog.h"
#include "CMIDISampleDumpParametersDialog.h"
#include "ClibaudiofileSaveParametersDialog.h"

#include "../backend/ASoundTranslator.h"
#include "../backend/AStatusComm.h"

CFrontendHooks::CFrontendHooks(CMainWindow *_mainWindow) :
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
	MIDISampleDumpParametersDialog(NULL),
	libaudiofileSaveParametersDialog(NULL)
{
	/*
	JACKPortChoiceDialog=new CJACKPortChoiceDialog(mainWindow);
	*/
}

CFrontendHooks::~CFrontendHooks()
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
	delete MIDISampleDumpParametersDialog;
	delete libaudiofileSaveParametersDialog;
}

void CFrontendHooks::doSetupAfterBackendIsSetup()
{
	mainWindow->restoreDockState();

	openDialog=new QFileDialog(mainWindow);
	openDialog->setWindowTitle(_("Open File"));
	openDialog->setAcceptMode(QFileDialog::AcceptOpen);
	openDialog->setNameFilters(getQtFileTypes());
	//openDialog->setReadOnly(true); // would be true if I supported it
	openDialog->setReadOnly(false);
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
	dirDialog->setFileMode(QFileDialog::DirectoryOnly);
	if(dirDialog->directory().absolutePath()!=gPromptDialogDirectory.c_str())
		dirDialog->setDirectory(gPromptDialogDirectory.c_str());

	newSoundDialog=new CNewSoundDialog(mainWindow);
	recordDialog=new CRecordDialog(mainWindow);
	rezSaveParametersDialog=new CRezSaveParametersDialog(mainWindow);
	macroActionParamsDialog=new CMacroActionParamsDialog(mainWindow);
	rawParametersDialog=new CRawParametersDialog(mainWindow);
	oggSaveParametersDialog=new COggSaveParametersDialog(mainWindow);
	mp3Dialog=new CMp3SaveParametersDialog(mainWindow);
	voxDialog=new CVoxOpenParametersDialog(mainWindow);
	MIDISampleDumpParametersDialog=new CMIDISampleDumpParametersDialog(mainWindow);
	libaudiofileSaveParametersDialog=new ClibaudiofileSaveParametersDialog(mainWindow);
}

void CFrontendHooks::setWhichClipboard(size_t whichClipboard)
{
	mainWindow->miscControlsWindow()->setWhichClipboard(whichClipboard);
}

const QStringList CFrontendHooks::getQtFileTypes() const
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

bool CFrontendHooks::promptForOpenSoundFilename(string &filename,bool &readOnly,bool &openAsRaw)
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
		readOnly=openDialog->isReadOnly();
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

bool CFrontendHooks::promptForOpenSoundFilenames(vector<string> &filenames,bool &readOnly,bool &openAsRaw)
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

		readOnly=openDialog->isReadOnly();
		openAsRaw=false;
		//openAsRaw=openAsRawCheckButton->getCheck();

		return true;
	}
	return false;
}

bool CFrontendHooks::promptForSaveSoundFilename(string &filename,bool &saveAsRaw)
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

bool CFrontendHooks::promptForNewSoundParameters(string &filename,bool &rawFormat,bool hideFilename,unsigned &channelCount,bool hideChannelCount,unsigned &sampleRate,bool hideSampleRate,sample_pos_t &length,bool hideLength)
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

bool CFrontendHooks::promptForDirectory(string &dirname,const string title)
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

bool CFrontendHooks::promptForRecord(ASoundRecorder *recorder)
{
	return recordDialog->exec(recorder);
}

bool CFrontendHooks::showRecordMacroDialog(string &macroName)
{
	QString t=macroName.c_str();
	t=QInputDialog::getText(NULL,_("Record Macro"),_("Macro Name:"),QLineEdit::Normal,t).trimmed();
	if(t=="")
		return false;
	macroName=t.toStdString();
	return true;
}

bool CFrontendHooks::showMacroActionParamsDialog(const AActionFactory *actionFactory,MacroActionParameters &macroActionParameters,CLoadedSound *loadedSound)
{
	return macroActionParamsDialog->showIt(actionFactory,macroActionParameters,loadedSound);
}

const string CFrontendHooks::promptForJACKPort(const string message,const vector<string> portNames)
{
	return "";
	/*
	if(JACKPortChoiceDialog->show(message,portNames))
		return JACKPortChoiceDialog->getPortName();
	else
		throw runtime_error(string(__func__)+" -- choice aborted");
		*/
}

bool CFrontendHooks::promptForRezSaveParameters(RezSaveParameters &parameters)
{
	return rezSaveParametersDialog->show(parameters);
}

bool CFrontendHooks::promptForRawParameters(RawParameters &parameters,bool showLoadRawParameters)
{
	return rawParametersDialog->show(parameters,showLoadRawParameters);
}

bool CFrontendHooks::promptForOggCompressionParameters(OggCompressionParameters &parameters)
{
	return oggSaveParametersDialog->show(parameters);
}

bool CFrontendHooks::promptForMp3CompressionParameters(Mp3CompressionParameters &parameters)
{
	return mp3Dialog->show(parameters);
}

bool CFrontendHooks::promptForVoxParameters(VoxParameters &parameters)
{
	return voxDialog->show(parameters);
}

#if defined(ENABLE_LADSPA)
#include "ActionParam/CChannelSelectDialog.h"
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
	return MIDISampleDumpParametersDialog->showForOpen(sysExChannel,waveformId);
}

bool CFrontendHooks::promptForSaveMIDISampleDump(int &sysExChannel,int &waveformId,int &loopType)
{
	return MIDISampleDumpParametersDialog->showForSave(sysExChannel,waveformId,loopType);
}

bool CFrontendHooks::promptForlibaudiofileSaveParameters(libaudiofileSaveParameters &parameters,const string formatName)
{
	return libaudiofileSaveParametersDialog->show(parameters,formatName);
}

