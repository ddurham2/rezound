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

#include "LADSPAActions.h"

#ifdef USE_LADSPA

#include "utils.h"

#include "../AFrontendHooks.h"

const vector<CLADSPAActionFactory *> getLADSPAActionFactories()
{
	vector<CLADSPAActionFactory *> v;

	const vector<const LADSPA_Descriptor *> pluginDescriptors=findLADSPAPlugins();
	for(size_t t=0;t<pluginDescriptors.size();t++)
	{
		// only include plugins that have at least one audio output port
		for(unsigned i=0;i<pluginDescriptors[t]->PortCount;i++)
		{
			const LADSPA_PortDescriptor pd=pluginDescriptors[t]->PortDescriptors[i];
			if(LADSPA_IS_PORT_OUTPUT(pd) && LADSPA_IS_PORT_AUDIO(pd))
			{
				v.push_back(new CLADSPAActionFactory(pluginDescriptors[t],NULL,gFrontendHooks->getLADSPAActionDialog(pluginDescriptors[t])));
				break;
			}
		}
	}

	return v;
}

#endif
