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

#ifndef __DSP_Convolver_H__
#define __DSP_Convolver_H__

#include "../../config/common.h"

#include <stdexcept>

#include <istring>

#include "Delay.h"


/* --- TSimpleConvolver ------------------------------------
 * 
 *	This class is a DSP block to do a sample by sample convolution of the given 
 *	array of coefficients with the input given by the repeated calls to processSample()
 *
 *	The frist template parameter specifies the type of the input samples (and is thus 
 *	the type also of the output, the return value of processSample() ).  And The second
 *	template parameter specifies the type of the coefficients.
 *
 *	Actually, I saw a code example of time-domain convolution in O(n^log2(3)) which beats
 * 	the current implementation O(n^2)
 *		Titled: "Time domain convolution with O(n^log2(3))"
 * 			http://www.musicdsp.org/archive.php?classid=3#66
 */
template<class sample_t,class coefficient_t> class TSimpleConvolver
{
public:
	TSimpleConvolver(const coefficient_t _coefficients[],size_t _coefficientCount) :
		coefficients(_coefficients),
		coefficientCount(_coefficientCount),
		coefficientCountSub1(_coefficientCount-1),
		delay(_coefficientCount)
	{
		if(coefficientCount<1)
			throw runtime_error(string(__func__)+" -- invalid coefficientCount: "+istring(coefficientCount));
	}

	virtual ~TSimpleConvolver()
	{
	}

	const sample_t processSample(const sample_t input)
	{
		coefficient_t output=input*coefficients[0];
		for(size_t t=coefficientCountSub1;t>0;t--)
			output+=delay.getSample(t)*coefficients[t];

		delay.putSample((coefficient_t)input);

		return (sample_t)output;
	}

private:
	const coefficient_t *coefficients; // aka, the impluse response
	const size_t coefficientCount;
	const size_t coefficientCountSub1;

	TDSPDelay<coefficient_t> delay;
};



#ifdef HAVE_LIBRFFTW

#include <rfftw.h>

#include <TAutoBuffer.h>

/*
 *	Written partly from stuff I learned in "The Scientist and 
 *	Engineer's Guide to Digital Signal Processing":
 *		http://www.dspguide.com
 *
 *	and partly from libfftw's documentation:
 *		http://www.fftw.org
 *
 * 	To use this DSP block it must be instantiated with a time-domain
 * 	set of convolution coefficients, called the 'filter kernel'.  
 *
 * 	Call beginWrite(); then, exactly getChunkSize() samples should be written 
 * 	with the writeSample() method.  
 * 	Then the beginRead() method should be called,  and the same number of
 * 	samples that were just written with writeSample() must be read with the 
 * 	readSample() method.  And this 3-part procedure can be repeated.  
 *
 * 	The one exception to writing exactly getChunkSize() samples is 
 * 	on the last chunk.  If there are not getChunkSize() samples
 * 	remaining in the input, the beginRead() method can be called
 * 	sooner.  And then readSample() can be called again for as many
 * 	samples as were written.
 *
 * 	Optionally, after this is all finished, since the complete
 * 	convolution actually creates the kernel filter's size - 1 
 * 	extra samples of output more than the size of the input, these
 * 	can be read by calling readEndingSample() for the filter kernel's
 * 	size - 1 times.
 *
 * 	The reset() method can be called if everything is to start over
 * 	as if just after construction
 *
 * 	And of course, the object can be safely destroyed without having 
 * 	read all the samples that were written.
 */


