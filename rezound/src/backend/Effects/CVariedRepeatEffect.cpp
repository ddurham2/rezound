#include "CVariedRepeatEffect.h"

#include <memory>

#include "../CActionParameters.h"

#include "../unit_conv.h"


CVariedRepeatEffect::CVariedRepeatEffect(const CActionSound &actionSound,const CLFODescription &_LFODescription,float _time) :
	AAction(actionSound),
	LFODescription(_LFODescription),
	time(_time),

	origTotalLength(0)
{
	// want also to be able to mix onto the existing data

	if(_time<0.0)
		throw(runtime_error(string(__func__)+" -- _time is negative"));
}

CVariedRepeatEffect::~CVariedRepeatEffect()
{
}

// and have options for mixing on top of what's already there

bool CVariedRepeatEffect::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	const sample_pos_t start=actionSound.start;
	const sample_pos_t stop=actionSound.stop;
	const sample_pos_t selectionLength=actionSound.selectionLength();
	const sample_pos_t lTime=s_to_samples(time,actionSound.sound->getSampleRate());

	moveSelectionToTempPools(actionSound,mmSelection,lTime);

	unsigned channelsDoneCount=0;
	for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
	{
		if(actionSound.doChannel[i])
		{
			CStatusBar statusBar(_("Varied Repeat -- Channel ")+istring(++channelsDoneCount)+"/"+istring(actionSound.countChannels()),0,lTime,true); 

			const CRezPoolAccesser src=actionSound.sound->getTempAudio(tempAudioPoolKey,i);
			CRezPoolAccesser dest=actionSound.sound->getAudio(i);

			auto_ptr<ALFO> LFO(gLFORegistry.createLFO(LFODescription,actionSound.sound->getSampleRate()));
			
			for(sample_pos_t t=0;t<lTime;t++)
			{
				const sample_pos_t repeat=(sample_pos_t)(LFO->getValue(t)*(selectionLength));
				for(sample_pos_t p=0;p<repeat && t<lTime;p++,t++)
					dest[start+t]=src[p];

				if(statusBar.update(t))
				{ // cancelled
					restoreSelectionFromTempPools(actionSound,actionSound.start,lTime);
					return false;
				}
			}

			if(!prepareForUndo)
				actionSound.sound->invalidatePeakData(i,actionSound.start,actionSound.stop);
		}
	}

	actionSound.stop=actionSound.start+lTime-1;

	if(!prepareForUndo)
		freeAllTempPools();

	return(true);
}

AAction::CanUndoResults CVariedRepeatEffect::canUndo(const CActionSound &actionSound) const
{
	return(curYes);
}

void CVariedRepeatEffect::undoActionSizeSafe(const CActionSound &actionSound)
{
	const sample_pos_t lTime=s_to_samples(time,actionSound.sound->getSampleRate());
	restoreSelectionFromTempPools(actionSound,actionSound.start,lTime);
}


// --------------------------------------------------

CVariedRepeatEffectFactory::CVariedRepeatEffectFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog) :
	AActionFactory(N_("Varied Repeat"),_("Repeat the Selection Variably According to an LFO"),channelSelectDialog,dialog)
{
}

CVariedRepeatEffectFactory::~CVariedRepeatEffectFactory()
{
}

CVariedRepeatEffect *CVariedRepeatEffectFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return(new CVariedRepeatEffect(
		actionSound,
		actionParameters->getLFODescription("LFO"),
		actionParameters->getDoubleParameter("Time")
		)
	);
}


