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

#ifndef __CBurnToCDAction_H__
#define __CBurnToCDAction_H__


#include "../../../config/common.h"

#include "../AAction.h"
#include <stdio.h>

class CBurnToCDAction : public AAction
{
public:
	CBurnToCDAction(const AActionFactory *factory,const CActionSound *actionSound,const string tempSpaceDir,const string pathTo_cdrdao,const unsigned burnSpeed,const unsigned gapBetweenTracks,const string device,const string extra_cdrdao_options,const bool selectionOnly,const bool testOnly);
	virtual ~CBurnToCDAction();

	static const string getExplanation();
	static const string detectDevice(const string pathTo_cdrdao);

protected:
	bool doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound *actionSound);
	CanUndoResults canUndo(const CActionSound *actionSound) const;

	bool doesWarrantSaving() const;

private:
	const string tempSpaceDir;
	const string pathTo_cdrdao;
	const unsigned burnSpeed;
	const unsigned gapBetweenTracks;
	const string device;
	const string extra_cdrdao_options;
	const bool selectionOnly;
	const bool testOnly;

	void showStatus(FILE *p);

};

class CBurnToCDActionFactory : public AActionFactory
{
public:
	CBurnToCDActionFactory(AActionDialog *channelSelectDialog);
	virtual ~CBurnToCDActionFactory();

	CBurnToCDAction *manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const;
};

#endif
