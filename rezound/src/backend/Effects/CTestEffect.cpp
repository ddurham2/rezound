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

#include "../DSP/Convolver.h"

CTestEffect::CTestEffect(const CActionSound &actionSound) :
	AAction(actionSound)
{
}

bool CTestEffect::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
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
			CRezPoolAccesser dest=actionSound.sound->getAudio(i);
			const CRezPoolAccesser src=prepareForUndo ? actionSound.sound->getTempAudio(tempAudioPoolKey,i) : actionSound.sound->getAudio(i);
			sample_pos_t srcOffset=prepareForUndo ? start : 0;

// --- Insert your test effect here -- BEGIN --------------------------------------------

/* Digital filter designed by mkfilter/mkshape/gencode   A.J. Fisher
   Command line: /www/usr/fisher/helpers/mkshape -c 1.3605442177e-03 5.0000000000e-01 255 -b 16 -l */

#define NZEROS 254
#define GAIN   2.318215027e+02

static float xcoeffs[] =
  { +0.7397766113, +0.7434692383, +0.7471313477, +0.7507934570,
    +0.7544250488, +0.7580566406, +0.7616577148, +0.7652282715,
    +0.7687988281, +0.7723388672, +0.7758789062, +0.7793884277,
    +0.7828674316, +0.7863159180, +0.7897644043, +0.7931823730,
    +0.7966003418, +0.7999877930, +0.8033447266, +0.8066711426,
    +0.8099975586, +0.8132934570, +0.8165588379, +0.8198242188,
    +0.8230285645, +0.8262329102, +0.8294372559, +0.8325805664,
    +0.8357238770, +0.8388366699, +0.8419189453, +0.8449707031,
    +0.8480224609, +0.8510437012, +0.8540039062, +0.8569641113,
    +0.8599243164, +0.8628234863, +0.8657226562, +0.8685607910,
    +0.8713989258, +0.8742065430, +0.8769836426, +0.8797607422,
    +0.8824768066, +0.8851623535, +0.8878479004, +0.8905029297,
    +0.8931274414, +0.8956909180, +0.8982543945, +0.9007873535,
    +0.9032897949, +0.9057922363, +0.9082336426, +0.9106445312,
    +0.9130249023, +0.9154052734, +0.9177246094, +0.9200134277,
    +0.9223022461, +0.9245300293, +0.9267578125, +0.9289245605,
    +0.9310607910, +0.9331970215, +0.9352722168, +0.9373474121,
    +0.9393615723, +0.9413757324, +0.9433288574, +0.9452514648,
    +0.9471740723, +0.9490356445, +0.9508666992, +0.9526672363,
    +0.9544372559, +0.9561767578, +0.9578857422, +0.9595642090,
    +0.9612121582, +0.9628295898, +0.9644165039, +0.9659423828,
    +0.9674682617, +0.9689331055, +0.9703979492, +0.9718017578,
    +0.9731750488, +0.9745178223, +0.9758300781, +0.9771118164,
    +0.9783630371, +0.9795532227, +0.9807434082, +0.9818725586,
    +0.9829711914, +0.9840698242, +0.9851074219, +0.9860839844,
    +0.9870605469, +0.9880065918, +0.9888916016, +0.9897766113,
    +0.9906005859, +0.9913940430, +0.9921569824, +0.9928894043,
    +0.9935607910, +0.9942321777, +0.9948425293, +0.9954223633,
    +0.9959716797, +0.9964904785, +0.9969787598, +0.9974060059,
    +0.9978332520, +0.9981994629, +0.9985351562, +0.9988403320,
    +0.9990844727, +0.9993286133, +0.9995117188, +0.9996948242,
    +0.9998168945, +0.9999084473, +0.9999389648, +0.9999694824,
    +0.9999389648, +0.9999084473, +0.9998168945, +0.9996948242,
    +0.9995117188, +0.9993286133, +0.9990844727, +0.9988403320,
    +0.9985351562, +0.9981994629, +0.9978332520, +0.9974060059,
    +0.9969787598, +0.9964904785, +0.9959716797, +0.9954223633,
    +0.9948425293, +0.9942321777, +0.9935607910, +0.9928894043,
    +0.9921569824, +0.9913940430, +0.9906005859, +0.9897766113,
    +0.9888916016, +0.9880065918, +0.9870605469, +0.9860839844,
    +0.9851074219, +0.9840698242, +0.9829711914, +0.9818725586,
    +0.9807434082, +0.9795532227, +0.9783630371, +0.9771118164,
    +0.9758300781, +0.9745178223, +0.9731750488, +0.9718017578,
    +0.9703979492, +0.9689331055, +0.9674682617, +0.9659423828,
    +0.9644165039, +0.9628295898, +0.9612121582, +0.9595642090,
    +0.9578857422, +0.9561767578, +0.9544372559, +0.9526672363,
    +0.9508666992, +0.9490356445, +0.9471740723, +0.9452514648,
    +0.9433288574, +0.9413757324, +0.9393615723, +0.9373474121,
    +0.9352722168, +0.9331970215, +0.9310607910, +0.9289245605,
    +0.9267578125, +0.9245300293, +0.9223022461, +0.9200134277,
    +0.9177246094, +0.9154052734, +0.9130249023, +0.9106445312,
    +0.9082336426, +0.9057922363, +0.9032897949, +0.9007873535,
    +0.8982543945, +0.8956909180, +0.8931274414, +0.8905029297,
    +0.8878479004, +0.8851623535, +0.8824768066, +0.8797607422,
    +0.8769836426, +0.8742065430, +0.8713989258, +0.8685607910,
    +0.8657226562, +0.8628234863, +0.8599243164, +0.8569641113,
    +0.8540039062, +0.8510437012, +0.8480224609, +0.8449707031,
    +0.8419189453, +0.8388366699, +0.8357238770, +0.8325805664,
    +0.8294372559, +0.8262329102, +0.8230285645, +0.8198242188,
    +0.8165588379, +0.8132934570, +0.8099975586, +0.8066711426,
    +0.8033447266, +0.7999877930, +0.7966003418, +0.7931823730,
    +0.7897644043, +0.7863159180, +0.7828674316, +0.7793884277,
    +0.7758789062, +0.7723388672, +0.7687988281, +0.7652282715,
    +0.7616577148, +0.7580566406, +0.7544250488, +0.7507934570,
    +0.7471313477, +0.7434692383, +0.7397766113,
  };

