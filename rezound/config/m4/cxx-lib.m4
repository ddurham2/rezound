dnl cxx-lib.m4 
dnl Written by Anthony Ventimiglia
dnl Modified by David W. Durham -- enahanced and now uses autoconf-2.5x concepts
dnl
dnl Copyright (C) 2002 - Anthony Ventimiglia
dnl 
dnl This file is part of ReZound, an audio editing application, 
dnl 
dnl ReZound is free software; you can redistribute it and/or modify it under i
dnl the  terms of the GNU General Public License as published by the 
dnl Free Software Foundation; either version 2 of the License, or (at your 
dnl option) any later version.
dnl 
dnl ReZound is distributed in the hope that it will be useful, but WITHOUT ANY
dnl WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS 
dnl FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
dnl details.
dnl 
dnl You should have received a copy of the GNU General Public License along 
dnl with this program; if not, write to 
dnl the Free Software Foundation, Inc., 59 Temple
dnl Place - Suite 330, Boston, MA  02111-1307, USA



dnl ajv_CXX_CHECK_LIB(Library Name, Class, header, URL [, -l flags [, true/false (error if not found) ] ])
dnl This is a check for a class contained in a C++ library. The test is 
dnl performed by test linking a minimal program. 
dnl
dnl Arguments passed
dnl 1: the name of the library with the lib prefix removed (eg FOX for libFOX)
dnl 2: The Class name to be used to check the link. It should be a class name
dnl 	that can also be used as a constructor with no parameters. But be
dnl 	sure that the constructor's definition is internal to the library
dnl 	and not defined in a header file, otherwise the test won't be accurate.
dnl 3: header file defining the tested class. This will be included in the test
dnl 	program. 
dnl 4: A URL to link to a download to find the latest version of a library 
dnl 	should the test fail. 
dnl 5: Extra -l flags to pass to linker for link check. Obviously this is an 
dnl		optional argument, but should you need to pass extra link flags (as 
dnl		with ccgnu) then you can pass them here. Make sure to pass a list of 
dnl		flags, and not just library names( eq: -lpthread -ldl )
dnl 6: 'true', or 'false' whether or not to error if the lib isn't found.  The default
dnl    if the argument is not passed is true.
dnl
dnl By checking for specific classes this check can be used to check if a 
dnl library version is compatible with the classes used in the application.
dnl
dnl This macro sets the option --disable-LIB-check to optionally
dnl override the check. The error report given should the test fail is 
dnl meant to help a user with the installation process. 
dnl 
dnl This macro defines some args that define the include or lib paths
dnl 
dnl This macro will also #define HAVE_LIBXXX where XXX is the capitalized
dnl normalized name of arg 1
dnl
dnl What a mess with the AS_TR_CPP($1); now if I knew how to assign a variable
dnl at the top that I could use everwhere else I would
dnl
AC_DEFUN(ajv_CXX_CHECK_LIB,

	dnl prepare to beable to AC_DEFINE the HAVE_LIBXXX #define
	[AH_TEMPLATE(AS_TR_CPP(HAVE_LIB$1),[defined by $0])]

	dnl create a --disable-LIB-check flag to configure
	[AC_ARG_ENABLE(AS_TR_CPP($1)-check,
		AC_HELP_STRING([--disable-@&t@AS_TR_CPP($1)-check],[Override the check for $1 library]),
		[
			enable_@&t@AS_TR_CPP($1)_check="$enableval"
		],
		[
			enable_@&t@AS_TR_CPP($1)_check="yes"
		]
	)]

	dnl create a --with-LIB-include flag to configure (which sets the -I flags to the lib's headers)
	[AC_ARG_WITH(AS_TR_CPP($1)-include,
		AC_HELP_STRING([--with-@&t@AS_TR_CPP($1)-include],[Specify path to $1 header files]),
		[ 	
			ajv_inc@&t@AS_TR_CPP($1)_path=-I$withval
			# I'm commenting this out, so passing the with option won't override check.
			# enable_@&t@AS_TR_CPP($1)_check="no"  
		],
		[
			ajv_inc@&t@AS_TR_CPP($1)_path=""
		]
	)]

	dnl create a --with-LIB-path flag to configure (which sets the -L flags to the lib)
	[AC_ARG_WITH(AS_TR_CPP($1)-path,
		AC_HELP_STRING([--with-@&t@AS_TR_CPP($1)-path],[Specify path to $1 libraries]),
		[	
			ajv_lib@&t@AS_TR_CPP($1)_path=-L$withval
			# I'm commenting this out, so passing the with option won't override check.
			# enable_@&t@AS_TR_CPP($1)_check="no" 
		],
		[
			ajv_lib@&t@AS_TR_CPP($1)_path=""
		]
	)]

	[AS_TR_CPP($1)_CXXFLAGS="$ajv_inc@&t@AS_TR_CPP($1)_path"]
	[AS_TR_CPP($1)_LIBS="$ajv_lib@&t@AS_TR_CPP($1)_path -l$1 $5"]


	if test [x"$enable_@&t@AS_TR_CPP($1)_check"] = x"yes"; then

		[AC_MSG_CHECKING(for $2 class in -l$1)]

		dnl these are not procedures, but are a statement that unconditionally make a substitution later
		[AC_SUBST(AS_TR_CPP($1)_CXXFLAGS)]
		[AC_SUBST(AS_TR_CPP($1)_LIBS)]

		[AC_LANG_PUSH(C++)]

		saved_CPPFLAGS="$CPPFLAGS"
		saved_CXXFLAGS="$CXXFLAGS"
		saved_LDFLAGS="$LDFLAGS"
		saved_CXX="$CXX"

		[CPPFLAGS="$CPPFLAGS $ajv_inc@&t@AS_TR_CPP($1)_path"]
		[CXXFLAGS="$CXXFLAGS $ajv_inc@&t@AS_TR_CPP($1)_path"]
		[LDFLAGS="$LDFLAGS $ajv_lib@&t@AS_TR_CPP($1)_path -l$1 $5"]
		
		builddir=`pwd`  # I'm assuming that the pwd will always be what $(top_builddir) will be set to in AC_OUTPUT

		# HACK: force AC_LINK_IFELSE to use libtool to link so it will get FOX's dependancies.  Personally, I think it should do this anyway because I've used AC_PROG_LIBTOOL to request libtool as the linker for the subsequent make
		CXX="$builddir/libtool --mode=link $CXX"

		[AC_LINK_IFELSE([
				#include <$3>
				int main() { $2 xxx; return 0; }
			],
			[ # worked
				AC_MSG_RESULT(yes)
				AC_DEFINE_UNQUOTED(AS_TR_CPP(HAVE_LIB$1))
			],
			[ # failed
				AC_MSG_RESULT(no)
				AS_TR_CPP($1)_CXXFLAGS=""
				AS_TR_CPP($1)_LIBS=""

				if ! test z"$6" = "zfalse"; then
					AC_MSG_ERROR([ 
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   !!!!! Fatal Error:  lib$1 missing, broken or outdated !!!!!
   
   My test couldn't find the class $2 in lib$1
   
   If you don't have $1, or need a more recent version, download 
   the latest version at: $4
 
   If you have $1 installed and the linker couldn't find it, you can specify
   the path by passing --with-@&t@AS_TR_CPP($1)-path=/path/to/$1 to configure.
   You may also have to pass the $1 header path with the 
   --with-@&t@AS_TR_CPP($1)-include=   option. Setting these flags overrides the library 
   test.

   If you have $1 and you believe it is up to date, you can override 
   this check by passing --disable-@&t@AS_TR_CPP($1)-check as an option to configure. 
   specifying --with-@&t@AS_TR_CPP($1)-path or --with-@&t@AS_TR_CPP($1)-include will override the
   test for lib$1
   
   If you believe this error message is a result of a bug in the 
   configure script, please report the bug to the Package Maintainers. 
   See docs/INSTALL for information on submitting bug-reports.
 
   When submitting a bug report about this issue, please choose: 
		\"... Compile/Configure Issue\"
   as the category of the bug.  Also, it would really help me if you 
   include a screendump or script file of the error, see script(1) man 
   page for info on how to record a script file.

   See config.log for compiler input and output.

   ReZound home page http://rezound.sourceforge.net
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
					])
				fi
			],
			[ # cross compiling
				AC_MSG_ERROR([Cannot test for FOX since we're cross compiling -- this situation has not been addressed])
			]
		)]

		CPPFLAGS="$saved_CPPFLAGS"
		CXXFLAGS="$saved_CXXFLAGS"
		LDFLAGS="$saved_LDFLAGS"
		CXX="$saved_CXX"

		[AC_LANG_POP(C++)]
	fi
)

