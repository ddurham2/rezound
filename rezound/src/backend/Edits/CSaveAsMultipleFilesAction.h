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

#ifndef __CSaveAsMultipleFilesAction_H__
#define __CSaveAsMultipleFilesAction_H__


#include "../../../config/common.h"

#include "../AAction.h"

class ASoundFileManager;

class CSaveAsMultipleFilesAction : public AAction
{
public:
	CSaveAsMultipleFilesAction(const CActionSound actionSound,ASoundFileManager *soundFileManager,const string directory,const string filenamePrefix,const string filenameSuffix,const string extension,bool openSavedSegments,unsigned segmentNumberOffset);
	virtual ~CSaveAsMultipleFilesAction();

protected:
	bool doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound &actionSound);
	CanUndoResults canUndo(const CActionSound &actionSound) const;

	bool doesWarrantSaving() const;

private:
	ASoundFileManager *soundFileManager;

	const string directory;
	const string filenamePrefix;
	const string filenameSuffix;
	const string extension;
	const bool openSavedSegments;
	const unsigned segmentNumberOffset;

};

class CSaveAsMultipleFilesActionFactory : public AActionFactory
{
public:
	CSaveAsMultipleFilesActionFactory(AActionDialog *channelSelectDialog);
	virtual ~CSaveAsMultipleFilesActionFactory();

	CSaveAsMultipleFilesAction *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const;
};

#endif
