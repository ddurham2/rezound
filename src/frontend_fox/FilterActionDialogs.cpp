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

#include "FilterActionDialogs.h"

#include "CStatusComm.h"

#include "../backend/ActionParamMappers.h"
#include "../backend/unit_conv.h"

#include <vector>


// --- convolution -----------------------


static const double froo=log(80.0/(24.0+80.0))/log(0.5); // this is to make the middle come out 0.00dB
class CActionParamMapper_dB_gain : public AActionParamMapper
{
public:
	CActionParamMapper_dB_gain(double defaultValue) :
		AActionParamMapper(defaultValue)
	{
	}

	virtual ~CActionParamMapper_dB_gain() {}

	double interpretValue(const double x)
	{ 
		return unitRange_to_otherRange_linear(pow(x,froo),-80,24);
	}

	double uninterpretValue(const double x)
	{
		return pow(otherRange_to_unitRange_linear(x,-80,24),1.0/froo);
	}
};


CConvolutionFilterDialog::CConvolutionFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	// ??? still need predelay sliders
	void *p0=newVertPanel(NULL,true);
			void *p1=newHorzPanel(p0,false);
				addSlider(p1,
					N_("Wet/Dry Mix"),
					"%",
					new CActionParamMapper_linear_range(50,-100,100),
					NULL,
					true
				);

				addSlider(p1,
					N_("Input Lowpass"),
					"Hz",
					new CActionParamMapper_linear(25000.0,25000,5,100000),
					NULL,
					false
				);

				addSlider(p1,
					N_("Input Gain"),
					"dB",
					new CActionParamMapper_dB_gain(0.0),
					dB_to_scalar,
					false
				);

				addSlider(p1,
					N_("Output Gain"),
					"dB",
					new CActionParamMapper_dB_gain(0.0),
					dB_to_scalar,
					false
				);

				addSlider(p1,
					N_("Predelay"),
					"ms",
					new CActionParamMapper_linear(50.0,100,1,500),
					NULL,
					false
				);


				void *p2=newVertPanel(p1,false);
					addDiskEntityEntry(p2,N_("Filter Kernel"),"$share/impulse_hall1.wav",FXDiskEntityParamValue::detAudioFilename,_("The Audio File to Use as the Filter Kernel"));

					void *p3=newHorzPanel(p2,false);
						addSlider(p3,
							N_("FK Gain"),
							"x",
							new CActionParamMapper_recipsym(0.06667,30,1,50),
							NULL,
							true
						);

						addSlider(p3,
							N_("FK Lowpass"),
							"Hz",
							new CActionParamMapper_linear(25000.0,25000,5,1000000),
							NULL,
							false
						);

						void *p4=newVertPanel(p3,false);
							addSlider(p4,
								N_("FK Rate"),
								"x",
								new CActionParamMapper_recipsym(1.0,2,2,10),
								NULL,
								true
							);

							addCheckBoxEntry(p4,N_("Reverse"),false);
			p1=newHorzPanel(p0,false);
				addCheckBoxEntry(p1,N_("Wrap Decay back to Beginning"),false);
}





// --- arbitrary FIR filter --------------

class CActionParamMapper_arbitraryFIRFilter_freq : public AActionParamMapper
{
public:
	// is set dynamically at runtime in by CArbitraryFIRFilterDialog
	unsigned baseFrequency;
	unsigned numberOfOctaves;

	CActionParamMapper_arbitraryFIRFilter_freq() :
		AActionParamMapper(),
		baseFrequency(20),
		numberOfOctaves(11)
	{
	}

	virtual ~CActionParamMapper_arbitraryFIRFilter_freq() {}

	double interpretValue(const double x)
	{
		return octave_to_freq(x*numberOfOctaves,baseFrequency);
	}

	double uninterpretValue(const double x)
	{
		return freq_to_octave(x,baseFrequency)/numberOfOctaves;
	}
};

class CActionParamMapper_arbitraryFIRFilter_amp : public AActionParamMapper
{
public:
	CActionParamMapper_arbitraryFIRFilter_amp(double defaultValue,int defaultScalar,int minScalar,int maxScalar) :
		AActionParamMapper(defaultValue,defaultScalar,minScalar,maxScalar)
	{
	}

	virtual ~CActionParamMapper_arbitraryFIRFilter_amp() {}

	// intention: 0dB change is always in the middle, and the range above/below the gets wider or narrower depending on s (but that s is really only a tenth of what it would normally be)
	double interpretValue(const double x)
	{
		return scalar_to_dB(x*2)*(getScalar()/10.0);
	}

