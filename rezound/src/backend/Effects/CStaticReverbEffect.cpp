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

#include "CStaticReverbEffect.h"

#include <math.h>

/*
 * Okay, I successfully implement a DOG-SLOW but very realistic reverb using convolution in the time domain.
 * I don't have time or really the know-how to implement it in the frequency domain, so I challenge some 
 * more able person to contact me about implementing this reverb using a better algorithm than my brute
 * force method.  I've look at some open source dsp libraries but some are lacking and some aren't even
 * autoconfiscated which would make porting harder for me.  
 * Another issue is probably that the library will work on arrays of floats, so I would have to copy
 * the data to an array, work on it, then copy it back... I'm not opposed to this, but if the DSP functions
 * use some idea like the STL's iterator concept (the STL's worst and best feature), it might be able to work
 * on the data directly except maybe for one other important factor... There may be a problem since sample_t
 * (as typedefed right now) cannot hold large values, and the computation of the transforms might need that.
 * So copying to arrays or not doesn't really matter to me, because it may be necessary or perhaps with some
 * numerical theoretic transforms it could be eliminated.
 *
 * Another issue to deal with is windowing the data properly for it to be transformed.  Another thing is that
 * most books on the topic that I've found use the FFT which handles 'complex' data, but a DCT (discrete cosine
 * transform using in mpeg) would work fine since it is the 'real' part only of the transform
 *
 *
 * So, if anyone out there is willing to give it a shot, I can explain precisely how this algorithm worked
 * I got the idea from: http://www.earlevel.com/Digital%20Audio/Reverb.html  which made years of pondering
 * about reverb all the sudden click!
 *
 * Also, if we build or find a library for doing the transforms, it should be usable and go along way for 
 * making actions for low-pass, hi-pass, filters, equalizing, time-stretching, pitch-shifting, etc!
 *
 */

//#define DO_IMPLEMENT // just define this to implement or not the effect (incase I want to play with it later

#ifdef DO_IMPLEMENT
	#define IMPULSE_LENGTH (sizeof(impulse)/sizeof(*impulse))
	#include "/home/ddurham/impulse/impulse.h"
#endif

CStaticReverbEffect::CStaticReverbEffect(const CActionSound &actionSound) :
	AAction(actionSound)
{
}

bool CStaticReverbEffect::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	const sample_pos_t start=actionSound.start;
	const sample_pos_t stop=actionSound.stop;
	const sample_pos_t selectionLength=actionSound.selectionLength();

#ifndef DO_IMPLEMENT
	throw(EUserMessage(("check "+string(__FILE__)+" for a message about this disabled effect").c_str()));
#endif

	if(prepareForUndo)
		moveSelectionToTempPools(actionSound,mmSelection,actionSound.selectionLength());

#ifdef DO_IMPLEMENT
	for(size_t t=0;t<IMPULSE_LENGTH;t++)
		//impulse[t]=fabs(impulse[t]);
		impulse[t]=(impulse[t]);
#endif

	for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
	{
		if(actionSound.doChannel[i])
		{
			BEGIN_PROGRESS_BAR("Reverb -- Channel "+istring(i),start,stop); 

			CRezPoolAccesser a=actionSound.sound->getAudio(i);

			if(prepareForUndo)
				a.copyData(start,actionSound.sound->getTempAudio(tempAudioPoolKey,i),0,selectionLength);

#ifdef DO_IMPLEMENT
			// brute force: for each sample, multiply it by the impulse response
			#define GAIN   3
			static float xv[IMPULSE_LENGTH+1]={0};

			for(sample_pos_t t=start;t<=stop;t++)
			{
				for (unsigned k = 0; k < IMPULSE_LENGTH; k++)
					xv[k] = xv[k+1];
				xv[IMPULSE_LENGTH] = a[t] / GAIN;
				//xv[0] = a[t] / GAIN;


				double sum = 0.0;
				for (unsigned k = 0; k <= IMPULSE_LENGTH; k++)
					sum += (impulse[IMPULSE_LENGTH-k-1] * xv[k]);

				a[t]=ClipSample(sum);

				UPDATE_PROGRESS_BAR(t);
			}
#endif



			END_PROGRESS_BAR();
		}
	}

	return(true);
}

AAction::CanUndoResults CStaticReverbEffect::canUndo(const CActionSound &actionSound) const
{
	return(curYes);
}

void CStaticReverbEffect::undoActionSizeSafe(const CActionSound &actionSound)
{
	restoreSelectionFromTempPools(actionSound,actionSound.start,actionSound.selectionLength());
}


// --------------------------------------------------

// ??? perhaps I don't have to be completly static... I could have parameters that increase initial delay, and others for filtering and others that shorten the impluse
CStaticReverbEffectFactory::CStaticReverbEffectFactory(AActionDialog *channelSelectDialog) :
	AActionFactory("Static Reverb","Static Reverb -- meaning I've used the impulse response of a room",false,channelSelectDialog,NULL,NULL)
{
}

CStaticReverbEffect *CStaticReverbEffectFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	return(new CStaticReverbEffect(actionSound));
}


