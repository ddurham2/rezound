#ifndef __CCompressorAction_H__
#define __CCompressorAction_H__
#include "../../config/common.h"

class CCompressorAction;
class CCompressorActionFactory;

#include "../AAction.h"

class CCompressorAction : public AAction
{
public:
	CCompressorAction(const CActionSound &actionSound);

protected:
	bool doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound &actionSound);
	CanUndoResults canUndo(const CActionSound &actionSound) const;

};

class CCompressorActionFactory : public AActionFactory
{
public:
	CCompressorActionFactory(AActionDialog *channelSelectDialog);

	CCompressorAction *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const;
};

#endif