	double uninterpretValue(const double x)
	{
		return dB_to_scalar(x/(getScalar()/10.0))/2;
	}
};

#include "../backend/DSP/Convolver.h"
class CActionParamMapper_arbitraryFIRFilter_kernelLength : public AActionParamMapper
{
public:
	// is set dynamically at runtime in by CArbitraryFIRFilterDialog
	unsigned baseFrequency;
	unsigned numberOfOctaves;

	CActionParamMapper_arbitraryFIRFilter_kernelLength(double defaultValue) :
		AActionParamMapper(defaultValue)
	{
	}

	virtual ~CActionParamMapper_arbitraryFIRFilter_kernelLength() {}

#ifdef HAVE_LIBFFTW
	double interpretValue(const double x)
	{
		if(goodFilterKernelSizes.size()<=0) // first time
			goodFilterKernelSizes=TFFTConvolverTimeDomainKernel<float,float>::getGoodFilterKernelSizes();

			// use min and max to place reasonable limits on the kernel size
		return min((size_t)(4*1024*1024),max((size_t)8,goodFilterKernelSizes[(size_t)ceil(x*(goodFilterKernelSizes.size()-2))])); // -2 cause I need padding in the actual convolver
	}

	double uninterpretValue(const double x)
	{
		if(goodFilterKernelSizes.size()<=0) // first time
			goodFilterKernelSizes=TFFTConvolverTimeDomainKernel<float,float>::getGoodFilterKernelSizes();

		for(size_t t=0;t<goodFilterKernelSizes.size();t++)
		{
			if(goodFilterKernelSizes[t]==x)
				return ((double)t)/(goodFilterKernelSizes.size()-2); // -2 cause I need padding in the actual convolver
		}
		return 0;
	}
#else 
	// just for show
	double interpretValue(const double x) { return floor(x*1024.0); }
	double uninterpretValue(const double x) { return x/1024.0; }
#endif

private:
	vector<size_t> goodFilterKernelSizes;
};



FXDEFMAP(CArbitraryFIRFilterDialog) CArbitraryFIRFilterDialogMap[]=
{
	//Message_Type			ID							Message_Handler

	FXMAPFUNC(SEL_COMMAND,		CArbitraryFIRFilterDialog::ID_BASE_FREQUENCY,		CArbitraryFIRFilterDialog::onFrequencyRangeChange),
	FXMAPFUNC(SEL_COMMAND,		CArbitraryFIRFilterDialog::ID_NUMBER_OF_OCTAVES,	CArbitraryFIRFilterDialog::onFrequencyRangeChange),
};

FXIMPLEMENT(CArbitraryFIRFilterDialog,CActionParamDialog,CArbitraryFIRFilterDialogMap,ARRAYNUMBER(CArbitraryFIRFilterDialogMap))

CArbitraryFIRFilterDialog::CArbitraryFIRFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	FXPacker *p0=newHorzPanel(NULL);
		addSlider(p0,
			N_("Wet/Dry Mix"),
			"%",
			new CActionParamMapper_linear_range(100.0,-100,100),
			NULL,
			true
		);

		FXPacker *p1=newVertPanel(p0,false);
			addGraph(p1,
				N_("Frequency Response"),
				N_("Frequency"),
				"Hz",
				freqMapper=new CActionParamMapper_arbitraryFIRFilter_freq,
				N_("Change"),
				"dB",
				new CActionParamMapper_arbitraryFIRFilter_amp(0.0,20,-100,100),
				dB_to_scalar
			);

			FXPacker *p2=newHorzPanel(p1,false);
				FXTextParamValue *baseFreq=addNumericTextEntry(p2,
					N_("Base Frequency"),
					"Hz",
					20,
					10,
					1000,
					_("This Sets the Lowest Frequency Displayed on the Graph.\nThis is the Frequency of the First Octave.")
				);
				baseFreq->setTarget(this);
				baseFreq->setSelector(ID_BASE_FREQUENCY);

				FXTextParamValue *numberOfOctaves=addNumericTextEntry(p2,
					N_("Number of Octaves"),
					"",
					11,
					1,
					15,
					_("This Sets the Number of Octaves Displayed on the Graph.\nBut Note that no Frequency Higher than Half of the Sound's Sampling Rate can be Affected Since it Cannot Contain a Frequency Higher than That.")
				);
				numberOfOctaves->setTarget(this);
				numberOfOctaves->setSelector(ID_NUMBER_OF_OCTAVES);

		p1=newVertPanel(p0,false);
			addSlider(p1,
				N_("Kernel Length"),
				"",
				new CActionParamMapper_arbitraryFIRFilter_kernelLength(1024),
				NULL,
				false
			); 
			setTipText("Kernel Length",_("This is the Size of the Filter Kernel Computed (in samples) From Your Curve to be Convolved with the Audio"));

			addCheckBoxEntry(p1,
				N_("Undelay"),
				true,
				_("This Counteracts the Delay Side-Effect of Custom FIR Filters")
			);

	onFrequencyRangeChange(NULL,0,NULL);
}

