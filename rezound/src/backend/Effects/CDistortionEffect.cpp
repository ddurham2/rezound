#include "CDistortionEffect.h"

#include "../DSP/Distorter.h"

#include "../CActionSound.h"
#include "../CActionParameters.h"

CDistortionEffect::CDistortionEffect(const CActionSound &actionSound,const CGraphParamValueNodeList &_curve) :
	AAction(actionSound),
	curve(_curve)
{
}

bool CDistortionEffect::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	const sample_pos_t start=actionSound.start;
	const sample_pos_t stop=actionSound.stop;
	const sample_pos_t selectionLength=actionSound.selectionLength();

	if(prepareForUndo)
		moveSelectionToTempPools(actionSound,mmSelection,actionSound.selectionLength());

	for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
	{
		if(actionSound.doChannel[i])
		{
			CStatusBar statusBar("Distortion -- Channel "+istring(i),start,stop,true);

			sample_pos_t srcPos=prepareForUndo ? 0 : start;
			const CRezPoolAccesser src=prepareForUndo ? actionSound.sound->getTempAudio(tempAudioPoolKey,i) : actionSound.sound->getAudio(i);
			// now 'src' is an accessor either directly into the sound or into the temp pool created for undo
			// so it's range of indexes is either [start,stop] or [0,selectionLength) respectively

			sample_pos_t destPos=start;
			CRezPoolAccesser dest=actionSound.sound->getAudio(i);
			
			TDSPDistorter<sample_t,MAX_SAMPLE> d(curve,curve);

			while(destPos<=stop)
			{
				dest[destPos]=d.processSample(src[srcPos++]);

				if(statusBar.update(destPos))
				{ // cancelled
					if(prepareForUndo)
						undoActionSizeSafe(actionSound);
					else
						actionSound.sound->invalidatePeakData(i,actionSound.start,destPos);
					return false;
				}

				destPos++;
			}


			if(!prepareForUndo)
				actionSound.sound->invalidatePeakData(i,actionSound.start,actionSound.stop);
		}
	}

	// set the new selection points (only necessary if the length of the sound has changed)
	//actionSound.stop=actionSound.start+selectionLength-1;

	return(true);
}

AAction::CanUndoResults CDistortionEffect::canUndo(const CActionSound &actionSound) const
{
	return(curYes);
}

void CDistortionEffect::undoActionSizeSafe(const CActionSound &actionSound)
{
	restoreSelectionFromTempPools(actionSound,actionSound.start,actionSound.selectionLength());
}


// --------------------------------------------------
//
CDistortionEffectFactory::CDistortionEffectFactory(AActionDialog *channelSelectDialog,AActionDialog *normalDialog) :
	AActionFactory("Distortion","Distortion",false,channelSelectDialog,normalDialog,NULL)
{
}

CDistortionEffect *CDistortionEffectFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	//CGraphParamValueNodeList curve;
	//curve.push_back(CGraphParamValueNode(0.0,0.5));
	//curve.push_back(CGraphParamValueNode(0.5,0.5));
	//curve.push_back(CGraphParamValueNode(1.0,0.5));
	//curve.push_back(CGraphParamValueNode(1.0,0.0));
	return(new CDistortionEffect(actionSound,actionParameters->getGraphParameter("Curve")));
}


