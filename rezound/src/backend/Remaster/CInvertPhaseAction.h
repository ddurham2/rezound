/* 
 * Copyright (C) 2003 - David W. Durham
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

#ifndef __CInvertPhaseAction_H__
#define __CInvertPhaseAction_H__

#include "../../../config/common.h"


#include "../AAction.h"

class CInvertPhaseAction : public AAction
{
public:
	CInvertPhaseAction(const CActionSound actionSound);
	virtual ~CInvertPhaseAction();

protected:
	bool doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound &actionSound);
	CanUndoResults canUndo(const CActionSound &actionSound) const;

private:
	bool allowCancel;

};

class CInvertPhaseActionFactory : public AActionFactory
{
public:
	CInvertPhaseActionFactory(AActionDialog *channelSelectDialog);
	virtual ~CInvertPhaseActionFactory();

	CInvertPhaseAction *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const;

	const string getExplanation() const;
};

#endif
