/* 
 * Copyright (C) 2002 - David W. Durham
 * 
 * This file is part of ReZound, an audio editing application.
 * 
 * ReZound is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 * 
 * ReZound is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */

#include "CAdaptiveNormalizeAction.h"

#include <utility>

#include "../CActionParameters.h"
#include "../unit_conv.h"

#include "../DSP/LevelDetector.h"

//??? could use a state machine or some sort of fuzzy state machine that doesnt raise the gain unless there is going to be a rise again in some minimum amount of time (actually could probably just use a delay line to do this)


CAdaptiveNormalizeAction::CAdaptiveNormalizeAction(const AActionFactory *factory,const CActionSound *actionSound,float _normalizationLevel,float _windowTime,float _maxGain,bool _lockChannels) :
	AAction(factory,actionSound),

	normalizationLevel(_normalizationLevel),
	windowTime(_windowTime),
	maxGain(_maxGain),
	lockChannels(_lockChannels)
{
}

CAdaptiveNormalizeAction::~CAdaptiveNormalizeAction()
{
}

template <class type> const type my_abs(const type v) { return v<0 ? -v : v; }

bool CAdaptiveNormalizeAction::doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo)
{
	const sample_pos_t start=actionSound->start;
	const sample_pos_t stop=actionSound->stop;
	const sample_pos_t selectionLength=actionSound->selectionLength();
	const unsigned channelCount=actionSound->sound->getChannelCount();

	if(prepareForUndo)
		moveSelectionToTempPools(actionSound,mmSelection,actionSound->selectionLength());

//??? I dont know what the relationship is, but as maxGain goes down, normalizationLevel has to go up for something to be audible..
	const mix_sample_t normalizationLevel=my_abs(dBFS_to_amp(this->normalizationLevel,MAX_SAMPLE));

	const float maxGain=dB_to_scalar(this->maxGain); 
	
	const sample_pos_t windowSize=min(selectionLength,(sample_pos_t)ms_to_samples(windowTime,actionSound->sound->getSampleRate()));
	const sample_pos_t hWindowSize=windowSize/2;


	// ??? I suppose src in this algorithm could be another audio file
	if(!lockChannels || actionSound->countChannels()<=1)
	{
		unsigned channelsDoneCount=0;
		for(unsigned i=0;i<actionSound->sound->getChannelCount();i++)
		{
			if(actionSound->doChannel[i])
			{

				CStatusBar statusBar(_("Adaptive Normalize -- Channel ")+istring(++channelsDoneCount)+"/"+istring(actionSound->countChannels()),start,stop,true);

				printf("normalization Level: %f -- windowSize: %d windowTime: %f maxGain: %f\n",normalizationLevel,windowSize,windowTime,maxGain);

				sample_pos_t srcPos=prepareForUndo ? 0 : start;
				const CRezPoolAccesser src=prepareForUndo ? actionSound->sound->getTempAudio(tempAudioPoolKey,i) : actionSound->sound->getAudio(i);
				// now 'src' is an accessor either directly into the sound or into the temp pool created for undo
				// so its range of indexes is either [start,stop] or [0,selectionLength) respectively

				const CRezPoolAccesser src_alt=src; // could be improved by possibly using a delay line instead of having another src to read from
				sample_pos_t destPos=start;
				CRezPoolAccesser dest=actionSound->sound->getAudio(i);

				CDSPRMSLevelDetector detector(windowSize);
				// prime the level detector
				for(sample_pos_t t=0;t<windowSize;t++)
					detector.updateLevel(src[srcPos+hWindowSize+t]);
				
				const sample_pos_t _stop=stop-hWindowSize;
				while(destPos<=_stop)
				{
					const mix_sample_t s_level=(mix_sample_t)(src_alt[srcPos+hWindowSize]);
					const mix_sample_t s_input=(mix_sample_t)(src[srcPos++]);
					const mix_sample_t currentLevel=detector.readLevel(s_level);

					if(normalizationLevel>(maxGain*currentLevel))
						dest[destPos]=ClipSample(s_input*maxGain);
					else
						dest[destPos]=ClipSample(s_input*normalizationLevel/currentLevel);

					if(statusBar.update(destPos))
					{ // cancelled
						if(prepareForUndo)
							undoActionSizeSafe(actionSound);
						else
							actionSound->sound->invalidatePeakData(i,actionSound->start,destPos);
						return false;
					}

					destPos++;
				}

				// copy remainder that we couldn't look ahead for
				{
					const mix_sample_t currentLevel=detector.readCurrentLevel();
					while(destPos<=stop)
					{
						const mix_sample_t s_input=src[srcPos++];

						if(normalizationLevel>(maxGain*currentLevel))
							dest[destPos++]=ClipSample(s_input*maxGain);
						else
							dest[destPos++]=ClipSample(s_input*normalizationLevel/currentLevel);
					}
				}
			}
		}
	}
	else 
	{ // same algorithm as above, but uses the max level of all channels for calculating the gain of a channel
		// set up 3 arrays of accessers (two srcs and one dest) for each channel to be processed
		unsigned nChannels=0;
		CRezPoolAccesser *dests[MAX_CHANNELS];
		const CRezPoolAccesser *srcs[MAX_CHANNELS];
		const CRezPoolAccesser *srcs_alt[MAX_CHANNELS];

		sample_pos_t destPos=start;
		sample_pos_t srcPos=prepareForUndo ? 0 : start;
		try
		{
			for(unsigned i=0;i<actionSound->sound->getChannelCount();i++)
			{
				if(actionSound->doChannel[i])
				{
					dests[nChannels]=new CRezPoolAccesser(actionSound->sound->getAudio(i));
					srcs[nChannels]=new CRezPoolAccesser(prepareForUndo ? actionSound->sound->getTempAudio(tempAudioPoolKey,i) : actionSound->sound->getAudio(i));
					srcs_alt[nChannels]=new CRezPoolAccesser(*(srcs[nChannels]));
					nChannels++;
				}
			}

			CDSPRMSLevelDetector detector(windowSize);
			// prime the level detector
			for(sample_pos_t t=0;t<windowSize;t++)
			{
				mix_sample_t s_level=(mix_sample_t)((*(srcs_alt[0]))[srcPos+hWindowSize+t]);
				for(unsigned i=1;i<nChannels;i++)
					s_level=max((mix_sample_t)((*(srcs_alt[i]))[srcPos+hWindowSize+t]),s_level);
				detector.updateLevel(s_level);
			}

			CStatusBar statusBar("Adaptive Normalize",start,stop,true);
			const sample_pos_t _stop=stop-hWindowSize;
			while(destPos<=_stop)
			{
				mix_sample_t s_level=(mix_sample_t)((*(srcs_alt[0]))[srcPos+hWindowSize]);
				for(unsigned i=1;i<nChannels;i++)
					s_level=max((mix_sample_t)((*(srcs_alt[i]))[srcPos+hWindowSize]),s_level);

				const mix_sample_t currentLevel=detector.readLevel(s_level);
				for(unsigned i=0;i<nChannels;i++)
				{
					const mix_sample_t s_input=(mix_sample_t)((*(srcs[i]))[srcPos]);
					if(normalizationLevel>(maxGain*currentLevel))
						(*(dests[i]))[destPos]=ClipSample(s_input*maxGain);
					else
						(*(dests[i]))[destPos]=ClipSample(s_input*normalizationLevel/currentLevel);
				}
				srcPos++;

				if(statusBar.update(destPos))
				{ // cancelled
					if(prepareForUndo)
						undoActionSizeSafe(actionSound);
					else
					{
						for(unsigned i=0;i<actionSound->sound->getChannelCount();i++)
						{
							if(actionSound->doChannel[i])
								actionSound->sound->invalidatePeakData(i,actionSound->start,destPos);
						}
					}

					for(unsigned i=0;i<nChannels;i++)
					{
						delete dests[i];
						delete srcs[i];
						delete srcs_alt[i];
					}
					return false;
				}

				destPos++;
			}

			// copy remainder that we couldn't look ahead for
			{
				const mix_sample_t currentLevel=detector.readCurrentLevel();
				while(destPos<=stop)
				{
					for(unsigned i=0;i<nChannels;i++)
					{
						const mix_sample_t s_input=(*(srcs[i]))[srcPos];

						if(normalizationLevel>(maxGain*currentLevel))
							(*(dests[i]))[destPos]=ClipSample(s_input*maxGain);
						else
							(*(dests[i]))[destPos]=ClipSample(s_input*normalizationLevel/currentLevel);
					}
					destPos++;
					srcPos++;
				}
			}
	
			for(unsigned i=0;i<nChannels;i++)
			{
				delete dests[i];
				delete srcs[i];
				delete srcs_alt[i];
			}
		}
		catch(...)
		{
			for(unsigned i=0;i<nChannels;i++)
			{
				delete dests[i];
				delete srcs[i];
				delete srcs_alt[i];
			}
			throw;
		}
	}

	if(!prepareForUndo)
	{
		for(unsigned i=0;i<actionSound->sound->getChannelCount();i++)
		{
			if(actionSound->doChannel[i])
				actionSound->sound->invalidatePeakData(i,actionSound->start,actionSound->stop);
		}
	}

	// set the new selection points (only necessary if the length of the sound has changed)
	//actionSound->stop=actionSound->start+selectionLength-1;


	return true;
}

