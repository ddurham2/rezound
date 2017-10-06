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

/*
??? I could create an LFO that has an extra parameter which could be anything
from a falling saw to a triangle to a rising saw but adjusting how the angles 
are aimed.    think of a V in a square where the bottom axis is 0 to 360 degrees
and the tops of the V hit the top two corners.  Then let the extra parameter 
say where the bottom point of the V hits on the bottom axis.  So, 180 degrees would
be a triangle wave at a phase of 90 deg. and zero or 359 degrees would be sawtooth
waves.   

The only reason I haven't done this already is that the frontend would need a 4th
slider which would have to be appropriately names, and second, because I would 
probably need an explanation of it by showing the user an image.

If I did have a 4th parameter, I could also use that to define the number of 
steps for a step function LFO.  But it would have a different name.. so CLFORegistry
would also have to return what the call the optional extra slider
*/

/*
??? OPTIMIZATION: I could conceivably make the constructor of the derived LFO
class calculate a lookup table and the base class could implement nextSample() 
to just use the calculated lookup table.  I would have to base the number of
entries in the table on a sample rate and the frequency.  I should calculated it
in the derived class's constructor, but I should also use static members (mutex
protected) to remember a cache of the last fwe calculated table and only 
recalculated it if the none of the parameters for the cached tables match the
current parameters.  This way when an action calculates it for 2 channels but 
reinstantiates the LFO for both channels it won't have to recalculate the table.
*/

#include "ALFO.h"

#include <math.h>

#include <stdexcept>
#include <istring>

#include "unit_conv.h"

// --------------------------------
ALFO::ALFO()
{
}

ALFO::~ALFO()
{
}






// -----------------------------------
// --- some LFO implementations ------
// -----------------------------------
// --- These should ordinarily be constructed by CLFORegistry::createLFO, not directly constructed
// 	- I would have to move them into the header file if I wanted to use them directly

/*
 * - LFO with simply a constant value of 1.0
 */
class CConstantLFO : public ALFO
{
public:
	CConstantLFO()
	{
	}

	virtual ~CConstantLFO()
	{
	}

	const float nextValue()
	{
		return 1.0;
	}

	const float getValue(const sample_pos_t time) const
	{
		return 1.0;
	}
};



/*
 * - Sine shaped LFO with a range of [-1,1] and a frequenct and initial value specified by the constructor's arguments
 * - Note: I use sinf which is (from glibc) a faster float version of sine rather than double percision
 * 	- This function should be available on other platforms because it is mentioned in the ANSI C99 specification
 */
class CSinLFO : public ALFO
{
public:
	// frequency is in hertz
	// initialAngle is in degrees
	// sampleRate is given to know how often nextValue will be called per cycle
	CSinLFO(float _frequency,float initialAngle,unsigned sampleRate) :
			// convert from herz to scalar to mul with counter
		frequency((1.0/((float)sampleRate/_frequency))*(2.0*M_PI)),
		initial(degrees_to_radians(initialAngle)/frequency),

			// initialize the counter to return the initial angle
		counter(initial)
	{
	}

	virtual ~CSinLFO()
	{
	}

	const float nextValue()
	{
		return sinf((counter++)*frequency);
	}

	const float getValue(const sample_pos_t time) const
	{
		return sinf((time+initial)*frequency);
	}

protected:
	float frequency;
	float initial;
	// ??? I probably do need to worry about counter wrap around
		// perhaps I could know a threshold when to fmod the counter
	float counter;
};



/* 
 * - Same as CSinLFO except it has a range of [0,1] (but it still has the same shape as sine)
 */
class CPosSinLFO : public CSinLFO
{
public:
	CPosSinLFO(float frequency,float initialAngle,unsigned sampleRate) :
		CSinLFO(frequency,initialAngle,sampleRate)
	{
	}

	virtual ~CPosSinLFO()
	{
	}

	const float nextValue()
	{
		return (sinf((counter++)*frequency)+1.0)/2.0;
	}

	const float getValue(const sample_pos_t time) const
	{
		return (sinf((time+initial)*frequency)+1.0)/2.0;
	}
};

/* 
 * - Same as CSinLFO except it has a range of [0,1] by taking abs(value)
 */
class CABSSinLFO : public CSinLFO
{
public:
	CABSSinLFO(float frequency,float initialAngle,unsigned sampleRate) :
		CSinLFO(frequency,initialAngle,sampleRate)
	{
	}

	virtual ~CABSSinLFO()
	{
	}

	const float nextValue()
	{
		return fabs(CSinLFO::nextValue());
	}

	const float getValue(const sample_pos_t time) const
	{
		return fabs(CSinLFO::getValue(time));
	}
};





class CRisingSawtoothLFO : public ALFO
{
public:
	// frequency is in hertz
	// initialAngle is in degrees
	// sampleRate is given to know how often nextValue will be called per cycle
	CRisingSawtoothLFO(float frequency,float initialAngle,unsigned sampleRate) :
		mod((unsigned)ceil(sampleRate/frequency)),
		div(sampleRate/frequency/2.0),
		counter((unsigned)((initialAngle+180.0)/360.0*sampleRate/frequency)%mod)
		
