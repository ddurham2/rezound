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

#ifndef __ASoundTranslator_H__
#define __ASoundTranslator_H__

#include "../../config/common.h"

#include <string>
#include <vector>

class CSound;

class ASoundTranslator
{
public:
	ASoundTranslator();
	virtual ~ASoundTranslator();

		// normally these return true, or they return false if cancelled
	bool loadSound(const string filename,CSound *sound) const;
	bool saveSound(const string filename,CSound *sound) const;

	virtual bool handlesExtension(const string extension) const=0;
	virtual bool supportsFormat(const string filename) const=0;
	virtual bool handlesRaw() const { return(false); }	// only the raw translator implementation should override this to return true

	virtual const vector<string> getFormatNames() const=0;			// return a list of format names than this derivation handles
	virtual const vector<vector<string> > getFormatExtensions() const=0;	// return a group of filename extensions for each format name that is supported


	static vector<const ASoundTranslator *> registeredTranslators;

protected:

	// ??? It might make sense to swap the return values of these before anyone codes
		// these should return true normally, or they should return false if cancelled
	virtual bool onLoadSound(const string filename,CSound *sound) const=0;
	virtual bool onSaveSound(const string filename,CSound *sound) const=0;

private:

};

#endif
