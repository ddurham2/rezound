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

#ifndef __CBiquadResFilter_H__
#define __CBiquadResFilter_H__

#include "../../../config/common.h"

class CBiquadResFilter;
class CBiquadResLowpassFilterFactory;
class CBiquadResHighpassFilterFactory;
class CBiquadResBandpassFilterFactory;

#include "../AAction.h"

class CBiquadResFilter : public AAction
{
public:
	enum FilterTypes
	{
		ftLowpass,
		ftHighpass,
		ftBandpass
	};

	CBiquadResFilter(const CActionSound &actionSound,FilterTypes filterType,float gain,float frequency,float bandwidth=0.0);
	virtual ~CBiquadResFilter();

protected:
	bool doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound &actionSound);
	CanUndoResults canUndo(const CActionSound &actionSound) const;

private:
	const FilterTypes filterType;
	const float gain;
	const float frequency; // cut-off frequency
	const float resonance; 

};




class CBiquadResLowpassFilterFactory : public AActionFactory
{
public:
	CBiquadResLowpassFilterFactory(AActionDialog *channelSelectDialog,AActionDialog *normalDialog);
	virtual ~CBiquadResLowpassFilterFactory();

	CBiquadResFilter *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const;
};

class CBiquadResHighpassFilterFactory : public AActionFactory
{
public:
	CBiquadResHighpassFilterFactory(AActionDialog *channelSelectDialog,AActionDialog *normalDialog);
	virtual ~CBiquadResHighpassFilterFactory();

	CBiquadResFilter *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const;
};

class CBiquadResBandpassFilterFactory : public AActionFactory
{
public:
	CBiquadResBandpassFilterFactory(AActionDialog *channelSelectDialog,AActionDialog *normalDialog);
	virtual ~CBiquadResBandpassFilterFactory();

	CBiquadResFilter *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const;
};

#endif
