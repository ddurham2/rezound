#include "CStubAction.h"

CStubAction::CStubAction(const CActionSound &actionSound) :
	AAction(actionSound)
{
}

bool CStubAction::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
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
			BEGIN_PROGRESS_BAR("Stub Action -- Channel "+istring(i),start,stop); 

			CRezPoolAccesser a=actionSound.sound->getAudio(i);

			if(prepareForUndo)
				a.copyData(start,actionSound.sound->getTempAudio(tempAudioPoolKey,i),0,selectionLength);


// --- Insert your test effect here -- BEGIN --------------------------------------------


						UPDATE_PROGRESS_BAR(t);

// --- Insert your test effect here -- END ----------------------------------------------


			if(!prepareForUndo)
				actionSound.sound->invalidatePeakData(i,actionSound.start,actionSound.stop);
			END_PROGRESS_BAR();
		}
	}

	return(true);
}

AAction::CanUndoResults CStubAction::canUndo(const CActionSound &actionSound) const
{
	return(curYes);
}

void CStubAction::undoActionSizeSafe(const CActionSound &actionSound)
{
	restoreSelectionFromTempPools(actionSound,actionSound.start,actionSound.selectionLength());
}


// --------------------------------------------------
//
CStubActionFactory::CStubActionFactory(AActionDialog *channelSelectDialog) :
	AActionFactory("Stub","Stub",false,channelSelectDialog,NULL,NULL)
{
}

CStubAction *CStubActionFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	return(new CStubAction(actionSound));
}


