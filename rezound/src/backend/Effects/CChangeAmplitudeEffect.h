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

#ifndef __CChangeAmplitudeEffect_H__
#define __CChangeAmplitudeEffect_H__

#include "../../../config/common.h"

#include "../AAction.h"
#include "../CGraphParamValueNode.h"

class CChangeAmplitudeEffect : public AAction
{
public:
	CChangeAmplitudeEffect(const CActionSound &actionSound,const CGraphParamValueNodeList &volumeCurve);

protected:
	bool doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound &actionSound);
	CanUndoResults canUndo(const CActionSound &actionSound) const;

private:
	const CGraphParamValueNodeList volumeCurve;
};


class CChangeVolumeEffectFactory : public AActionFactory
{
public:
	CChangeVolumeEffectFactory(AActionDialog *channelSelectDialog,AActionDialog *normalDialog);

	CChangeAmplitudeEffect *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const;
};

class CGainEffectFactory : public AActionFactory
{
public:
	CGainEffectFactory(AActionDialog *channelSelectDialog,AActionDialog *normalDialog,AActionDialog *advancedDialog);

	CChangeAmplitudeEffect *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const;
};

#endif
