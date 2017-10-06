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

#ifndef __CChangeTempoAction_H__
#define __CChangeTempoAction_H__

#include "../../../config/common.h"

#include "../AAction.h"

class CChangeTempoAction : public AAction
{
public:
	CChangeTempoAction(const AActionFactory *factory,const CActionSound *actionSound,const double tempoChange,bool useAntiAliasFilter,unsigned antiAliasFilterLength,bool useQuickSeek,unsigned sequenceLength,unsigned seekingWindowLength,unsigned overlapLength);
	virtual ~CChangeTempoAction();

protected:
	bool doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound *actionSound);
	CanUndoResults canUndo(const CActionSound *actionSound) const;

private:
	sample_pos_t undoRemoveLength;
	const double tempoChange;

	bool useAntiAliasFilter;
	unsigned antiAliasFilterLength;
	bool useQuickSeek;
	unsigned sequenceLength;
	unsigned seekingWindowLength;
	unsigned overlapLength;
};


class CChangeTempoActionFactory : public AActionFactory
{
public:
	CChangeTempoActionFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog);
	virtual ~CChangeTempoActionFactory();

	CChangeTempoAction *manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const;
};

#endif
