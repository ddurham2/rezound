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

#include "CReverseEffect.h"

CReverseEffect::CReverseEffect(const CActionSound &actionSound) :
	AAction(actionSound)
{
}

bool CReverseEffect::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
	{
		if(actionSound.doChannel[i])
		{
			CRezPoolAccesser a=actionSound.sound->getAudio(i);
			CRezPoolAccesser b=actionSound.sound->getAudio(i);

			// reverse by having one point go from start up to middle 
			// and one go from end back to to middle and swap samples
			// along the way

			sample_pos_t p1=actionSound.start;
			sample_pos_t p2=actionSound.stop;
			sample_pos_t d=actionSound.selectionLength()/2;

			BEGIN_PROGRESS_BAR("Reversing -- Channel "+istring(i),0,d);

			for(sample_pos_t t=0;t<d;t++)
			{
				sample_t temp=a[p1];
				a[p1++]=b[p2];
				b[p2--]=temp;

				UPDATE_PROGRESS_BAR(t);
			}

			END_PROGRESS_BAR();

			actionSound.sound->invalidatePeakData(i,actionSound.start,actionSound.stop);
		}
	}

	return(true);
}

AAction::CanUndoResults CReverseEffect::canUndo(const CActionSound &actionSound) const
{
	return(curYes);
}

void CReverseEffect::undoActionSizeSafe(const CActionSound &actionSound)
{
	CActionSound a(actionSound);
	doActionSizeSafe(a,false);
}


// --------------------------------------------------
//
CReverseEffectFactory::CReverseEffectFactory(AActionDialog *channelSelectDialog) :
	AActionFactory("Reverse","Reverse",false,channelSelectDialog,NULL,NULL,false)
{
}

CReverseEffect *CReverseEffectFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	return(new CReverseEffect(actionSound));
}


