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

#include "CNativeSoundClipboard.h"

#include <stdexcept>

#include <istring>

typedef TPoolAccesser<sample_t,CSound::PoolFile_t > CClipboardPoolAccesser;

CNativeSoundClipboard::CNativeSoundClipboard(const string description,const string _workingFilename) :
	ASoundClipboard(description),
	workingFilename(_workingFilename),
	workingFile(REZOUND_POOLFILE_BLOCKSIZE,"ReZoundC"),

	channelCount(0),
	sampleRate(0),
	length(0)
{
	workingFile.openFile(workingFilename,true); 
	workingFile.clear();
}

CNativeSoundClipboard::~CNativeSoundClipboard()
{
	workingFile.closeFile(false,true);
}

void CNativeSoundClipboard::copyFrom(const CSound *sound,const bool whichChannels[MAX_CHANNELS],sample_pos_t start,sample_pos_t length)
{
	clearWhichChannels();
	workingFile.clear();
	for(unsigned i=0;i<sound->getChannelCount();i++)
	{
		if(whichChannels[i])
		{
			const string poolName="Channel "+istring(i);
			CClipboardPoolAccesser dest=workingFile.createPool<sample_t>(poolName);

			// ??? progress bar.. either call this in 100 chunks or have a call back function
			dest.copyData(0,sound->getAudio(i),start,length,true);

			this->whichChannels[i]=true;
			this->length=length;
			this->sampleRate=sound->getSampleRate();
			this->channelCount=sound->getChannelCount();
		}
	}
}

bool CNativeSoundClipboard::isReadOnly() const
{
	return(false);
}



void CNativeSoundClipboard::copyTo(CSound *sound,unsigned destChannel,unsigned srcChannel,sample_pos_t start,sample_pos_t length,MixMethods mixMethod,bool invalidatePeakData)
{
	if(destChannel>sound->getChannelCount())
		throw(runtime_error(string(__func__)+" -- destChannel (+"+istring(destChannel)+") out of range ("+istring(sound->getChannelCount())+")"));
	if(srcChannel>channelCount)
		throw(runtime_error(string(__func__)+" -- srcChannel (+"+istring(srcChannel)+") out of range ("+istring(channelCount)+")"));
	if(!whichChannels[srcChannel])
		throw(runtime_error(string(__func__)+" -- data does not exist in clipboard for srcChannel (+"+istring(srcChannel)+")"));
	if(length>getLength(sound->getSampleRate()))
		throw(runtime_error(string(__func__)+" -- length ("+istring(length)+")is greater than the amount of data in the clipboard ("+istring(getLength(sound->getSampleRate())+")")));


	const string poolName="Channel "+istring(srcChannel);
	const CRezPoolAccesser src=workingFile.getPoolAccesser<sample_t>(poolName);

	// ??? would need to handle sampleRate conversion... SHOULD: put this functionality in mixSound so everyone could benefit from it
	sound->mixSound(destChannel,start,src,0,sampleRate,length,mixMethod,invalidatePeakData);
}

sample_pos_t CNativeSoundClipboard::getLength(unsigned _sampleRate) const
{
			// ??? probably want to divide first
	return((sample_pos_t)((sample_fpos_t)length*(sample_fpos_t)_sampleRate/(sample_fpos_t)sampleRate));
}

bool CNativeSoundClipboard::isEmpty() const
{
	return(length<=0);
}

