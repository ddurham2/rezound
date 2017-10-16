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

#ifndef __CMetersWindow_H__
#define __CMetersWindow_H__

#include "../../config/common.h"
#include "fox_compat.h"

#include <vector>

#include "../backend/CSound_defs.h"

class CLevelMeter;
class CBalanceMeter;
class CAnalyzer;
class CStereoPhaseMeter;

class ASoundPlayer;

class CMetersWindow : public FXHorizontalFrame
{
	FXDECLARE(CMetersWindow)
public:
	CMetersWindow(FXComposite *parent);
	virtual ~CMetersWindow();

	void setSoundPlayer(ASoundPlayer *soundPlayer);

	void resetGrandMaxPeakLevels();

	bool isLevelMetersEnabled() const;
	void enableLevelMeters(bool enable);

	bool isStereoPhaseMetersEnabled() const;
	void enableStereoPhaseMeters(bool enable);

	bool isFrequencyAnalyzerEnabled() const;
	void enableFrequencyAnalyzer(bool enable);

	enum
	{
		ID_UPDATE_CHORE=FXTopWindow::ID_LAST,
		ID_UPDATE_TIMEOUT,
		ID_LABEL_FRAME,
		ID_GRAND_MAX_PEAK_LEVEL_LABEL,

		ID_LAST
	};

	long onUpdateMeters(FXObject *sender,FXSelector,void*);

	long onLabelFrameConfigure(FXObject *sender,FXSelector,void*);

	long onResetGrandMaxPeakLevels(FXObject *sender,FXSelector,void*);

	void showHideAll();
	
protected:
	CMetersWindow() {}

private:
	FXFont *statusFont;
	FXPacker *levelMetersFrame;
		FXPacker *headerFrame;
			FXPacker *labelFrame;
			FXLabel *grandMaxPeakLevelLabel;
		FXPacker *balanceMetersFrame;
			FXLabel *balanceMetersRightMargin;

	CAnalyzer *analyzer;

	std::vector<sample_t> samplingForStereoPhaseMeters;
	vector<CStereoPhaseMeter *> stereoPhaseMeters;

	vector<CLevelMeter *> levelMeters;
	vector<CBalanceMeter *> balanceMeters;

	ASoundPlayer *soundPlayer;

	FXTimer *timeout;

};


#endif
