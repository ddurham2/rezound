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

#ifndef __CGenerateToneAction_H__
#define __CGenerateToneAction_H__

#include "../../../config/common.h"


#include "../AAction.h"

class CGenerateToneAction : public AAction
{
public:
	enum ToneTypes
	{
		ttSineWave=0,
		ttSquareWave=1,
		ttRisingSawtoothWave=2,
		ttFallingSawtoothWave=3,
		ttTriangleWave=4,
	};
							    // length in seconds
	CGenerateToneAction(const CActionSound actionSound,const float length,const float volume,const float frequency,const ToneTypes toneType); 
	virtual ~CGenerateToneAction();
	
protected:
	bool doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound &actionSound);
	CanUndoResults canUndo(const CActionSound &actionSound) const;

private:

	const float length;
	const float volume;
	const float frequency;
	const ToneTypes toneType;

	sample_pos_t origLength;
};

class CGenerateToneActionFactory : public AActionFactory
{
public:
	CGenerateToneActionFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog);
	virtual ~CGenerateToneActionFactory();

	CGenerateToneAction *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const;
};

#endif
