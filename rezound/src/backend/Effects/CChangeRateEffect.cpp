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

#include "CChangeRateEffect.h"

#include "../CActionSound.h"
#include "../CActionParameters.h"

/* TODO:
 * - There is still just the slightest clicky stuff if I do a pure sinewave and also adjust the rate by drawing a sine wave by hand in the advanced curved dialog
 *   	- I'm guessing it's caused by discontinuity between segments... When I read ahead by one sample to do the interpolation, that sample I'm reading ahead should be according to the next segment's rate if I'm doing the last sample of the current segment
 *
 * - I could check if the rate is an integer we could easily do integer math to make it faster
 *      - Often you get a rate of 2 when converting sample rates
 *      - And the integer math would create the exact result as the floating point
 *
 */

CChangeRateEffect::CChangeRateEffect(const AActionFactory *factory,const CActionSound *actionSound,const CGraphParamValueNodeList &_rateCurve) :
	AAction(factory,actionSound),
	undoRemoveLength(0),
	rateCurve(_rateCurve)
{
	// we must truncate rateCurves values to some practical limits
	// truncate <0.01 to 0.01
	for(unsigned t=0;t<rateCurve.size();t++)
	{
		if(rateCurve[t].y<0.01)
			rateCurve[t].y=0.01;
	}
}

CChangeRateEffect::~CChangeRateEffect()
{
}

#include <stdio.h> // ??? just for the printf below
bool CChangeRateEffect::doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo)
{
	const sample_pos_t oldLength=actionSound->selectionLength();
	const sample_pos_t newLength=getNewSelectionLength(actionSound);

	if(newLength==NIL_SAMPLE_POS)
		throw(EUserMessage(_("newLength calulated out of range -- action not taken")));

	// we are gonna use the undo selection copy as the source buffer while changing the rate
	// we have a fudgeFactor of 2 because we may read ahead up to 2 when changing the rate
	moveSelectionToTempPools(actionSound,mmSelection,newLength,2); 

	undoRemoveLength=newLength;

	unsigned channelsDoneCount=0;
	for(unsigned i=0;i<actionSound->sound->getChannelCount();i++)
	{
		if(actionSound->doChannel[i])
		{
			CStatusBar statusBar(_("Changing Rate -- Channel ")+istring(++channelsDoneCount)+"/"+istring(actionSound->countChannels()),actionSound->start,actionSound->start+newLength,true); 
	
			// here, we're using the undo data as a source from which to calculate the new data
			const CRezPoolAccesser src=actionSound->sound->getTempAudio(tempAudioPoolKey,i);
			CRezPoolAccesser dest=actionSound->sound->getAudio(i);

			sample_pos_t writePos=actionSound->start;
			for(unsigned x=0;x<rateCurve.size()-1;x++)
			{
					// ??? but I've fixed some boundry conditions since trying it so perhaps I should try again
				// ??? I tried using interpretGraphNodes, but it seems to be like maybe one sample off in reading
				// at the beginning of a segment; either that or the interpolated sample positions differing between
				// rates between segments started to make it self more pronounced.   The way I'm doing it here, is 
				// keeping readStart/Stop doubles throughout instead of ceiling start and flooring stop.
				
				const CGraphParamValueNode &rateNode1=rateCurve[x];
				const CGraphParamValueNode &rateNode2=rateCurve[x+1];

				const sample_fpos_t readStart=(rateNode1.x*oldLength);
				const sample_fpos_t readStop=(rateNode2.x*oldLength);

				if(readStart==readStop)
					continue; // vertical rate change

				const double s1=rateNode1.y;
				const double s2=rateNode2.y;

				const sample_fpos_t m=(s2-s1)/(readStop-readStart);

				sample_pos_t _subWriteLength=(sample_pos_t)sample_fpos_round(getWriteLength(oldLength,rateNode1,rateNode2));
				if(_subWriteLength>0 && writePos+_subWriteLength>=(dest.getSize()+1)) // fudge fix for the above rounding creating one too many samples
					_subWriteLength--;
				const sample_pos_t subWriteLength=_subWriteLength;

				if(m!=0)
				{ // rate is going up or down
					const sample_fpos_t b=m*readStart-s1;
					const sample_fpos_t c=(1.0/m)*sample_fpos_log(sample_fpos_fabs(s1));

					for(sample_pos_t t=0;t<subWriteLength;t++)
					{
						const sample_fpos_t dReadPos=(sample_fpos_exp(m*(t+c))+b)/m;
						const sample_pos_t iReadPos=(sample_pos_t)dReadPos;
						const float p2=dReadPos-iReadPos;	// i.e. 53.4 -> .4 (40%)
						const float p1=1.0-p2;			// i.e. 53.4 -> .6 (60%)

						// here the fudgeFactor is used with iReadPos+1 when we made the undo backup
						dest[writePos++]=(sample_t)(p1*src[iReadPos]+p2*src[iReadPos+1]);
		
						if(statusBar.update(writePos))
						{ // cancelled 
							restoreSelectionFromTempPools(actionSound,actionSound->start,undoRemoveLength);
							return false;
						}
					}
				}
				else
				{ // rate is constant
							//??? Use CSound::mixSound() or TSoundStretcher itself
					if(s1!=1.0)
					{
						const sample_fpos_t subReadLength=(readStop-readStart)+1;
						for(sample_pos_t t=0;t<subWriteLength;t++)
						{
							const sample_fpos_t dReadPos=t*subReadLength/subWriteLength+readStart;
							const sample_pos_t iReadPos=(sample_pos_t)dReadPos;
							const float p2=dReadPos-iReadPos;	// i.e. 53.4 -> .4 (40%)
							const float p1=1.0-p2;			// i.e. 53.4 -> .6 (60%)

							// here the fudgeFactor is used with iReadPos+1 when we made the undo backup
							dest[writePos++]=(sample_t)(p1*src[iReadPos]+p2*src[iReadPos+1]);

							if(statusBar.update(writePos))
							{ // cancelled 
								restoreSelectionFromTempPools(actionSound,actionSound->start,undoRemoveLength);
								return false;
							}
						}
					}
					else
					{ // just copy the data
						sample_pos_t copyLength=(sample_pos_t)((readStop-readStart)+1);
						if(copyLength>0 && writePos+copyLength>=(dest.getSize()+1)) // fudge fix
							copyLength--;
						// whup, can't do status bar ??? unless I were to call copyData in 100 chunks perhaps
						// 	 ??? need to do
						dest.copyData((sample_pos_t)writePos,src,(sample_pos_t)readStart,copyLength);
						writePos+=copyLength;
					}
				}
			}

			// ??? I think I fixed everything below by using rint always on the return value of getWriteLength even in getNewSelectionlength
					// ??? if writePos!=expectedWritePos
					// ??? there's a glitch at the end... I may be able to solve it by just copying one more sample.. or repeating the very last sample
					// ??? but I'm sure it's happening because I'm truncating always instead of using floor/ceil
			if((writePos-actionSound->start)!=(newLength))
				printf("****************************** CRAP %d SAMPLES OFF FROM EXPECTED\n",(sample_pos_t)(writePos-actionSound->start)-(newLength));
			else
				printf("YEAH NO SAMPLES OFF!!!\n");
		}
	}

	actionSound->stop=actionSound->start+newLength-1;

	if(!prepareForUndo)
		freeAllTempPools();

	return(true);
}

