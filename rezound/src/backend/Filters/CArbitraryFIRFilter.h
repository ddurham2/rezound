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

#ifndef __CArbitraryFIRFilter_H__
#define __CArbitraryFIRFilter_H__

#include "../../../config/common.h"


class CArbitraryFIRFilter;
class CArbitraryFIRFilterFactory;

#include "../AAction.h"
#include "../CGraphParamValueNode.h"

#if 0 // no no, wait a minute.. I'm going to try to let there be definitions of the number of octaves on the frontend widget
/*
	I have to make this define because:
	On the frontend widget, to be 'industry standard' I want to make the 
	horizontal axis have octaves at equally spaced intervals and with a
	lowest base frequency at the far left.  Secondly, for presets in the
	frontend to work, I don't need the preset to affect sounds differently
	that simply have different sampling rates.  So, I should not make it
	so that the right most edge of the frontend widget represents the 
	sampleRate/2 for what sample is being filtered.

	Hence, I define this value below to be the highest frequency that 
	one would work with.
*/
#define ARBITRARY_FIR_MAX_FREQUENCY 48000
#endif

class CArbitraryFIRFilter : public AAction
{
public:
	CArbitraryFIRFilter(const CActionSound &actionSound,const float wetdryMix,const CGraphParamValueNodeList &freqResponse,const unsigned kernelLength,const bool removeDelay);
	virtual ~CArbitraryFIRFilter();

protected:
	bool doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound &actionSound);
	CanUndoResults canUndo(const CActionSound &actionSound) const;

private:
	const float wetdryMix;
	CGraphParamValueNodeList freqResponse;

	const unsigned kernelLength;

	const bool removeDelay;

};

class CArbitraryFIRFilterFactory : public AActionFactory
{
public:
	CArbitraryFIRFilterFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog);
	virtual ~CArbitraryFIRFilterFactory();

	CArbitraryFIRFilter *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const;
};

#endif
