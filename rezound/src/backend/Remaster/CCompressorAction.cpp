#include "CCompressorAction.h"

#include <algorithm>

#include "../DSP/Compressor.h"
#include "../unit_conv.h"

#include "../CActionParameters.h"

CCompressorAction::CCompressorAction(const CActionSound &actionSound,float _windowTime,float _threshold,float _ratio,float _attackTime,float _releaseTime,float _inputGain,float _outputGain,bool _syncChannels) :
	AAction(actionSound),

	windowTime(_windowTime),
	threshold(_threshold),
	ratio(_ratio),
	attackTime(_attackTime),
	releaseTime(_releaseTime),
	inputGain(_inputGain),
	outputGain(_outputGain),
	syncChannels(_syncChannels)
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

	if(!syncChannels)
	{
		for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
		{
			if(actionSound.doChannel[i])
			{
				CStatusBar statusBar("Compressor -- Channel "+istring(i),start,stop,true);

				sample_pos_t srcPos=prepareForUndo ? 0 : start;
				const CRezPoolAccesser src=prepareForUndo ? actionSound.sound->getTempAudio(tempAudioPoolKey,i) : actionSound.sound->getAudio(i);

				sample_pos_t destPos=start;
				CRezPoolAccesser dest=actionSound.sound->getAudio(i);

				CDSPCompressor compressor(
					ms_to_samples(windowTime,actionSound.sound->getSampleRate()),
					dBFS_to_amp(threshold),
					ratio,
					ms_to_samples(attackTime,actionSound.sound->getSampleRate()),
					ms_to_samples(releaseTime,actionSound.sound->getSampleRate())
				);

				// initialize the compressor's level detector
				//while((destPos++)<min(stop,4000))
					//compressor.initSample(src[srcPos++]);

				while(destPos<=stop)
				{
					
					const mix_sample_t s=(mix_sample_t)(src[srcPos++]*inputGain);
					dest[destPos]=ClipSample(compressor.processSample(s,s)*outputGain);

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
	}
	else // the level should be determined from all the samples in a frame but all frames in a sample should be affected by that singular level
	{
		CDSPCompressor	compressor(
			ms_to_samples(windowTime,actionSound.sound->getSampleRate()),
			dBFS_to_amp(threshold),
			ratio,
			ms_to_samples(attackTime,actionSound.sound->getSampleRate()),
			ms_to_samples(releaseTime,actionSound.sound->getSampleRate())
		);

		sample_pos_t srcPos=prepareForUndo ? 0 : start;
		const CRezPoolAccesser *srces[MAX_CHANNELS]; // ??? I could be more efficient by being able to call operator= for elements of an existing array of accessors so I didn't have to do the extra dereference

		sample_pos_t destPos=start;
		CRezPoolAccesser *dests[MAX_CHANNELS]; // ??? I could be more efficient by being able to call operator= for elements of an existing array of accessors so I didn't have to do the extra dereference

		size_t channelCount=0;
		for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
		{
			if(actionSound.doChannel[i])
			{
				srces[channelCount]=new CRezPoolAccesser(prepareForUndo ? actionSound.sound->getTempAudio(tempAudioPoolKey,i) : actionSound.sound->getAudio(i));
				dests[channelCount]=new CRezPoolAccesser(actionSound.sound->getAudio(i));
				channelCount++;
			}
		}

		try
		{
			CStatusBar statusBar("Compressor ",start,stop,true);

			// initialize the compressor's level detector
			//while((destPos++)<min(stop,4000))
				//compressor.initSample(src[srcPos++]);

			mix_sample_t inputFrame[MAX_CHANNELS];
			while(destPos<=stop)
			{
				for(unsigned t=0;t<channelCount;t++)
					inputFrame[t]=(mix_sample_t)((*(srces[t]))[srcPos]*inputGain);
				srcPos++;

				compressor.processSampleFrame(inputFrame,channelCount);

				for(unsigned t=0;t<channelCount;t++)
					(*(dests[t]))[destPos]=ClipSample(inputFrame[t]*outputGain);
		
				if(statusBar.update(destPos))
				{ // cancelled
					if(prepareForUndo)
						undoActionSizeSafe(actionSound);
					// cleanup
					for(size_t t=0;t<channelCount;t++)
					{
						if(!prepareForUndo)
							actionSound.sound->invalidatePeakData(t,actionSound.start,destPos);

						delete srces[t];
						delete dests[t];
					}
					return false;
				}

				destPos++;
			}

			for(size_t t=0;t<channelCount;t++)
			{
				if(!prepareForUndo)
					actionSound.sound->invalidatePeakData(t,actionSound.start,actionSound.stop);

				delete srces[t];
				delete dests[t];
			}
		}
		catch(...)
		{
			for(size_t t=0;t<channelCount;t++)
			{
				delete srces[t];
				delete dests[t];
			}
			throw;
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
CCompressorActionFactory::CCompressorActionFactory(AActionDialog *channelSelectDialog,AActionDialog *normalDialog) :
	AActionFactory("Dynamic Range Compressor","Dynamic Range Compressor",false,channelSelectDialog,normalDialog,NULL)
{
}

CCompressorAction *CCompressorActionFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	return(new CCompressorAction(
		actionSound,
		actionParameters->getDoubleParameter("Window Time"),
		actionParameters->getDoubleParameter("Threshold"),
		actionParameters->getDoubleParameter("Ratio"),
		actionParameters->getDoubleParameter("Attack Time"),
		actionParameters->getDoubleParameter("Release Time"),
		actionParameters->getDoubleParameter("Input Gain"),
		actionParameters->getDoubleParameter("Output Gain"),
		true//actionParameters->getBoolParameter("Lock Channels") ??? need to implement a checkbox parameter on the frontend --> I've done this now.. now I need to use it!
	));
}


