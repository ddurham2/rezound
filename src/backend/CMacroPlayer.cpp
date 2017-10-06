#include "CMacroPlayer.h"
#include "settings.h"
#include "AAction.h"
#include "ASoundFileManager.h"
#include "CActionParameters.h"
#include "CLoadedSound.h"
#include "CSoundPlayerChannel.h"
#include "AFrontendHooks.h"
#include "ASoundClipboard.h"

#include <CNestedDataFile/CNestedDataFile.h>
#include <TTempVarSetter.h>

#include <algorithm>
#include <map>
using namespace std;

const vector<string> CMacroPlayer::getMacroNames(const CNestedDataFile *macroStore)
{
	return macroStore->getValue<vector<string> >("MacroNames");
}

CMacroPlayer::CMacroPlayer(const CNestedDataFile *_macroStore,const string _macroName) :
	macroStore(_macroStore),
	macroName(_macroName)
{
}

CMacroPlayer::~CMacroPlayer()
{
}

/*
 * returns true iff all actions succeed
 * returns false when the first action returns false
 * count will be updated to contain the number of actions that succeeded (and went on the undoStack) in either case
 */
bool CMacroPlayer::doMacro(ASoundFileManager *soundFileManager)
{
	// ??? it would be nice if it were possible only to undo the whole macro and only use one undo space rather than each action doing its own .. perhaps the prepareForUndoo flag could finally come in handy
	printf("%s -- running macro: %s\n",__func__,macroName.c_str());

	map<CLoadedSound *,unsigned> actionCountsPerSound;
	try
	{
		// for each action in the macro
		unsigned actionCount=macroStore->getValue<unsigned>(macroName DOT "actionCount");
		bool ret=true;
		for(unsigned actionIndex=0;actionIndex<actionCount;actionIndex++)
		{
			const string key=macroName DOT "action"+istring(actionIndex,3,true);

			// retrieve the action name (that must match one of the registered action names)
			const string actionName=macroStore->getValue<string>(key DOT "actionName");

			if(macroStore->keyExists(key DOT "activeSoundIndex"))
			{
				const size_t activeSoundIndex=macroStore->getValue<size_t>(key DOT "activeSoundIndex");
				if(activeSoundIndex>=soundFileManager->getOpenedCount())
				{ // index is out of range (could prompt for which sound to set to active at this point rather than bailing completely)
					Error(_("At this point when the macro was recorded the selected sound was changed to position: ")+istring(activeSoundIndex+1)+"\n\n"+_("No audio file is loaded in this position now as the macro is being played back.\n\nThe macro playback is bailing."));
					ret=false;
					break;
				}

				soundFileManager->setActiveSound(activeSoundIndex);
			}

			printf("action to run: %s\n",actionName.c_str());
			if(gRegisteredActionFactories.find(actionName)==gRegisteredActionFactories.end())
				throw runtime_error(string(__func__)+" -- action not found with name: "+actionName);

			AActionFactory *actionFactory=gRegisteredActionFactories[actionName];

			CLoadedSound *loadedSound=soundFileManager->getActive();

			// set the start and stop positions according to how the macro says
			if(loadedSound && macroStore->getValue<bool>(key DOT "selectionPositionsAreApplicable"))
			{
				sample_fpos_t startPosition=loadedSound->channel->getStartPosition();
				sample_fpos_t stopPosition=loadedSound->channel->getStopPosition();
				const sample_fpos_t audioLength=loadedSound->sound->getLength();
				const sample_fpos_t selectionLength=stopPosition-startPosition;

				const sample_fpos_t recStartPosition=macroStore->getValue<sample_pos_t>(key DOT "positioning" DOT "startPosition");
				const sample_fpos_t recStopPosition=macroStore->getValue<sample_pos_t>(key DOT "positioning" DOT "stopPosition");
				const sample_fpos_t recAudioLength=macroStore->getValue<sample_pos_t>(key DOT "positioning" DOT "audioLength");
				const sample_fpos_t recSelectionLength=recStopPosition-recStartPosition;

				const AFrontendHooks::MacroActionParameters::SelectionPositioning recStartPosPositioning=(AFrontendHooks::MacroActionParameters::SelectionPositioning)macroStore->getValue<unsigned>(key DOT "positioning" DOT "startPosPositioning");
				const AFrontendHooks::MacroActionParameters::SelectionPositioning recStopPosPositioning=(AFrontendHooks::MacroActionParameters::SelectionPositioning)macroStore->getValue<unsigned>(key DOT "positioning" DOT "stopPosPositioning");

				const string recStartPosCueName=macroStore->getValue<string>(key DOT "positioning" DOT "startPosCueName");
				const string recStopPosCueName=macroStore->getValue<string>(key DOT "positioning" DOT "stopPosCueName");

				size_t cueIndex;
				switch(recStartPosPositioning)
				{
					case AFrontendHooks::MacroActionParameters::spLeaveAlone: 
						break;
					case AFrontendHooks::MacroActionParameters::spAbsoluteTimeFromBeginning: 
						startPosition=recStartPosition;
						break;
					case AFrontendHooks::MacroActionParameters::spAbsoluteTimeFromEnd: 
						startPosition=audioLength-(recAudioLength-recStartPosition);
						break;
					case AFrontendHooks::MacroActionParameters::spProportionateTimeFromBeginning: 
						startPosition=(audioLength-1)/((recAudioLength-1)/recStartPosition);
						break;
					case AFrontendHooks::MacroActionParameters::spAbsoluteTimeFromStopPosition: 
						// handled after the stop position is calculated
						break;
					case AFrontendHooks::MacroActionParameters::spProportionateTimeFromStopPosition: 
						// handled after the stop position is calculated
						break;
					case AFrontendHooks::MacroActionParameters::spSameCueName: 
						if(loadedSound->sound->containsCue(recStartPosCueName,cueIndex))
							startPosition=loadedSound->sound->getCueTime(cueIndex);
						else
							Warning(_("The macro being played was flagged to set the start position to the position of a cue with the name: ")+recStartPosCueName+"\n\n"+_("However, no cue by that name can be found.  The start position will be left unchanged."));
						break;
					default:
						throw runtime_error(string(__func__)+" -- unhandled startPositioning: "+istring(recStartPosPositioning));
				}

				switch(recStopPosPositioning)
				{
					case AFrontendHooks::MacroActionParameters::spLeaveAlone: 
						break;
					case AFrontendHooks::MacroActionParameters::spAbsoluteTimeFromBeginning: 
						stopPosition=recStopPosition;
						break;
					case AFrontendHooks::MacroActionParameters::spAbsoluteTimeFromEnd: 
						stopPosition=audioLength-(recAudioLength-recStopPosition);
						break;
					case AFrontendHooks::MacroActionParameters::spProportionateTimeFromBeginning: 
						stopPosition=(audioLength-1)/((recAudioLength-1)/recStopPosition);
						break;
					case AFrontendHooks::MacroActionParameters::spAbsoluteTimeFromStartPosition: 
						stopPosition=startPosition+(recSelectionLength);
						break;
					case AFrontendHooks::MacroActionParameters::spProportionateTimeFromStartPosition: 
						stopPosition=startPosition+(audioLength/(recAudioLength/recSelectionLength));
						break;
					case AFrontendHooks::MacroActionParameters::spSameCueName: 
						if(loadedSound->sound->containsCue(recStopPosCueName,cueIndex))
							stopPosition=loadedSound->sound->getCueTime(cueIndex);
						else
							Warning(_("The macro being played was flagged to set the stop position to the position of a cue with the name: ")+recStopPosCueName+"\n\n"+_("However, no cue by that name can be found.  The stop position will be left unchanged."));
						break;
					default:
						throw runtime_error(string(__func__)+" -- unhandled startPositioning: "+istring(recStopPosPositioning));
				}

					// these two cases have to be done *after* the stop position is calcuated
				switch((int/*avoiding gcc warning about missing cases*/)recStartPosPositioning)
				{
					case AFrontendHooks::MacroActionParameters::spAbsoluteTimeFromStopPosition: 
						startPosition=stopPosition-(recSelectionLength);
						break;
					case AFrontendHooks::MacroActionParameters::spProportionateTimeFromStopPosition: 
						startPosition=stopPosition-(audioLength/(recAudioLength/recSelectionLength));
						break;
				}

				// make sure the positions are within range
				startPosition=max((sample_fpos_t)0,min(audioLength,startPosition));
				stopPosition=max((sample_fpos_t)0,min(audioLength,stopPosition));

				if(startPosition>stopPosition)
				{ // swap the positions if they come out backwards
					sample_fpos_t t=startPosition;
					startPosition=stopPosition;
					stopPosition=t;
				}

					// it is not supposed to matter what order these are called despite where the positions may currently be
				loadedSound->channel->setStartPosition((sample_pos_t)sample_fpos_floor(startPosition));
				loadedSound->channel->setStopPosition((sample_pos_t)sample_fpos_ceil(stopPosition));
			}
			
			// retrieve the parameters for doing the action
			CActionParameters actionParameters(soundFileManager);
			actionParameters.readFromFile(macroStore,key DOT "parameters");

			// perhaps see if the user wants to use the action parameters stored in the macro or not
			bool actionParametersFilledOut=true;
			if(macroStore->getValue<bool>(key DOT "askToPromptForActionParametersAtPlayback") && actionFactory->hasDialog())
			{
				if(Question(_("Action: ")+actionName+"\n\n"+_("This action in the macro was marked to ask this question when the macro was recorded.\n\nWould you like to use choose new action parameters now, overriding the ones chosen when the macro was recorded?"),yesnoQues)==yesAns)
					actionParametersFilledOut=false;
			}

			// set the clipboard to the same one as when the macro was recorded
			{
				const string clipboardDesc=macroStore->getValue<string>(key DOT "selectedClipboardDescription");
				if(clipboardDesc!="")
				{
					bool found=false;
					for(size_t t=0;t<AAction::clipboards.size();t++)
					{
						if(AAction::clipboards[t]->getDescription()==clipboardDesc)
						{
							gFrontendHooks->setWhichClipboard(t);
							found=true;
							break;
						}
					}
					if(!found)
						Warning(_("When the macro was recorded a clipboard existed with the description: ")+clipboardDesc+"\n\n"+_("However, now there is no clipboard with that description."));
				}
			}

			// backup and temporarily set the crossfade settings
			TTempVarSetter<CrossfadeEdgesTypes> t1(gCrossfadeEdges, (CrossfadeEdgesTypes)macroStore->getValue<unsigned>(key DOT "positioning" DOT "crossfadeEdges") );
			TTempVarSetter<float> t2(gCrossfadeStartTime, macroStore->getValue<float>(key DOT "positioning" DOT "crossfadeStartTime") );
			TTempVarSetter<float> t3(gCrossfadeStopTime, macroStore->getValue<float>(key DOT "positioning" DOT "crossfadeStopTime") );
			TTempVarSetter<CrossfadeFadeMethods> t4(gCrossfadeFadeMethod, (CrossfadeFadeMethods)macroStore->getValue<unsigned>(key DOT "positioning" DOT "crossfadeFadeMethod") );

			bool wentOntoUndoStack=false; /*??? this value isn't really necessary.. I could just see if the stack's size changed and assume from that*/

			// do the action
			if(!actionFactory->performAction(loadedSound,&actionParameters,false,actionParametersFilledOut,wentOntoUndoStack))
			{
				if(dynamic_cast<CRunMacroActionFactory *>(actionFactory)==NULL)
				{ // only maybe ask this question if it wasn't a CRunMacroActionFactory that returned false (which we do on purpose)

					if((actionIndex+1)<actionCount)
					{ // there are still more actions to go
						if(Question(_("The action did not succeed.\n\nDo you want to continue running the macro's remaining actions?"),yesnoQues)!=yesAns)
						{
							ret=false;
							break;
						}
					}
					else
					{
						ret=false;
						break;
					}
				}
			}

			if(actionFactory->requiresALoadedSound && wentOntoUndoStack)
				actionCountsPerSound[loadedSound]++;
		}

		addRanMacroActions(actionCountsPerSound,soundFileManager,macroName);
		return ret;
	}
	catch(...)
	{
		addRanMacroActions(actionCountsPerSound,soundFileManager,macroName);
		throw;
	}

	return true;
}

void CMacroPlayer::addRanMacroActions(const map<CLoadedSound *,unsigned> &actionCountsPerSound,ASoundFileManager *soundFileManager,const string macroName)
{
	for(map<CLoadedSound *,unsigned>::const_iterator i=actionCountsPerSound.begin(); i!=actionCountsPerSound.end(); i++)
	{
		if(i->second>1)
		{ // "run" a CRanMacroAction on the sound so that undo will prompt for undoing all the macro's actions or just one
			CActionParameters actionParameters(soundFileManager);
			actionParameters.setValue<string>("Macro Name",macroName);
			actionParameters.setValue<unsigned>("ActionCount",i->second);
			
			ranMacroActionFactory.performAction(i->first,&actionParameters,true);
		}
	}
}
