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

/*
 * This class can be derived from to make easier opening a pipe to an 
 * application to load and save non-native file formats
 */

#include "ApipedSoundTranslator.h"

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <stdexcept>

#include "mypopen.h"

#include <CPath.h>

ApipedSoundTranslator::ApipedSoundTranslator()
{
}

ApipedSoundTranslator::~ApipedSoundTranslator()
{
}

const string ApipedSoundTranslator::findAppOnPath(const string appName)
{
	return CPath::which(appName);
}

/* translate \ to \\ and " to \" in the given filename and put quotes around filename */
// ??? This would probably need to behave a little differently on WIN32 unless it can use " and \ in pathnames
const string ApipedSoundTranslator::escapeFilename(const string _filename)
{
	string filename=_filename;
	for(size_t t=0;t<filename.size();t++)
	{
		if(filename[t]=='\\' || filename[t]=='"')
		{
			filename.insert(t,"\\");
			t++;
		}
	}
	return "\""+filename+"\"";
}

bool ApipedSoundTranslator::checkThatFileExists(const string filename)
{
	return CPath(filename).exists();
}

void ApipedSoundTranslator::removeExistingFile(const string filename)
{
	if(CPath(filename).exists())
	{
		if(unlink(filename.c_str())!=0)
		{
			int err=errno;
			throw(runtime_error(string(__func__)+" -- error removing file, '"+filename+"' -- "+strerror(err)));
		}
	}
}

/* this wasn't defined on solaris or bsd using gcc when I ported the code */
#if defined(rez_OS_SOLARIS) || defined(rez_OS_BSD)
	typedef void (*sighandler_t) (int);
#endif

static sighandler_t origSIGPIPE_Handler;
bool ApipedSoundTranslator::SIGPIPECaught;
void SIGPIPE_Handler(int sig)
{
	ApipedSoundTranslator::SIGPIPECaught=true;
}

void ApipedSoundTranslator::setupSIGPIPEHandler()
{
	SIGPIPECaught=false;

	// setup a signal handler to handle a SIGPIPE in case the application crashes or doesn't like something
	origSIGPIPE_Handler=signal(SIGPIPE,SIGPIPE_Handler);
}

void ApipedSoundTranslator::restoreOrigSIGPIPEHandler()
{
	if(origSIGPIPE_Handler!=NULL && origSIGPIPE_Handler!=SIG_ERR)
		signal(SIGPIPE,origSIGPIPE_Handler);
}


FILE *ApipedSoundTranslator::popen(const string cmdLine,const string mode,FILE **errStream)
{
	FILE *p=mypopen(cmdLine.c_str(),mode.c_str(),errStream);
	if(p==NULL)
	{
		int err=errno;
		throw(runtime_error(string(__func__)+" -- error creating pipe to command: '"+cmdLine+"' -- "+strerror(err)));
	}

	return p;
}

int ApipedSoundTranslator::pclose(FILE *p)
{
	if(p!=NULL)
		return mypclose(p);
	else
		return 0;
}

