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

#ifndef __CEnvelope_H__
#define __CEnvelope_H__

#include "../../config/common.h"


class CEnvelope;

class CEnvelope
{
public:

    CEnvelope();
    CEnvelope(int _attackFactor,int _releaseFactor);
    CEnvelope(const CEnvelope &src);

    void init();

    CEnvelope &operator=(const CEnvelope &src);
    bool operator==(const CEnvelope &rhs);

    // we're in trouble if attackFactor or releaseFactor are in use and are zero!!!
    int attackFactor,releaseFactor;
    int envelopeCount;
    bool attacking,releasing;

};

#endif

