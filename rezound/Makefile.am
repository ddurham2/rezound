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
SUBDIRS=\
	src\
	share

## These extra files and directories will be included in the distribution. by
## using make-dist by default many common filenames are automatically included
## such as AUTHORS, COPYING, etc the bootstrap script really shouldn't be a part
## of a final package, but it is useful for developers who might want to make
## changes to the configure scripts or makefiles.
EXTRA_DIST= \
	config \
	docs \
	bootstrap
