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

#ifndef __CSelectionEdit_H__
#define __CSelectionEdit_H__

#include "../../../config/common.h"


//#include "../CSoundPlayerChannel.h"
#include "../AAction.h"

enum Selections
{
	sSelectAll=0,			// select everything
	sSelectToBeginning=1,		// make start selection position move to the beginning
	sSelectToEnd=2,			// make stop selection position move to the end
	sFlopToBeginning=3,		// move the stop position to the start position and then the start position to the beginning
	sFlopToEnd=4,			// move the start position to the stop position and then the stop position to the end
	sSelectToSelectStart=5,		// move the start position forward to the stop position
	sSelectToSelectStop=6,		// move the stop position backward to the start position

		// !!! If I add more... change gSelectionNames AND gSelectionDescriptions in CSelectionEdit.cpp !!!
};


class CSelectionEdit : public AAction
{
public:
	/*
		For selection changes, actionSound is to have the undoPositions
		and the new positions are passed in as parameters or a predefined
		selection.
	*/
	CSelectionEdit(const CActionSound actionSound,Selections selection);
	CSelectionEdit(const CActionSound actionSound,sample_pos_t selectStart,sample_pos_t selectStop);
	virtual ~CSelectionEdit();


protected:
	bool doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound &actionSound);
	CanUndoResults canUndo(const CActionSound &actionSound) const;

	bool doesWarrantSaving() const;

private:

	Selections selection;
	sample_pos_t selectStart,selectStop;

};

/*
 * Here the factory has a parameter of how to construct the action before
 * it's performed.  This is because a selection edit is fairly simple and
 * I didn't want to create a differen AAction derivation for each type of
 * selection change
 */

class CSelectionEditFactory : public AActionFactory
{
public:
	CSelectionEditFactory(Selections selection);

	CSelectionEdit *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const;

private:
	Selections selection;
};

/* 
 * Here is another factory for the same CSelectionAction which has specific 
 * positions to use for the selection change
 */
class CSelectionEditPositionFactory : public AActionFactory
{
public:
	CSelectionEditPositionFactory();

	CSelectionEdit *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const;

	sample_pos_t selectStart,selectStop;
};



#endif
