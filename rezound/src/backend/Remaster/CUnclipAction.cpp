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

#include "CUnclipAction.h"

#include <algorithm>
#include <iostream>

#include "../unit_conv.h"

CUnclipAction::CUnclipAction(const CActionSound actionSound) :
	AAction(actionSound)
{
}

CUnclipAction::~CUnclipAction()
{
}

/*
 *    Three control point Bezier interpolation
 *       mu ranges from 0 to 1, start to end of the curve
 *    Obtained from: http://astronomy.swin.edu.au/pbourke/curves/bezier/ and modified
 */
struct XY { XY(sample_fpos_t _x,double _y) : x(_x),y(_y) { } sample_fpos_t x; double y; };
static const XY Bezier3(const XY p1,const XY p2,const XY p3,double mu)
{
	double mu2 = mu * mu;
	double mum1 = 1 - mu;
	double mum12 = mum1 * mum1;
	return XY(
		p1.x * mum12 + 2.0 * p2.x * mum1 * mu + p3.x * mu2,
		p1.y * mum12 + 2.0 * p2.y * mum1 * mu + p3.y * mu2
		);
}

/*
 * Returns a point which is the intersection of the two lines defined by point 1, p2 and it's slope, slope1  and line from p2 and slope2
 */
static const XY Intersect(const XY p1,double m1,const XY p2,double m2)
{
	const sample_fpos_t x1=p1.x;
	const double y1=p1.y;
	const sample_fpos_t x2=p2.x;
	const double y2=p2.y;

	//y==m1*(x-x1)+y1;
	//y==m2*(x-x2)+y2;
	// I set these two equations equal to each other and solved for x to get the complex thing below
	
	sample_fpos_t x=(-m2*x2+y2+m1*x1-y2)/(m1-m2);
	double y=m1*(x-x1)+y1;

	return XY(x,y);
}



