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

#ifndef __ASoundRecorder_H__
#define __ASoundRecorder_H__


#include "../../config/common.h"

#include "ASound.h" // for mix_sample_t

/*
 - This class should be derived from to do sound records for a specific
 platform.  Then it should remain abstract (the protected methods) to be
 derived again to actual do something with the data that is recorded (i.e.
 one derivation to write to a file, and another perhaps to encode into .mp3
 then write to a file... etc)

 - In the future we will need a way to get a realtime audio status

*/
class ASoundRecorder
{
public:

    ASoundRecorder();
    virtual ~ASoundRecorder();

    virtual void init(const unsigned channels,const unsigned sampleRate)=0;
    virtual void deinit()=0;

    virtual void start()=0;
    virtual void redo()=0;

    virtual bool isInitialized() const=0;

protected:

    unsigned channels;
    unsigned sampleRate;

    // These functions should return true on success and false on failure

    // This function should be called before any data will be recorded
    virtual bool onStart()=0;
    // This function should be called when a data chunk is recorded
    virtual bool onData(void *dataRecorded,size_t bytesRecorded)=0;
    // This function should be called after all data is recorded
    virtual bool onStop(bool error)=0;
    // This function should be called upon redo request
    virtual bool onRedo()=0;

};

#endif
