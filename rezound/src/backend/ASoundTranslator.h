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

#include "CSound_defs.h"

class ASoundTranslator
{
public:
	ASoundTranslator();
	virtual ~ASoundTranslator();

	/* 
	 * Normally returns true, or return false if cancelled.
	 */
	bool loadSound(const string filename,CSound *sound) const;

	/*
	 * Normally returns true, or return false if cancelled.
	 * useLastUserPrefs says: if possible, avoid prompting the user for saving 
	 * preferences (i.e. compression type, audio format, etc) and use the last 
	 * user chosen set of information.
	 */
	bool saveSound(const string filename,const CSound *sound,bool useLastUserPrefs=false) const;
	bool saveSound(const string filename,const CSound *sound,const sample_pos_t saveStart,const sample_pos_t saveLength,bool useLastUserPrefs=false) const;

	// filename is passed, but that is just because sometimes at the point at which we want to let the extension determine the format, the filename might also help determine it too
	virtual bool handlesExtension(const string extension,const string filename) const=0;
	virtual bool supportsFormat(const string filename) const=0; // this will only get called iff the file exists and is a regular file
	virtual bool handlesRaw() const { return(false); }	// only the raw translator implementation should override this to return true

	virtual const vector<string> getFormatNames() const=0;			// return a list of format names than this derivation handles
	virtual const vector<vector<string> > getFormatFileMasks() const=0;	// return a group of filemasks for each format name that is supported (i.e. "*.wav")

	// returns a translator object that can handle loading the given file (detected either by the file contents or filename extention)
	// an exception is thrown if no translator can handle it
	// ??? perhaps I should have some enum here which indicates a desired format incase the extension is non-standard
	static const ASoundTranslator *findTranslator(const string filename,bool byExtensionOnly,bool isRaw);

	static const ASoundTranslator *findRawTranslator();
	static const vector<const ASoundTranslator *> getTranslators();
	static const vector<string> getFlatFormatList(); // returns a flattened list of every supported extension followed by a " [" then the format name then "]'

protected:

	// ??? It might make sense to swap the return values of these before anyone codes
		// these should return true normally, or they should return false if cancelled
	virtual bool onLoadSound(const string filename,CSound *sound) const=0;
	virtual bool onSaveSound(const string filename,const CSound *sound,const sample_pos_t saveStart,const sample_pos_t saveStop,bool useLastUserPrefs) const=0;

private:
	// this vectors is to be a list of all implemented (and enabled) ASoundTranslator derived classes
	static vector<const ASoundTranslator *> registeredTranslators;
	static void buildRegisteredTranslatorsVector();

};

#endif