	{
	}

	virtual ~CRisingSawtoothLFO()
	{
	}

	const float nextValue()
	{
		const float v=(counter++)/div-1.0;
		counter%=mod;
		return v;
	}

	const float getValue(const sample_pos_t time) const
	{
		return (time%mod)/div-1.0;
	}

protected:
	const unsigned mod;
	float div;

	unsigned counter;
};

class CPosRisingSawtoothLFO : public CRisingSawtoothLFO
{
public:
	CPosRisingSawtoothLFO(float frequency,float initialAngle,unsigned sampleRate) :
		CRisingSawtoothLFO(frequency,initialAngle,sampleRate)
		
	{
		div*=2.0;
	}

	virtual ~CPosRisingSawtoothLFO()
	{
	}

	const float nextValue()
	{
		const float v=(counter++)/div;
		counter%=mod;
		return v;
	}

	const float getValue(const sample_pos_t time) const
	{
		return (time%mod)/div;
	}
};

class CFallingSawtoothLFO : public CRisingSawtoothLFO
{
public:
	// frequency is in hertz
	// initialAngle is in degrees
	// sampleRate is given to know how often nextValue will be called per cycle
	CFallingSawtoothLFO(float frequency,float initialAngle,unsigned sampleRate) :
		CRisingSawtoothLFO(frequency,initialAngle,sampleRate)
		
	{
	}

	virtual ~CFallingSawtoothLFO()
	{
	}

	const float nextValue()
	{
		const float v=(counter++)/div-1.0;
		counter%=mod;
		return -v;
	}

	const float getValue(const sample_pos_t time) const
	{
		return -(time%mod)/div-1.0;
	}

};


class CPosFallingSawtoothLFO : public CRisingSawtoothLFO
{
public:
	CPosFallingSawtoothLFO(float frequency,float initialAngle,unsigned sampleRate) :
		CRisingSawtoothLFO(frequency,initialAngle,sampleRate)
		
	{
		div*=2.0;
	}

	virtual ~CPosFallingSawtoothLFO()
	{
	}

	const float nextValue()
	{
		const float v=(counter++)/div;
		counter%=mod;
		return 1.0-v;
	}

	const float getValue(const sample_pos_t time) const
	{
		return 1.0-(time%mod)/div;
	}
};


/* square is just derived from SinLFO and if the Sin is >=0 then we return 1 else -1 */
class CSquareLFO : public CSinLFO
{
public:
	CSquareLFO(float frequency,float initialAngle,unsigned sampleRate) :
		CSinLFO(frequency,initialAngle,sampleRate)
	{
	}

	virtual ~CSquareLFO()
	{
	}

	const float nextValue()
	{
		return CSinLFO::nextValue()>=0.0 ? 1.0 : -1.0;
	}

	const float getValue(const sample_pos_t time) const
	{
		return CSinLFO::getValue(time)>=0.0 ? 1.0 : -1.0;
	}
};

/* square is just derived from SinLFO and if the Sin is >=0 then we return 1 else 0 */
class CPosSquareLFO : public CSinLFO
{
public:
	CPosSquareLFO(float frequency,float initialAngle,unsigned sampleRate) :
		CSinLFO(frequency,initialAngle,sampleRate)
	{
	}

	virtual ~CPosSquareLFO()
	{
	}

	const float nextValue()
	{
		return CSinLFO::nextValue()>=0.0 ? 1.0 : 0.0;
	}

	const float getValue(const sample_pos_t time) const
	{
		return CSinLFO::getValue(time)>=0.0 ? 1.0 : 0.0;
	}
};









#if 0 // not implemented yet.. 

The sine is deformable in that you can set where the positive peak 
of the sine should be, and the negative peak will be semetric with it.

I had to do a piecewise function where you use a multiplier on the x 
coordinate to do the two pieces from 0..target and (360-target)..360 (called 'R1')
And there is another multiplier as well as a phase to use between
target..(360-target) (called 'R2')

A natural sine will be produced at target=90

I suppose target has to be between 0..180

The formulas: (target IS in degrees)
	R1Mul=90.0/target
	
	R2Mul=1.0/(2.0-(1.0/R1Mul))
	R2Phase=0.75*M_PI - M_PI/(2.0*R1Mul)

//I derived these on paper which looks really nasty.. so I'll just use them
	

// ---------------------------------------------
class CDeformableSinLFO : public ALFO
{
public:
	// frequency is in hertz
	// initialAngle is in degrees
	// sampleRate is given to know how often nextValue will be called per cycle
	CDeformableSinLFO(float _frequency,float initialAngle,float peakAngle,unsigned sampleRate) :
			// convert from herz to scalar to mul with counter
		frequency((1.0/((float)sampleRate/_frequency))*(2.0*M_PI)),
		initial(degrees_to_radians(initialAngle)/frequency),

			// initialize the counter to return the initial angle
		counter(initial)
	{
	}

	virtual ~CDeformableSinLFO()
	{
	}

