#include "CVariedRepeatEffect.h"

#include <stdexcept>

//#include "../CActionSound.h"
#include "../CActionParameters.h"

#include "../unit_conv.h"
#include "../ALFO.h"


CVariedRepeatEffect::CVariedRepeatEffect(const CActionSound &actionSound,float _LFOFreq,float _LFOPhase,float _time) :
	AAction(actionSound),

	LFOFreq(_LFOFreq),

	LFOPhase(_LFOPhase),
	
	time(_time),

	origTotalLength(0)
{
	// want also to be able to mix onto the existing data

	if(_time<0.0)
		throw(runtime_error(string(__func__)+" -- _time is negative"));
}

// and have options for mixing on top of what's already there

bool CVariedRepeatEffect::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	const sample_pos_t start=actionSound.start;
	const sample_pos_t stop=actionSound.stop;
	const sample_pos_t selectionLength=actionSound.selectionLength();
	const sample_pos_t lTime=s_to_samples(time,actionSound.sound->getSampleRate());

	moveSelectionToTempPools(actionSound,mmSelection,lTime);

/*
	origTotalLength=actionSound.sound->getLength();

	if(lTime>selectionLength)
		actionSound.sound->addSpace(actionSound.doChannel,stop,lTime-selectionLength);
*/

	for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
	{
		if(actionSound.doChannel[i])
		{

			BEGIN_PROGRESS_BAR("Varied Repeat -- Channel "+istring(i),0,lTime); 

			const CRezPoolAccesser src=actionSound.sound->getTempAudio(tempAudioPoolKey,i);
			CRezPoolAccesser dest=actionSound.sound->getAudio(i);

/*
			if(prepareForUndo)
				a.copyData(start,actionSound.sound->getTempAudio(tempAudioPoolKey,i),0,selectionLength);
*/

			CPosSinLFO LFO(LFOFreq,LFOPhase,actionSound.sound->getSampleRate());

			for(sample_pos_t t=0;t<lTime;t++)
			{
				const sample_pos_t repeat=(sample_pos_t)(LFO.getValue(t)*(selectionLength));
				for(sample_pos_t p=0;p<repeat && t<lTime;p++,t++)
					dest[start+t]=src[p];

				UPDATE_PROGRESS_BAR(t);
			}

			END_PROGRESS_BAR();
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
//
CVariedRepeatEffectFactory::CVariedRepeatEffectFactory(AActionDialog *channelSelectDialog,AActionDialog *normalDialog) :
	AActionFactory("Varied Repeat","Varied Repeat",false,channelSelectDialog,normalDialog,NULL)
{
}

CVariedRepeatEffect *CVariedRepeatEffectFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	//return(new CVariedRepeatEffect(actionSound,0.1,280.0,5.0));
	return(new CVariedRepeatEffect(
		actionSound,
		actionParameters->getDoubleParameter("LFO Freq"),
		actionParameters->getDoubleParameter("LFO Phase"),
		actionParameters->getDoubleParameter("Time")
		)
	);
}


