#ifndef __CVariedRepeatEffect_H__
#define __CVariedRepeatEffect_H__
#include "../../../config/common.h"

class CVariedRepeatEffect;
class CVariedRepeatEffectFactory;

#include "../AAction.h"
#include "../ALFO.h"

class CVariedRepeatEffect : public AAction
{
public:
	CVariedRepeatEffect(const AActionFactory *factory,const CActionSound *actionSound,const CLFODescription &LFODescription,float _time);
	virtual ~CVariedRepeatEffect();

protected:
	bool doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound *actionSound);
	CanUndoResults canUndo(const CActionSound *actionSound) const;

private:
	CLFODescription LFODescription;
	float time;

	sample_pos_t origTotalLength;

};

class CVariedRepeatEffectFactory : public AActionFactory
{
public:
	CVariedRepeatEffectFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog);
	virtual ~CVariedRepeatEffectFactory();

	CVariedRepeatEffect *manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const;
};

#endif
