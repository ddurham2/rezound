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

#include "CLADSPAActionDialog.h"

#ifdef USE_LADSPA

#include <string>
#include <algorithm>

#include "../backend/unit_conv.h"
#include "../backend/LADSPA/utils.h"
#include "../backend/CActionSound.h"
#include "../backend/CActionParameters.h"
#include "../backend/CSound.h" // for getSampleRate()

#include "ActionParamMappers.h"


CLADSPAActionDialog::CLADSPAActionDialog(FXWindow *mainWindow,const LADSPA_Descriptor *_desc) :
	CActionParamDialog(mainWindow,true,"LADSPA"),
	pluginDesc(_desc)
{
	// create tab with routing separate from controls

	FXPacker *p0=newVertPanel(NULL);
	FXPacker *p1=new FXHorizontalFrame(p0,FRAME_RAISED | LAYOUT_TOP|LAYOUT_FILL_X,0,0,0,0, 2,2,2,2, 0,0);
	
	// build information frame
	{
		FXPacker *p;

		p=new FXMatrix(p1,2,MATRIX_BY_COLUMNS|LAYOUT_LEFT,0,0,0,0, 0,0,0,0, 3,0);
			new FXLabel(p,"Name:",NULL,LABEL_NORMAL|LAYOUT_RIGHT);
			new FXLabel(p,pluginDesc->Name);

			if(strcmp(pluginDesc->Maker,"")!=0)
			{
				new FXLabel(p,"Author:",NULL,LABEL_NORMAL|LAYOUT_RIGHT);
				new FXLabel(p,pluginDesc->Maker);
			}

			if(strcmp(pluginDesc->Copyright,"None")!=0)
			{
				new FXLabel(p,"Copyright:",NULL,LABEL_NORMAL|LAYOUT_RIGHT);
				new FXLabel(p,pluginDesc->Copyright);
			}
	}

	bool hasControls=false;
	for(unsigned t=0;t<pluginDesc->PortCount;t++)
	{
		const LADSPA_PortDescriptor portDesc=pluginDesc->PortDescriptors[t];
		if(LADSPA_IS_PORT_CONTROL(portDesc) && LADSPA_IS_PORT_INPUT(portDesc))
		{
			hasControls=true;
			break;
		}
	}


	if(hasControls)
		p1=new FXTabBook(p0,NULL,0,TABBOOK_NORMAL|LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_NONE, 0,0,0,0, 0,0,0,0);
	else
		p1=p0;

	// create controls tab
	if(hasControls)
	{
		new FXTabItem((FXTabBook *)p1,_("Controls"),NULL,TAB_TOP_NORMAL);

		FXPacker *scrollWindowParent=new FXPacker(p1,FRAME_RAISED | LAYOUT_FILL_X|LAYOUT_FILL_Y);
		FXScrollWindow *scrollWindow=new FXScrollWindow(scrollWindowParent,LAYOUT_FILL_X|LAYOUT_FILL_Y); /* ??? perhaps all CActionParamValue panels could scroll like this if requested */
			FXPacker *p2=newHorzPanel(scrollWindow,false);

		for(unsigned t=0;t<pluginDesc->PortCount;t++)
		{
			const LADSPA_PortDescriptor portDesc=pluginDesc->PortDescriptors[t];
			const LADSPA_PortRangeHint portRangeHint=pluginDesc->PortRangeHints[t];
			const LADSPA_PortRangeHintDescriptor portRangeHintDesc=portRangeHint.HintDescriptor;

			if(LADSPA_IS_PORT_CONTROL(portDesc) && LADSPA_IS_PORT_INPUT(portDesc))
			{
				// determine min and max default scalars and value

				// ??? for now this will have to do, but later it would be better to reset parameters for ports that are sample rates and have it depend on the action sound passed to show()
				LADSPA_Data defaultValue=0.0;
				const int hasDefault= getLADSPADefault(&portRangeHint,44100,&defaultValue)==0;

				if(!hasDefault)
					defaultValue=0.0;

				if(LADSPA_IS_HINT_TOGGLED(portRangeHintDesc))
					addCheckBoxEntry(p2,pluginDesc->PortNames[t],defaultValue>0.0,"");
				else
				{
					printf("name:%s:%s int:%d l:%d,%f u:%d,%f logrth:%d sr:%d default:%d,%f\n",
						pluginDesc->Name,
						pluginDesc->PortNames[t],

						LADSPA_IS_HINT_INTEGER(portRangeHintDesc),

						LADSPA_IS_HINT_BOUNDED_BELOW(portRangeHintDesc),
						portRangeHint.LowerBound,

						LADSPA_IS_HINT_BOUNDED_ABOVE(portRangeHintDesc),
						portRangeHint.UpperBound,

						LADSPA_IS_HINT_LOGARITHMIC(portRangeHintDesc),

						LADSPA_IS_HINT_SAMPLE_RATE(portRangeHintDesc),
						hasDefault,

						defaultValue);

					double minValue=0;
					double maxValue=10000;

					if(LADSPA_IS_HINT_BOUNDED_BELOW(portRangeHintDesc))
						minValue=portRangeHint.LowerBound;
					if(LADSPA_IS_HINT_BOUNDED_ABOVE(portRangeHintDesc))
						maxValue=portRangeHint.UpperBound;

					// make sure default is in range (because a default might not have been given)
					defaultValue=min(maxValue,max(minValue,(double)defaultValue));

					int minScalar=1;
					int maxScalar=max(minScalar, (int)ceil(maxValue/10));

					// determine a reasonable scalar
					double initialScalar=(minScalar+maxScalar)/2;
					if(hasDefault)
						initialScalar=max(1.0,min((double)maxScalar,(double)ceil(defaultValue)));

					// beacuse we're using a range, scaled below, I need to divide the min/max value by however high the user can set the scalar
					maxValue/=(double)maxScalar; 
					minValue/=(double)maxScalar; 

					if(LADSPA_IS_HINT_LOGARITHMIC(portRangeHintDesc))
					{
						#warning needs to be logrithmic
						CActionParamMapper_linear_range_scaled *m=new CActionParamMapper_linear_range_scaled(defaultValue,minValue,maxValue,(int)initialScalar,minScalar,maxScalar);

						if(LADSPA_IS_HINT_INTEGER(portRangeHintDesc))
							m->floorTheValue(true);

						if(LADSPA_IS_HINT_SAMPLE_RATE(portRangeHintDesc))
							sampleRateMappers.push_back(make_pair((string)pluginDesc->PortNames[t],m));
			
						addSlider(p2,pluginDesc->PortNames[t],"",m,NULL,false);
					}
					else // linear
					{
						printf("minScalar: %d maxScalar: %d minValue: %f maxValue: %f\n",minScalar,maxScalar,minValue,maxValue);
						CActionParamMapper_linear_range_scaled *m=new CActionParamMapper_linear_range_scaled(defaultValue,minValue,maxValue,(int)initialScalar,minScalar,maxScalar);

						if(LADSPA_IS_HINT_INTEGER(portRangeHintDesc))
							m->floorTheValue(true);

						if(LADSPA_IS_HINT_SAMPLE_RATE(portRangeHintDesc))
							sampleRateMappers.push_back(make_pair((string)pluginDesc->PortNames[t],m));
			
						addSlider(p2,pluginDesc->PortNames[t],"",m,NULL,false);
					}
				}
			}
		}
	}

	// create routing tab
	{
		if(hasControls)
			new FXTabItem((FXTabBook *)p1,_("Routing"),NULL,TAB_TOP_NORMAL);

		FXPacker *p2=newHorzPanel(p1,false);
		addPluginRoutingParam(p2,N_("Channel Mapping"),pluginDesc);
	}
}


bool CLADSPAActionDialog::show(CActionSound *actionSound,CActionParameters *actionParameters)
{
	// set scalar to reflect sample rate
	for(size_t t=0;t<sampleRateMappers.size();t++)
	{
		if((unsigned)sampleRateMappers[t].second->getScalar()!=actionSound->sound->getSampleRate())
		{ // only interrupt previous value if the sample rate is now different
			getSliderParam(sampleRateMappers[t].first)->setScalar(actionSound->sound->getSampleRate());
			getSliderParam(sampleRateMappers[t].first)->setValue(sampleRateMappers[t].second->getDefaultValue());
			getSliderParam(sampleRateMappers[t].first)->updateNumbers();
		}
	}
	if(CActionParamDialog::show(actionSound,actionParameters))
	{
		// divide values that are sample rates by the sample rate
		for(size_t t=0;t<sampleRateMappers.size();t++)
		{
			actionParameters->setDoubleParameter(
				sampleRateMappers[t].first,
				actionParameters->getDoubleParameter(sampleRateMappers[t].first)/actionSound->sound->getSampleRate()
			);
		}

		return true;
	}
	else
		return false;

}

CLADSPAActionDialog::~CLADSPAActionDialog()
{
}

#endif // USE_LADSPA
