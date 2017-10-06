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

#ifndef __utils_H__
#define __utils_H__

#include "../../../config/common.h"


#include "ladspa.h"

#include <vector>

// searches the LADSPA_PATH env var and returns a vector of descriptors for all plugins found
const vector<const LADSPA_Descriptor *> findLADSPAPlugins();


// Find the default value for a port. Return 0 if a default is found and -1 if not
int getLADSPADefault(const LADSPA_PortRangeHint * psPortRangeHint,const unsigned long lSampleRate,LADSPA_Data * pfResult);


// return a default mapping of channels which should be usable for most plugins
#include "../CPluginMapping.h"
	// throws a runtime_error when it cannot determine a default mapping
const CPluginMapping getLADSPADefaultMapping(const LADSPA_Descriptor *plugin,const CSound *sound);


#endif
