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

#ifndef CCXX_path_H_
#define CCXX_path_H_

#include "../../config/common.h"

/*
	This was first written and tested under linux, however a few provisions
	have been made for a win32 port.  
	Also, several functions used are probably only available under linux, i.e.
		realpath, dirname, basename
	But there should be some equivalent under other unices, and implementation
	changes should be made when this file is compiled under these other platforms.
 */

#ifndef WIN32
	#include <sys/types.h>
	#include <sys/stat.h>

	#include <errno.h>
	#include <unistd.h>

	#include <fcntl.h>

	#include <limits.h>
	#include <stdlib.h>

	#include <string.h>

	#include <libgen.h>  // for basename and dirname
#else
	#error unimplemented for win32 platform (yet)
#endif

#include <stdexcept>
#include <string>


// this needs to be placed somewhere once in the application
#ifdef WIN32
#define DECLARE_STATIC_CPATH const char CPath::dirDelim='\\';
#else
#define DECLARE_STATIC_CPATH const char CPath::dirDelim='/';
#endif



class CPath
{
public:
	static const char dirDelim; 

	CPath(const string &_path="") :
		path(NULL),
		_exists(false)
	{
		setPath(_path);
	}

	virtual ~CPath()
	{
		free(path);
	}

	/*
		changes the path from what it was constructed with
	*/
	void setPath(const string &_path)
	{
		if(_path.size()>=PATH_MAX)
			throw runtime_error(string(__func__)+" -- path is >= than PATH_MAX -- '"+path+"'");

		if(path!=NULL)
			free(path);
		path=strdup(_path.c_str());

		if(stat(path,&statBuf)!=0)
		{
			_exists=false;
			if(errno!=ENOENT)
				throw runtime_error(string(__func__)+" -- error stat-ing path name -- '"+path+"' -- "+strerror(errno));
		}
		else
			_exists=true;
	}

	/*
		returns the path constructed with or the last one passed to setPath
	*/
	const char *getPath() const
	{
		return(path);
	}

	/*
		returns true iff the pathname exists and we could access it
	*/
	bool exists() const
	{
		return(_exists);
	}

	/*
		possbily creates the path and/or updates its time(s) to current
	*/
	bool touch(bool canCreate=true,bool throwIfError=true) const
	{
		if(canCreate)
		{
			int fd = open (path, O_WRONLY | O_CREAT | O_NONBLOCK | O_NOCTTY,S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
			if (fd == -1)
			{	
				if(throwIfError)
					throw runtime_error(string(__func__)+" -- error touching path name '"+path+"' -- "+strerror(errno));
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
					throw runtime_error(string(__func__)+" -- error touching path name '"+path+"'-- "+strerror(errno));
				else
					return(false);
			}

			if(fd!=-1)
				close(fd);
		}

		return(true);
	}

	/*
		returns the absolute path resolved from a possibly relative path
	*/
	const string realPath() const
	{
		if(!_exists)
			return("");

		char resolvedPath[PATH_MAX];
		char *result=realpath(path,resolvedPath);
		if(result!=NULL)
			return(result);
		else
			return("");
	}

	/*
		returns the extension of the path name (i.e. returns "ext" for "/dir1/dir2/file.ext")
	*/
	const string extension() const
	{
		// find the right-most '.' which is also after the right-most dirDelim 

		char *lastDot=strrchr(path,'.');
		if(lastDot==NULL) // go ahead and bail if there wasn't even a '.' in the path
			return("");

		char *lastDirDelim=strrchr(path,CPath::dirDelim);

		if(lastDot>lastDirDelim)
			return(lastDot+1);
		else
			return("");
		
	}

	/*
		returns just the directory name(s) part of the path (i.e. returns "/dir1/dir2" for "/dir1/dir2/file.ext")
	*/
	const string dirName() const
	{
		// make a copy because the function modifies the contents
		char tmp[PATH_MAX];
		strcpy(tmp,path);

		return(dirname(tmp));
	}
	
	/*
		returns just the filename part of the path (i.e. returns "file.ext" for "/dir1/dir2/file.ext")
	*/
	const string baseName() const
	{
		// make a copy because the function modifies the contents
		char tmp[PATH_MAX];
		strcpy(tmp,path);

		return(basename(tmp));
	}

	/*
		returns the number of bytes allocated for the file
		either returns 0 or throws an exception if it doesn't exist or wasn't accessible
	*/
	long getSize(bool throwIfError=true) const
	{
		if(!_exists)
		{
			if(throwIfError)
				throw(runtime_error(string(__func__)+" -- path did not exist or was inaccessible -- '"+path+"'"));
			else
				return(0);
		}

		return(statBuf.st_size);
	}

	// many other things could be returned from the stat
	//time_t get ... Time() const;


	// returns true even if it's a link to a regular file
	bool isRegularFile() const
	{
		return S_ISREG(statBuf.st_mode);
	}

	bool isLink() const
	{
		return S_ISLNK(statBuf.st_mode);
	}

	bool isDevice() const
	{
		return S_ISCHR(statBuf.st_mode) || S_ISBLK(statBuf.st_mode);
	}

private:
	char *path;
	bool _exists;
	struct stat statBuf;

};

#endif
