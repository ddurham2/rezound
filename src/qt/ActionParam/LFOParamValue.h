/* 
 * Copyright (C) 2007 - David W. Durham
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

#ifndef __LFOParamValue_H__
#define __LFOParamValue_H__

#include "../../../config/common.h"
#include "../qt_compat.h"

#include <string>

#include "../backend/ALFO.h"

class CNestedDataFile;
class ConstantParamValue;

#include "ui_LFOParamValue.h"

class LFOParamValue : public QWidget, public Ui::LFOParamValue
{
	Q_OBJECT
public:
    LFOParamValue(const char *name,const string ampUnits,const string ampTitle,const double maxAmp,const string freqUnits,const double maxFreq,const bool hideBipolarLFOs);
    virtual ~LFOParamValue();

	int getDefaultWidth();
	int getDefaultHeight();
	void setMinSize(int minWidth,int minHeight);

	const CLFODescription getValue();

	const string getName() const;

	void readFromFile(const string &prefix,CNestedDataFile *f);
	void writeToFile(const string &prefix,CNestedDataFile *f);

private Q_SLOTS:
	void on_LFOTypeComboBox_currentIndexChanged(int index);

private:
	const string name;

	ConstantParamValue *amplitudeSlider;
	ConstantParamValue *frequencySlider;
	ConstantParamValue *phaseSlider;
};

#endif
