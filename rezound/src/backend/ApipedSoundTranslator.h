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

#ifndef __ApipedSoundTranslator_H__
#define __ApipedSoundTranslator_H__

#include "../../config/common.h"

#include "ASoundTranslator.h"

#include <stdio.h>

class ApipedSoundTranslator : public ASoundTranslator
{
public:
	ApipedSoundTranslator();
	virtual ~ApipedSoundTranslator();

	// a method like this should be implemented in the derived
	// class for knowing whether to register the translator or not
	//static bool checkForApp();

protected:
	// utility functions that all SoundTranslator classes that pipe to an application for I/O will need

	static const string findAppOnPath(const string appName);	// search $PATH for a given application name
	static const string escapeFilename(const string filename);	// used to escape necessary characters for placing filenames on the command line that might get broken into multiple arguments because of spaces
	static bool checkThatFileExists(const string filename);		// should be done before attempting to open pipe
	static void removeExistingFile(const string filename);		// should be done before attempting to open pipe

	static void setupSIGPIPEHandler(); 				// should be called and SIGPIPECaught checked whenever saving and writing to pipe
	static void restoreOrigSIGPIPEHandler(); 			// should be called after saving is finished or when an exception is caught and bailing
	static bool SIGPIPECaught;

	static FILE *popen(const string cmdLine,const string mode,FILE **errStream);
	static int pclose(FILE *p);

	friend void SIGPIPE_Handler(int sig);
};

#endif
