#include "CRunMacroAction.h"

#include "settings.h"
#include "CActionSound.h"
#include "CActionParameters.h"
#include "AStatusComm.h"

#include "ASoundFileManager.h"
#include "CLoadedSound.h"
#include "CSoundPlayerChannel.h"

#include <CPath.h>
#include "CMacroPlayer.h"

CRunMacroAction::CRunMacroAction(const AActionFactory *factory,const CActionSound *actionSound,ASoundFileManager *_soundFileManager,const string _macroName) :
	AAction(factory,actionSound),
	soundFileManager(_soundFileManager),
	macroName(_macroName),
	actionCount(0)
{
}

CRunMacroAction::~CRunMacroAction()
{
}

bool CRunMacroAction::doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo)
{
	CMacroPlayer p(gUserMacroStore,macroName);
	p.doMacro(soundFileManager,actionCount);
	if(actionCount>0)
	{
		if(actionSound)
		{
			// set our start and stop positions the same as the last action set them
			// and for lack of a better way, obtain these values like this ...
			for(size_t t=0;t<soundFileManager->getOpenedCount();t++)
			{
				if(soundFileManager->getSound(t)->sound==actionSound->sound)
				{
					actionSound->start=soundFileManager->getSound(t)->channel->getStartPosition();
					actionSound->stop=soundFileManager->getSound(t)->channel->getStopPosition();
				}
			}
		}
		return true;
	}
	else
		return false; // no sense in going on the undo stack if we nothing was done
}

AAction::CanUndoResults CRunMacroAction::canUndo(const CActionSound *actionSound) const
{
	return curYes; // hmm.. maybe.. depends on the macro
}

#include "main_controls.h"
void CRunMacroAction::undoActionSizeSafe(const CActionSound *actionSound)
{
	/* we call undo() from main_controls.h which pops the action from the stack before starting the undo process */
	if(actionCount>0)
	{
		// .. need to manipulate the undo stack and we are currently in the undo stack ..
		if(undoActionRecursionCount<=1 && actionCount>1)
		{
			// as the user whether to undo all actions or just the last action in the macro (if actionCount>1) need to undo actionCount actions
			if(Question("Undo all actions in macro '"+macroName+"'?  Else, undo only the last action performed in this macro.",yesnoQues)==yesAns)
			{
				// undo all actions
				for(unsigned t=0;t<actionCount;t++)
					undo(soundFileManager); // I really ought to passed the specify CLoadedSound into undo() because right now it just uses the 'active' sound (which isn't necessarily the sound we're working on)
			}
			else
			{
				// undo one action
				undo(soundFileManager); // I really ought to passed the specify CLoadedSound into undo() because right now it just uses the 'active' sound (which isn't necessarily the sound we're working on)
			}
		}
		else
		{
			// undo all actions
			for(unsigned t=0;t<actionCount;t++)
				undo(soundFileManager); // I really ought to passed the specify CLoadedSound into undo() because right now it just uses the 'active' sound (which isn't necessarily the sound we're working on)
		}
	}
}


// --------------------------------------------------

CRunMacroActionFactory::CRunMacroActionFactory(AActionDialog *dialog) :
	AActionFactory(N_("Run Macro"),"",NULL,dialog,false,false)
{
	setLockSoundMutex(false); 
	requiresALoadedSound=false;
}

CRunMacroActionFactory::~CRunMacroActionFactory()
{
}

CRunMacroAction *CRunMacroActionFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return new CRunMacroAction(this,actionSound,actionParameters->getSoundFileManager(),actionParameters->getStringParameter("Macro Name"));
}


