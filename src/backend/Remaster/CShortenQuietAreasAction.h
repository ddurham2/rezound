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

#ifndef __CShortenQuietAreasAction_H__
#define __CShortenQuietAreasAction_H__

#include "../../../config/common.h"

class CShortenQuietAreasAction;
class CShortenQuietAreasActionFactory;

#include "../AAction.h"

#include <auto_array.h> // from misc/

class CShortenQuietAreasAction : public AAction
{
public:
	CShortenQuietAreasAction(const AActionFactory *factory,const CActionSound *actionSound,const float quietThreshold,const float quietTime,const float unquietTime,const float detectorWindow,float shortenFactor/*[0..1]*/);
	virtual ~CShortenQuietAreasAction();

protected:
	bool doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound *actionSound);
	CanUndoResults canUndo(const CActionSound *actionSound) const;

private:
	const float quietThreshold;
	const float quietTime;
	const float unquietTime;
	const float detectorWindow;
	const float shortenFactor;

	sample_pos_t undoRemoveLength;

	void alterQuietAreaLength(CActionSound *actionSound,sample_pos_t &destPos,const sample_pos_t unquietCounter,const sample_pos_t dQuietBeginPos,const sample_pos_t dQuietEndPos,const sample_pos_t sQuietBeginPos,const sample_pos_t sQuietEndPos,auto_array<const CRezPoolAccesser> &srces,auto_array<const CRezPoolAccesser> &alt_srces,auto_array<CRezPoolAccesser> &dests);
};

class CShortenQuietAreasActionFactory : public AActionFactory
{
public:
	CShortenQuietAreasActionFactory(AActionDialog *normalDialog);
	virtual ~CShortenQuietAreasActionFactory();

	CShortenQuietAreasAction *manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const;
};

#endif
