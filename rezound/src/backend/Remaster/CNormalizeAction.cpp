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

#include "CNormalizeAction.h"

#include "../CActionParameters.h"
#include "../unit_conv.h"

#include <algorithm>
#include <vector>

CNormalizeAction::CNormalizeAction(const CActionSound &actionSound,float _normalizationLevel,unsigned _regionCount,bool _lockChannels) :
	AAction(actionSound),

	normalizationLevel(_normalizationLevel),
	regionCount(_regionCount),
	lockChannels(_lockChannels)
{
}

CNormalizeAction::~CNormalizeAction()
{
}

template <class type> const type my_abs(const type v) { return v<0 ? -v : v; }

bool CNormalizeAction::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	const sample_pos_t start=actionSound.start;
	const sample_pos_t stop=actionSound.stop;
	const sample_pos_t selectionLength=actionSound.selectionLength();
	const unsigned channelCount=actionSound.sound->getChannelCount();

	if(prepareForUndo)
		moveSelectionToTempPools(actionSound,mmSelection,actionSound.selectionLength());

	const mix_sample_t normalizationLevel=my_abs(dBFS_to_amp(this->normalizationLevel,MAX_SAMPLE));
	const unsigned regionCount=min((sample_pos_t)selectionLength,max((sample_pos_t)1,(sample_pos_t)this->regionCount));
	const sample_pos_t regionLength=selectionLength/regionCount;

	// keep up with the max sample value in each region for each channel and the position of that max in the audio
	vector<mix_sample_t> maxValues[MAX_CHANNELS];
	vector<sample_pos_t> maxValuePositions[MAX_CHANNELS];

	// get max amplitude per region for each channel
	for(unsigned i=0;i<channelCount;i++)
	{
		if(actionSound.doChannel[i])
		{
			const CRezPoolAccesser src=prepareForUndo ? actionSound.sound->getTempAudio(tempAudioPoolKey,i) : actionSound.sound->getAudio(i);
			sample_pos_t srcOffset=prepareForUndo ? 0 : start;
			sample_pos_t posAdd=prepareForUndo ? start : 0; // add this to the positions incase src is a tempPool for undo purposes because it would start at 0 instead of start

			CStatusBar statusBar("Analyzing -- Channel "+istring(i),srcOffset,srcOffset+selectionLength,true); 

			for(unsigned t=0;t<regionCount;t++)
			{
				maxValues[i].push_back(src[srcOffset]);
				maxValuePositions[i].push_back(srcOffset+posAdd);
				for(sample_pos_t j=0;j<regionLength;j++)
				{
					if(my_abs(src[srcOffset])>maxValues[i][t])
					{
						maxValues[i][t]=my_abs(src[srcOffset]);
						maxValuePositions[i][t]=srcOffset+posAdd;
					}

					if(statusBar.update(srcOffset))
					{ // cancelled
						if(prepareForUndo)
							undoActionSizeSafe(actionSound);
						return false;
					}
			
					srcOffset++;
				}
			}
		}
	}

	// if the channels are to be locked, then find the max among all 
	// channels per region and make all channels think that is the max
	if(lockChannels)
	{
		for(unsigned t=0;t<regionCount;t++)
		{
			mix_sample_t maxValue=0;
			sample_pos_t maxValuePosition=0;
			for(unsigned i=0;i<channelCount;i++)
			{
				if(actionSound.doChannel[i])
				{
					if(maxValues[i][t]>maxValue)
					{
						maxValue=maxValues[i][t];
						maxValuePosition=maxValuePositions[i][t];
					}
				}
			}

			for(unsigned i=0;i<channelCount;i++)
			{
				if(actionSound.doChannel[i])
				{
					maxValues[i][t]=maxValue;
					maxValuePositions[i][t]=maxValuePosition;
				}
			}
		}
		
	}	

	// save some calculations if the regionCount is 1
	if(regionCount==1)
	{
		// now adjust the amplitude of the data according to the maxValues
		for(unsigned i=0;i<channelCount;i++)
		{
			if(actionSound.doChannel[i])
			{
				CRezPoolAccesser dest=actionSound.sound->getAudio(i);
				const CRezPoolAccesser src=prepareForUndo ? actionSound.sound->getTempAudio(tempAudioPoolKey,i) : actionSound.sound->getAudio(i);
				sample_pos_t srcOffset=prepareForUndo ? 0 : start;
				sample_pos_t destPos=start;

				CStatusBar statusBar("Normalizing -- Channel "+istring(i),start,stop,true); 

				const float gain=(float)normalizationLevel/(float)maxValues[i][0];
				for(sample_pos_t j=0;j<selectionLength;j++)
				{
					dest[destPos++]=ClipSample(src[srcOffset++]*gain);

					if(statusBar.update(destPos))
					{ // cancelled
						if(prepareForUndo)
							undoActionSizeSafe(actionSound);
						else
							actionSound.sound->invalidatePeakData(i,actionSound.start,destPos);
						return false;
					}
				}
			
				if(!prepareForUndo)
					actionSound.sound->invalidatePeakData(i,actionSound.start,actionSound.stop);
			}
		}
	}
	// play connect the dots with the maxValues and normalize based on that
	else
	{
		// create endpoints by duplicating the first and last maxValues for each channel
		// this is so that the connect-the-dots game we'll play with the maxValues will 
		// always start at the beginning and end at the end
		for(unsigned i=0;i<channelCount;i++)
		{
			if(actionSound.doChannel[i])
			{
				maxValues[i].insert(maxValues[i].begin(),maxValues[i][0]);
				maxValuePositions[i].insert(maxValuePositions[i].begin(),start);

				maxValues[i].insert(maxValues[i].end(),maxValues[i][maxValues[i].size()-1]);
				maxValuePositions[i].insert(maxValuePositions[i].end(),stop);
			}
		}


		sample_fpos_t fNormalizationLevel=normalizationLevel;

		// now adjust the amplitude of the data according to the maxValues
		for(unsigned i=0;i<channelCount;i++)
		{
			if(actionSound.doChannel[i])
			{
				CRezPoolAccesser dest=actionSound.sound->getAudio(i);
				const CRezPoolAccesser src=prepareForUndo ? actionSound.sound->getTempAudio(tempAudioPoolKey,i) : actionSound.sound->getAudio(i);
				sample_pos_t srcOffset=prepareForUndo ? 0 : start;
				sample_pos_t destPos=start;

				CStatusBar statusBar("Normalizing -- Channel "+istring(i),start,stop,true); 

				for(unsigned t=0;t<=regionCount;t++)
				{
					// I'm doing all calculations in sample_fpos_t because of the mixes math with the positions and sample values
					const sample_fpos_t x1=maxValuePositions[i][t];
					const sample_fpos_t y1=maxValues[i][t];

					const sample_fpos_t x2=maxValuePositions[i][t+1];
					const sample_fpos_t y2=maxValues[i][t+1];

					const sample_fpos_t length=(x2-x1);

					sample_pos_t _x2=(sample_pos_t)x2;
					for(sample_pos_t j=(sample_pos_t)x1;j<_x2;j++)
					{
						const sample_fpos_t y=y1+(((y2-y1)*(j-x1))/length);
						float gain=(float)(fNormalizationLevel/y);

						dest[destPos++]=ClipSample(src[srcOffset++]*gain);

						if(statusBar.update(destPos))
						{ // cancelled
							if(prepareForUndo)
								undoActionSizeSafe(actionSound);
							else
								actionSound.sound->invalidatePeakData(i,actionSound.start,destPos);
							return false;
						}
					}
				}
			
				if(!prepareForUndo)
					actionSound.sound->invalidatePeakData(i,actionSound.start,actionSound.stop);
			}
		}
	}

	return(true);
}

AAction::CanUndoResults CNormalizeAction::canUndo(const CActionSound &actionSound) const
{
	return(curYes);
}

void CNormalizeAction::undoActionSizeSafe(const CActionSound &actionSound)
{
	restoreSelectionFromTempPools(actionSound,actionSound.start,actionSound.selectionLength());
}


// --------------------------------------------------

CNormalizeActionFactory::CNormalizeActionFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog) :
	AActionFactory("Normalize","Normalize Amplitude (Some of this technique is experimental, please let me know if you approve or disapprove of the result's quality)",channelSelectDialog,dialog)
{
}

CNormalizeActionFactory::~CNormalizeActionFactory()
{
}

CNormalizeAction *CNormalizeActionFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return(new CNormalizeAction(actionSound,actionParameters->getDoubleParameter("Normalization Level"),actionParameters->getUnsignedParameter("Region Count"),actionParameters->getBoolParameter("Lock Channels")));
}


