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

#include "CDelayEffect.h"

#include <stdexcept>

#include <istring>

#include "../CActionParameters.h"
#include "../CActionSound.h"

#include "../DSP/DelayEffect.h"
#include "../unit_conv.h"



/*TODO
 * Need to make it auto extend the length and/or go into the subsequent audio after the stop position
 *
 * - need to create a way to do ping-pong delay... that alternate sending the taps to the different channels
 *   	- I *think* all I need to do is send the feedback to the opposite channel instead of it's own channel
 *   		- but the diagram adds and extra gain paramter on the input
 * - probably want a gain parameter than adjust how much of the original signals get's mixed to the delayed results
 */


// ??? I should make these vectors instead of arrays
CDelayEffect::CDelayEffect(const CActionSound &actionSound,size_t _tapCount,float *_tapTimes,float *_tapGains,float *_tapFeedbacks) :
	AAction(actionSound),

	tapCount(_tapCount),

	tapTimes(NULL),
	tapGains(NULL),
	tapFeedbacks(NULL)
{

	if(tapCount<=0)
		throw(runtime_error(string(__func__)+" -- invalid tapCount: "+istring(tapCount)));

	tapTimes=new float[tapCount];
	tapGains=new float[tapCount];
	tapFeedbacks=new float[tapCount];

	for(size_t t=0;t<tapCount;t++)
	{
		tapTimes[t]=ms_to_samples(_tapTimes[t],actionSound.sound->getSampleRate());
		tapGains[t]=_tapGains[t];
		tapFeedbacks[t]=_tapFeedbacks[t];
	}
}

CDelayEffect::~CDelayEffect()
{
	delete [] tapTimes;
	delete [] tapGains;
	delete [] tapFeedbacks;
}

bool CDelayEffect::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	const sample_pos_t start=actionSound.start;
	const sample_pos_t stop=actionSound.stop;

	if(prepareForUndo)
		moveSelectionToTempPools(actionSound,mmSelection,actionSound.selectionLength());

	for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
	{
		if(actionSound.doChannel[i])
		{
			CStatusBar statusBar("Delay -- Channel "+istring(i),start,stop); 

			CRezPoolAccesser dest=actionSound.sound->getAudio(i);
			const CRezPoolAccesser src=prepareForUndo ? actionSound.sound->getTempAudio(tempAudioPoolKey,i) : actionSound.sound->getAudio(i);

			CDSPDelayEffect delayEffect(
				tapCount,
				tapTimes,
				tapGains,
				tapFeedbacks
				);

			sample_pos_t srcP=prepareForUndo ? 0 : start;
			for(sample_pos_t t=start;t<=stop;t++)
			{
				dest[t]=ClipSample(delayEffect.processSample(src[srcP++]));
				statusBar.update(t);
			}
		}
	}

	return(true);
}

AAction::CanUndoResults CDelayEffect::canUndo(const CActionSound &actionSound) const
{
	return(curYes);
}

void CDelayEffect::undoActionSizeSafe(const CActionSound &actionSound)
{
	restoreSelectionFromTempPools(actionSound,actionSound.start,actionSound.selectionLength());
}



// ---------------------------------------------

// ??? else it will be a "Tapped Delay"

CSimpleDelayEffectFactory::CSimpleDelayEffectFactory(AActionDialog *channelSelectDialog,AActionDialog *normalDialog) :
	AActionFactory("Simple Delay (Echo)","Simple Delay Effect",false,channelSelectDialog,normalDialog,NULL)
{
}

CDelayEffect *CSimpleDelayEffectFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	/*
	size_t tapCount=4;
	float tapTimes[]=	{375,	375,	175,	375,	250,	250,	250,	250,	250};
	float tapGains[]=	{0.7,	0.7,	0.7,	0.7,	0.5,	0.6,	0.7,	0.8,	0.9};
	float tapFeedbacks[]=	{0.0,	0.0,	0.2,	0.0,	0.0,	0.0,	0.0,	0.0,	0.0};
	*/

	/*
	size_t tapCount=1;
	float tapTimes[]=	{325,	375,	175,	375,	250,	250,	250,	250,	250};
	float tapGains[]=	{0.6,	0.7,	0.7,	0.7,	0.5,	0.6,	0.7,	0.8,	0.9};
	float tapFeedbacks[]=	{0.4,	0.2,	0.2,	0.0,	0.0,	0.0,	0.0,	0.0,	0.0};

	return(new CDelayEffect(actionSound,
		tapCount,
		tapTimes,
		tapGains,
		tapFeedbacks
	));
	*/


	// need to be able to get vectors from actionParameters
	float a=actionParameters->getDoubleParameter("Delay");
	float b=actionParameters->getDoubleParameter("Gain");
	float c=actionParameters->getDoubleParameter("Feedback");

	return(new CDelayEffect(actionSound,
		1,
		&a,
		&b,
		&c
	));
}

