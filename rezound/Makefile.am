# vim:tw=78
# ReZound Makefile.am Written by Anthony Ventimiglia
## Process this file with automake to create Makefile.in
##
##
## Copyright (C) 2002 - Anthony Ventimiglia
## 
## This file is part of ReZound, an audio editing application, 
## 
## ReZound is free software; you can redistribute it and/or modify it under the
## terms of the GNU General Public License as published by the Free Software
## Foundation; either version 2 of the License, or (at your option) any later
## version.
## 
## ReZound is distributed in the hope that it will be useful, but WITHOUT ANY
## WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
## A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
## 
## You should have received a copy of the GNU General Public License along with
## this program; if not, write to the Free Software Foundation, Inc., 59 Temple
## Place - Suite 330, Boston, MA  02111-1307, USA

## I used config/am_include.mk for common definitions
include $(top_srcdir)/config/am_include.mk

## Descend into SUBDIRS and run make. Look at the Makefile.am files in the
## subdirectories Start at frontend_fox/Makefile.am to see how everything works.
SUBDIRS= \
	docs \
	share \
	src

## These extra files and directories will be included in the distribution. by
## using make-dist by default many common filenames are automatically included
## such as AUTHORS, COPYING, etc the bootstrap script really shouldn't be a part
## of a final package, but it is useful for developers who might want to make
## changes to the configure scripts or makefiles.
	## dnl since some of this is in SUBDIRS see if this is not necessary
EXTRA_DIST= \
	config \
	docs \
	share \
	bootstrap \
	libtool.GL.patch \
	README.GL_PROBLEMS

## This removes stuff from the distribution which may be present
## from a cvs checkout or other build reasons
dist-hook:
	rm -rf `find $(distdir) -type d -name CVS`	# remove all CVS directories
	rm -rf `find $(distdir) -type f -name Makefile` # from including docs and share on the EXTRA_DIST line
	echo rm -rf `find $(distdir) -type f -name \.\#\*`	# CVS revision files left around for some reason
	rm -rf $(distdir)/src/images/WORK		# makes the tarball much smaller
	rm -rf $(distdir)/src/images/.xvpics		# created by ee I think
	rm -rf $(distdir)/src/misc/missing/generated	# doesn't need to go with the distro


ACLOCAL_AMFLAGS = -I config/m4
