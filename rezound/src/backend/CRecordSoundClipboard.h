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

#ifndef __CRecordSoundClipboard_H__
#define __CRecordSoundClipboard_H__

#include "../../config/common.h"

class CRecordSoundClipboard;

#include "ASoundClipboard.h"
#include "CSound.h"

class CRecordSoundClipboard : public ASoundClipboard
{
public:
	CRecordSoundClipboard(const string description,const string workingFilename);
	virtual ~CRecordSoundClipboard();

	void copyFrom(const CSound *sound,const bool whichChannels[MAX_CHANNELS],sample_pos_t start,sample_pos_t length);
	bool isReadOnly() const;

	bool prepareForCopyTo();
	void copyTo(CSound *sound,unsigned destChannel,unsigned srcChannel,sample_pos_t start,sample_pos_t length,MixMethods mixMethod,SourceFitTypes fitSrc,bool invalidatePeakData);
	sample_pos_t getLength(unsigned sampleRate) const;
	bool isEmpty() const;

	unsigned getSampleRate() const;

private:
	const string workingFilename;
	unsigned sampleRate;

	CSound *workingFile;
};

#endif

