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

#ifndef __CCueAction_H__
#define __CCueAction_H__

#include "../../../config/common.h"


#include "../AAction.h"

class CAddCueAction : public AAction
{
public:
	CAddCueAction(const CActionSound actionSound,const string cueName,const sample_pos_t cueTime,const bool isAnchored);
	virtual ~CAddCueAction();


protected:
	bool doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound &actionSound);
	CanUndoResults canUndo(const CActionSound &actionSound) const;

private:
	const string cueName;
	const sample_pos_t cueTime;
	const bool isAnchored;

};


class CAddCueActionFactory : public AActionFactory
{
public:
	CAddCueActionFactory(AActionDialog *normalDialog);

	CAddCueAction *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const;

private:

};


// ------------------------------------------

class CRemoveCueAction : public AAction
{
public:
	CRemoveCueAction(const CActionSound actionSound,const size_t cueIndex);
	virtual ~CRemoveCueAction();


protected:
	bool doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound &actionSound);
	CanUndoResults canUndo(const CActionSound &actionSound) const;

private:
	const size_t removeCueIndex;

};


class CRemoveCueActionFactory : public AActionFactory
{
public:
	CRemoveCueActionFactory();

	CRemoveCueAction *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const;

private:
};

// ------------------------------------------

class CReplaceCueAction : public AAction
{
public:
	CReplaceCueAction(const CActionSound actionSound,const size_t cueIndex,const string cueName,const sample_pos_t cueTime,const bool isAnchored);
	virtual ~CReplaceCueAction();


protected:
	bool doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound &actionSound);
	CanUndoResults canUndo(const CActionSound &actionSound) const;

private:
	const size_t cueIndex;
	const string cueName;
	const sample_pos_t cueTime;
	const bool isAnchored;

};


class CReplaceCueActionFactory : public AActionFactory
{
public:
	CReplaceCueActionFactory(AActionDialog *normalDialog);

	CReplaceCueAction *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const;

private:

};



#endif
