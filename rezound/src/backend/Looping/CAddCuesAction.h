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

#ifndef __CAddCuesAction_H__
#define __CAddCuesAction_H__

#include "../../../config/common.h"

class CAddCuesAction;
class CAddCuesActionFactory;

#include "../AAction.h"

class CAddCuesAction : public AAction
{
public:
	CAddCuesAction(const AActionFactory *factory,const CActionSound *actionSound,const string cueName,const unsigned cueCount,const bool anchoredInTime);
	CAddCuesAction(const AActionFactory *factory,const CActionSound *actionSound,const string cueName,const double timeInterval,const bool anchoredInTime);
	virtual ~CAddCuesAction();

protected:
	bool doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound *actionSound);
	CanUndoResults canUndo(const CActionSound *actionSound) const;

private:
	const string cueName;
	const unsigned cueCount;
	const double timeInterval;
	const bool anchoredInTime;

};

class CAddNCuesActionFactory : public AActionFactory
{
public:
	CAddNCuesActionFactory(AActionDialog *channelSelectDialog);
	virtual ~CAddNCuesActionFactory();

	CAddCuesAction *manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const;
};

class CAddTimedCuesActionFactory : public AActionFactory
{
public:
	CAddTimedCuesActionFactory(AActionDialog *channelSelectDialog);
	virtual ~CAddTimedCuesActionFactory();

	CAddCuesAction *manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const;
};

#endif
