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

#ifndef __CNoiseGateAction_H__
#define __CNoiseGateAction_H__

#include "../../../config/common.h"


class CNoiseGateAction;
class CNoiseGateActionFactory;

#include "../AAction.h"

/*
	I called this a noise gate for now, but in harmony central's description of what I implemented, it can be used as an expander too.... 
	But I thought expanders increase the volume when below the threshold... perhaps there is a scalar parameter I would have which said to cut or boost the volume according to the threshold
	I would seem that a compressor would work in a very similar way... the gain would start to increase as the sound got above a threshold
*/

class CNoiseGateAction : public AAction
{
public:
	CNoiseGateAction(const CActionSound &actionSound,const float windowTime,const float threshold,const float gainAttackTime,const float gainReleaseTime);
	virtual ~CNoiseGateAction();

	static const string getExplanation();

protected:
	bool doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound &actionSound);
	CanUndoResults canUndo(const CActionSound &actionSound) const;

private:
	const float windowTime;		// in ms
	const float threshold;		// in dBFS
	const float gainAttackTime;	// in ms
	const float gainReleaseTime;	// in ms

};

class CNoiseGateActionFactory : public AActionFactory
{
public:
	CNoiseGateActionFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog);
	virtual ~CNoiseGateActionFactory();

	CNoiseGateAction *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const;
};

#endif
