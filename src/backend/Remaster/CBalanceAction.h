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

#ifndef __CBalanceAction_H__
#define __CBalanceAction_H__

#include "../../../config/common.h"

#include "../AAction.h"
#include "../CGraphParamValueNode.h"

class CBalanceAction : public AAction
{
public:
	enum BalanceTypes
	{
		btStrictBalance=0,
		bt1xPan=1,
		bt2xPan=2
	};

	CBalanceAction(const AActionFactory *factory,const CActionSound *actionSound,const CGraphParamValueNodeList &balanceCurve,const unsigned channelA,const unsigned channelB,const BalanceTypes balanceType);
	virtual ~CBalanceAction();

	static const string getBalanceTypeExplanation();

protected:
	bool doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound *actionSound);
	CanUndoResults canUndo(const CActionSound *actionSound) const;

private:
	const CGraphParamValueNodeList balanceCurve;
	const unsigned channelA,channelB;
	const BalanceTypes balanceType;
};


class CSimpleBalanceActionFactory : public AActionFactory
{
public:
	CSimpleBalanceActionFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog);
	virtual ~CSimpleBalanceActionFactory();

	CBalanceAction *manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const;
};

class CCurvedBalanceActionFactory : public AActionFactory
{
public:
	CCurvedBalanceActionFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog);
	virtual ~CCurvedBalanceActionFactory();

	CBalanceAction *manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const;
};

#endif