long CArbitraryFIRFilterDialog::onFrequencyRangeChange(FXObject *sender,FXSelector sel,void *ptr)
{
	FXGraphParamValue *g=getGraphParam("Frequency Response");

	FXTextParamValue *baseFrequency=getTextParam("Base Frequency");
	FXTextParamValue *numberOfOctaves=getTextParam("Number of Octaves");

	freqMapper->baseFrequency=(unsigned)baseFrequency->getValue();
	freqMapper->numberOfOctaves=(unsigned)numberOfOctaves->getValue();

	g->updateNumbers();

	return 1;
}




// --- morphing arbitrary FIR filter --------------

FXDEFMAP(CMorphingArbitraryFIRFilterDialog) CMorphingArbitraryFIRFilterDialogMap[]=
{
	//Message_Type			ID								Message_Handler

	FXMAPFUNC(SEL_COMMAND,		CMorphingArbitraryFIRFilterDialog::ID_BASE_FREQUENCY,		CMorphingArbitraryFIRFilterDialog::onFrequencyRangeChange),
	FXMAPFUNC(SEL_COMMAND,		CMorphingArbitraryFIRFilterDialog::ID_NUMBER_OF_OCTAVES,	CMorphingArbitraryFIRFilterDialog::onFrequencyRangeChange),
	FXMAPFUNC(SEL_COMMAND,		CMorphingArbitraryFIRFilterDialog::ID_USE_LFO_CHECKBOX,		CMorphingArbitraryFIRFilterDialog::onUseLFOCheckBox),
	FXMAPFUNC(SEL_COMMAND,		CMorphingArbitraryFIRFilterDialog::ID_COPY_1_TO_2,		CMorphingArbitraryFIRFilterDialog::on1To2Button),
	FXMAPFUNC(SEL_COMMAND,		CMorphingArbitraryFIRFilterDialog::ID_COPY_2_TO_1,		CMorphingArbitraryFIRFilterDialog::on1To2Button),
	FXMAPFUNC(SEL_COMMAND,		CMorphingArbitraryFIRFilterDialog::ID_SWAP_1_AND_2,		CMorphingArbitraryFIRFilterDialog::on1To2Button),
};

FXIMPLEMENT(CMorphingArbitraryFIRFilterDialog,CActionParamDialog,CMorphingArbitraryFIRFilterDialogMap,ARRAYNUMBER(CMorphingArbitraryFIRFilterDialogMap))

