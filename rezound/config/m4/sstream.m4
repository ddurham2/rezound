# vim:sw=4:ts=4
dnl sstream.m4 
dnl Written by Anthony Ventimiglia
dnl
dnl Copyright (C) 2002 - David W. Durham
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

dnl This Checks for the C++ STL sstream header which may be missing on some 
dnl systems, (specifically Debian 2.2r4) I perform the check by trying to 
dnl preprocess $include <sstream>. if the test passes 
dnl we define HAVE_SSTREAM which is undefined by default in acconfig.h
AC_DEFUN(ajv_CHECK_HEADER_SSTREAM, dnl
[AC_MSG_CHECKING(for STL sstream header)] dnl
cat > chk_hdr_sstr.cc <<EOF
int main(){}
EOF
$ac_cpp chk_hdr_sstr.cc >/dev/null 2>chk_hdr_sstr.err
if test $? = 0; then
	AC_MSG_RESULT(yes)
	AC_DEFINE(HAVE_SSTREAM)
else
	AC_MSG_RESULT(no)
fi
rm -rf chk_hdr_sstr.err
rm -rf chk_hdr_sstr.cc
rm -rf a.out dnl
)
