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

	bool doesWarrantSaving() const { return false; }

private:
	ASoundFileManager *soundFileManager;
	const string macroName;

};

class CRunMacroActionFactory : public AActionFactory
{
public:
	CRunMacroActionFactory(AActionDialog *dialog);
	virtual ~CRunMacroActionFactory();

	CRunMacroAction *manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const;
};

// ---------------------------------------------------------------

class CRanMacroAction : public AAction
{
public:
	CRanMacroAction(const AActionFactory *factory,const CActionSound *actionSound,ASoundFileManager *soundFileManager,const string macroName,unsigned actionCount);
	virtual ~CRanMacroAction();

protected:
	bool doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound *actionSound);
	CanUndoResults canUndo(const CActionSound *actionSound) const;

	bool doesWarrantSaving() const { return false; }

private:
	ASoundFileManager *soundFileManager;
	const string macroName;
	unsigned actionCount;

};

class CRanMacroActionFactory : public AActionFactory
{
public:
	CRanMacroActionFactory();
	virtual ~CRanMacroActionFactory();

	CRanMacroAction *manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const;
};
#endif
