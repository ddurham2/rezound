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

#include "drawPortion.h"

#include <math.h>

#include <exception>
#include <algorithm>

#include "settings.h"

#include "../backend/CSound.h"
#include "../backend/unit_conv.h"

//static FXColor playStatusColor=FXRGB(255,0,0);
static FXColor axisColor=FXRGB(64,64,180);
static FXColor dBAxisColor=FXRGB(45,45,45);

FXColor backGroundColor=FXRGB(0,10,58);
static FXColor selectedBackGroundColor=FXRGB(46,98,110);

static FXColor waveformColor=FXRGB(255,255,255);

#define DARKENED_WAVE_ADD (-192)
static FXColor darkenedWaveformColor=FXRGB(min(255, FXREDVAL(waveformColor)+DARKENED_WAVE_ADD),min(255,FXGREENVAL(waveformColor)+DARKENED_WAVE_ADD),min(255,FXBLUEVAL(waveformColor)+DARKENED_WAVE_ADD)); // waveformColor + DARKENED_WAVE_ADD


#define SELECTED_WAVE_ADD 142
static FXColor selectedWaveformColor=FXRGB(min(255, FXREDVAL(selectedBackGroundColor)+SELECTED_WAVE_ADD),min(255,FXGREENVAL(selectedBackGroundColor)+SELECTED_WAVE_ADD),min(255,FXBLUEVAL(selectedBackGroundColor)+SELECTED_WAVE_ADD)); // selectedBackGroundColor + SELECTED_WAVE_ADD
	
FXColor clippedWaveformColor=FXRGB(255,0,127); // pink to stand out

static inline float sample_to_y(sample_t sampleValue,float vertZoomFactor,int vertOffset,float channelOffset);

/* TODO
 * 	there seems to be a slight drawing bug (STILL) in that if I zoom all the way in to the sound
 * 	and set the start position just up against the first non-zero sample (and zero samples preceed the audio)
 * 	when I zoom out it the selected portion doesn't look to cover the whole audio... same for the end postion too except still shifted to the left
 *
 */


// ??? some of these int types would need to be changed to be sample_pos_t or int64_t if >31bits zoomed-in lengths were supported
/*
 * NOTE: need to lock sound's size before calling this to be safe
 *
 * left: 		X position in the dc of where to start drawing
 * width:		how far past left to draw
 * dc:			the drawing context to draw on
 * sound:		the sound to render from
 * canvasWidth:		the total width of the canvas that is being drawn on
 * drawSelectStart:	the X position in the dc where the selection begins
 * drawSelectStop:	the X position in the dc where the selection ends
 * horzZoomFactor:	how many sample positions are represented by one pixels.  hence, 1.0 is zoomed all the way in
 * hOffset:		how many sample positions the left edge of the canvas is offset
 * vertZoomFactor:	how many sample values are represented by one pixel. hence 1.0 is zoomed all the way in
 * vOffset:		how many sample values the middle of a channel is offset by
 * darkened:		true if the whole drawing is to be somewhat darkened
 * invert colors:	true if to bitwise not any color when drawing
 *
 * NOTE: on vertZoomFactor and vOffset, it should be thought of as rendering a single channel.  IOW, and caculations done
 * outside this function should not be concerned with how many channels are being rendered on screen.
 * NOTE: on vertZoomFactor and vOffset, values should be calculated as if there were only MAX_WAVE_HEIGHT possible sample
 * values i.e. whether sample_t is float, int16_t or int32_t, etc these values should be calculated that there are 
 * MAX_WAVE_HEIGHT number of sample values.
 *
 */
