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
	length(0),

	origLength(0)
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
	return false;
}



void CNativeSoundClipboard::copyTo(CSound *sound,unsigned destChannel,unsigned srcChannel,sample_pos_t start,sample_pos_t length,MixMethods mixMethod,SourceFitTypes fitSrc,bool invalidatePeakData)
{
	if(destChannel>sound->getChannelCount())
		throw runtime_error(string(__func__)+" -- destChannel (+"+istring(destChannel)+") out of range ("+istring(sound->getChannelCount())+")");
	if(srcChannel>channelCount)
		throw runtime_error(string(__func__)+" -- srcChannel (+"+istring(srcChannel)+") out of range ("+istring(channelCount)+")");
	if(!whichChannels[srcChannel])
		throw runtime_error(string(__func__)+" -- data does not exist in clipboard for srcChannel (+"+istring(srcChannel)+")");
	if(length>getLength(sound->getSampleRate()) && fitSrc==sftNone)
		throw runtime_error(string(__func__)+" -- length ("+istring(length)+")is greater than the amount of data in the clipboard ("+istring(getLength(sound->getSampleRate())+")"));


	const string poolName="Channel "+istring(srcChannel);
	const CRezPoolAccesser src=workingFile.getPoolAccesser<sample_t>(poolName);

	sound->mixSound(destChannel,start,src,0,sampleRate,length,mixMethod,fitSrc,invalidatePeakData,true);
}

sample_pos_t CNativeSoundClipboard::getLength(unsigned _sampleRate) const
{
			// ??? probably want to divide first
	return (sample_pos_t)((sample_fpos_t)length*(sample_fpos_t)_sampleRate/(sample_fpos_t)sampleRate);
}

bool CNativeSoundClipboard::isEmpty() const
{
	return length<=0;
}

void CNativeSoundClipboard::temporarilyShortenLength(unsigned _sampleRate,sample_pos_t changeTo)
{
	if(changeTo>getLength(_sampleRate))
		throw runtime_error(string(__func__)+" -- changeTo is greater than the current length");
	if(changeTo==getLength(_sampleRate))
		return;

	sample_pos_t newLength=(sample_pos_t)sample_fpos_floor((sample_fpos_t)changeTo/(sample_fpos_t)sampleRate*(sample_fpos_t)_sampleRate);
	origLength=length;

	for(unsigned i=0;i<channelCount;i++)
	{
		if(whichChannels[i])
		{
			const string tempPoolName="Temp Moved Channel "+istring(i);
			CClipboardPoolAccesser dest=workingFile.createPool<sample_t>(tempPoolName);
			CClipboardPoolAccesser src=workingFile.getPoolAccesser<sample_t>("Channel "+istring(i));

			dest.moveData(0,src,newLength,origLength-newLength);
		}
	}
	length=newLength;
	
}

void CNativeSoundClipboard::undoTemporaryShortenLength()
{
	if(origLength!=0)
	{
		for(unsigned i=0;i<channelCount;i++)
		{
			if(whichChannels[i])
			{
				const string tempPoolName="Temp Moved Channel "+istring(i);
				CClipboardPoolAccesser dest=workingFile.getPoolAccesser<sample_t>("Channel "+istring(i));
				CClipboardPoolAccesser src=workingFile.getPoolAccesser<sample_t>(tempPoolName);

				dest.moveData(length,src,0,origLength-length);

				workingFile.removePool(tempPoolName);
			}
		}
		length=origLength;

		origLength=0;
	}
}

unsigned CNativeSoundClipboard::getSampleRate() const
{
	return sampleRate;
}

