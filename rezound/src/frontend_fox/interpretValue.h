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

#ifndef __interpretValue_H__
#define __interpretValue_H__

#include "../../config/common.h"

/*
 * These are some interpretValue_xxx/uninterpretValue_xxx functions that are used 
 * commonly in the action parameter dialogs.
 *
 * ??? What I'd like to do eventually is create a base class with interpretValue() and
 * uninterpretValue() methods, the constructor could take range parameters and there
 * would simply be derived classes for different ways to map the domain of [0,1] to 
 * the range.  Then the parameters to the add...ParamValue() methods in CActionParamDialog
 * have only one arg instead of two, and it's not a function pointer, but an object reference
 */

#include <math.h>
#include "../backend/unit_conv.h"

// ----------------------------------------------------------------------------

static const double interpretValue_bipolar_scalar(const double x,const int s)
{ // [0,1] -> [-s,s] -- slider pos to dB
	return unitRange_to_otherRange_linear(x,-s,s);
}

static const double uninterpretValue_bipolar_scalar(const double x,const int s)
{ // [-s,s] -> [0,1] -- dB to slider pos
	return otherRange_to_unitRange_linear(x,-s,s);
}

// ----------------------------------------------------------------------------

static const double interpretValue_dBFS(const double x,const int s)
{ // [0,1] -> (-inf,0] -- slider pos to dBFS
	return amp_to_dBFS(x,1.0);
}

static const double uninterpretValue_dBFS(const double x,const int s)
{ // (-inf,0] -> [0,1] -- dBFS to slider pos
	return dBFS_to_amp(x,1.0);
}

// ----------------------------------------------------------------------------

static const double interpretValue_scalar(const double x,const int s)
{ // [0,1] -> [0,s]
	return x*s;
}

static const double uninterpretValue_scalar(const double x,const int s)
{ // [0,s] -> [0,1]
	return x/s;
}

// ----------------------------------------------------------------------------

/* 'recipsym' means reciprocally symetric as the range of the function is reciprocally symetric around 1 */
static const double interpretValue_recipsym_scalar(const double x,const int s)
{ // [0,1] -> [1/s,s]
	return unitRange_to_recipsymRange_exp(x,s);
}

static const double uninterpretValue_recipsym_scalar(const double x,const int s)
{ // [1/s,s] -> [0,1]
	return recipsymRange_to_unitRange_exp(x,s);
}

#endif
