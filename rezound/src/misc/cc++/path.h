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

#ifndef CCXX_path_H_
#define CCXX_path_H_


#include "../../../config/common.h"

#ifndef CCXX_CONFIG_H_
	#include <cc++/config.h>
#endif


#include <string>

//see BBB note below  #include <cc++/exception.h>

#ifndef WIN32
	#include <sys/types.h>
	#include <sys/stat.h>
#else
	#error unimplemented for win32 platform
#endif


// see BBB note below (this is what I did instead)
#include <stdexcept>
typedef runtime_error PathException;



#ifdef  CCXX_NAMESPACES
namespace ost {
#endif

/* BBB: I tried using this, but it wouldn't retain the whole exception message at the throw... it did if I used runtime_error from stdexcept
   so, did what's above instead instead ...
class PathException : public Exception
{
public:
	PathException(std::string str) : Exception(str) {};
};
*/



class Path
{
public:
	static const char dirDelim; // either '\' or '/' depending on the platform

	Path(const std::string &path="");
	virtual ~Path();

	/*
		changes the path from what it was constructed with
	*/
	void setPath(const std::string &path);

	/*
		returns the path constructed with or the last one passed to setPath
	*/
	const char *getPath() const;

	/*
		returns true iff the pathname exists and we could access it
	*/
	bool Exists() const;

	/*
		possbily creates the path and/or updates its time(s) to current
	*/
	bool Touch(bool canCreate=true,bool throwIfError=true) const;

	/*
		returns the absolute path resolved from a possibly relative path
	*/
	const std::string RealPath() const;

	/*
		returns the extension of the path name (i.e. returns "ext" for "/dir1/dir2/file.ext")
	*/
	const std::string Extension() const;

	/*
		returns just the directory name(s) part of the path (i.e. returns "/dir1/dir2" for "/dir1/dir2/file.ext")
	*/
	const std::string DirName() const;
	
	/*
		returns just the filename part of the path (i.e. returns "file.ext" for "/dir1/dir2/file.ext")
	*/
	const std::string BaseName() const;

	/*
		returns the number of bytes allocated for the file
		either returns 0 or throws an exception if it doesn't exist or wasn't accessible
	*/
	long getSize(bool throwIfError=true) const;
	// many other things could be returned from the stat
	//time_t get ... Time() const;


private:
	char *path;
	bool exists;
	struct stat statBuf;

};

#ifdef  CCXX_NAMESPACES
}
#endif


#endif
