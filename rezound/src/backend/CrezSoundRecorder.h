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

#ifndef __CrezSoundRecorder_H__
#define __CrezSoundRecorder_H__


#include "../../config/common.h"

#include "ASoundRecorder.h"
#include "CrezSound.h"

template <class SoundRecorder> class  TrezSoundRecorder : public SoundRecorder
{
public:

    TrezSoundRecorder(const string filename);
    virtual ~TrezSoundRecorder();

protected:

    bool onStart();
    bool onData(void *dataRecorded,size_t bytesRecorded);
    bool onStop(bool error);
    bool onRedo();

private:

    CrezSound *sound;
    string filename;
    int position;
    CRezPoolAccesser *accessers[MAX_CHANNELS];


};

#ifdef __WIN32_OS__
    #include <AWinSoundRecorder.h>
    #ifdef __EXPORT_CLASSES__
        __declspec(dllexport) typedef TrezSoundRecorder<AWinSoundRecorder> CrezSoundRecorder;
    #else
        typedef TrezSoundRecorder<AWinSoundRecorder> CrezSoundRecorder;
    #endif
#endif
// else define another



#include "CrezSoundRecorder.cpp"

#endif