void drawPortion(int left,int width,FXDCWindow *dc,CSound *sound,int canvasWidth,int canvasHeight,int drawSelectStart,int drawSelectStop,double horzZoomFactor,sample_pos_t hOffset,float vertZoomFactor,int vOffset,bool darkened,bool invertColors)
{
	vertZoomFactor*=(float)sound->getChannelCount();
	vOffset/=(int)sound->getChannelCount();

	try
	{
		struct // created this just because the code came over from C++Builder
		{
			int Left,Right,Top,Bottom;
		} R;


		// Clip Boundries
		int top,bottom,right;

		if(left<0)
		{
			width+=left;
			left=0;
		}

		if(width<=0)
			return;

		top=0;
		right=left+width-1;
		bottom=canvasHeight-1;

		if(right>=canvasWidth)
		{
			right=canvasWidth;
			width=(right-left)+1;
		}

		if(right<0 || left>=canvasWidth)
			return;

		//static int g=0; // used to watch when actually drawing takes place for testing purposes
		//printf("draw: c: %d   left: %d right: %d width: %d\n",g++,left,right,width);

		// Draw the waveform and axies
		if(sound!=NULL && !sound->isEmpty())
		{
			float _channelTop=0;
			const float channelHeight=(float)canvasHeight/(float)sound->getChannelCount();

			float channelOffset=channelHeight/2;
			if(left<0)
			{
				width-=(-left);
				left=0;
			}

			for(unsigned i=0;i<sound->getChannelCount();i++)
			{
				const int channelTop=(int)round(_channelTop);

				const CRezPoolAccesser a=sound->getAudio(i);
				const sample_pos_t soundLength=sound->getLength();

				// draw waveform
				for(int x=left;x<=right;x++)
				{
					const sample_pos_t dataPosition=(sample_pos_t)((x+hOffset)*horzZoomFactor);

					// this is false when drawing the right margin of unused space or when zoomed out on a sound of very few samples
					if(dataPosition<soundLength)
					{
						const sample_pos_t next_dataPosition=(sample_pos_t)((x+hOffset+1)*horzZoomFactor);

						RPeakChunk r=sound->getPeakData(i,dataPosition,next_dataPosition,a);

						float min_y=sample_to_y(r.max,vertZoomFactor,vOffset,channelOffset);
						if(min_y<channelOffset-channelHeight/2)
							min_y=channelOffset-channelHeight/2;
						else if(min_y>channelOffset+channelHeight/2)
							min_y=channelOffset+channelHeight/2;

						float max_y=sample_to_y(r.min,vertZoomFactor,vOffset,channelOffset);
						if(max_y<channelOffset-channelHeight/2)
							max_y=channelOffset-channelHeight/2;
						else if(max_y>channelOffset+channelHeight/2)
							max_y=channelOffset+channelHeight/2;


						// determine the color to draw the waveform and background
						FXColor wfc;
						FXColor bgc;

						if(x>=drawSelectStart && x<=drawSelectStop)
						{ // drawing in selected region
							wfc=selectedWaveformColor;
							bgc=selectedBackGroundColor;
						}
						else
						{ // not drawing in selected region
							if(!darkened)
							{
								wfc=waveformColor;
								bgc=backGroundColor;
							}
							else
							{
								wfc=darkenedWaveformColor;
								bgc=backGroundColor;
							}
						}

						// but if sample was clipped, make it stand out
						if(gRenderClippingWarning && (r.min<=MIN_SAMPLE || r.max>=MAX_SAMPLE))
						{
							// make sure we draw the pink line to or thru the axis
							if(r.min<=MIN_SAMPLE && r.max<0)
								min_y=channelOffset+vOffset;
							if(r.max>=MAX_SAMPLE && r.min>0)
								max_y=channelOffset+vOffset;

							wfc=clippedWaveformColor;
						}


						// Doing it by drawing the background as a line above and below the min and max 
						// sample position pretty much eliminates the flicker I used to get when 
						// painting the entire background and then drawing the waveform on top of that.

						const int _min_y=(int)round(min_y);
						const int _max_y=(int)round(max_y);
						
						// draw the background
						dc->setForeground(invertColors ? ~bgc : bgc);
						dc->drawLine(x,channelTop,x,_min_y-1);
						dc->drawLine(x,_max_y+1,x,(int)round(channelTop+channelHeight));

						// draw the waveform
						dc->setForeground(invertColors ? ~wfc : wfc);
						dc->drawLine(x,_min_y,x,_max_y);
					}
					else
					{
						dc->setForeground(invertColors ? ~backGroundColor : backGroundColor);
						dc->drawLine(x,(int)round(channelTop),x,(int)round(channelTop+channelHeight));
					}
				}

				dc->setFunction(BLT_SRC_XOR_DST); // xor the axis on so it doesn't cover up waveform

				// draw axis
				{
					dc->setForeground(invertColors ? ~axisColor : axisColor);

					float y=round(channelOffset+vOffset);
					if(y<channelOffset-channelHeight/2)
						y=channelOffset-channelHeight/2;
					else if(y>channelOffset+channelHeight/2)
						y=channelOffset+channelHeight/2;

					dc->drawLine(left,(int)y,right,(int)y);
				}
		
				// draw -6dB lines
				{
					int y;
					dc->setForeground(invertColors ? ~dBAxisColor : dBAxisColor);

					y=(int)round(sample_to_y(dBFS_to_amp(-6.0),vertZoomFactor,vOffset,channelOffset));
					if(y>=channelOffset-channelHeight/2 && y<=channelOffset+channelHeight/2)
						dc->drawLine(left,y,right,y);

					y=(int)round(sample_to_y(-dBFS_to_amp(-6.0),vertZoomFactor,vOffset,channelOffset));
					if(y>=channelOffset-channelHeight/2 && y<=channelOffset+channelHeight/2)
						dc->drawLine(left,y,right,y);
				}

				dc->setFunction(); // set draw mode back to default


				channelOffset+=channelHeight;
				_channelTop+=channelHeight;
			}
		}
		else
		{
			// Paint the background empty
			dc->setForeground(backGroundColor);
			dc->setFillStyle(FILL_SOLID);
			dc->fillRectangle(left,0,width,canvasHeight);
		}
	}
	catch(exception &e)
	{
		printf("error -- %s\n",e.what());
	}
}

float sample_to_y(sample_t sampleValue,float vertZoomFactor,int vertOffset,float channelOffset)
{
	//           the sample value, normalized to [-1,1], scaled to MAX_WAVE_HEIGHT (max height/2 because range is -1 to +1) 
	//                                                               then scaled by the vertical zoom factor and finally offset
	return ( ( (-(float)sampleValue/MAX_SAMPLE) * (MAX_WAVE_HEIGHT/2) ) / vertZoomFactor) + channelOffset + vertOffset;
	//          negated because up the screen represents greater sample values
}

