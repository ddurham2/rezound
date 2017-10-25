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

#ifndef __CTextParamValue_H__
#define __CTextParamValue_H__

#include <QWidget>

#include "../../../config/common.h"
#include "../qt_compat.h"

#include "ui_CTextParamValue.h"

#include <string>

class CNestedDataFile;

class CTextParamValue : public QWidget, public Ui::CTextParamValue
{
	Q_OBJECT
public:
	CTextParamValue(const char *name,const double initialValue,const double minValue,const double maxValue); // make it behave as a numeric entry
	CTextParamValue(const char *name,const string initialValue); // make it behave like a text entry
	virtual ~CTextParamValue();

	void showFrame(bool v);
	void enableMargin(bool v);

	void setUnits(const string units,const string tipText="");

	const double getValue();
	void setValue(const double value);

	const string getText();
	void setText(const string text);

	void setRange(const double minValue,const double maxValue);

	void setStep(const double step);
	const double getStep() const;

	void setDecimals(const int decimals);
	const int getDecimals() const;

	const string getName() const;

	void setTipText(const string &text);
	string getTipText() const;

	void readFromFile(const string &prefix,CNestedDataFile *f);
	void writeToFile(const string &prefix,CNestedDataFile *f);

Q_SIGNALS:
	void changed();

private Q_SLOTS:
	void on_valueTextBox_textChanged(const QString &text);
	void on_valueSpinner_valueChanged(double v);

private:
	const string name;

	const bool isNumeric;

	const string initialValue;
};

#endif