// ??? I should be able to remove the extra element I allocate and the setting of the last element to zero from when I was using macros to access the data (which I'm not doing anymore).. just make sure the current implementation wouldn't expact that extra element to be there
template <class sample_t,class coefficient_t> class TFFTConvolverTimeDomainKernel
{
	// ??? there are perhaps some optimizations like using memset instead of for-loops
public:
	TFFTConvolverTimeDomainKernel(const coefficient_t filterKernel[],size_t filterKernelSize) :
		M(filterKernelSize),
		W(getFFTWindowSize()),fW(W),
		N(W-M+1),

		data(W),dataPos(0),
		xform(W+1),

		p(rfftw_create_plan_specific(W, FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE, data,1,xform,1)),
		un_p(rfftw_create_plan_specific(W, FFTW_COMPLEX_TO_REAL, FFTW_ESTIMATE, xform,1,data,1)),

		kernel_real(W/2+1),
		kernel_img(W/2+1),

		overlap(M-1),
		overlapPos(0)
	{
		if(p==NULL || un_p==NULL)
			throw runtime_error(string(__func__)+" -- fftw had an error creating plans");

		// ??? might want to check coefficient_t against fftw_real
		
		prepareFilterKernel(filterKernel,data,xform,p,un_p);
		//printf("chosen window size: %d\n",W);

		reset();
	}

	virtual ~TFFTConvolverTimeDomainKernel()
	{
		rfftw_destroy_plan(p);
		rfftw_destroy_plan(un_p);
	}

	// the NEW set of coefficients MUST be the same size as the original one at construction
	void setNewFilterKernel(const coefficient_t filterKernel[])
	{
		TAutoBuffer<fftw_real> data(W),xform(W+1);
		rfftw_plan p=rfftw_create_plan_specific(W, FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE, data,1,xform,1);
		rfftw_plan un_p=rfftw_create_plan_specific(W, FFTW_COMPLEX_TO_REAL, FFTW_ESTIMATE, xform,1,data,1);

		prepareFilterKernel(filterKernel,data,xform,p,un_p);

		rfftw_destroy_plan(p);
		rfftw_destroy_plan(un_p);
	}


	const size_t getChunkSize() const
	{
		return N;
	}

	void beginWrite()
	{
		dataPos=0;
	}

	void writeSample(const sample_t s)
	{
		data[dataPos++]=s;
	}

	void beginRead()
	{
		if(dataPos>N) // inappropriate use
			throw runtime_error(string(__func__)+" -- writeSample() was called too many times: "+istring(dataPos)+">TFFTConvolver::getChunkSize() ("+istring(N)+")");
		// pad data with zero
		while(dataPos<W)
			data[dataPos++]=0;

		// do the fft data --> xform
		rfftw_one(p, data, xform);
		xform[W]=0; // to help out macros
						

			// ??? and here is where I could just as easily deconvolve by dividing with some flag
		// multiply the frequency-domain kernel with the now frequency-domain audio
		for(size_t t=0;t<=W/2;t++)
		{
			const fftw_real re=xform[t];
			const fftw_real im= (t==0 || t==W/2) ? 0 : xform[W-t];

			// complex multiplication
			if(t!=0 && t!=W/2)
				xform[W-t]=re*kernel_img[t] + im*kernel_real[t];	// im= re*k_im + im*k_re
			xform[t]=re*kernel_real[t] - im*kernel_img[t];			// re= re*k_re - im*k_im
		}

		// do the inverse fft xfrorm --> data
		rfftw_one(un_p, xform, data);

		// add the last segment's overlap to this segment
		for(size_t t=0;t<M-1;t++)
			data[t]+=overlap[t];

		// save the samples that will overlap the next segment
		for(size_t t=N;t<W;t++)
			overlap[t-N]=data[t];

		dataPos=0;
	}

	const fftw_real readSample()
	{
		return data[dataPos++]/fW;
	}

	const fftw_real readEndingSample()
	{
		return overlap[overlapPos++]/fW;
	}

	void reset()
	{
		dataPos=0;
		overlapPos=0;

		for(size_t t=0;t<M-1;t++)
			overlap[t]=0;
	}

	const size_t getFilterKernelSize() const
	{
		return M;
	}

	static const vector<size_t> getGoodFilterKernelSizes()
	{
		//                                                                                 2^13 5^6   2^14  3^9   2^15  3^10  2^16  5^7   7^6    2^17   3^11   2^18   5^8    2^19   3^12   7^7    2^20    3^13    5^9     2^21    2^22    3^14    7^8     2^23    5^10    3^15     2^24     11^7
		static const size_t fftw_good_sizes[]={0,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,15625,16384,19683,32768,59049,65536,78125,117649,131072,177147,262144,390625,524288,531441,823543,1048576,1594323,1953125,2097152,4194304,4782969,5764801,8388608,9765625,14348907,16777216,19487171};
		static const size_t num_sizes=sizeof(fftw_good_sizes)/sizeof(*fftw_good_sizes);

		vector<size_t> v;
		for(unsigned t=0;t<num_sizes;t++)
			v.push_back(fftw_good_sizes[t]);
		return v;
	}

private:

	const size_t M; // length of filter kernel
	const size_t W; // fft window size
		const fftw_real fW;
	const size_t N; // length of audio chunk to be processed each fft window (W-M+1)

	TAutoBuffer<fftw_real> data;
	size_t dataPos;

	TAutoBuffer<fftw_real> xform;

	rfftw_plan p;
	rfftw_plan un_p;

	TAutoBuffer<fftw_real> kernel_real;
	TAutoBuffer<fftw_real> kernel_img;

	TAutoBuffer<fftw_real> overlap;
	size_t overlapPos;


	const size_t getFFTWindowSize() const
	{
		if(M<=0)
			throw runtime_error(string(__func__)+" -- filter kernel length is <= 0 -- "+istring(M));

		const vector<size_t> fftw_good_sizes=getGoodFilterKernelSizes();

		// the window size to use should be the first item in fftw_good_sizes that can accomidate the filterKernel, then one more bigger
		for(size_t t=0;t<fftw_good_sizes.size()-1;t++)
		{
			if(fftw_good_sizes[t]>=M)
				return fftw_good_sizes[t+1];
		}

		throw runtime_error(string(__func__)+" -- cannot handle a filter kernel of size "+istring(M)+" -- perhaps simply another element needs to be added to fftw_good_sizes");
	}
	
	void prepareFilterKernel(const coefficient_t filterKernel[],fftw_real data[],fftw_real xform[],rfftw_plan p,rfftw_plan un_p)
	{
			// ??? perhaps a parameter could be passed that would indicate what the filterKernel is.. whether it's time-domain or freq domain already

		// convert the given time-domain kernel into a frequency-domain kernel
		for(size_t t=0;t<M;t++) // copy to data
			data[t]=filterKernel[t];
		for(size_t t=M;t<W;t++) // pad with zero
			data[t]=0;

		rfftw_one(p, data, xform);
		xform[W]=0; // to help out macros

		// copy into kernel_real and kernel_img
		for(size_t t=0;t<=W/2;t++)
		{
			kernel_real[t]=xform[t];
			kernel_img[t]= (t==0 || t==W/2) ? 0 : xform[W-t];
		}
	}
		
};


