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
	// curve's x's and y's should be in dBFS
	CDistortionEffect(const CActionSound &actionSound,const CGraphParamValueNodeList &_curve);
	virtual ~CDistortionEffect();

protected:
	bool doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound &actionSound);
	CanUndoResults canUndo(const CActionSound &actionSound) const;

private:
	CGraphParamValueNodeList curve;
};

class CDistortionEffectFactory : public AActionFactory
{
public:
	CDistortionEffectFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog);
	virtual ~CDistortionEffectFactory();

	CDistortionEffect *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const;
};

#endif
