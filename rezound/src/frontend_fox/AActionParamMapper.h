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

#ifndef __AActionParamMapper_H__
#define __AActionParamMapper_H__

#include "../../config/common.h"

class AActionParamMapper
{
public:
	AActionParamMapper(double defaultValue=0.0,int defaultScalar=0,int _minScalar=0,int _maxScalar=0);
	virtual ~AActionParamMapper();

	double getDefaultValue() const;
	
	virtual double interpretValue(double x)=0;	// implement to convert [0,1] to parameter's value
	virtual double uninterpretValue(double x)=0;	// implement to convert parameter's value to [0,1]

	void setScalar(int _scalar);
	int getScalar() const;

	int getDefaultScalar() const;
	int getMinScalar() const;
	int getMaxScalar() const;

private:
	const double defaultValue;

	const int defaultScalar,minScalar,maxScalar;

	int scalar;
};

#endif
