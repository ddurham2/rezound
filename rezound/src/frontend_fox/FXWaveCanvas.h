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

#ifndef __FXWaveCanvas_H__
#define __FXWaveCanvas_H__

/*
 * This is an FXCanvas widget that can draw the waveform on itself.
 * It is given a CSound object to from which get the audio data.
 * It is given a horzontal and vertical zoom and a horizontal and
 * vertical offset parameters for adjusting the view.  
 *
 * This object, of course, has all the events of an FXCanvas for
 * detecting things such as mouse events.
 *
 * ??? Potientially and frequency analysis mode should be supported.
 * 	- I should store calculated data for FFT just like I do
 */

#include "../../config/common.h"
#include "fox_compat.h"

#include "../backend/CSound_defs.h"
class CLoadedSound;

class FXWaveCanvas : public FXCanvas
{
	FXDECLARE(FXWaveCanvas)
public:
	FXWaveCanvas(CLoadedSound* loadedSound, FXComposite* p, FXObject* tgt=NULL, FXSelector sel=0, FXuint opts=FRAME_NORMAL, FXint x=0, FXint y=0, FXint w=0, FXint h=0);
	virtual ~FXWaveCanvas();

	void updateFromEdit(bool undoing=false);

	enum LastChangedPositions { lcpNone,lcpStart,lcpStop };

	enum HorzRecenterTypes { hrtNone,hrtAuto,hrtStart,hrtStop,hrtLeftEdge };

	// v is 0 to 1 (0 is all the way zoomed out, 1 is all the way zoomed in)
	void setHorzZoom(double v,HorzRecenterTypes horzRecenterType=hrtNone);
	const double getHorzZoom() const;

	// returns the width of the wave according to the zoom factor
	const sample_pos_t getHorzSize() const;

	void setHorzOffset(sample_pos_t v);
	const sample_pos_t getHorzOffset() const;



	// v is 0 to 1 (0 is all the way zoomed out, 1 is all the way zoomed in)
	void setVertZoom(float v);
	const float getVertZoom() const;

	// returns the width of the wave according to the zoom factor
	const int getVertSize() const;

	void setVertOffset(int v);
	const int getVertOffset() const;


	const sample_pos_t getHorzOffsetToCenterTime(sample_pos_t time) const;
	const sample_pos_t getHorzOffsetToCenterStartPos() const;
	const sample_pos_t getHorzOffsetToCenterStopPos() const;
	void showAmount(double seconds,sample_pos_t pos,int marginPixels=0);

	const bool isStartPosOnScreen() const;
	const bool isStopPosOnScreen() const;

	const sample_fpos_t getRenderedDrawSelectStart() const;
	const sample_fpos_t getRenderedDrawSelectStop() const;
	const sample_fpos_t getDrawSelectStart() const;
	const sample_fpos_t getDrawSelectStop() const;

	const sample_pos_t snapPositionToCue(sample_pos_t p) const;
	const FXint getCueScreenX(size_t cueIndex) const;
	const sample_pos_t getCueTimeFromX(FXint screenX) const;

	const sample_pos_t getSamplePosForScreenX(FXint X) const;
	void setSelectStartFromScreen(FXint X);
	void setSelectStopFromScreen(FXint X);
	void updateFromSelectionChange(LastChangedPositions lastChangedPosition=lcpNone);

	void drawPlayPosition(sample_pos_t dataPosition,bool justErasing,bool scrollToMakeVisible,FXScrollArea *optScrollArea);


	long onPaint(FXObject *object,FXSelector sel,void *ptr);
	bool first;

protected:
	FXWaveCanvas() {}

private:
	void drawPortion(int left,int width,FXDCWindow *dc);

	CLoadedSound *loadedSound;

	sample_fpos_t horzZoomFactor;
	float vertZoomFactor;

	sample_pos_t horzOffset;
	sample_fpos_t prevHorzZoomFactor_horzOffset;

	int vertOffset;

	FXint prevDrawPlayStatusX;
	sample_pos_t renderedStartPosition,renderedStopPosition;

	double lastHorzZoom;
	float lastVertZoom;

	LastChangedPositions lastChangedPosition;
	bool lastDrawWasUnsuccessful;
};

#endif