CMorphingArbitraryFIRFilterDialog::CMorphingArbitraryFIRFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	FXPacker *p0,*p1,*p2,*p3;

	p0=newVertPanel(NULL);
		p1=newHorzPanel(p0,false);
			FXGraphParamValue *g1=addGraph(p1,
				N_("Frequency Response 1"),
				N_("Frequency"),
				"Hz",
				freqMapper=new CActionParamMapper_arbitraryFIRFilter_freq,
				N_("Change"),
				"dB",
				new CActionParamMapper_arbitraryFIRFilter_amp(0.0,20,-100,100),
				dB_to_scalar
			);
			g1->setMinSize(450,280);

			p2=new FXVerticalFrame(p1,LAYOUT_CENTER_Y, 0,0,0,0, 0,0,0,0, 0,0);
				new FXButton(p2,FXString("->\t")+_("Copy Response 1 to Response 2"),NULL,this,ID_COPY_1_TO_2,BUTTON_NORMAL|LAYOUT_FILL_X);
				new FXButton(p2,FXString("<>\t")+_("Swap Response 1 and Response 2"),NULL,this,ID_SWAP_1_AND_2,BUTTON_NORMAL|LAYOUT_FILL_X);
				new FXButton(p2,FXString("<-\t")+_("Copy Response 2 to Response 1"),NULL,this,ID_COPY_2_TO_1,BUTTON_NORMAL|LAYOUT_FILL_X);

			FXGraphParamValue *g2=addGraph(p1,
				N_("Frequency Response 2"),
				N_("Frequency"),
				"Hz",
				freqMapper,
				N_("Change"),
				"dB",
				new CActionParamMapper_arbitraryFIRFilter_amp(0.0,20,-100,100),
				dB_to_scalar
			);
			g2->setMinSize(450,280);
			
		p1=newHorzPanel(p0,false);
			p2=newHorzPanel(p1,false);
			p2->setLayoutHints(p2->getLayoutHints()&(~LAYOUT_FILL_X));

				FXConstantParamValue *wetdryMix=addSlider(p2,
					N_("Wet/Dry Mix"),
					"%",
					new CActionParamMapper_linear_range(100.0,-100,100),
					NULL,
					true
				);
				wetdryMix->setMinSize(0,150);

				p3=newVertPanel(p2,false);
					FXCheckBoxParamValue *useLFOCheckBox=addCheckBoxEntry(p3,
						N_("Use LFO"),
						false
					);
					useLFOCheckBox->setTarget(this);
					useLFOCheckBox->setSelector(ID_USE_LFO_CHECKBOX);

					FXLFOParamValue *lfo=addLFO(p3,
						N_("Sweep LFO"),
						"ms",
						"",
						0,
						"Hz",
						20,
						true
					);
					lfo->setMinSize(0,170);

				p3=newVertPanel(p2,false);
					FXConstantParamValue *kernelLength=addSlider(p3,
						N_("Kernel Length"),
						"",
						new CActionParamMapper_arbitraryFIRFilter_kernelLength(1024),
						NULL,
						false
					); 
					setTipText("Kernel Length",_("This is the Size of the Filter Kernel Computed (in samples) From Your Curve to be Convolved with the Audio"));
					kernelLength->setMinSize(0,150);

					addCheckBoxEntry(p3,
						N_("Undelay"),
						true,
						_("This Counteracts the Delay Side-Effect of Custom FIR Filters")
					);

			p2=newVertPanel(p1,false);
			p2->setLayoutHints(p2->getLayoutHints()&(~LAYOUT_FILL_Y));
			p2->setLayoutHints(p2->getLayoutHints()&(~LAYOUT_FILL_X));
				FXTextParamValue *baseFrequency=addNumericTextEntry(p2,
					N_("Base Frequency"),
					"Hz",
					20,
					10,
					1000,
					_("This Sets the Lowest Frequency Displayed on the Graph.\nThis is the Frequency of the First Octave.")
				);
					baseFrequency->setTarget(this);
					baseFrequency->setSelector(ID_BASE_FREQUENCY);
	
				FXTextParamValue *numberOfOctaves=addNumericTextEntry(p2,
					N_("Number of Octaves"),
					"",
					11,
					1,
					15,
					_("This Sets the Number of Octaves Displayed on the Graph.\nBut Note that no Frequency Higher than Half of the Sound's Sampling Rate can be Affected Since it Cannot Contain a Frequency Higher than That.")
				);
					numberOfOctaves->setTarget(this);
					numberOfOctaves->setSelector(ID_NUMBER_OF_OCTAVES);
	

	onFrequencyRangeChange(NULL,0,NULL);
	onUseLFOCheckBox(useLFOCheckBox,0,NULL);
}

long CMorphingArbitraryFIRFilterDialog::onFrequencyRangeChange(FXObject *sender,FXSelector sel,void *ptr)
{
	FXGraphParamValue *g1=getGraphParam("Frequency Response 1");
	FXGraphParamValue *g2=getGraphParam("Frequency Response 2");

	FXTextParamValue *baseFrequency=getTextParam("Base Frequency");
	FXTextParamValue *numberOfOctaves=getTextParam("Number of Octaves");

	freqMapper->baseFrequency=(unsigned)baseFrequency->getValue();
	freqMapper->numberOfOctaves=(unsigned)numberOfOctaves->getValue();

	g1->updateNumbers();
	g2->updateNumbers();

	return 1;
}

long CMorphingArbitraryFIRFilterDialog::onUseLFOCheckBox(FXObject *sender,FXSelector sel,void *ptr)
{
	if(((FXCheckBoxParamValue *)sender)->getValue())
		getLFOParam("Sweep LFO")->enable();
	else
		getLFOParam("Sweep LFO")->disable();
	return 1;
}

long CMorphingArbitraryFIRFilterDialog::on1To2Button(FXObject *sender,FXSelector sel,void *ptr)
{
	switch(FXSELID(sel))
	{
	case ID_COPY_1_TO_2:
		getGraphParam("Frequency Response 2")->copyFrom(getGraphParam("Frequency Response 1"));
		break;
	case ID_COPY_2_TO_1:
		getGraphParam("Frequency Response 1")->copyFrom(getGraphParam("Frequency Response 2"));
		break;
	case ID_SWAP_1_AND_2:
		getGraphParam("Frequency Response 1")->swapWith(getGraphParam("Frequency Response 2"));
		break;
	default:
		throw runtime_error(string(__func__)+" -- unhandled selector");
	}
	return 0;
}

