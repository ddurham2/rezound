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
#include "../backend/unit_conv.h"



static const double interpretValue_filter(const double x,const int s) { return(x*s); }
static const double uninterpretValue_filter(const double x,const int s) { return(x/s); }

static const double interpretValue_gain(const double x,const int s) { return(unitRange_to_bipolarRange_exp(x,s)); }
static const double uninterpretValue_gain(const double x,const int s) { return(bipolarRange_to_unitRange_exp(x,s)); }



// --- convolution -----------------------

static const double interpretValue_wetdry_mix(const double x,const int s) { return(x*200.0-100.0); }
static const double uninterpretValue_wetdry_mix(const double x,const int s) { return((x+100.0)/200.0); }

static const double froo=log(80.0/(24.0+80.0))/log(0.5); // this is to make the middle come out 0.00dB
static const double interpretValue_dB_gain(const double x,const int s) { return(unitRange_to_otherRange_linear(pow(x,froo),-80,24)); }
static const double uninterpretValue_dB_gain(const double x,const int s) { return(pow(otherRange_to_unitRange_linear(x,-80,24),1.0/froo)); }

static const double interpretValue_delayTime(const double x,const int s) { return(x*s); }
static const double uninterpretValue_delayTime(const double x,const int s) { return(x/s); }

CConvolutionFilterDialog::CConvolutionFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,N_("Convolution Filter"))
{
	// ??? still need predelay sliders
	void *p0=newVertPanel(NULL,true);
			void *p1=newHorzPanel(p0,false);
				addSlider(p1,N_("Wet/Dry Mix"),"%",interpretValue_wetdry_mix,uninterpretValue_wetdry_mix,NULL,50.0,0,0,0,true);

				addSlider(p1,N_("Input Lowpass"),"Hz",interpretValue_filter,uninterpretValue_filter,NULL,25000.0,5,100000,25000,false);
				addSlider(p1,N_("Input Gain"),"dB",interpretValue_dB_gain,uninterpretValue_dB_gain,dB_to_scalar,0.0,0,0,0,false);
				addSlider(p1,N_("Output Gain"),"dB",interpretValue_dB_gain,uninterpretValue_dB_gain,dB_to_scalar,0.0,0,0,0,false);
				addSlider(p1,N_("Predelay"),"ms",interpretValue_delayTime,uninterpretValue_delayTime,NULL,50.0,1,500,100,false);

				void *p2=newVertPanel(p1,false);
					addDiskEntityEntry(p2,N_("Filter Kernel"),"$share/impulse_hall1.wav",FXDiskEntityParamValue::detAudioFilename,_("The Audio File to Use as the Filter Kernel"));

					void *p3=newHorzPanel(p2,false);
						addSlider(p3,N_("FK Gain"),"x",interpretValue_gain,uninterpretValue_gain,NULL,0.06667,1,50,30,true);
						addSlider(p3,N_("FK Lowpass"),"Hz",interpretValue_filter,uninterpretValue_filter,NULL,25000.0,5,100000,25000,false);
						void *p4=newVertPanel(p3,false);
							addSlider(p4,N_("FK Rate"),"x",interpretValue_gain,uninterpretValue_gain,NULL,1.0,2,10,2,true);
							addCheckBoxEntry(p4,N_("Reverse"),false);
			p1=newHorzPanel(p0,false);
				addCheckBoxEntry(p1,N_("Wrap Decay back to Beginning"),false);
}





// --- arbitrary FIR filter --------------

static unsigned gBaseFrequency=20; // is set dynamically at runtime in CArbitraryFIRFilterDialog::show (but would be better if these functions had user data *'s)
static unsigned gNumberOfOctaves=11; // is set dynamically at runtime in CArbitraryFIRFilterDialog::show (but would be better if these functions had user data *'s)
static const double interpretValue_freq(const double x,const int s) { return octave_to_freq(x*gNumberOfOctaves,gBaseFrequency); }
static const double uninterpretValue_freq(const double x,const int s) { return freq_to_octave(x,gBaseFrequency)/gNumberOfOctaves; }

// intention: 0dB change is always in the middle, and the range above/below the gets wider or narrower depending on s (but that s is really only a tenth of what it would normally be)
static const double interpretValue_FIR_change(const double x,const int s) { return scalar_to_dB(x*2)*(s/10.0); }
static const double uninterpretValue_FIR_change(const double x,const int s) { return dB_to_scalar(x/(s/10.0))/2; }

