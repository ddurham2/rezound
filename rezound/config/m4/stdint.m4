# vim:sw=4:ts=4
dnl stdint.m4 
dnl Written by Anthony Ventimiglia
dnl    modified by David W. Durham 6/22/2002
dnl    Anythony may want to generalize this file into something where "stdint.h"
dnl     is passed as an argument so it can be used for more than just stdint.h
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

dnl ajv_CHECK_HEADER_STDINT
dnl This Checks for the ANSI C99 stdint.h header which may be missing on some 
dnl systems, (specifically Debian 2.2r4) I perform the check by trying to 
dnl preprocess #include <stdint.h>. if the test fails then the directory is
dnl generated, src/misc/missing/generated and src/misc/missing/stdint.h-missing
dnl is copied into that directory.  If the test passes we remove any existing
dnl stdint.h file in that directory.

AC_DEFUN(ajv_CHECK_HEADER_STDINT, dnl
[AC_MSG_CHECKING(for STL stdint.h header)] 
cat > chk_hdr_sstr.cc <<EOF
#include <stdint.h>
int main(){}
EOF
$CXXCPP chk_hdr_sstr.cc >/dev/null 2>chk_hdr_sstr.err
if test x"$?" = x"0"; then
	AC_MSG_RESULT(yes)
	rm -f $srcdir/src/misc/missing/generated/stdint.h
	rmdir $srcdir/src/misc/missing/generated 2>/dev/null
else
	mkdir $srcdir/src/misc/missing/generated 2>/dev/null
	cp $srcdir/src/misc/missing/stdint.h-missing $srcdir/src/misc/missing/generated/stdint.h
	AC_MSG_RESULT(no)
fi
rm -rf chk_hdr_sstr.err
rm -rf chk_hdr_sstr.cc
rm -rf a.out dnl
)
