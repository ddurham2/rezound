// Copyright (C) 2002 Open Source Telecom Corporation. (David W. Durham)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
// As a special exception to the GNU General Public License, permission is
// granted for additional uses of the text contained in its release
// of Common C++.
//
// The exception is that, if you link the Common C++ library with other
// files to produce an executable, this does not by itself cause the
// resulting executable to be covered by the GNU General Public License.
// Your use of that executable is in no way restricted on account of
// linking the Common C++ library code into it.
//
// This exception does not however invalidate any other reasons why
// the executable file might be covered by the GNU General Public License.
//
// This exception applies only to the code released under the
// name Common C++.  If you copy code from other releases into a copy of
// Common C++, as the General Public License permits, the exception does
// not apply to the code that you add in this way.  To avoid misleading
// anyone as to the status of such modified files, you must delete
// this exception notice from them.
//
// If you write modifications of your own for Common C++, it is your
// choice whether to permit this exception to apply to your modifications.
// If you do not wish that, delete this exception notice.

#include "path.h"

/*
	This was first written and tested under linux, however a few provisions
	have been made for a win32 port.  
	Also, several functions used are probably only available under linux, i.e.
		realpath, dirname, basename
	But there should be some equivalent under other unices, and implementation
	changes should be made when this file is compiled under these other platforms.
 */

#include <errno.h>

#ifndef WIN32
	#include <unistd.h>

	#include <fcntl.h>

	#include <limits.h>
	#include <stdlib.h>

	#include <string.h>

	#include <libgen.h>  // for basename and dirname
#else
	#error unimplemented for win32 platform
#endif


#ifdef  CCXX_NAMESPACES
namespace ost {
using namespace std;
#endif

#ifdef WIN32
	const char Path::dirDelim='\\';
#else
	const char Path::dirDelim='/';
#endif

Path::Path(const string &_path) :
	path(NULL),
	exists(false)
{
	setPath(_path);
}

Path::~Path()
{
	free(path);
}

void Path::setPath(const string &_path)
{
	if(_path.size()>=PATH_MAX)
		throw PathException(string(__func__)+" -- path is >= than PATH_MAX -- '"+path+"'");

	if(path!=NULL)
		free(path);
	path=strdup(_path.c_str());

	if(stat(path,&statBuf)!=0)
	{
		exists=false;
		if(errno!=ENOENT)
			throw PathException(string(__func__)+" -- error stat-ing path name -- '"+path+"' -- "+string(strerror(errno)));
	}
	else
		exists=true;
}

const char *Path::getPath() const
{
	return(path);
}

bool Path::Exists() const
{
	return(exists);
}

bool Path::Touch(bool canCreate,bool throwIfError) const
{
	if(canCreate)
	{
		int fd = open (path, O_WRONLY | O_CREAT | O_NONBLOCK | O_NOCTTY,S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		if (fd == -1)
		{	
			if(throwIfError)
				throw PathException(string(__func__)+" -- error touching path name '"+path+"'-- "+string(strerror(errno)));
			else
				return(false);
		}
		close(fd);
	}
	else
	{
		int fd = open (path, O_WRONLY | O_NONBLOCK | O_NOCTTY,S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		if (fd == -1 && errno!=ENOENT)
		{	
			if(throwIfError)
				throw PathException(string(__func__)+" -- error touching path name '"+path+"'-- "+string(strerror(errno)));
			else
				return(false);
		}

		if(fd!=-1)
			close(fd);
	}

	return(true);
}

const string Path::RealPath() const
{
	if(!exists)
		return("");

	char resolvedPath[PATH_MAX];
	char *result=realpath(path,resolvedPath);
	if(result!=NULL)
		return(result);
	else
		return("");
}

const string Path::Extension() const
{
	// find the right-most '.' which is also after the right-most dirDelim 

	char *lastDot=strrchr(path,'.');
	if(lastDot==NULL) // go ahead and bail if there wasn't even a '.' in the path
		return("");

	char *lastDirDelim=strrchr(path,dirDelim);

	if(lastDot>lastDirDelim)
		return(lastDot+1);
	else
		return("");
	
}

const string Path::DirName() const
{
	// make a copy because the function modifies the contents
	char tmp[PATH_MAX];
	strcpy(tmp,path);

	return(dirname(tmp));
}

const string Path::BaseName() const
{
	// make a copy because the function modifies the contents
	char tmp[PATH_MAX];
	strcpy(tmp,path);

	return(basename(tmp));
}


// many other things could possibly be returned from the stat
long Path::getSize(bool throwIfError) const
{
	if(!exists)
	{
		if(throwIfError)
			throw(PathException(string(__func__)+" -- path did not exist or was inaccessible -- '"+path+"'"));
		else
			return(0);
	}

	return(statBuf.st_size);
}

//time_t get ... Path::Time() const;

#ifdef  CCXX_NAMESPACES
}
#endif
