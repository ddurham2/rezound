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

#ifndef __CReverseEffect_H__
#define __CReverseEffect_H__

#include "../../../config/common.h"


class CReverseEffect;
class CReverseEffectFactory;

#include "../AAction.h"

class CReverseEffect : public AAction
{
public:
	CReverseEffect(const AActionFactory *factory,const CActionSound *actionSound);
	virtual ~CReverseEffect();

protected:
	bool doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound *actionSound);
	CanUndoResults canUndo(const CActionSound *actionSound) const;

private:
	bool allowCancel;

};

class CReverseEffectFactory : public AActionFactory
{
public:
	CReverseEffectFactory(AActionDialog *channelSelectDialog);
	virtual ~CReverseEffectFactory();

	CReverseEffect *manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const;
};

#endif