	const float nextValue()
	{
		return sinf((counter++)*frequency);
	}

	const float getValue(const sample_pos_t time) const
	{
		return sinf((time+initial)*frequency);
	}

protected:
	float frequency;
	float initial;
	// ??? I probably do need to worry about counter wrap around
		// perhaps I could know a threshold when to fmod the counter
	float counter;

	
};

#endif

// -----------------------------------------------












// --- CLFORegistry definition --------------------------------------------

/* ??? I should make this a real registry where each ALFO has to return a name, an the registry searches thru a vector instead fo hardcoding... then I could possibly load LFOs dynamically */
const CLFORegistry gLFORegistry;

CLFORegistry::CLFORegistry()
{
}

CLFORegistry::~CLFORegistry()
{
}

const size_t CLFORegistry::getCount() const
{
	return 10;
}

const string CLFORegistry::getName(const size_t index) const
{
	switch(index)
	{
	case 0:
		return N_("Sine Wave [-1,1]");
	case 1:
		return N_("Sine Wave [ 0,1]");
	case 2:
		return N_("ABS Sine Wave [ 0,1]");
	case 3:
		return N_("Constant");
	case 4:
		return N_("Rising Sawtooth Wave [-1,1]");
	case 5:
		return N_("Rising Sawtooth Wave [ 0,1]");
	case 6:
		return N_("Falling Sawtooth Wave [-1,1]");
	case 7:
		return N_("Falling Sawtooth Wave [ 0,1]");
	case 8:
		return N_("Square Wave [-1,1]");
	case 9:
		return N_("Square Wave [ 0,1]");
	}
	throw runtime_error(string(__func__)+" -- unhandled index: "+istring(index));
}

const bool CLFORegistry::isBipolar(const size_t index) const
{
	switch(index)
	{
	case 0:	// sin
		return true;
	case 1:	// pos sin
		return false;
	case 2:	// abs(sin)
		return false;
	case 3: // constant
		return false;
	case 4:	// rising saw
		return true;
	case 5:	// pos rising saw
		return false;
	case 6:	// falling saw
		return true;
	case 7: // pos falling saw
		return false;
	case 8: // square
		return true;
	case 9: // pos square
		return false;
	}
	throw runtime_error(string(__func__)+" -- unhandled index: "+istring(index));
}

const size_t CLFORegistry::getIndexByName(const string name) const
{
	if(name=="Sine Wave [-1,1]")
		return 0;
	else if(name=="Sine Wave [ 0,1]")
		return 1;
	else if(name=="ABS Sine Wave [ 0,1]")
		return 2;
	else if(name=="Constant")
		return 3; // ??? the frontend relies on this being 3
	else if(name=="Rising Sawtooth Wave [-1,1]")
		return 4;
	else if(name=="Rising Sawtooth Wave [ 0,1]")
		return 5;
	else if(name=="Falling Sawtooth Wave [-1,1]")
		return 6;
	else if(name=="Falling Sawtooth Wave [ 0,1]")
		return 7;
	else if(name=="Square Wave [-1,1]")
		return 8;
	else if(name=="Square Wave [ 0,1]")
		return 9;
	else
		throw runtime_error(string(__func__)+" -- unhandled name: '"+name+"'");
}

ALFO *CLFORegistry::createLFO(const CLFODescription &desc,const unsigned sampleRate) const
{
	switch(desc.LFOType)
	{
	case 0:
		return new CSinLFO(desc.freq,desc.phase,sampleRate);
	case 1:
		return new CPosSinLFO(desc.freq,desc.phase,sampleRate);
	case 2:
		return new CABSSinLFO(desc.freq,desc.phase,sampleRate);
	case 3:
		return new CConstantLFO; // ??? the frontend relies on this being 3
	case 4:
		return new CRisingSawtoothLFO(desc.freq,desc.phase,sampleRate);
	case 5:
		return new CPosRisingSawtoothLFO(desc.freq,desc.phase,sampleRate);
	case 6:
		return new CFallingSawtoothLFO(desc.freq,desc.phase,sampleRate);
	case 7:
		return new CPosFallingSawtoothLFO(desc.freq,desc.phase,sampleRate);
	case 8:
		return new CSquareLFO(desc.freq,desc.phase,sampleRate);
	case 9:
		return new CPosSquareLFO(desc.freq,desc.phase,sampleRate);
	}
	throw runtime_error(string(__func__)+" -- unhandled LFOType (index): "+istring(desc.LFOType));
}

void CLFODescription::writeToFile(CNestedDataFile *f,const string key) const
{
	f->setValue<float>(key DOT "amp",amp);
	f->setValue<float>(key DOT "freq",freq);
	f->setValue<float>(key DOT "phase",phase);
	f->setValue<size_t>(key DOT "type",LFOType);
}

void CLFODescription::readFromFile(const CNestedDataFile *f,const string key)
{
	amp=f->getValue<float>(key DOT "amp");
	freq=f->getValue<float>(key DOT "freq");
	phase=f->getValue<float>(key DOT "phase");
	LFOType=f->getValue<size_t>(key DOT "type");
}

