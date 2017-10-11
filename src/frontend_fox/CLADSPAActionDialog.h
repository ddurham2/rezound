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

#ifndef __CLADSPAActionDialog_H__
#define __CLADSPAActionDialog_H__

#include "../../config/common.h"

#ifdef ENABLE_LADSPA

#include "fox_compat.h"

#include "CActionParamDialog.h"
#include "../backend/LADSPA/ladspa.h"

#include <vector>
#include <utility>

class AActionParamMapper;
class CActionSound;
class CActionParameters;

class CLADSPAActionDialog : public CActionParamDialog
{
public:
	CLADSPAActionDialog(FXWindow *mainWindow,const LADSPA_Descriptor *desc);
	virtual ~CLADSPAActionDialog();

	bool show(CActionSound *actionSound,CActionParameters *actionParameters);
private:
	const LADSPA_Descriptor *pluginDesc;
	vector<pair<string, AActionParamMapper *> > sampleRateMappers;
};

#endif // ENABLE_LADSPA

#endif
