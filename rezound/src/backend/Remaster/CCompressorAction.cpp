#include "CCompressorAction.h"

#include <algorithm>

#include "../DSPBlocks.h"

CCompressorAction::CCompressorAction(const CActionSound &actionSound) :
	AAction(actionSound)
{
}

bool CCompressorAction::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	const sample_pos_t start=actionSound.start;
	const sample_pos_t stop=actionSound.stop;
	const sample_pos_t selectionLength=actionSound.selectionLength();

	// ??? boy... if AAction had it abstracted out better so that I could make as many backups of sections as I wanted, I could essientially backup only the sections that need to be ... except then I would need feedback from the compressor DSP block to know when compression started and stopped... and it wouldn't be for all channels... and... I'd have to 
	// well.. then I would have to look ahead at the treshold crossings... it would be harder than I think to do undo efficently ... but it would be interesting.. maybe I could make two passes thru the data

	if(prepareForUndo)
		moveSelectionToTempPools(actionSound,mmSelection,actionSound.selectionLength());

	for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
	{
		if(actionSound.doChannel[i])
		{
			BEGIN_PROGRESS_BAR("Compressor -- Channel "+istring(i),start,stop); 

			sample_pos_t srcPos=prepareForUndo ? 0 : start;
			const CRezPoolAccesser src=prepareForUndo ? actionSound.sound->getTempAudio(tempAudioPoolKey,i) : actionSound.sound->getAudio(i);

			sample_pos_t destPos=start;
			CRezPoolAccesser dest=actionSound.sound->getAudio(i);

			CDSPCompressor compressor(8000,6000,/*0.5*/1.0/1.585,4000,200000);

			// initialize the compressor's level detector
			//while((destPos++)<min(stop,4000))
				//compressor.initSample(src[srcPos++]);

			while(destPos<=stop)
			{
				dest[destPos]=compressor.processSample(src[srcPos++]);	
				UPDATE_PROGRESS_BAR(destPos);
				destPos++;
			}


			if(!prepareForUndo)
				actionSound.sound->invalidatePeakData(i,actionSound.start,actionSound.stop);
			END_PROGRESS_BAR();
		}
	}

	// set the new selection points (only necessary if the length of the sound has changed)
	//actionSound.stop=actionSound.start+selectionLength-1;

	return(true);
}

AAction::CanUndoResults CCompressorAction::canUndo(const CActionSound &actionSound) const
{
	return(curYes);
}

void CCompressorAction::undoActionSizeSafe(const CActionSound &actionSound)
{
	restoreSelectionFromTempPools(actionSound,actionSound.start,actionSound.selectionLength());
}


// --------------------------------------------------
//
CCompressorActionFactory::CCompressorActionFactory(AActionDialog *channelSelectDialog) :
	AActionFactory("Compressor","Compressor",false,channelSelectDialog,NULL,NULL)
{
}

CCompressorAction *CCompressorActionFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	return(new CCompressorAction(actionSound));
}


