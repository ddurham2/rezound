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

#ifndef __CRotateEdit_H__
#define __CRotateEdit_H__

#include "../../../config/common.h"


#include "../AAction.h"

class CRotateEdit : public AAction
{
public:
	enum RotateTypes { rtLeft, rtRight };

	CRotateEdit(const CActionSound actionSound,const RotateTypes rotateType,const double amount);
	virtual ~CRotateEdit();

protected:
	bool doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound &actionSound);
	CanUndoResults canUndo(const CActionSound &actionSound) const;

private:
	const RotateTypes rotateType;
	const sample_pos_t amount;

	static void rotate(const CActionSound &actionSound,const RotateTypes rotateType,const sample_pos_t amount);

};

class CRotateLeftEditFactory : public AActionFactory
{
public:
	CRotateLeftEditFactory(AActionDialog *channelSelectDialog,AActionDialog *normalDialog);

	CRotateEdit *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const;
};

class CRotateRightEditFactory : public AActionFactory
{
public:
	CRotateRightEditFactory(AActionDialog *channelSelectDialog,AActionDialog *normalDialog);

	CRotateEdit *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const;
};

#endif
