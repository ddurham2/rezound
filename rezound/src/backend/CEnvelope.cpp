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

#include "CEnvelope.h"

CEnvelope::CEnvelope()
{
    init();
}

CEnvelope::CEnvelope(int _attackFactor,int _releaseFactor)
{
    init();
    attackFactor=_attackFactor;
    releaseFactor=_releaseFactor;
}

CEnvelope::CEnvelope(const CEnvelope &src)
{
    init();
    operator=(src);
}

void CEnvelope::init()
{
    releaseFactor=attackFactor=0;
    attacking=false;
    releasing=false;
    envelopeCount=0;
}

CEnvelope &CEnvelope::operator=(const CEnvelope &src)
{
    init();
    attackFactor=src.attackFactor;
    releaseFactor=src.releaseFactor;
    return(*this);
}

bool CEnvelope::operator==(const CEnvelope &rhs)
{
    if(attackFactor!=rhs.attackFactor)
        return(false);
    if(releaseFactor!=rhs.releaseFactor)
        return(false);
    return(true);
}