bool CUnclipAction::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	const sample_pos_t start=actionSound.start;
	const sample_pos_t stop=actionSound.stop;
	const sample_pos_t selectionLength=actionSound.selectionLength();

	if(prepareForUndo)
		moveSelectionToTempPools(actionSound,mmSelection,actionSound.selectionLength());

	for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
	{
		#warning my clipped region finding algorithm needs help.. its finding things which are not region which need to be repaired and  making up data for them
		/* However the making up of new data is I believe on the right track.. it needs some tweaking 
		 * in that it doesn't always create data which is continuous with the original data.
		 */

		if(actionSound.doChannel[i])
		{
			BEGIN_PROGRESS_BAR("Unclip -- Channel "+istring(i),start,stop);

			CRezPoolAccesser a=actionSound.sound->getAudio(i);

			if(prepareForUndo)
				// copy from the temp pool back to the dest and just use from then on
				a.copyData(start,actionSound.sound->getTempAudio(tempAudioPoolKey,i),0,selectionLength);

			const sample_t threshold=512; // some fraction of a dB I suppose
			const sample_pos_t minFlatDuration=6; // the minimum duration which the slope remains within range of the threshold marking a clipped region
			const sample_pos_t maxFlatDuration=1000; // the maximum duration which the slope remains within range of the threshold marking a clipped region
			const sample_pos_t minNonFlatDuration=8; // the minimum duration which the slope remains outside the range of the threshold marking a the possible surrounding of a clipped region

			{

			/* --  find clipped concave up peaks (  \_/  )
			 * - What I attempt to do here is find where the first derivate changes from <0 to 0 for a while then >0 (in principle).
			 * - Actually, I check for <-threadhold to [-threadhold..+threshold] for a while then >+threshold where threshold is some value relativly near zero
			 * - I do this by doing a floored division by the threshold on the calculated slope
			 * - When I do find this clipped region of data, I approximate two lines thru the surrounding correct data on each side of the 
			 *   flat section and approximate the natural data for the flat section be drawing a bezier curve between those two lines
			 */
			sample_pos_t localStart,localStop;
			sample_pos_t lessThanZeroSlopeCount=0,zeroSlopeCount=0,greaterThanZeroSlopeCount=0;
			sample_pos_t pos=start;
			while((++pos)<=stop)
			{
				const sample_t s1=a[pos-1];
				const sample_t s2=a[pos];
				const sample_t slope=(s2-s1)/threshold;
				//cout << (s2-s1) << "\n";

				if(slope<0)
				{ // possibly starting a local minumum
					localStart=pos;
					zeroSlopeCount=0;
					greaterThanZeroSlopeCount=0;

					lessThanZeroSlopeCount++;
				}
				else if(slope==0)
				{
					if(lessThanZeroSlopeCount>=minNonFlatDuration)
						zeroSlopeCount++;
					else
					{
						// start over counting 
						lessThanZeroSlopeCount=0;
						greaterThanZeroSlopeCount=0;
						zeroSlopeCount=0;
					}
				}
				else //if(slope>0)
				{
					if(greaterThanZeroSlopeCount>=minNonFlatDuration)
					{
						if(zeroSlopeCount>=minFlatDuration && zeroSlopeCount<=maxFlatDuration)
						{
							localStop=pos-1-minNonFlatDuration;
							cout << "found a clipped region: " << localStart << " .. " << localStop << endl;

							// find the average of all the flat region
							mix_sample_t avgOfFlatRegion=0;
							for(sample_pos_t t=localStart;t<=localStop;t++)
								avgOfFlatRegion+=a[t];
							avgOfFlatRegion/=(int)((localStop-localStart)+1);

							// find the average slope of the line going thru the data to the left of the flat region
							mix_sample_t avgLeftSlope=0;
							for(sample_pos_t t=localStart-minNonFlatDuration;t<localStart;t++)
								avgLeftSlope+=a[t+1]-a[t];
							avgLeftSlope/=(int)minNonFlatDuration;

							cout << "avgLeftSlope: " << avgLeftSlope << endl;
								

							// find the average slope of the line going thru the data to the right of the flat region
							mix_sample_t avgRightSlope=0;
							for(sample_pos_t t=localStop;t<localStop+minNonFlatDuration;t++)
								avgRightSlope+=a[t+1]-a[t];
							avgRightSlope/=(int)minNonFlatDuration;

							cout << "avgRightSlope: " << avgRightSlope << endl;


							// now draw a bezier curve between the two lines to guess at the natural data
							// and actually, we're going to mix in the incorrect data's differences too by
							// adding the bezier curve to the data-avgOfFlagRegion
							const XY p1(localStart,a[localStart]);
							const XY p3(localStop,a[localStop]);
							const XY p2(Intersect(p1,avgLeftSlope,p3,avgRightSlope));

								cout << "p1: " << p1.x << "," << p1.y << endl;
								cout << "p2: " << p2.x << "," << p2.y << endl;
								cout << "p3: " << p3.x << "," << p3.y << endl;

							for(sample_pos_t t=localStart;t<=localStop;t++)
							{
								const XY p=Bezier3(p1,p2,p3,otherRange_to_unitRange_linear(t,localStart,localStop));

								//cout << "p: " << p.x << "," << p.y << endl;

								a[t]=ClipSample(p.y/*+(a[t]-avgOfFlatRegion)*/);
							}


						}

						// start over counting 
						lessThanZeroSlopeCount=0;
						greaterThanZeroSlopeCount=0;
						zeroSlopeCount=0;
					}
					else
						greaterThanZeroSlopeCount++;
				}

				UPDATE_PROGRESS_BAR(pos);
			}

			}

			RESET_PROGRESS_BAR()

			{
			//                                         _
			/* --  find clipped concave down peaks (  / \  )
			 * - What I attempt to do here is find where the first derivate changes from >0 to 0 for a while then <0 (in principle).
			 * - Actually, I check for >+threadhold to [-threadhold..+threshold] for a while then <-threshold where threshold is some value relativly near zero
			 * - ... similar as above
			 */
			sample_pos_t localStart,localStop;
			sample_pos_t greaterThanZeroSlopeCount=0,zeroSlopeCount=0,lessThanZeroSlopeCount=0;
			sample_pos_t pos=start;
			while((++pos)<=stop)
			{
				const sample_t s1=a[pos-1];
				const sample_t s2=a[pos];
				const sample_t slope=(s2-s1)/threshold;
				//cout << (s2-s1) << "\n";

				if(slope>0)
				{ // possibly starting a local maximum
					localStart=pos;
					zeroSlopeCount=0;
					lessThanZeroSlopeCount=0;

					greaterThanZeroSlopeCount++;
				}
				else if(slope==0)
				{
					if(greaterThanZeroSlopeCount>=minNonFlatDuration)
						zeroSlopeCount++;
					else
					{
						// start over counting 
						greaterThanZeroSlopeCount=0;
						lessThanZeroSlopeCount=0;
						zeroSlopeCount=0;
					}
				}
				else //if(slope<0)
				{
					if(lessThanZeroSlopeCount>=minNonFlatDuration)
					{
						if(zeroSlopeCount>=minFlatDuration && zeroSlopeCount<=maxFlatDuration)
						{
							localStop=pos-1-minNonFlatDuration;
							cout << "found a clipped region: " << localStart << " .. " << localStop << endl;

							// find the average of all the flat region
							mix_sample_t avgOfFlatRegion=0;
							for(sample_pos_t t=localStart;t<=localStop;t++)
								avgOfFlatRegion+=a[t];
							avgOfFlatRegion/=(int)((localStop-localStart)+1);

							// find the average slope of the line going thru the data to the left of the flat region
							mix_sample_t avgLeftSlope=0;
							for(sample_pos_t t=localStart-minNonFlatDuration;t<localStart;t++)
								avgLeftSlope+=a[t+1]-a[t];
							avgLeftSlope/=(int)minNonFlatDuration;

							cout << "avgLeftSlope: " << avgLeftSlope << endl;
								

							// find the average slope of the line going thru the data to the right of the flat region
							mix_sample_t avgRightSlope=0;
							for(sample_pos_t t=localStop;t<localStop+minNonFlatDuration;t++)
								avgRightSlope+=a[t+1]-a[t];
							avgRightSlope/=(int)minNonFlatDuration;

							cout << "avgRightSlope: " << avgRightSlope << endl;


							// now draw a bezier curve between the two lines to guess at the natural data
							// and actually, we're going to mix in the incorrect data's differences too by
							// adding the bezier curve to the data-avgOfFlagRegion
							const XY p1(localStart,a[localStart]);
							const XY p3(localStop,a[localStop]);
							const XY p2(Intersect(p1,avgLeftSlope,p3,avgRightSlope));

								cout << "p1: " << p1.x << "," << p1.y << endl;
								cout << "p2: " << p2.x << "," << p2.y << endl;
								cout << "p3: " << p3.x << "," << p3.y << endl;

							for(sample_pos_t t=localStart;t<=localStop;t++)
							{
								const XY p=Bezier3(p1,p2,p3,otherRange_to_unitRange_linear(t,localStart,localStop));

								//cout << "p: " << p.x << "," << p.y << endl;

								a[t]=ClipSample(p.y/*+(a[t]-avgOfFlatRegion)*/);
							}


						}

						// start over counting 
						greaterThanZeroSlopeCount=0;
						lessThanZeroSlopeCount=0;
						zeroSlopeCount=0;
					}
					else
						lessThanZeroSlopeCount++;
				}

				UPDATE_PROGRESS_BAR(pos);
			}

			}

			// ??? could do this just for the areas where I work on the local min or maxes
			// invalid if we didn't prepare for undo (which created new and invalidated space)
			if(!prepareForUndo)
				actionSound.sound->invalidatePeakData(i,start,stop);
			
			END_PROGRESS_BAR();
		}
		


		//break; // ??? remove LATER!!!
	}

	return(true);
}

AAction::CanUndoResults CUnclipAction::canUndo(const CActionSound &actionSound) const
{
	// should check some size constraint
	return(curYes);
}

void CUnclipAction::undoActionSizeSafe(const CActionSound &actionSound)
{
	restoreSelectionFromTempPools(actionSound,actionSound.start,actionSound.selectionLength());
}



// ------------------------------

CUnclipActionFactory::CUnclipActionFactory(AActionDialog *channelSelectDialog) :
	AActionFactory("Unclip (just messing around)","(NOTE: I was just messing around trying this.  It is really useless as is.  I'll work on it later) Unclip is used to fix by guessing the natural shape of a very short\nregions in the audio where the peak of a waveform was clipped",false,channelSelectDialog,NULL,NULL)
{
}

CUnclipAction *CUnclipActionFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	return(new CUnclipAction(actionSound));
}

