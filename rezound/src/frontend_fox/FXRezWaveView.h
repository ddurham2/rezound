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

#include <fox/fx.h>

class CLoadedSound;
#include "../backend/CSound_defs.h"

// these need to be static class members
enum UnitTypes { utSeconds, utSamples };
enum ViewTypes { vtNormal, vtLoopSplit };

class FXWaveScrollArea;

class FXRezWaveView : public FXPacker
{
	FXDECLARE(FXRezWaveView)
public:
	FXRezWaveView(FXComposite* p,CLoadedSound *_loadedSound);
	virtual ~FXRezWaveView();

	void setHorzZoomFactor(double v,FXint keyboardState=0);
	double getHorzZoomFactor();
	double getMaxHorzZoomFactor();

	void setVertZoomFactor(double v);
	double getMaxVertZoomFactor();

	void horzScroll(FXint x);

	void drawPlayPosition(sample_pos_t dataPosition,bool justErasing,bool scrollToMakeVisible);

	void centerStartPos();
	void centerStopPos();

	void redraw();

	FXint getCanvasWidth();

	enum LastChangedPosition
	{
		lcpNone,
		lcpStart,
		lcpStop
	};

	void updateFromSelectionChange(LastChangedPosition _lastChangedPosition=lcpNone);

	void updateFromEdit();

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

	FXWindow *rulerPanel;

	FXWaveScrollArea *waveScrollArea;

};

#endif