bool CMorphingArbitraryFIRFilterDialog::validateOnOkay()
{
	if(getGraphParam("Frequency Response 1")->getNodes().size()!=getGraphParam("Frequency Response 2")->getNodes().size())
	{
		Error(_("Frequency Response 1 and Frequency Response 2 must contain the same number of nodes"));
		return false;
	}
	return true;
}

#include "../backend/Filters/CMorphingArbitraryFIRFilter.h"
const string CMorphingArbitraryFIRFilterDialog::getExplanation() const
{
	return CMorphingArbitraryFIRFilter::getExplanation();
}









// --- single pole lowpass ---------------

CSinglePoleLowpassFilterDialog::CSinglePoleLowpassFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addSlider(p,
			N_("Gain"),
			"x",
			new CActionParamMapper_recipsym(1.0,2,2,50),
			NULL,
			true
		);

		addSlider(p,
			N_("Cutoff Frequency"),
			"Hz",
			new CActionParamMapper_linear(500.0,5000,5,100000),
			NULL,
			false
		);
}

// --- single pole highpass --------------

CSinglePoleHighpassFilterDialog::CSinglePoleHighpassFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addSlider(p,
			N_("Gain"),
			"x",
			new CActionParamMapper_recipsym(1.0,2,2,50),
			NULL,
			true
		);

		addSlider(p,
			N_("Cutoff Frequency"),
			"Hz",
			new CActionParamMapper_linear(1000.0,10000,5,100000),
			NULL,
			false
		);
}

// --- bandpass --------------------------

CBandpassFilterDialog::CBandpassFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addSlider(p,
			N_("Gain"),
			"x",
			new CActionParamMapper_recipsym(1.0,2,2,50),
			NULL,
			true
		);

		addSlider(p,
			N_("Center Frequency"),
			"Hz",
			new CActionParamMapper_linear(1000.0,10000,5,100000),
			NULL,
			false
		);

		addSlider(p,
			N_("Band Width"),
			"Hz",
			new CActionParamMapper_linear(500.0,1000,5,100000),
			NULL,
			false
		);
}

// --- notch -----------------------------

CNotchFilterDialog::CNotchFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addSlider(p,
			N_("Gain"),
			"x",
			new CActionParamMapper_recipsym(1.0,2,2,50),
			NULL,
			true
		);

		addSlider(p,
			N_("Center Frequency"),
			"Hz",
			new CActionParamMapper_linear(1000.0,10000,5,100000),
			NULL,
			false
		);

		addSlider(p,
			N_("Band Width"),
			"Hz",
			new CActionParamMapper_linear(500.0,1000,5,100000),
			NULL,
			false
		);
}



// --- biquad resonant lowpass -----------

CBiquadResLowpassFilterDialog::CBiquadResLowpassFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addSlider(p,
			N_("Gain"),
			"x",
			new CActionParamMapper_recipsym(1.0,2,2,50),
			NULL,
			true
		);

		addSlider(p,
			N_("Cutoff Frequency"),
			"Hz",
			new CActionParamMapper_linear(500.0,5000,5,100000),
			NULL,
			false
		);

		addSlider(p,
			N_("Resonance"),
			"x",
			new CActionParamMapper_recipsym(2.0,2,1,20),
			NULL,
			true
		);
}

// --- biquad resonant highpass ----------

CBiquadResHighpassFilterDialog::CBiquadResHighpassFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addSlider(p,
			N_("Gain"),
			"x",
			new CActionParamMapper_recipsym(1.0,2,2,50),
			NULL,
			true
		);

		addSlider(p,
			N_("Cutoff Frequency"),
			"Hz",
			new CActionParamMapper_linear(500.0,5000,5,100000),
			NULL,
			false
		);

		addSlider(p,
			N_("Resonance"),
			"x",
			new CActionParamMapper_recipsym(2.0,2,1,20),
			NULL,
			true
		);
}

// --- biquad resonant bandpass ----------

CBiquadResBandpassFilterDialog::CBiquadResBandpassFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow)
{
	void *p=newHorzPanel(NULL);
		addSlider(p,
			N_("Gain"),
			"x",
			new CActionParamMapper_recipsym(1.0,2,2,50),
			NULL,
			true
		);

		addSlider(p,
			N_("Center Frequency"),
			"Hz",
			new CActionParamMapper_linear(500.0,5000,5,100000),
			NULL,
			false
		);

		addSlider(p,
			N_("Resonance"),
			"x",
			new CActionParamMapper_recipsym(2.0,2,1,20),
			NULL,
			true
		);
}

