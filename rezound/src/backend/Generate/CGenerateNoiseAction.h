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

#ifndef __CGenerateNoiseAction_H__
#define __CGenerateNoiseAction_H__

#include "../../../config/common.h"


#include "../AAction.h"

class CGenerateNoiseAction : public AAction
{
public:
	enum NoiseTypes
	{
		ntWhite=0,
		ntPink=1,
		// others still unassigned are brown,blue,green,black,grey,violet,binary.
	};

	enum StereoImage
	{
		siIndependent=0,
		siMono=1,
		siInverse=2,
		siSpatialStereo=3
	};
							    // length in seconds
	CGenerateNoiseAction(const CActionSound actionSound,const double noiseLength,const double volume, const NoiseTypes noiseType, const StereoImage stereoImage); 
	virtual ~CGenerateNoiseAction();
	
protected:
	bool doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound &actionSound);
	CanUndoResults canUndo(const CActionSound &actionSound) const;

private:
			#warning this should be defined by the selection, or give the user a choice
	const double noiseLength; // length in seconds
	const double volume;	// multiplier 0(silent) to 1(most loud)
	const NoiseTypes noiseType;
	const StereoImage stereoImage;

	// ??? what's this comment for?
	// 'Spatial stereo' may be omitted if we have a spatial stereo
	// transform.

	sample_pos_t origLength;



	// --- algorithm stuff ----------------------------

	double getRandNoiseVal(const int noiseChannel);

	// Pink noise stuff that follows adapted from Phil Burk's (??? URL?)

	// patest_pinknoise.c
	#define PINK_MAX_RANDOM_ROWS (30)
	#define PINK_RANDOM_BITS (16)
	#define PINK_RANDOM_SHIFT ((sizeof(long)*8)-PINK_RANDOM_BITS)
	struct PinkNoise
	{
		long pink_Rows[PINK_MAX_RANDOM_ROWS];
		long pink_RunningSum;
		int pink_Index;
		int pink_IndexMask;
		float pink_Scalar;
	} pinkL,pinkR;

	void initializePinkNoise(PinkNoise *pink,int numRows);
	static double generatePinkNoise(PinkNoise *pink);

};

class CGenerateNoiseActionFactory : public AActionFactory
{
public:
	CGenerateNoiseActionFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog);
	virtual ~CGenerateNoiseActionFactory();

	CGenerateNoiseAction *manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters) const;
};

#endif