/*
	//#define NC 11025
	#define NC 3000
	float coeffs[NC];

	for(int t=0;t<NC;t++)
	{
		coeffs[t]=((((float)rand()/(float)RAND_MAX)*400.0)-200.0)+pow((1.0-(float)t/(float)NC),2.0);
		if(t>0)
			coeffs[t]/=2;
		else
			coeffs[t]=1.0;
	}
*/

			BEGIN_PROGRESS_BAR("Filtering -- Channel "+istring(i),start,stop); 
			TDSPConvolver<mix_sample_t,float> convolver(xcoeffs,sizeof(xcoeffs)/sizeof(*xcoeffs));
			//TDSPConvolver<mix_sample_t,float> convolver(coeffs,NC);
			for(sample_pos_t t=start;t<=stop;t++)
			{
				//dest[t]=ClipSample(convolver.processSample(src[t-srcOffset])/NC);
				dest[t]=ClipSample(convolver.processSample(src[t-srcOffset])/GAIN);
				UPDATE_PROGRESS_BAR(t);
			}

		
			END_PROGRESS_BAR();


// --- Insert your test effect here -- END ----------------------------------------------


			if(!prepareForUndo)
				actionSound.sound->invalidatePeakData(i,actionSound.start,actionSound.stop);
		}
	}

	return(true);
}

AAction::CanUndoResults CTestEffect::canUndo(const CActionSound &actionSound) const
{
	return(curYes);
}

void CTestEffect::undoActionSizeSafe(const CActionSound &actionSound)
{
	restoreSelectionFromTempPools(actionSound,actionSound.start,actionSound.selectionLength());
}


// --------------------------------------------------
//
CTestEffectFactory::CTestEffectFactory(AActionDialog *channelSelectDialog) :
	AActionFactory("Test","Test",false,channelSelectDialog,NULL,NULL)
{
}

CTestEffect *CTestEffectFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	return(new CTestEffect(actionSound));
}


