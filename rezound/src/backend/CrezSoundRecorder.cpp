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

#ifndef __CrezSoundRecorder_CPP__
#define __CrezSoundRecorder_CPP__

#include "CrezSoundRecorder.h"

template <class SoundRecorder> TrezSoundRecorder<SoundRecorder>::TrezSoundRecorder(const string _filename) :
	SoundRecorder(),
	filename(_filename)
{
}

template <class SoundRecorder> TrezSoundRecorder<SoundRecorder>::~TrezSoundRecorder()
{
}

template <class SoundRecorder> bool TrezSoundRecorder<SoundRecorder>::onStart()
{
    sound=new CrezSound(sampleRate,channels,bits,0);
    sound->setFilename(filename);
    for(unsigned t=0;t<channels;t++)
        accessers[t]=new CRezPoolAccesser(sound->getData(t));
    position=0;
    return(true);
}

template <class SoundRecorder> bool TrezSoundRecorder<SoundRecorder>::onData(void *_dataRecorded,size_t bytesRecorded)
{
    // channels better be >0
    if(bits==16)
    {
        size_t samplesRecorded=bytesRecorded/2;
        sound->addSpace(sound->getLength(),samplesRecorded);

        for(unsigned t=0;t<channels;t++)
        {
            sample_t *dataRecorded=(sample_t *)_dataRecorded;
            dataRecorded+=t; // offset for the right sample
            sample_pos_t p=position;

            for(size_t i=0;i<samplesRecorded;i+=channels)
            {
                (*accessers[t])[p]=dataRecorded[i];
                p++;
            }
        }
        position+=samplesRecorded;
        return(true);
    }
    else
    {
        // error
        return(false);
    }
}

template <class SoundRecorder> bool TrezSoundRecorder<SoundRecorder>::onStop(bool error)
{
    for(unsigned t=0;t<channels;t++)
        delete accessers[t];
    delete sound;
    return(true);
}

template <class SoundRecorder> bool TrezSoundRecorder<SoundRecorder>::onRedo()
{
    position=0;
    sound->removeSpace(0,sound->getLength());
    return(true);
}

#endif
