#ifndef __CVariedRepeatEffect_H__
#define __CVariedRepeatEffect_H__
#include "../../../config/common.h"

class CVariedRepeatEffect;
class CVariedRepeatEffectFactory;

#include "../AAction.h"

class CVariedRepeatEffect : public AAction
{
public:
	CVariedRepeatEffect(const CActionSound &actionSound,float _LFOFreq,float _LFOPhase,float _time);

protected:
	bool doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound &actionSound);
	CanUndoResults canUndo(const CActionSound &actionSound) const;

private:
	float LFOFreq;
	float LFOPhase;
	float time;

	sample_pos_t origTotalLength;

};

class CVariedRepeatEffectFactory : public AActionFactory
{
public:
	CVariedRepeatEffectFactory(AActionDialog *channelSelectDialog,AActionDialog *normalDialog);

	CVariedRepeatEffect *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const;
};

#endif
