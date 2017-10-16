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

#ifndef __DSP_PitchChanger_H__
#define __DSP_PitchChanger_H__

#include "../../config/common.h"

#ifdef HAVE_LIBSOUNDTOUCH

/* NOTE:
 *   - The template is called TPitchChanger which used the libSoundTouch library.  And 
 *     this is not to be confused with the preexisting TSoundStretcher template which 
 *     changes the rate (speed and pitch).
 */

#include <vector>

#include "../CSound_defs.h"

#include <soundtouch/SoundTouch.h>
using namespace soundtouch;

template<class src_type> class TPitchChanger
{
public:
	/*
	 * src is the data to read
	 * srcOffset is the first sample that this class will look at
  	 * length is the length of samples beyond the srcOffset that should be read and is the number of output samples
	 * deltaSemitones is the how much + or - to change the pitch
	 * frameSize can be passed if the src is actually interlaced data of more than 1 channel of audio
	 * frameOffset should be passed when frameSize is given other than one to say which channel in the interlaced data should be processed
	 */
	TPitchChanger(const src_type &_src,const sample_pos_t _srcOffset,const sample_pos_t _length,const float _deltaSemitones,unsigned srcSampleRate,unsigned _frameSize=1,unsigned _frameOffset=0) :
		src(_src),
		srcOffset(_srcOffset),
		length(_length),
		deltaSemitones(_deltaSemitones),
		frameSize(_frameSize),
		frameOffset(_frameOffset),

		pos(0),
		srcEnd(srcOffset+length),
		flushed(false),

		outputBuffer(1024),
		outputBufferOffset(0),
		outputBufferSize(0),

		inputBuffer(outputBuffer.size())
	{
		if(frameSize==0)
			throw(runtime_error(string(__func__)+" -- frameSize is 0"));
		if(frameOffset>=frameSize)
			throw(runtime_error(string(__func__)+" -- frameOffset is >= frameSize: "+istring(frameOffset)+">="+istring(frameSize)));

		changer.setChannels(1); // ??? need to have a way of allowing more than one channel so that phase across channels is preserved
		changer.setSampleRate(srcSampleRate);
		changer.setPitchSemiTones(deltaSemitones);
	}

	virtual ~TPitchChanger()
	{
	}

	const sample_t getSample()
	{
		if(outputBufferOffset<outputBufferSize)
			// data available to read
			return convert_sample<soundtouch::SAMPLETYPE,sample_t>(outputBuffer[outputBufferOffset++]);
		else if(changer.numSamples() > 0)
		{	// read more data from changer
			outputBufferSize=changer.receiveSamples(outputBuffer.data(),outputBuffer.size());
			outputBufferOffset=0;
		}
		else /*if(changer.numSamples() <= 0)*/
		{	// write more data to changer
			if(pos<srcEnd)
			{
				const size_t chunkSize=min((sample_pos_t)inputBuffer.size(),srcEnd-pos);
				size_t t;
				// unfortunately I'm having to make a copy into inputBuffer before giving it to changer
				for(t=0;t<chunkSize;t++)
					inputBuffer[t]=convert_sample<sample_t,soundtouch::SAMPLETYPE>(src[((pos++)*frameSize)+frameOffset]);
				changer.putSamples(inputBuffer.data(),t);
			}
			else
			{	// no input to give to changer
				if(!flushed)
				{
					changer.flush();
					flushed=true;
				}
				else
					return 0;
			}
		}

		// recur
		return getSample();
	}

	// algorithm tuning (libSoundTouch specific.. see SoundTouch.h)
	bool setSetting(uint settingId,uint value)
	{
		return changer.setSetting(settingId,value);
	}

private:
	const src_type src;
	const sample_pos_t srcOffset;
	const sample_pos_t length;
	const float deltaSemitones;
	const sample_pos_t frameSize;
	const sample_pos_t frameOffset;

	sample_pos_t pos;
	sample_pos_t srcEnd;
	bool flushed;

	std::vector<soundtouch::SAMPLETYPE> outputBuffer;
	size_t outputBufferOffset;
	size_t outputBufferSize;

	std::vector<soundtouch::SAMPLETYPE> inputBuffer;

	SoundTouch changer;
};

#endif // HAVE_LIBSOUNDTOUCH

#endif 
