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

#include "AActionParamMapper.h"

#include <algorithm>

#warning a better name might be "Normalizer"

AActionParamMapper::AActionParamMapper(double _defaultValue,int _defaultScalar,int _minScalar,int _maxScalar) :
	defaultValue(_defaultValue),

	// if minScalar!=maxScalar make sure _defaultScalar is in range, otherwise just set it
	defaultScalar(_minScalar==_maxScalar ? _defaultScalar : min(_maxScalar,max(_minScalar,_defaultScalar))),
	minScalar(_minScalar),
	maxScalar(_maxScalar),

	scalar(defaultScalar)
{
}

AActionParamMapper::~AActionParamMapper()
{
}

double AActionParamMapper::getDefaultValue() const
{
	return defaultValue;
}


int AActionParamMapper::getDefaultScalar() const
{
	return defaultScalar;
}

int AActionParamMapper::getMinScalar() const
{
	return minScalar;
}

int AActionParamMapper::getMaxScalar() const
{
	return maxScalar;
}


void AActionParamMapper::setScalar(int _scalar)
{
	if(minScalar!=maxScalar)
		scalar=min(maxScalar,max(minScalar,_scalar));
	else
		scalar=_scalar;
}

int AActionParamMapper::getScalar() const
{
	return scalar;
}

