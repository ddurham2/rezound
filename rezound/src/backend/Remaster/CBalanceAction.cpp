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

#include "CBalanceAction.h"

#include <stdexcept>

#include "../CActionSound.h"
#include "../CActionParameters.h"

CBalanceAction::CBalanceAction(const CActionSound &actionSound,const CGraphParamValueNodeList &_balanceCurve,const unsigned _channelA,const unsigned _channelB,const BalanceTypes _balanceType) :
	AAction(actionSound),
	balanceCurve(_balanceCurve),
	channelA(_channelA),
	channelB(_channelB),
	balanceType(_balanceType)
{
	if(balanceCurve.size()<2)
		throw runtime_error(string(__func__)+" -- graph parameter 0 contains less than 2 nodes");
}

CBalanceAction::~CBalanceAction()
{
}

bool CBalanceAction::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	if(channelA==channelB)
		return true;

	// set so the backup will only backup what's necessary 
	actionSound.noChannels();
	actionSound.doChannel[channelA]=true;
	actionSound.doChannel[channelB]=true;

	if(prepareForUndo)
		moveSelectionToTempPools(actionSound,mmSelection,actionSound.selectionLength());

	const sample_pos_t start=actionSound.start;
	const sample_pos_t selectionLength=actionSound.selectionLength();

	CStatusBar statusBar("Changing Balance",0,selectionLength,true);

	sample_pos_t srcPos=prepareForUndo ? 0 : start;
	const CRezPoolAccesser srcA=prepareForUndo ? actionSound.sound->getTempAudio(tempAudioPoolKey,channelA) : actionSound.sound->getAudio(channelA);
	const CRezPoolAccesser srcB=prepareForUndo ? actionSound.sound->getTempAudio(tempAudioPoolKey,channelB) : actionSound.sound->getAudio(channelB);

	sample_pos_t destPos=start;
	CRezPoolAccesser destA=actionSound.sound->getAudio(channelA);
	CRezPoolAccesser destB=actionSound.sound->getAudio(channelB);

	CGraphParamValueIterator iter(balanceCurve,selectionLength);
	if(balanceType==btStrictBalance)
	{ // -1 (-100%) means normal A and no B, 0 means normal A and B, 1 means no A and normal B
		for(sample_pos_t t=0;t<selectionLength;t++)
		{
			const float b=iter.next();
			if(b<0.0)
			{
				destA[destPos]=srcA[srcPos];
				destB[destPos]=(sample_t)(srcB[srcPos]*(1.0f+b));
			}
			else if(b>0.0)
			{
				destA[destPos]=(sample_t)(srcA[srcPos]*(1.0f-b));
				destB[destPos]=srcB[srcPos];
			}

			srcPos++;
			destPos++;

			if(statusBar.update(t))
			{ // cancelled
				if(prepareForUndo)
					undoActionSizeSafe(actionSound);
				else
				{
					actionSound.sound->invalidatePeakData(channelA,actionSound.start,t);
					actionSound.sound->invalidatePeakData(channelB,actionSound.start,t);
				}
				return false;
			}
		}
	}
	else if(balanceType==bt1xPan)
	{ // -1 means normal A and no B, 0 means half A and half B, 1 means no A and normal B
		for(sample_pos_t t=0;t<selectionLength;t++)
		{
			const float b=(iter.next()+1.0f)/2.0f;
			destA[destPos]=ClipSample(srcA[srcPos]*(1.0f-b));
			destB[destPos]=ClipSample(srcB[srcPos]*(b));
	
			srcPos++;
			destPos++;
	
			if(statusBar.update(t))
			{ // cancelled
				if(prepareForUndo)
					undoActionSizeSafe(actionSound);
				else
				{
					actionSound.sound->invalidatePeakData(channelA,actionSound.start,t);
					actionSound.sound->invalidatePeakData(channelB,actionSound.start,t);
				}
				return false;
			}
		}
	}
	else if(balanceType==bt2xPan)
	{ // -1 means 2x A and no B, 0 means normal A and normal B, 1 means no A and 2x B
		for(sample_pos_t t=0;t<selectionLength;t++)
		{
			const float b=iter.next()+1.0f;
			destA[destPos]=ClipSample(srcA[srcPos]*(2.0f-b));
			destB[destPos]=ClipSample(srcB[srcPos]*(b));
	
			srcPos++;
			destPos++;
	
			if(statusBar.update(t))
			{ // cancelled
				if(prepareForUndo)
					undoActionSizeSafe(actionSound);
				else
				{
					actionSound.sound->invalidatePeakData(channelA,actionSound.start,t);
					actionSound.sound->invalidatePeakData(channelB,actionSound.start,t);
				}
				return false;
			}
		}
	}
	else
		throw runtime_error(string(__func__)+" -- unhandled balanceType: "+istring(balanceType));

	return true;
}

AAction::CanUndoResults CBalanceAction::canUndo(const CActionSound &actionSound) const
{
	return curYes;
}

void CBalanceAction::undoActionSizeSafe(const CActionSound &_actionSound)
{
	if(channelA==channelB)
		return;

	CActionSound actionSound(_actionSound);

	// set so the backup will only restore what was backed up (wouldn't be necessary accept AAction passes a temporary actionSound to doActionSizeSafe so it's modifications to actionSound didn't stick)
	actionSound.noChannels();
	actionSound.doChannel[channelA]=true;
	actionSound.doChannel[channelB]=true;

	restoreSelectionFromTempPools(actionSound,actionSound.start,actionSound.selectionLength());
}

const string CBalanceAction::getBalanceTypeExplaination()
{
	return "\
Strict Balance: -100% means normal A and no B, 0 means normal A and normal B, 100% means no A and normal B\n\
1x Balance: -100% means normal A and no B, 0 means half A and half B, 100% means no A and normal B\n\
2x Balance: -100% means 2x A and no B, 0 means normal A and normal B, 100% means no A and 2x B\
";
}





// ---------------------------------------------

CSimpleBalanceActionFactory::CSimpleBalanceActionFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog) :
	AActionFactory("Balance","Balance",channelSelectDialog,dialog)
{
}

CSimpleBalanceActionFactory::~CSimpleBalanceActionFactory()
{
}

CBalanceAction *CSimpleBalanceActionFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return new CBalanceAction(
		actionSound,
		actionParameters->getGraphParameter("Balance"),
		actionParameters->getUnsignedParameter("Channel A"),
		actionParameters->getUnsignedParameter("Channel B"),
		(CBalanceAction::BalanceTypes)actionParameters->getUnsignedParameter("Balance Type")
	);
}

// ---------------------------------------------

CCurvedBalanceActionFactory::CCurvedBalanceActionFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog) :
	AActionFactory("Curved Balance","Curved Balance",channelSelectDialog,dialog)
{
}

CCurvedBalanceActionFactory::~CCurvedBalanceActionFactory()
{
}

CBalanceAction *CCurvedBalanceActionFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const
{
	return new CBalanceAction(
		actionSound,
		actionParameters->getGraphParameter("Balance Curve"),
		actionParameters->getUnsignedParameter("Channel A"),
		actionParameters->getUnsignedParameter("Channel B"),
		(CBalanceAction::BalanceTypes)actionParameters->getUnsignedParameter("Balance Type")
	);
}


