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

#ifndef __FXLFOParamValue_H__
#define __FXLFOParamValue_H__

#include "../../config/common.h"
#include "fox_compat.h"

#include <string>

#include <fox/fx.h>

#include "../backend/ALFO.h"

class CNestedDataFile;
class FXConstantParamValue;

class FXLFOParamValue : public FXVerticalFrame
{
	FXDECLARE(FXLFOParamValue);
public:
	FXLFOParamValue(FXComposite *p,int opts,const char *name,const string ampUnits,const string ampTitle,const double maxAmp,const string freqUnits,const double maxFreq,const bool hideBipolarLFOs);
	virtual ~FXLFOParamValue();

	long onLFOTypeChange(FXObject *sender,FXSelector sel,void *ptr);

	const CLFODescription getValue();

	const string getName() const;

	void enable();
	void disable();

/*
	void setTipText(const FXString &text);
	FXString getTipText() const;
*/
	void readFromFile(const string &prefix,CNestedDataFile *f);
	void writeToFile(const string &prefix,CNestedDataFile *f);

	enum
	{
		ID_LFO_TYPE_COMBOBOX=FXVerticalFrame::ID_LAST,

		ID_LAST
	};


protected:
	FXLFOParamValue() {}

private:
	const string name;

	FXLabel *titleLabel;
	FXHorizontalFrame *sliders;
		FXConstantParamValue *amplitudeSlider;
		FXConstantParamValue *frequencySlider;
		FXConstantParamValue *phaseSlider;
	FXListBox *LFOTypeComboBox;

	FXFont *textFont;
};

#endif
