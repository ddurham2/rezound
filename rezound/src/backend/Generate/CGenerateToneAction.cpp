/* 
 * Copyright (C) 2003 - Marc Brevoort
 * Modified David W. Durham
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

#include "CGenerateToneAction.h"

#include "../CActionParameters.h"

#include <istring>

CGenerateToneAction::CGenerateToneAction(const CActionSound actionSound,const float _length,const float _volume,const float _frequency,const ToneTypes _toneType):
	AAction(actionSound),
	length(_length),	// seconds
	volume(_volume),	// 0 to 1 (a multiplier)
	frequency(_frequency),	// Hz
	toneType(_toneType),
	origLength(0)
{
	if(length<0)
		throw runtime_error(string(__func__)+" -- length is less than zero: "+istring(length));
}

CGenerateToneAction::~CGenerateToneAction()
{
}

bool CGenerateToneAction::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	origLength=actionSound.sound->getLength();
	const sample_pos_t start=actionSound.start;
	const sample_pos_t sampleCount=(sample_pos_t)(length*actionSound.sound->getSampleRate());

	if(sampleCount==0) 
	{
		actionSound.stop=actionSound.start;
		return true;
	}
	
	actionSound.sound->addSpace(actionSound.doChannel,actionSound.start,sampleCount,true);
	actionSound.stop=(actionSound.start+sampleCount)-1;

	// new space is made for the tone, now calculate the values.

	unsigned channelsDoneCount=0;
        for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
        {
                if(actionSound.doChannel[i]) 
		{
			CStatusBar statusBar(_("Generating Tone -- Channel ")+istring(++channelsDoneCount)+"/"+istring(actionSound.countChannels()),0,sampleCount,true);

                	CRezPoolAccesser dest=actionSound.sound->getAudio(i);
			
			#define STATUS_UPDATE 						\
				if(statusBar.update(t))					\
				{ /* cancelled */					\
					if(prepareForUndo)				\
						undoActionSizeSafe(actionSound);	\
					return false;					\
				}


			/* ???
			 * I've read that there are cleaner ways to compute these waveforms.  But the only way
			 * I could see them as being 'cleaner' is that the pure math doesn't sound so great on
			 * typical DAC hardware.  So anyway, if someone knows how and is willing to replace these
			 * routines with 'better sounding' ones, be my guest.
			 */
			switch(toneType)
			{
			case ttSineWave: {
				const float freq=(2.0*M_PI)/(actionSound.sound->getSampleRate()/frequency);
				for(sample_pos_t t=0;t<sampleCount;t++)
				{
					dest[t+start]=convert_sample<float,sample_t>(sinf(t*freq)*volume);
					STATUS_UPDATE
				}
				break;
			}

			case ttSquareWave: {
				const float freq=(2.0*M_PI)/(actionSound.sound->getSampleRate()/frequency);
				const sample_t s_pos=convert_sample<float,sample_t>(volume);
				const sample_t s_neg=convert_sample<float,sample_t>(-volume);
				for(sample_pos_t t=0;t<sampleCount;t++)
				{
					// The idea is that when sin is positive we should create the max value else the min value
					dest[t+start]=sinf(t*freq)>=0 ? s_pos : s_neg;
					STATUS_UPDATE
				}
				break;
			}

			case ttRisingSawtoothWave:
			case ttFallingSawtoothWave: {
				const float c= (toneType==ttRisingSawtoothWave) ? 1.0 : -1.0;

				/* ??? this may not be the best implementation since the periodLength is integer division */
				const int periodLength=(int)(actionSound.sound->getSampleRate()/frequency);
				for(sample_pos_t t=0;t<sampleCount;t++)
				{
					const sample_pos_t tt=t+periodLength/2; // tt must be offset to start the wave on 0

					// The idea is that t%p will wrap every p samples where p is the periodLength
					//
					//          (      t%p  )
					// saw(t) = ( 2 * ----- ) - 1
					//          (       p   )
					//
					// basically [0,periodLength) is mapped to [-1,1) then * volume
					const float s= ((2.0f*(tt%periodLength)/periodLength)-1.0f)*volume;
					dest[t+start]=convert_sample<float,sample_t>(c*s);
					STATUS_UPDATE
				}
				break;
			}

			case ttTriangleWave: {
				const int periodLength=(int)(2*(actionSound.sound->getSampleRate()/frequency));
				for(sample_pos_t t=0;t<sampleCount;t++)
				{
					const sample_pos_t tt=t+periodLength/4; // tt must be offset to start the wave on 0

					// I derived this from doubling the period on a sawtooth and taking the abs
					//
					//               |  4 * (t%p)     |
					// triangle(t) = | ---------- - 2 | - 1
					//               |      p         |
					//
					const float s= (fabsf((4.0f*(float)(tt%periodLength)/periodLength)-2.0f)-1)*volume;
					dest[t+start]=convert_sample<float,sample_t>(-s);
					STATUS_UPDATE
				}
				break;
			}

			default:
				throw runtime_error(string(__func__)+" -- unhandled toneType: "+istring(toneType));
			}
		}
	}
	return true;
}

AAction::CanUndoResults CGenerateToneAction::canUndo(const CActionSound &actionSound) const
{
	return curYes;
}

void CGenerateToneAction::undoActionSizeSafe(const CActionSound &actionSound)
{
	const sample_pos_t sampleCount=(sample_pos_t)(length*actionSound.sound->getSampleRate());
	actionSound.sound->removeSpace(actionSound.doChannel,actionSound.start,sampleCount,origLength);
}



// ------------------------------

CGenerateToneActionFactory::CGenerateToneActionFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog) :
	AActionFactory(N_("Generate Tone"),_("Generate Tone"),channelSelectDialog,dialog)
{
}

CGenerateToneActionFactory::~CGenerateToneActionFactory()
{
}

// ??? t'would be nice if I had two factories: 1 for insert noise and 1 for replace selection with noise
CGenerateToneAction *CGenerateToneActionFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return new CGenerateToneAction(
		actionSound,
		actionParameters->getDoubleParameter("Length"),
		actionParameters->getDoubleParameter("Volume"),
		actionParameters->getDoubleParameter("Frequency"),
		(CGenerateToneAction::ToneTypes)actionParameters->getUnsignedParameter("Tone Type")
	);
}	

