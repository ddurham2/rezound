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


static const double interpretValue_linear_real(const double x,const int s) { return x*s; }
static const double uninterpretValue_linear_real(const double x,const int s) { return x/s; }

static const double interpretValue_linear_int(const double x,const int s) { return floor(x*s); }
static const double uninterpretValue_linear_int(const double x,const int s) { return floor(x/s); }

#warning I still need to make these behave logarithmically
static const double interpretValue_logarithmic_real(const double x,const int s) { return x*s; }
static const double uninterpretValue_logarithmic_real(const double x,const int s) { return x/s; }

static const double interpretValue_logarithmic_int(const double x,const int s) { return floor(x*s); }
static const double uninterpretValue_logarithmic_int(const double x,const int s) { return floor(x/s); }


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
		FXScrollWindow *scrollWindow=new FXScrollWindow(scrollWindowParent,LAYOUT_FILL_X|LAYOUT_FILL_Y); /* ??? perhaps all CActionParamValue panels could do this if asked for */
			FXPacker *p2=newHorzPanel(scrollWindow,false);

		// ??? I need a way of having two opened sound files go to the action (I'm planning this), and then
		// these two sounds might need to go to two inputs on the plugin.  For instance, the vocoder plugin
		// has two inputs the first is the formant and the second is the carrier to use... Right now that would
		// mean that the left channel was the first and the right channel was the second.  Perhaps some dialog
		// could be shown (when I knew to show it) how to route the channels into the plugin

		for(unsigned t=0;t<pluginDesc->PortCount;t++)
		{
			#warning much more work needs to be done on the sliders about sample rates and ranges mins and maxes, might require some changes to how CActionParamDialog works
			const LADSPA_PortDescriptor portDesc=pluginDesc->PortDescriptors[t];
			const LADSPA_PortRangeHint portRangeHint=pluginDesc->PortRangeHints[t];
			const LADSPA_PortRangeHintDescriptor portRangeHintDesc=portRangeHint.HintDescriptor;

			if(LADSPA_IS_PORT_CONTROL(portDesc) && LADSPA_IS_PORT_INPUT(portDesc))
			{
				// ??? for now this will have to do, but later it would be better to reset parameters for ports that are sample rates and have it depend on the action sound passed to show()
				LADSPA_Data defaultValue=0.0;
				const int hasDefault= getLADSPADefault(&portRangeHint,44100,&defaultValue)==0;

				if(!hasDefault)
					defaultValue=0.0;

				// determine min and max and make sure default is in range

				if(LADSPA_IS_HINT_TOGGLED(portRangeHintDesc))
					addCheckBoxEntry(p2,pluginDesc->PortNames[t],defaultValue>0.0,"");
				else
				{
					/*
					printf("name:%s:%s l:%d,%f u:%d,%f sr:%d default:%d,%f\n",
						pluginDesc->Name,
						pluginDesc->PortNames[t],

						LADSPA_IS_HINT_BOUNDED_BELOW(portRangeHintDesc),
						portRangeHint.LowerBound,

						LADSPA_IS_HINT_BOUNDED_ABOVE(portRangeHintDesc),
						portRangeHint.UpperBound,

						LADSPA_IS_HINT_SAMPLE_RATE(portRangeHintDesc),
						hasDefault,

						defaultValue);
					*/

					double minValue=0;
					double maxValue=50000;

					if(LADSPA_IS_HINT_BOUNDED_BELOW(portRangeHintDesc))
						minValue=floor(portRangeHint.LowerBound);
					if(LADSPA_IS_HINT_BOUNDED_ABOVE(portRangeHintDesc))
						maxValue=ceil(portRangeHint.UpperBound);

	//??? really need to use min and max in interpret/uninterpret value since I'm using the ranges of the scalar this way

	// better yet.. when the min and maxs are less than 1 (so the scalars are less 
	// than 1) change the implementation for the interpret and uninterpret values 
	// some how
	//
	// perhaps change interpret and uninterpret to function objects instead of 
	// functions so that I can instantiate real limits and not use the scalar for 
	// that

					if(LADSPA_IS_HINT_SAMPLE_RATE(portRangeHintDesc))
					{
						// This really needs to be done on each time a new sound is used
						// or better yet.. display on the screen the multiplication with 
						// the sample rate, but still internally store the fractions
						minValue*=44100;
						maxValue*=44100;
						defaultValue*=44100;
					}
			
					// determine a reasonable scalar
					double initialScalar=maxValue;
					if(hasDefault)
						initialScalar=max(1.0,min(maxValue,(double)(10*defaultValue)));  // if we have a default make the initial scalar 10 times the default value, but don'e go over the max (and not less than 1)

	// ??? if it's a sample rate, then I would want to set the optRetValueConv parameter

					if(!hasDefault)
						defaultValue=max((LADSPA_Data)minValue,min((LADSPA_Data)maxValue,defaultValue));

					if(LADSPA_IS_HINT_LOGARITHMIC(portRangeHintDesc))
					{
						if(LADSPA_IS_HINT_INTEGER(portRangeHintDesc))
							addSlider(p2,pluginDesc->PortNames[t],"",interpretValue_logarithmic_int,uninterpretValue_logarithmic_int,NULL,defaultValue, (int)minValue,(int)maxValue,(int)initialScalar,false);
						else
							addSlider(p2,pluginDesc->PortNames[t],"",interpretValue_logarithmic_real,uninterpretValue_logarithmic_real,NULL,defaultValue, (int)minValue,(int)maxValue,(int)initialScalar,false);
					}
					else // linear
					{
						if(LADSPA_IS_HINT_INTEGER(portRangeHintDesc))
							addSlider(p2,pluginDesc->PortNames[t],"",interpretValue_linear_int,uninterpretValue_linear_int,NULL,defaultValue, (int)minValue,(int)maxValue,(int)initialScalar,false);
						else
							addSlider(p2,pluginDesc->PortNames[t],"",interpretValue_linear_real,uninterpretValue_linear_real,NULL,defaultValue, (int)minValue,(int)maxValue,(int)initialScalar,false);
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

CLADSPAActionDialog::~CLADSPAActionDialog()
{
}

#endif // USE_LADSPA
