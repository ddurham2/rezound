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

#ifndef __CConvolutionFilter_H__
#define __CConvolutionFilter_H__

#include "../../../config/common.h"


class CConvolutionFilter;
class CConvolutionFilterFactory;

#include "../AAction.h"

class CConvolutionFilter : public AAction
{
public:
	CConvolutionFilter(const CActionSound &actionSound,const float wetdryMix,const float inputGain,const float outputGain,const float inputLowpassFreq,const float predelay,const string filterKernelFilename,const bool openFilterKernelAsRaw,const float filterKernelGain,const float filterKernelLowpassFreq,const float filterKernelRate,const bool reverseFilterKernel,const bool warpDecay);
	virtual ~CConvolutionFilter();

protected:
	bool doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound &actionSound);
	CanUndoResults canUndo(const CActionSound &actionSound) const;

private:
	const float wetdryMix;
	const float inputGain;
	const float outputGain;
	const float inputLowpassFreq;
	const float predelay; // ms

	const string filterKernelFilename;
	const bool openFilterKernelAsRaw;
	const float filterKernelGain;
	const float filterKernelLowpassFreq;
	const float filterKernelRate;
	const bool reverseFilterKernel;
	const bool wrapDecay;

};

class CConvolutionFilterFactory : public AActionFactory
{
public:
	CConvolutionFilterFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog);
	virtual ~CConvolutionFilterFactory();

	CConvolutionFilter *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const;
};

#endif
