# vim:sw=4:ts=4
dnl cxx-lib.m4 
dnl Written by Anthony Ventimiglia
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

dnl ajv_CXX_CHECK_LIB(Library Name, Class, header, URL)
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
dnl
dnl By checking for specific classes this check can be used to check if a 
dnl library version is compatible with the classes used in the application.
dnl
dnl This macro also sets the option --disable-LIB-check to optionally
dnl override the check. The error report given should the test fail is 
dnl meant to help a user with the installation process. 
dnl 
dnl This macro will also #define HAVE_LIBXXX where XXX is the capitalized
dnl normalized name if arg 1
AC_DEFUN(ajv_CXX_CHECK_LIB, dnl
[AC_ARG_WITH($1-include, dnl
[  --with-$1-include	  Specify path to $1 header files], dnl
ajv_inc$1_path=-I$withval, ajv_inc$1_path="")] dnl
[AC_ARG_WITH($1-path,[  --with-$1-path	  Specify path to $1 libraries], dnl
ajv_lib$1_path=-L$withval, ajv_lib$1_path="")] dnl
[AC_ARG_ENABLE($1-check, dnl
[  --disable-$1-check	  Override the check for $1 library], dnl
[ enable_$1_check=$enableval ], dnl
[ enable_$1_check="yes" ])]
if test "$enable_$1_check" = "yes"; then
	[AC_MSG_CHECKING(for $2 class in -l$1)] 
	cat > ajv_chk_cxx_lib_$1.cc << EOF
#include <$3>
int main()
{ $2 xxx; }
EOF
	$CXX -l$1 $5 $ajv_lib$1_path $ajv_inc$1_path ajv_chk_cxx_lib_$1.cc >/dev/null 2>ajv_chk_cxx_lib_$1.err
	if test $? = 0; then
		AC_MSG_RESULT(yes)
		LDFLAGS="$ajv_lib$1_path $LDFLAGS"
		CXXFLAGS="$ajv_inc$1_path $CXXFLAGS"
		rm -f ajv_chk_cxx_lib_$1.cc
		rm -f ajv_chk_cxx_lib_$1.err
	else
		AC_MSG_RESULT(no)
		rm -f ajv_chk_cxx_lib_$1.cc
		rm -f ajv_chk_cxx_lib_$1.err
		[AC_MSG_ERROR([ 
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   !!!!! Fatal Error:  lib$1 missing, broken or outdated !!!!!
   
   My test couldn't link the class $2 in lib$1
   
   If you don't have $1, or need a more recent version, download 
   the latest version at: $4
 
   If you have $1 installed and the linker couldn't find it, you can specify
   the path by passing --with-$1-path=/path/to/$1 to configure.

   If you have $1 and you believe it is up to date, you can override 
   this check by passing --disable-$1-check as an option to configure. 
   
   If you believe this  error message is a result of a bug in the 
   configure script, please report the bug to the Package Maintainers. 
   See docs/INSTALL for information on submitting bug-reports.
 
   When submitting a bug report about this issue, please choose: 
		\"... Compile/Configure Issue\"
   as the category of the bug.  Also, it would really help us if you 
   include a screendump or script file of the error, see script(1) man 
   page for info on how to record a script file.

   ReZound home page http://rezound.sourceforge.net
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
])]
	fi
fi
)

dnl ajv_CHECK_LIB_ABORT
dnl This is used to check the results of an AC_CHECK_LIB test. By running 
dnl confdefs.h through cpp after the check we can abort if the test failed
dnl and print a message similar to the above. First it runs AC_CHECK_LIB
dnl then checks the result. It also gives an option to override the test.
dnl AS well as an option to pass the path to the library.
dnl
dnl arguments:
dnl	1. Library name with lib prefix stripped
dnl 2. Funciton to check for
dnl 3. cpp macro to test for, this will be a capitalized library name, eg
dnl		if testing for libaudiofile, the macro would be LIBAUDIOFILE
dnl 	I'm sure I could have used sed or tr to do this, but I didn't feel
dnl 	like it. 
dnl
dnl 4. URL to download library given in abort message.
AC_DEFUN(ajv_CHECK_LIB_ABORT, dnl
[AC_ARG_WITH($1-include, dnl
[  --with-$1-include	  Specify path to $1 header files], dnl
ajv_inc$1_path=-I$withval, ajv_inc$1_path="")] dnl
[AC_ARG_WITH($1-path,[  --with-$1-path	  Specify path to $1 libraries], dnl
ajv_lib$1_path=-L$withval, ajv_lib$1_path="")] dnl
[AC_ARG_ENABLE($1-check, dnl
[  --disable-$1-check     Override the check for $1 library ], dnl
[enable_$1_check=$enableval], dnl
[enable_$1_check="yes" ])]
if test "$enable_$1_check" = "yes"; then
	[AC_CHECK_LIB($1, $2)]
	cat > ajv_ck_lib_$1.c <<EOF
#include "confdefs.h"
#ifndef HAVE_LIB$3
#error $1 library check failed
#endif
EOF
	$CPP $ajv_lib$1_path $ajv_inc$1_path ajv_ck_lib_$1.c >/dev/null 2>ajv_ck_lib_$1.err
	if test $? = 0; then
		LDFLAGS="$ajv_lib$1_path $LDFLAGS"
		CXXFLAGS="$ajv_inc$1_path $CXXFLAGS"
		rm -f ajv_ck_lib_$1.c
		rm -f ajv_ck_lib_$1.err
	else
		rm -f ajv_ck_lib_$1.c
		rm -f ajv_ck_lib_$1.err
		[AC_MSG_ERROR([ 
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   !!!!! Fatal Error:  lib$1 missing, broken or outdated !!!!!
   
   My test couldn't link the function $2() in lib$1
   
   If you don't have $1, or need a more recent version, download 
   the latest version at: $4

   If you have $1 installed and the linker couldn't find it, you can specify
   the path by passing --with-$1-path=/path/to/$1 to configure.
 
   If you have $1 and you believe it is up to date, you can override 
   this check by passing --disable-$1-check as an option to configure. 
   
   If you believe this  error message is a result of a bug in the 
   configure script, please report the bug to the Package Maintainers. 
   See docs/INSTALL for information on submitting bug-reports.
 
   When submitting a bug report about this issue, please choose: 
		\"... Compile/Configure Issue\"
   as the category of the bug.  Also, it would really help us if you 
   include a screendump or script file of the error, see script(1) man 
   page for info on how to record a script file.

   ReZound home page http://rezound.sourceforge.net
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
])]
	fi
fi
)
