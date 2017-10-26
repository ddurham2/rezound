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

#include "LADSPAActionDialog.h"

#ifdef ENABLE_LADSPA

#include <QGridLayout>
#include <QScrollArea>

#include <string>
#include <algorithm>

#include "../backend/unit_conv.h"
#include "../backend/LADSPA/utils.h"
#include "../backend/CActionSound.h"
#include "../backend/CActionParameters.h"
#include "../backend/CSound.h" // for getSampleRate()

#include "../backend/ActionParamMappers.h"


LADSPAActionDialog::LADSPAActionDialog(QWidget *mainWindow,const LADSPA_Descriptor *_desc) :
	ActionParamDialog(mainWindow,true,"LADSPA"),
	pluginDesc(_desc),
	GUIInitialized(false)
{
}

void LADSPAActionDialog::initGUI()
{
	if(GUIInitialized)
		return;
	GUIInitialized=true;

	// create tab with routing separate from controls

	QWidget *p0=newVertPanel(NULL,false);
	p0->layout()->setContentsMargins(0,0,0,0);
	//FXPacker *p1=new FXHorizontalFrame(p0,FRAME_RAISED | LAYOUT_TOP|LAYOUT_FILL_X,0,0,0,0, 2,2,2,2, 0,0);
	
	// build information frame
	{
		QWidget *p=newHorzPanel(p0);
		QGridLayout *l=new QGridLayout;
			l->setContentsMargins(0,0,0,0);
		delete p->layout();
		p->setLayout(l);
			int row=0;
			l->addWidget(new QLabel("Name:"),row,0,Qt::AlignRight);
			l->addWidget(new QLabel(pluginDesc->Name),row,1,Qt::AlignLeft);
			row++;

			if(strcmp(pluginDesc->Maker,"")!=0)
			{
				l->addWidget(new QLabel("Author:"),row,0,Qt::AlignRight);
				l->addWidget(new QLabel(pluginDesc->Maker),row,1,Qt::AlignLeft);
				row++;
			}

			if(strcmp(pluginDesc->Copyright,"None")!=0)
			{
				l->addWidget(new QLabel("Copyright:"),row,0,Qt::AlignRight);
				l->addWidget(new QLabel(pluginDesc->Copyright),row,1,Qt::AlignLeft);
				row++;
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


	QWidget *rw=NULL; // routing widget
	if(hasControls)
	{
		QTabWidget *tw=new QTabWidget;
		p0->layout()->addWidget(tw);
		QScrollArea *sa=new QScrollArea; /* ??? perhaps all CActionParamValue panels could scroll like this if requested */
		sa->setFrameShape(QFrame::NoFrame);
		tw->addTab(sa,_("Controls"));
		tw->addTab(rw=newHorzPanel(NULL),_("Routing"));

		// create controls
		//QWidget *scrollWindowParent=new FXPacker(p1,FRAME_RAISED | LAYOUT_FILL_X|LAYOUT_FILL_Y);
		sa->setWidgetResizable(true);
		QWidget *p1=newHorzPanel(NULL);
		sa->setWidget(p1);

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
					addCheckBoxEntry(p1,pluginDesc->PortNames[t],defaultValue>0.0,"");
				else
				{
					/*
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
					*/

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
						// may need to be logrithmic, but maybe the scalar spinner solves the problem 
						CActionParamMapper_linear_range_scaled *m=new CActionParamMapper_linear_range_scaled(defaultValue,minValue,maxValue,(int)initialScalar,minScalar,maxScalar);

						if(LADSPA_IS_HINT_INTEGER(portRangeHintDesc))
							m->floorTheValue(true);

						if(LADSPA_IS_HINT_SAMPLE_RATE(portRangeHintDesc))
							sampleRateMappers.push_back(make_pair((string)pluginDesc->PortNames[t],m));
			
						addSlider(p1,pluginDesc->PortNames[t],"",m,NULL,false);
					}
					else // linear
					{
						//printf("minScalar: %d maxScalar: %d minValue: %f maxValue: %f\n",minScalar,maxScalar,minValue,maxValue);
						CActionParamMapper_linear_range_scaled *m=new CActionParamMapper_linear_range_scaled(defaultValue,minValue,maxValue,(int)initialScalar,minScalar,maxScalar);

						if(LADSPA_IS_HINT_INTEGER(portRangeHintDesc))
							m->floorTheValue(true);

						if(LADSPA_IS_HINT_SAMPLE_RATE(portRangeHintDesc))
							sampleRateMappers.push_back(make_pair((string)pluginDesc->PortNames[t],m));
			
						addSlider(p1,pluginDesc->PortNames[t],"",m,NULL,false);
					}
				}
			}
		}

	}
	else
		rw=p0;

	// create routing tab
	addPluginRoutingParam(rw,N_("Channel Mapping"),pluginDesc);
}


#include "StatusComm.h"
#include "settings.h" // for gSettingsRegistry
#include "../misc/CNestedDataFile/CNestedDataFile.h"
bool LADSPAActionDialog::show(CActionSound *actionSound,CActionParameters *actionParameters)
{
	// doing lazy GUI initializing because it can be SO time expensive given the number of LADSPA plugins that may be loaded
	initGUI();

	// warn user about fftw and LADSPA combined
	if(gSettingsRegistry->getValue<bool>("LADSPA_FFTW_warning_shown")!=true)
	{
		gSettingsRegistry->setValue<bool>("LADSPA_FFTW_warning_shown","true");
		Warning(_("THIS IS THE ONLY TIME THIS WARNING WILL BE SHOWN. SO READ IT!!!\n\nSOME LADSPA plugins use libfftw.  libfftw can be configured to use either floats or doubles as its native sample format (but not both at the same time).  If the LADSPA plugins on this system were compiled with a differently configured libfftw than ReZound was built with, then you may receive garbage from the LADSPA plugin.  If this is the case, then either rebuild ReZound or the LADSPA plugins to make the types match."));
	}

	// set scalar to reflect sample rate
	for(size_t t=0;t<sampleRateMappers.size();t++)
	{
		if((unsigned)sampleRateMappers[t].second->getScalar()!=actionSound->sound->getSampleRate())
		{ // only interrupt previous value if the sample rate is now different
			getSliderParam(sampleRateMappers[t].first)->setScalar(actionSound->sound->getSampleRate());
			getSliderParam(sampleRateMappers[t].first)->setValue(sampleRateMappers[t].second->getDefaultValue());
			//getSliderParam(sampleRateMappers[t].first)->updateNumbers();
		}
	}
	if(ActionParamDialog::show(actionSound,actionParameters))
	{
		// divide values that are sample rates by the sample rate
		for(size_t t=0;t<sampleRateMappers.size();t++)
		{
			actionParameters->setValue<double>(
				sampleRateMappers[t].first,
				actionParameters->getValue<double>(sampleRateMappers[t].first)/actionSound->sound->getSampleRate()
			);
		}

		return true;
	}
	else
		return false;

}

LADSPAActionDialog::~LADSPAActionDialog()
{
}

#endif // ENABLE_LADSPA