template <class sample_t,class coefficient_t> class TFFTConvolverFrequencyDomainKernel : public TFFTConvolverTimeDomainKernel<sample_t,coefficient_t>
{
public:
	// magnitude is the frequency domain gains to apply to the frequences, responseSize is the length of the magnitude array and the phase is optional, but if it's given, then it must be the same length as magnitude
			// ??? explain that better
	TFFTConvolverFrequencyDomainKernel(const coefficient_t magnitude[],size_t responseSize,const coefficient_t phase[]=NULL) :
		TFFTConvolverTimeDomainKernel<sample_t,coefficient_t>(convertToTimeDomain(magnitude,phase,responseSize),(responseSize-1)*2)
	{
		delete [] tempKernel;
	}

	virtual ~TFFTConvolverFrequencyDomainKernel()
	{
	}

	/* the responseSize must match the original responseSize at construction or results are undefined */
	void setNewMagnitudeArray(const coefficient_t magnitude[],size_t responseSize,const coefficient_t phase[]=NULL)
	{
		setNewFilterKernel(convertToTimeDomain(magnitude,phase,responseSize));
		delete [] tempKernel;
	}


private:
	
	coefficient_t *tempKernel;

	static double polar_to_rec_real(double mag,double phase)
	{
		return mag*cos(phase);
	}

	static double polar_to_rec_img(double mag,double phase)
	{
		return mag*sin(phase);
	}

	const coefficient_t *convertToTimeDomain(const coefficient_t magnitude[],const coefficient_t _phase[],size_t responseSize)
	{
		tempKernel=NULL;

		// allocate zeroed-out phase response if it wasn't given
		coefficient_t *phase=new coefficient_t[responseSize];
		if(_phase==NULL)
		{ // create all zero phase array
			for(size_t t=0;t<responseSize;t++)
				phase[t]=0;
		}
		else
		{ // copy from given phase arrray
			for(size_t t=0;t<responseSize;t++)
				phase[t]=_phase[t];

			// assert this property of the real DFT
			phase[0]=0;
			phase[responseSize-1]=0;
		}

		// convert given polar coordinate frequency domain kernel to rectangular coordinates
		TAutoBuffer<coefficient_t> real(responseSize);
		TAutoBuffer<coefficient_t> imaginary(responseSize);
		for(size_t t=0;t<responseSize;t++)
		{
			real[t]=polar_to_rec_real(magnitude[t],phase[t]);
			imaginary[t]=polar_to_rec_img(magnitude[t],phase[t]);
		}
		

		// convert rectangular coordinate frequency domain kernel to the time domain
		const size_t W=(responseSize-1)*2;
		TAutoBuffer<fftw_real> xform(W);
		for(size_t t=0;t<responseSize;t++)
		{ // setup the array in the form fftw expects: [  r0,r1,r2, ... rW/2, iW/2-1, ... i2, i1 ]
			xform[t]=real[t];
			if(t!=0 && t!=W/2)
				xform[W-t]=imaginary[t];
		}
		TAutoBuffer<fftw_real> data(W);
		rfftw_plan un_p=rfftw_create_plan(W, FFTW_COMPLEX_TO_REAL, FFTW_ESTIMATE);
		rfftw_one(un_p, xform, data);	// xform -> data
		rfftw_destroy_plan(un_p);


		// fftw scales the output of the complex->real transform by W (undo this)
		for(size_t t=0;t<W;t++)
			data[t]/=W;


		// now I have to shift, truncate and window the resulting time domain kernel

		// truncate to M samples
		const size_t M=responseSize;

		// rotate the kernel forward by M/2 (also unfortunately causes a delay in the filtered signal by M/2 samples)
		TAutoBuffer<coefficient_t> temp(W);
		for(size_t t=0;t<W;t++)
			temp[(t+M/2)%W]=data[t];

		// apply the hamming window to the kernel centered around M/2 and truncate anything past M (and put finally into tempKernel which will be returned)
		tempKernel=new coefficient_t[W];
		for(size_t t=0;t<W;t++)
		{
			if(t<=M)
				tempKernel[t]=temp[t]*(0.54-0.46*cos(2.0*M_PI*t/M));
			else
				tempKernel[t]=0;

		}

		// filterKernel is now ~ twice the length as the original given magnitude array, except it's padded with zeros beyond what was given

#if 0
		// test the filter kernel
		{
			TAutoBuffer<fftw_real> data(W);
			TAutoBuffer<fftw_real> xform(W);

			for(size_t t=0;t<W;t++)
				data[t]=tempKernel[t];

			rfftw_plan p=rfftw_create_plan(W, FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE);
			rfftw_one(p, data, xform);	// data -> xform
			rfftw_destroy_plan(p);

			
			// avoid the first and last element cause the imaginary parts aren't in xform
			FILE *f=fopen("/home/ddurham/code/rezound/o","w");
			for(size_t t=1;t<W/2-1;t++)
				fprintf(f,"%d\t%f\n",t,sqrt(pow(xform[t],2.0)+pow(xform[W-t],2.0)));
			fclose(f);

		}
#endif

		delete [] phase;

		return tempKernel;
	}
};



/* some helper functions for if/when I need to convert fftw's rectangular transformed output to polar coordinates and back
#include <math.h>
static float rec_to_polar_mag(float real,float img) 
{
	return sqrt(real*real+img*img);
}

static float rec_to_polar_phase(float real,float img)
{
	if(real==0) real=1e-20;
	float phase=atan(img/real);
	if(real<0 && img<0) phase=phase-M_PI;
	else if(real<0 && img>=0) phase=phase+M_PI;
	return phase;
}

static float polar_to_rec_real(float mag,float phase)
{
	return mag*cos(phase);
}

static float polar_to_rec_img(float mag,float phase)
{
	return mag*sin(phase);
}
*/



#endif

#endif
