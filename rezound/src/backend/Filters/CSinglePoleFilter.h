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

#ifndef __CSinglePoleFilter_H__
#define __CSinglePoleFilter_H__

#include "../../../config/common.h"


class CSinglePoleFilter;
class CSinglePoleLowpassFilterFactory;
class CSinglePoleHighpassFilterFactory;
class CBandpassFilterFactory;
class CNotchFilterFactory;

#include "../AAction.h"

class CSinglePoleFilter : public AAction
{
public:
	enum FilterTypes
	{
		ftLowpass,
		ftHighpass,
		ftBandpass,
		ftNotch // aka band-reject
	};

	CSinglePoleFilter(const CActionSound &actionSound,FilterTypes filterType,float gain,float frequency,float bandwidth=0.0);

protected:
	bool doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound &actionSound);
	CanUndoResults canUndo(const CActionSound &actionSound) const;

private:
	const FilterTypes filterType;
	const float gain;
	const float frequency; // cut-off frequency or band frequency
	const float bandwidth; // only applies to bandpass and notch

};




class CSinglePoleLowpassFilterFactory : public AActionFactory
{
public:
	CSinglePoleLowpassFilterFactory(AActionDialog *channelSelectDialog,AActionDialog *normalDialog);

	CSinglePoleFilter *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const;
};

class CSinglePoleHighpassFilterFactory : public AActionFactory
{
public:
	CSinglePoleHighpassFilterFactory(AActionDialog *channelSelectDialog,AActionDialog *normalDialog);

	CSinglePoleFilter *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const;
};

// ??? Technically, I don't know if "single-pole" is applicable terminology for bandpass and notch filters

class CBandpassFilterFactory : public AActionFactory
{
public:
	CBandpassFilterFactory(AActionDialog *channelSelectDialog,AActionDialog *normalDialog);

	CSinglePoleFilter *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const;
};

class CNotchFilterFactory : public AActionFactory
{
public:
	CNotchFilterFactory(AActionDialog *channelSelectDialog,AActionDialog *normalDialog);

	CSinglePoleFilter *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const;
};

#endif
