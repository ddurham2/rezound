#ifndef __CStubAction_H__
#define __CStubAction_H__

class CStubAction;
class CStubActionFactory;

#include "AAction.h"

class CStubAction : public AAction
{
public:
	CStubAction(const CActionSound &actionSound);

protected:
	bool doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound &actionSound);
	CanUndoResults canUndo(const CActionSound &actionSound) const;

};

class CStubActionFactory : public AActionFactory
{
public:
	CStubActionFactory(AActionDialog *channelSelectDialog);

	CStubAction *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const;
};

#endif
