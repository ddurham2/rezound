# vim:sw=4:ts=4
dnl cxx.m4 
dnl Written by Anthony Ventimiglia
dnl
dnl Copyright (C) 2002 - Anthony Ventimiglia
dnl 
dnl This file is part of ReZound, an audio editing application, 
dnl 
dnl ReZound is free software; you can redistribute it and/or modify it under the
dnl terms of the GNU General Public License as published by the Free Software
dnl Foundation; either version 2 of the License, or (at your option) any later
dnl version.
dnl 
dnl ReZound is distributed in the hope that it will be useful, but WITHOUT ANY
dnl WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS 
dnl FOR
dnl A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
dnl 
dnl You should have received a copy of the GNU General Public License along with
dnl this program; if not, write to the Free Software Foundation, Inc., 59 Temple
dnl Place - Suite 330, Boston, MA  02111-1307, USA

dnl These macros check if gcc will work with extra warning flags. Specifically
dnl it was found that 2.96 works with -Wno-unused-variable and 
dnl -Wno-unused-function while 2.95.2 does not If the flags will work, we'll
dnl add them to our gcc CXXFLAGS

dnl ajv_CXX_FLAG
dnl Usage - Pass one argument- the Compiler flag to check for. If the falg is
dnl found to be valid, it is included in CXXFLAGS otherwise, it's left out
AC_DEFUN(ajv_CXX_FLAG, dnl
[AC_MSG_CHECKING(if $CXX accepts $1 )] 
cat > ajv_cxx_flag.cc << EOF
int main(){}
EOF
$CXX $1 ajv_cxx_flag.cc >/dev/null 2>ajv_cxx_flag.err
if test x"$?" = x"0"; then
	AC_MSG_RESULT(yes)
	CXXFLAGS="$CXXFLAGS $1"
else
	AC_MSG_RESULT(no)
fi
rm -rf ajv_cxx_flag.cc
rm -rf ajv_cxx_flag.err
)
