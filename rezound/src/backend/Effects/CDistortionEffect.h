#ifndef __CDistortionEffect_H__
#define __CDistortionEffect_H__
#include "../../config/common.h"

class CDistortionEffect;
class CDistortionEffectFactory;

#include "../AAction.h"
#include "../CGraphParamValueNode.h"

class CDistortionEffect : public AAction
{
public:
	CDistortionEffect(const CActionSound &actionSound,const CGraphParamValueNodeList &_curve);
	virtual ~CDistortionEffect();

protected:
	bool doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound &actionSound);
	CanUndoResults canUndo(const CActionSound &actionSound) const;

private:
	const CGraphParamValueNodeList curve;
};

class CDistortionEffectFactory : public AActionFactory
{
public:
	CDistortionEffectFactory(AActionDialog *channelSelectDialog,AActionDialog *normalDialog);
	virtual ~CDistortionEffectFactory();

	CDistortionEffect *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const;
};

#endif
