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
	macroName(_macroName)
{
}

CRunMacroAction::~CRunMacroAction()
{
}

bool CRunMacroAction::doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo)
{
	CMacroPlayer p(gUserMacroStore,macroName);
	p.doMacro(soundFileManager);
	return false; // pretend it was cancelled so this object doesn't go onto the undo stack
}

AAction::CanUndoResults CRunMacroAction::canUndo(const CActionSound *actionSound) const
{
	return curYes; // doesn't matter.. always returning false from doActionSizeSafe anyway
}

void CRunMacroAction::undoActionSizeSafe(const CActionSound *actionSound)
{
	throw runtime_error(string(__func__)+" -- should not be called");
}


// --------------------------------------------------

CRunMacroActionFactory::CRunMacroActionFactory(AActionDialog *dialog) :
	AActionFactory(N_("Run Macro"),"",NULL,dialog,false,false)
{
	requiresALoadedSound=false;
}

CRunMacroActionFactory::~CRunMacroActionFactory()
{
}

CRunMacroAction *CRunMacroActionFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return new CRunMacroAction(this,actionSound,actionParameters->getSoundFileManager(),actionParameters->getValue<string>("Macro Name"));
}



// --- for undoing a macro --------------------------------------------

CRanMacroAction::CRanMacroAction(const AActionFactory *factory,const CActionSound *actionSound,ASoundFileManager *_soundFileManager,const string _macroName,unsigned _actionCount) :
	AAction(factory,actionSound),
	soundFileManager(_soundFileManager),
	macroName(_macroName),
	actionCount(_actionCount)
{
}

CRanMacroAction::~CRanMacroAction()
{
}

bool CRanMacroAction::doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo)
{
	// just running this for the ability to run undoActionSizeSafe
	return true;
}

AAction::CanUndoResults CRanMacroAction::canUndo(const CActionSound *actionSound) const
{
	return curYes;
}

#include "main_controls.h"
void CRanMacroAction::undoActionSizeSafe(const CActionSound *actionSound)
{
	/* we call undo() from main_controls.h which pops the action from the stack before starting the undo process */

	// .. need to manipulate the undo stack and we are currently in the undo stack ..
	if(undoActionRecursionCount<=1)
	{
		// as the user whether to undo all actions or just the last action in the macro (if actionCount>1) need to undo actionCount actions
		if(Question(_("Macro: ")+macroName+"\n\n"+_("Undo all actions that the macro contributed?  Else, undo only the last action performed from this macro."),yesnoQues)==yesAns)
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

// -------------------------------------------------------------------------------

CRanMacroActionFactory::CRanMacroActionFactory() :
	AActionFactory(N_("Ran Macro"),"",NULL,NULL,false,false)
{
}

CRanMacroActionFactory::~CRanMacroActionFactory()
{
}

CRanMacroAction *CRanMacroActionFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return new CRanMacroAction(this,actionSound,actionParameters->getSoundFileManager(),actionParameters->getValue<string>("Macro Name"),actionParameters->getValue<unsigned>("ActionCount"));
}