// define the interpretValue_kernelLength and uninterpretValue_kernelLength functions
#ifdef HAVE_LIBFFTW
	#include "../backend/DSP/Convolver.h"
	static vector<size_t> goodFilterKernelSizes;
	static const double interpretValue_kernelLength(const double x,const int s) 
	{ 
		if(goodFilterKernelSizes.size()<=0) // first time
			goodFilterKernelSizes=TFFTConvolverTimeDomainKernel<float,float>::getGoodFilterKernelSizes();

			// use min and max to place reasonable limits on the kernel size
		return min((unsigned)(4*1024*1024),max((size_t)8,goodFilterKernelSizes[(size_t)ceil(x*(goodFilterKernelSizes.size()-2))])); // -2 cause I need padding in the actual convolver
	}
	static const double uninterpretValue_kernelLength(const double x,const int s) 
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
	static const double interpretValue_kernelLength(const double x,const int s) { return floor(x*1024.0); }
	static const double uninterpretValue_kernelLength(const double x,const int s) { return x/1024.0; }
#endif


FXDEFMAP(CArbitraryFIRFilterDialog) CArbitraryFIRFilterDialogMap[]=
{
	//Message_Type			ID							Message_Handler

	FXMAPFUNC(SEL_COMMAND,		CArbitraryFIRFilterDialog::ID_BASE_FREQUENCY,		CArbitraryFIRFilterDialog::onFrequencyRangeChange),
	FXMAPFUNC(SEL_COMMAND,		CArbitraryFIRFilterDialog::ID_NUMBER_OF_OCTAVES,	CArbitraryFIRFilterDialog::onFrequencyRangeChange),
};

FXIMPLEMENT(CArbitraryFIRFilterDialog,CActionParamDialog,CArbitraryFIRFilterDialogMap,ARRAYNUMBER(CArbitraryFIRFilterDialogMap))

CArbitraryFIRFilterDialog::CArbitraryFIRFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,N_("Arbitrary FIR Filter"))
{
	FXPacker *p0=newHorzPanel(NULL);
		addSlider(p0,N_("Wet/Dry Mix"),"%",interpretValue_wetdry_mix,uninterpretValue_wetdry_mix,NULL,100.0,0,0,0,true);
		FXPacker *p1=newVertPanel(p0,false);
			addGraph(p1,N_("Frequency Response"),N_("Frequency"),"Hz",interpretValue_freq,uninterpretValue_freq,N_("Change"),"dB",interpretValue_FIR_change,uninterpretValue_FIR_change,dB_to_scalar,-100,100,20);
			FXPacker *p2=newHorzPanel(p1,false);
				addNumericTextEntry(p2,N_("Base Frequency"),"Hz",20,10,1000,_("This Sets the Lowest Frequency Displayed on the Graph.\nThis is the Frequency of the First Octave."));
				addNumericTextEntry(p2,N_("Number of Octaves"),"",11,1,15,_("This Sets the Number of Octaves Displayed on the Graph.\nBut Note that no Frequency Higher than Half of the Sound's Sampling Rate can be Affected Since it Cannot Contain a Frequency Higher than That."));

		p1=newVertPanel(p0,false);
			addSlider(p1,N_("Kernel Length"),"",interpretValue_kernelLength,uninterpretValue_kernelLength,NULL,1024,0,0,0,false); 
			setTipText("Kernel Length",_("This is the Size of the Filter Kernel Computed (in samples) From Your Curve to be Convolved with the Audio"));
			addCheckBoxEntry(p1,N_("Undelay"),true,_("This Counteracts the Delay Side-Effect of Custom FIR Filters"));

		// ??? I suppose if the add... methods would return the widget, then I could avoid this method call
	FXTextParamValue *w=getTextParam("Base Frequency");
		w->setTarget(this);
		w->setSelector(ID_BASE_FREQUENCY);

	w=getTextParam("Number of Octaves");
		w->setTarget(this);
		w->setSelector(ID_NUMBER_OF_OCTAVES);

	onFrequencyRangeChange(NULL,0,NULL);

// need to try to make some buttons or deformation sliders that change the symmetry (build this symmetry changing code into CGraphParamValue nodes) and I should build this into FXGraphParamValue so that all the widgets can have it
}

long CArbitraryFIRFilterDialog::onFrequencyRangeChange(FXObject *sender,FXSelector sel,void *ptr)
{
	FXGraphParamValue *g=getGraphParam("Frequency Response");

	FXTextParamValue *baseFrequency=getTextParam("Base Frequency");
	FXTextParamValue *numberOfOctaves=getTextParam("Number of Octaves");

	gBaseFrequency=(unsigned)baseFrequency->getValue();
	gNumberOfOctaves=(unsigned)numberOfOctaves->getValue();

	g->updateNumbers();

	return 1;
}

