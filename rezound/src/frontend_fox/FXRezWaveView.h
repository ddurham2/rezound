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

#ifndef __FXRezWaveView_H__
#define __FXRezWaveView_H__

#include "../../config/common.h"
#include "fox_compat.h"

class CLoadedSound;
#include "../backend/CSound_defs.h"

#include "FXWaveCanvas.h"

/* just possibilities
// these need to be static class members
enum UnitTypes { utSeconds, utSamples };
enum ViewTypes { vtNormal, vtLoopSplit };
*/

class FXWaveScrollArea;
class FXWaveRuler;

class FXRezWaveView : public FXPacker
{
	FXDECLARE(FXRezWaveView)
public:
	FXRezWaveView(FXComposite* p,CLoadedSound *_loadedSound);
	virtual ~FXRezWaveView();

	void setHorzZoom(double v,FXWaveCanvas::HorzRecenterTypes horzRecenterType);
	double getHorzZoom() const;

	void setVertZoom(float v);
	float getVertZoom() const;

	void setHorzOffset(const sample_pos_t offset);
	sample_pos_t getHorzOffset() const;

	void setVertOffset(const int offset);
	int getVertOffset() const;

	void drawPlayPosition(sample_pos_t dataPosition,bool justErasing,bool scrollToMakeVisible);

	void centerStartPos();
	void centerStopPos();
	void showAmount(double seconds,sample_pos_t pos,int marginPixels=0);

	void redraw();

	void updateFromSelectionChange(FXWaveCanvas::LastChangedPositions lastChangedPosition=FXWaveCanvas::lcpNone);
	void updateFromEdit(bool undoing=false);

	void updateRuler();
	void updateRulerFromScroll(int deltaX,FXEvent *event);

	sample_pos_t getSamplePosForScreenX(FXint X) const;

	enum
	{
		ID_RULER_PANEL=FXVerticalFrame::ID_LAST,
		ID_LAST
	};

	enum
	{
		SEL_SELECTION_CHANGED=::SEL_LAST,

		SEL_ADD_CUE,
		SEL_ADD_CUE_AT_START_POSITION,
		SEL_ADD_CUE_AT_STOP_POSITION,
		SEL_REMOVE_CUE,
		SEL_EDIT_CUE,
		SEL_SHOW_CUE_LIST,
		
		SEL_LAST
	};

	void getWaveSize(int &top,int &height);

protected:
	FXRezWaveView() {}

private:
	friend class FXWaveScrollArea;
	friend class FXWaveRuler;

	FXWaveRuler *rulerPanel;

	FXWaveScrollArea *waveScrollArea;

};

#endif
