#include "CMacroRecorder.h"

#include "settings.h"
#include "AStatusComm.h"
#include "CActionParameters.h"
#include "AFrontendHooks.h"
#include "CLoadedSound.h"
#include "CSoundPlayerChannel.h"
#include "CSound.h"
#include "AAction.h"
#include "ASoundClipboard.h"

#include <CNestedDataFile/CNestedDataFile.h>

CMacroRecorder::CMacroRecorder() :
	recording(false),
	file(NULL)
{
}

CMacroRecorder::~CMacroRecorder()
{
	// would like to handle an unstopped recording, but it's possible that the CNestedDataFile we know about has been closed..
}


bool CMacroRecorder::isRecording() const
{
	return recording;
}

void CMacroRecorder::startRecording(CNestedDataFile *_file,const string _macroName)
{
	if(recording)
		throw runtime_error(string(__func__)+" -- already recording");

	macroName=_macroName;
	file=_file;

	key=macroName;

	if(file->keyExists(key))
	{
		if(Question(_("A macro already exists with the name: ")+macroName+"\n\n"+_("Do you want to overwrite it?"),yesnoQues)==yesAns)
			removeMacro(file,macroName);
		else
			return;
	}

	actionCount=0;
	file->setValue<unsigned>(key DOT "actionCount",actionCount);

	recording=true;
}

void CMacroRecorder::stopRecording()
{
	if(recording)
	{
		recording=false;

		// finish up
		if(actionCount<=0)
		{
			Message(_("Nothing was recorded in the macro: ")+macroName);
			file->removeKey(key);
		}
		else
		{
			// now add the macro name to the selectable list of macro's to play
			vector<string> macroNames=file->getValue<vector<string> >("MacroNames");
			macroNames.push_back(macroName);

			// sort the macro names
			sort(macroNames.begin(),macroNames.end()); 

			// remove duplicates
			unique(macroNames.begin(),macroNames.end());

			// write new macro name list to file
			file->setValue<vector<string> >("MacroNames",macroNames);

		}

		file->save();
	}
}

void CMacroRecorder::pushAction(const string actionName,const CActionParameters *actionParameters,CLoadedSound *loadedSound)
{
	if(!recording)
		throw runtime_error(string(__func__)+" -- not recording");

	if(gRegisteredActionFactories.find(actionName)==gRegisteredActionFactories.end())
	{
		Message(_("The action just performed will not be recorded in your macro.")+string("\n\n")+actionName);
		return;
	}

	const AActionFactory *actionFactory=gRegisteredActionFactories[actionName];

	// ask the user how they want to handle certain things for this action when the macro is played back
	AFrontendHooks::MacroActionParameters macroActionParameters;
	gFrontendHooks->showMacroActionParamsDialog(actionFactory,macroActionParameters,loadedSound);

	const string actionKey=key DOT "action"+istring(actionCount,3,true);

	file->setValue<string>(actionKey DOT "actionName",actionName);
	file->setValue<bool>(actionKey DOT "askToPromptForActionParametersAtPlayback",macroActionParameters.askToPromptForActionParametersAtPlayback);
	file->setValue<string>(actionKey DOT "selectedClipboardDescription",AAction::clipboards[gWhichClipboard]->getDescription());
	file->setValue<bool>(actionKey DOT "selectionPositionsAreApplicable",actionFactory->selectionPositionsAreApplicable);
	if(loadedSound && actionFactory->selectionPositionsAreApplicable)
	{
		file->setValue<sample_pos_t>(actionKey DOT "positioning" DOT "startPosition",loadedSound->channel->getStartPosition());
		file->setValue<sample_pos_t>(actionKey DOT "positioning" DOT "stopPosition",loadedSound->channel->getStopPosition());
		file->setValue<sample_pos_t>(actionKey DOT "positioning" DOT "audioLength",loadedSound->sound->getLength());
		file->setValue<unsigned>(actionKey DOT "positioning" DOT "startPosPositioning",macroActionParameters.startPosPositioning);
		file->setValue<unsigned>(actionKey DOT "positioning" DOT "stopPosPositioning",macroActionParameters.stopPosPositioning);
		file->setValue<string>(actionKey DOT "positioning" DOT "startPosCueName",macroActionParameters.startPosCueName);
		file->setValue<string>(actionKey DOT "positioning" DOT "stopPosCueName",macroActionParameters.stopPosCueName);

		// ??? NOTE: not storing the crossfade edges method that was used
	}

	actionParameters->writeToFile(file,actionKey DOT "parameters");

	file->setValue<unsigned>(key DOT "actionCount",++actionCount);
}

void CMacroRecorder::popAction(const string actionName)
{
	if(!recording)
		throw runtime_error(string(__func__)+" -- not recording");

	if(gRegisteredActionFactories.find(actionName)==gRegisteredActionFactories.end())
		return; // wouldn't have been recorded

	if(actionCount>0)
	{
		const string actionKey=key DOT "action"+istring(actionCount-1,3,true);
		
		if(file->getValue<string>(actionKey DOT "actionName")==actionName)
		{
			// forget the last action we recorded
			actionCount--;
			file->removeKey(actionKey);
			file->setValue<unsigned>(key DOT "actionCount",actionCount);
		}
		// else otherwise, we didn't add this action for whatever reason (probably wasn't in the action registry)
	}
}

void CMacroRecorder::removeMacro(CNestedDataFile *file,const string macroName)
{
	// get the current list of macro names
	vector<string> macroNames=file->getValue<vector<string> >("MacroNames");

	// remove the name from the list
	for(vector<string>::iterator i=macroNames.begin();i!=macroNames.end();i++)
	{
		if((*i)==macroName)
		{
			i=macroNames.erase(i);
			if(i==macroNames.end()) // still not quite sure about the best way to iterate over a vector while removing items
				break;
		}
	}

	// write new macro name list to file
	file->setValue<vector<string> >("MacroNames",macroNames);

	// now remove the macro definition from the file (??? if I could rename keys, then we could always write to a temporary name and name it for real afterwards)
	file->removeKey(macroName); 
}
