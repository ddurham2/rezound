#ifndef __CCompressorAction_H__
#define __CCompressorAction_H__
#include "../../config/common.h"

class CCompressorAction;
class CCompressorActionFactory;

#include "../AAction.h"

class CCompressorAction : public AAction
{
public:
	CCompressorAction(const CActionSound &actionSound,float windowTime,float threshold,float ratio,float attackTime,float releaseTime,bool syncChannels);

protected:
	bool doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound &actionSound);
	CanUndoResults canUndo(const CActionSound &actionSound) const;

private:
	const float windowTime;
	const float threshold;
	const float ratio;
	const float attackTime;
	const float releaseTime;
	const bool syncChannels;
};

class CCompressorActionFactory : public AActionFactory
{
public:
	CCompressorActionFactory(AActionDialog *channelSelectDialog,AActionDialog *normalDialog);

	CCompressorAction *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const;
};

#endif