sample_fpos_t CChangeRateEffect::getWriteLength(sample_pos_t oldLength,const CGraphParamValueNode &rateNode1,const CGraphParamValueNode &rateNode2) const
{
	double s1=rateNode1.y;
	double s2=rateNode2.y;
	sample_fpos_t readStart=rateNode1.x*oldLength;
	sample_fpos_t readStop=rateNode2.x*oldLength;
	if(readStart==readStop) // vertical rate change
		return(0.0);

	sample_fpos_t m=(s2-s1)/(readStop-readStart);

	if(m!=0.0) // do this check in the caller function too
	{
		sample_fpos_t c=(1.0/m)*sample_fpos_log(sample_fpos_fabs(s1));
		sample_fpos_t wp=(1.0/m)*sample_fpos_log(sample_fpos_fabs(m*readStop-m*readStart+s1))-c;
		return(wp);
	}
	else
		return(1.0/s1*(readStop-readStart));
}

sample_pos_t CChangeRateEffect::getNewSelectionLength(const CActionSound *actionSound) const
{
	sample_pos_t oldLength=actionSound->selectionLength();
	sample_fpos_t tNewLength=0.0;
	for(unsigned t=0;t<rateCurve.size()-1;t++)
		tNewLength+=sample_fpos_round(getWriteLength(oldLength,rateCurve[t],rateCurve[t+1]));

	if(tNewLength<0.0 || tNewLength>MAX_LENGTH)
		return(NIL_SAMPLE_POS);

	return((sample_pos_t)tNewLength); // round?
}

AAction::CanUndoResults CChangeRateEffect::canUndo(const CActionSound *actionSound) const
{
	return(curYes);
}

void CChangeRateEffect::undoActionSizeSafe(const CActionSound *actionSound)
{
	restoreSelectionFromTempPools(actionSound,actionSound->start,undoRemoveLength);
}



// ---------------------------------------------

CSimpleChangeRateEffectFactory::CSimpleChangeRateEffectFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog) :
	AActionFactory(N_("Change Rate"),"",channelSelectDialog,dialog)
{
}

CSimpleChangeRateEffectFactory::~CSimpleChangeRateEffectFactory()
{
}

CChangeRateEffect *CSimpleChangeRateEffectFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	CGraphParamValueNodeList g;
	g.push_back(CGraphParamValueNode(0.0,actionParameters->getValue<double>("Rate Change")));
	g.push_back(CGraphParamValueNode(1.0,actionParameters->getValue<double>("Rate Change")));
	return new CChangeRateEffect(this,actionSound,g);
}

// ---------------------------------------------

CCurvedChangeRateEffectFactory::CCurvedChangeRateEffectFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog) :
	AActionFactory(N_("Curved Change Rate"),_("Change Rate According to a Curve"),channelSelectDialog,dialog)
{
}

CCurvedChangeRateEffectFactory::~CCurvedChangeRateEffectFactory()
{
}

CChangeRateEffect *CCurvedChangeRateEffectFactory::manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const
{
	if(actionParameters->getValue<CGraphParamValueNodeList>("Rate Curve").size()<2)
		throw(runtime_error(string(__func__)+" -- nodes contains less than 2 nodes"));
	return new CChangeRateEffect(this,actionSound,actionParameters->getValue<CGraphParamValueNodeList>("Rate Curve"));
}



