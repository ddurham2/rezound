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

#include "CrawSoundTranslator.h"
#include "CSound.h"

#include <stdexcept>

CrawSoundTranslator::CrawSoundTranslator()
{
}

CrawSoundTranslator::~CrawSoundTranslator()
{
}

void CrawSoundTranslator::onLoadSound(const string filename,CSound *sound) const
{
	throw(runtime_error(string(__func__)+" -- unimplemented"));
}

void CrawSoundTranslator::onSaveSound(const string filename,CSound *sound) const
{
	throw(runtime_error(string(__func__)+" -- unimplemented"));
}


bool CrawSoundTranslator::handlesExtension(const string extension) const
{
	return(extension=="raw");
}

bool CrawSoundTranslator::supportsFormat(const string filename) const
{
	return(true); // this necessitates CrawSoundTranslator being the last translator consulted
}