#include "../backend/CActionSound.h"
#include "../backend/CSound.h"
bool CArbitraryFIRFilterDialog::show(CActionSound *actionSound,CActionParameters *actionParameters)
{
	//gSampleRate=actionSound->sound->getSampleRate();
	return CActionParamDialog::show(actionSound,actionParameters);
}








// --- single pole lowpass ---------------

CSinglePoleLowpassFilterDialog::CSinglePoleLowpassFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,N_("Single Pole Lowpass Filter"))
{
	void *p=newHorzPanel(NULL);
		addSlider(p,N_("Gain"),"x",interpretValue_gain,uninterpretValue_gain,NULL,1.0,2,50,2,true);
		addSlider(p,N_("Cutoff Frequency"),"Hz",interpretValue_filter,uninterpretValue_filter,NULL,500.0,5,100000,5000,false);
}

// --- single pole highpass --------------

CSinglePoleHighpassFilterDialog::CSinglePoleHighpassFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,N_("Single Pole Highpass Filter"))
{
	void *p=newHorzPanel(NULL);
		addSlider(p,N_("Gain"),"x",interpretValue_gain,uninterpretValue_gain,NULL,1.0,2,50,2,true);
		addSlider(p,N_("Cutoff Frequency"),"Hz",interpretValue_filter,uninterpretValue_filter,NULL,1000.0,5,100000,10000,false);
}

// --- bandpass --------------------------

CBandpassFilterDialog::CBandpassFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,N_("Bandpass Filter"))
{
	void *p=newHorzPanel(NULL);
		addSlider(p,N_("Gain"),"x",interpretValue_gain,uninterpretValue_gain,NULL,1.0,2,50,2,true);
		addSlider(p,N_("Center Frequency"),"Hz",interpretValue_filter,uninterpretValue_filter,NULL,1000.0,5,100000,10000,false);
		addSlider(p,N_("Band Width"),"Hz",interpretValue_filter,uninterpretValue_filter,NULL,500.0,5,100000,1000,false);
}

// --- notch -----------------------------

CNotchFilterDialog::CNotchFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,N_("Notch Filter"))
{
	void *p=newHorzPanel(NULL);
		addSlider(p,N_("Gain"),"x",interpretValue_gain,uninterpretValue_gain,NULL,1.0,2,50,2,true);
		addSlider(p,N_("Center Frequency"),"Hz",interpretValue_filter,uninterpretValue_filter,NULL,1000.0,5,100000,10000,false);
		addSlider(p,N_("Band Width"),"Hz",interpretValue_filter,uninterpretValue_filter,NULL,500.0,5,100000,1000,false);
}



// --- biquad resonant lowpass -----------

CBiquadResLowpassFilterDialog::CBiquadResLowpassFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,N_("Biquad Resonant Lowpass Filter"))
{
	void *p=newHorzPanel(NULL);
		addSlider(p,N_("Gain"),"x",interpretValue_gain,uninterpretValue_gain,NULL,1.0,2,50,2,true);
		addSlider(p,N_("Cutoff Frequency"),"Hz",interpretValue_filter,uninterpretValue_filter,NULL,500.0,5,100000,5000,false);
		addSlider(p,N_("Resonance"),"x",interpretValue_gain,uninterpretValue_gain,NULL,2.0,1,20,2,true);
}

// --- biquad resonant highpass ----------

CBiquadResHighpassFilterDialog::CBiquadResHighpassFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,N_("Biquad Resonant Highpass Filter"))
{
	void *p=newHorzPanel(NULL);
		addSlider(p,N_("Gain"),"x",interpretValue_gain,uninterpretValue_gain,NULL,1.0,2,50,2,true);
		addSlider(p,N_("Cutoff Frequency"),"Hz",interpretValue_filter,uninterpretValue_filter,NULL,500.0,5,100000,5000,false);
		addSlider(p,N_("Resonance"),"x",interpretValue_gain,uninterpretValue_gain,NULL,2.0,1,20,2,true);
}

// --- biquad resonant bandpass ----------

CBiquadResBandpassFilterDialog::CBiquadResBandpassFilterDialog(FXWindow *mainWindow) :
	CActionParamDialog(mainWindow,N_("Biquad Resonant Bandpass Filter"))
{
	void *p=newHorzPanel(NULL);
		addSlider(p,N_("Gain"),"x",interpretValue_gain,uninterpretValue_gain,NULL,1.0,2,50,2,true);
		addSlider(p,N_("Center Frequency"),"Hz",interpretValue_filter,uninterpretValue_filter,NULL,500.0,5,100000,5000,false);
		addSlider(p,N_("Resonance"),"x",interpretValue_gain,uninterpretValue_gain,NULL,2.0,1,20,2,true);
}

