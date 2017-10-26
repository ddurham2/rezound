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

#ifndef ConstantParamValue_H__
#define ConstantParamValue_H__

class ConstantParamValue;

#include "../../../config/common.h"
#include "../qt_compat.h"

#include <string>

#include "../../backend/CSound_defs.h"

#include "../../backend/AActionParamMapper.h"
#include "ui_ConstantParamValue.h"

class CNestedDataFile;

class ConstantParamValue : public QFrame, public Ui::ConstantParamValue
{
	Q_OBJECT
public:
	// display as a slider and a value entry (with optional scalar control)
	// interpretValue should return the value of the slider at the given x where 0<=x<=1 and uninterpretValue should do the inverse
	// minScalar and maxScalar are the min and max values of the scalar spinner, if they are equal, the scalar spinner will not be shown
    ConstantParamValue(AActionParamMapper *valueMapper,bool showInverseButton,const char *name);
    virtual ~ConstantParamValue();

	int getDefaultWidth();
	int getDefaultHeight();
	void setMinSize(int minWidth,int minHeight);

	void setUnits(const string units);

	const double getValue() const;
	void setValue(const double value);

	const int getScalar() const;
	void setScalar(const int scalar);

	const int getMinScalar() const;
	const int getMaxScalar() const;

	const string getName() const;

	void setTipText(const string &text);
	string getTipText() const;

	void enable();
	void disable();

	void readFromFile(const string &prefix,CNestedDataFile *f);
	void writeToFile(const string &prefix,CNestedDataFile *f) const;

protected:
	bool eventFilter(QObject *w,QEvent *e);

private Q_SLOTS:
	// call if the tick labels, slider position, or edit box value need to be reevaluated
	void updateNumbers();

	void on_slider_valueChanged(int value);
	void on_scalarSpinner_valueChanged(int value);
	void on_valueTextBox_editingFinished();
	void on_inverseButton_clicked();

private:
	const string name;

	AActionParamMapper *valueMapper;

	mutable double retValue;

	void prvSetValue(const double value);
	double defaultValue;

	int minWidth,minHeight;
};

#endif
