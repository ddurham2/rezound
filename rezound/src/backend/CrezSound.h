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

#ifndef __CrezSound_H__
#define __CrezSound_H__


#include "../../config/common.h"

#ifdef __EXPORT_CLASSES__
class __declspec(dllexport) CrezSound;
#endif

#include "ASound.h"

class CrezSound : public ASound
{
public:

    CrezSound(const string &filename,const unsigned sampleRate,const unsigned channels,const sample_pos_t size);
    CrezSound();

    void loadSound(const string filename);
    void saveSound(const string filename);

};

#endif

