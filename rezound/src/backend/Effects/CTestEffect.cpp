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

#include "CTestEffect.h"

#include <TAutoBuffer.h>

#include "../DSP/Convolver.h"

CTestEffect::CTestEffect(const AActionFactory *factory,const CActionSound *actionSound) :
	AAction(factory,actionSound)
{
}

CTestEffect::~CTestEffect()
{
}

bool CTestEffect::doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo)
{
	const sample_pos_t start=actionSound->start;
	const sample_pos_t stop=actionSound->stop;
	const sample_pos_t selectionLength=actionSound->selectionLength();

	// ??? since the convolution creates a signal that is N+M-1 points in length at the end, I should have a couple of options
	// 	- 1) if the stop == getLength()-1 then add M samples of space at the end
	// 	- 2) if the stop != getLength()-1 then maybe let the M extra samples bleed into the region after the stop position.. possibly adding space to the end of the sound if necessary (perform one check at the beginning for this)
	// 		- this would require me to back up M sample more data for the saving for undo... and now.. crossfading is either not applicable at all.. or at least not at the end of the selection.. or maybe it is

	if(prepareForUndo)
		moveSelectionToTempPools(actionSound,mmSelection,actionSound->selectionLength());

#if 0
	// load impulse response of reverb
	#define IMPULSE_LENGTH (sizeof(impulse)/sizeof(*impulse))
	#include "/home/ddurham/impulse/impulse.h"
	
	const sample_pos_t M=IMPULSE_LENGTH; // length of filter kernel
	TAutoBuffer<float> filter_kernel(M);
	for(sample_pos_t t=0;t<M;t++)
		filter_kernel[t]=impulse[t];

#elif 0
	// create the time-domain filter kernel
	const float freq=100.0f/(float)actionSound->sound->getSampleRate(); // cutoff frequency of this low-pass filter (fraction of the sample rate 0..0.5)
	const sample_pos_t M=200; // length of filter kernel
	float filter_kernel[M];
	for(sample_pos_t i=0;i<M;i++) 
	{
		if(i!=M/2)
			filter_kernel[i]=sinf(2*M_PI*freq*(i-M/2))/(i-M/2);
		else // avoid div by 0
			filter_kernel[i]=2*M_PI*freq;
	}
#elif 0
	
	// create the time-domain filter kernel
	const sample_pos_t M=1000; // length of filter kernel
	float filter_kernel[M];
	for(sample_pos_t i=0;i<M;i++) 
	{
		filter_kernel[i]=(sinf((i+10)/50.0)/3.0)*cos((i-13)/10.0)/3;
		//filter_kernel[i]=(sinf(i/50.0)/3.0)*(cos(i/10.0)/(i/(M/10.0)+1));
		//filter_kernel[i]=sinf(i/50.0)*cos(i/10.0)/(cos((float)i/M*M_PI)+2.0);
	}
#else
	
	// create the time-domain filter kernel
	const sample_pos_t M=200; // length of filter kernel
	float filter_kernel[M];
	for(sample_pos_t i=0;i<M;i++) 
	{
		filter_kernel[i]=pow((float)(i)/M,2)+sinf(i/10);
	}
#endif

	// apply the blackman window to the kernel
	for(sample_pos_t i=0;i<M;i++) 
		filter_kernel[i]*=(0.42-0.5*cos(2*M_PI*i/M)+0.08*cos(4*M_PI*i/M));


#if 1
	// normalize the time-domain filter kernel
	double sum=0;
	for(sample_pos_t i=0;i<M;i++)
		sum+=filter_kernel[i];
	for(sample_pos_t i=0;i<M;i++)
		filter_kernel[i]/=sum;
#endif

#if 0
	// convert a time-domain low-pass filter kernel to a high-pass
	for(sample_pos_t i=0;i<M;i++) 
		filter_kernel[i]=-filter_kernel[i];
	filter_kernel[M/2]+=1.0;
#endif

#ifdef HAVE_LIBRFFTW
	TFFTConvolverTimeDomainKernel<sample_t,float> c(filter_kernel,M);

	for(unsigned i=0;i<actionSound->sound->getChannelCount();i++)
	{
		if(actionSound->doChannel[i])
		{
			CRezPoolAccesser dest=actionSound->sound->getAudio(i);
			const CRezPoolAccesser src=prepareForUndo ? actionSound->sound->getTempAudio(tempAudioPoolKey,i) : actionSound->sound->getAudio(i);
			sample_pos_t srcOffset=prepareForUndo ? 0 : start;

			c.reset();

			CStatusBar statusBar("Filtering -- Channel "+istring(i),start,stop); 

			sample_pos_t srcPos=srcOffset;
			sample_pos_t destPos=start;
			while(destPos<=stop)
			{
				const sample_pos_t count=min((sample_pos_t)c.getChunkSize(),stop-destPos+1);

				// write to the convolver
				c.beginWrite();
				for(sample_pos_t t=0;t<count;t++)
					c.writeSample(src[srcPos++]);

				// read from the convolver
				c.beginRead();
				for(sample_pos_t t=0;t<count;t++)
					dest[destPos++]=ClipSample(c.readSample());

				statusBar.update(destPos);
			}

			/* could do this.. but would need to back that data up too for undo
			// read M-1 extra samples from convolution
			for(sample_pos_t t=0;t<M-1;t++,destPos++)
				dest[destPos]=ClipSample(dest[destPos]+c.readSample());
			*/


			if(!prepareForUndo)
				actionSound->sound->invalidatePeakData(i,actionSound->start,actionSound->stop);
		}
	}
#else
	for(unsigned i=0;i<actionSound->sound->getChannelCount();i++)
	{
		if(actionSound->doChannel[i])
		{
			CRezPoolAccesser dest=actionSound->sound->getAudio(i);
			const CRezPoolAccesser src=prepareForUndo ? actionSound->sound->getTempAudio(tempAudioPoolKey,i) : actionSound->sound->getAudio(i);
			sample_pos_t srcOffset=prepareForUndo ? start : 0;

			TSimpleConvolver<mix_sample_t,float> convolver(filter_kernel,M);

			CStatusBar statusBar("Filtering -- Channel "+istring(i),start,stop); 

			for(sample_pos_t t=start;t<=stop;t++)
			{
				dest[t]=ClipSample(convolver.processSample(src[t-srcOffset]));
				statusBar.update(t);
			}

			if(!prepareForUndo)
				actionSound->sound->invalidatePeakData(i,actionSound->start,actionSound->stop);
		}
	}
#endif

	return(true);
}

AAction::CanUndoResults CTestEffect::canUndo(const CActionSound *actionSound) const
{
	return(curYes);
}

void CTestEffect::undoActionSizeSafe(const CActionSound *actionSound)
{
	restoreSelectionFromTempPools(actionSound,actionSound->start,actionSound->selectionLength());
}


// --------------------------------------------------

CTestEffectFactory::CTestEffectFactory(AActionDialog *channelSelectDialog) :
	AActionFactory("Test","",channelSelectDialog,NULL)
{
}

CTestEffectFactory::~CTestEffectFactory()
{
}

CTestEffect *CTestEffectFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	return(new CTestEffect(this,actionSound));
}