AAction::CanUndoResults CAdaptiveNormalizeAction::canUndo(const CActionSound *actionSound) const
{
	return curYes;
}

void CAdaptiveNormalizeAction::undoActionSizeSafe(const CActionSound *actionSound)
{
	restoreSelectionFromTempPools(actionSound,actionSound->start,actionSound->selectionLength());
}

const string CAdaptiveNormalizeAction::getExplanation()
{
	return "This action uses an RMS level detector (defined by the window time) and adjusts the amplitude inversely proportional to the signal level.  As the level drops, the gain increases; as the level rises, the gain decreases.  There is also a max gain limit so that silence does not get amplified by infinity.\n(when the maximum gain limit is not being imposed) The formula is:\n\n      output = input * normalization level / current signal level\n\nIf \"Lock Channels\" is enabled, then the signal level will be calculated across all selected channels which will preserve stereo phase.";
}


// --------------------------------------------------

CAdaptiveNormalizeActionFactory::CAdaptiveNormalizeActionFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog) :
	AActionFactory(N_("Adaptive Normalize"),_("Adaptive Normalize"),channelSelectDialog,dialog)
{
}

CAdaptiveNormalizeActionFactory::~CAdaptiveNormalizeActionFactory()
{
}

CAdaptiveNormalizeAction *CAdaptiveNormalizeActionFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return new CAdaptiveNormalizeAction(this,actionSound,
		actionParameters->getValue<float>("Normalization Level"),
		actionParameters->getValue<float>("Window Time"),
		actionParameters->getValue<float>("Maximum Gain"),
		actionParameters->getValue<bool>("Lock Channels"))
	;
}

