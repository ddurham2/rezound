#ifndef __CRunMacroAction_H__
#define __CRunMacroAction_H__
#include "../../config/common.h"

class CRunMacroAction;
class CRunMacroActionFactory;

#include "AAction.h"
class ASoundFileManager;

class CRunMacroAction : public AAction
{
public:
	CRunMacroAction(const AActionFactory *factory,const CActionSound *actionSound,ASoundFileManager *soundFileManager,const string macroName);
	virtual ~CRunMacroAction();

protected:
	bool doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound *actionSound);
	CanUndoResults canUndo(const CActionSound *actionSound) const;

private:
	ASoundFileManager *soundFileManager;
	const string macroName;
	unsigned actionCount;

};

class CRunMacroActionFactory : public AActionFactory
{
public:
	CRunMacroActionFactory(AActionDialog *dialog);
	virtual ~CRunMacroActionFactory();

	CRunMacroAction *manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const;
};

#endif
