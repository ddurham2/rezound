#ifndef __CStubAction_H__
#define __CStubAction_H__
#include "../../config/common.h"

class CStubAction;
class CStubActionFactory;

#include "AAction.h"

class CStubAction : public AAction
{
public:
	CStubAction(const CActionSound &actionSound);
	virtual ~CStubAction();

protected:
	bool doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound &actionSound);
	CanUndoResults canUndo(const CActionSound &actionSound) const;

};

class CStubActionFactory : public AActionFactory
{
public:
	CStubActionFactory(AActionDialog *channelSelectDialog);
	virtual ~CStubActionFactory();

	CStubAction *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const;
};

#endif
