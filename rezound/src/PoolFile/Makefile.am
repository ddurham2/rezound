# ReZound PoolFile/Makefile.am written by Anthony Ventimiglia
## Process this file with automake to create Makefile.in
##
## Copyright (C) 2002 - Anthony Ventimiglia
## 
## This file is part of ReZound, an audio editing application, but
## could be used for other applications which could use the PoolFile
## class's functionality.
## 
## PoolFile is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published
## by the Free Software Foundation; either version 2 of the License,
## or (at your option) any later version.
## 
## PoolFile is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
## 
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA


include $(top_srcdir)/config/am_include.mk

##
## I have to include this because the build depends on the headers in 
## DiskTable are a necessity.
EXTRA_DIST= \
	DiskTable \
	TPoolAccesser.cpp \
	TPoolFile.cpp \
	TStaticPoolAccesser.cpp 

noinst_HEADERS= \
	   CMultiFile.h \
	   TPoolAccesser.h \
	   TPoolFile.h \
	   TStaticPoolAccesser.h
	   




## noinst_LTLIBRARIES tells Libtool that we will be creating libraries that
## won't be installed The LT means we'll be using LibTool to do it. It's really
## easiest to build one library for each sub directory which will all be linked
## together in the final build directory
noinst_LTLIBRARIES=libPoolFile.la
## notice how all abbreviations are converted to _ in Automakes syntax.
## libPoolFile.la becomes libPoolFile_la when we write the macro for automake.
## here we list all the source files that will be used to build libPoolFile.la
libPoolFile_la_SOURCES=CMultiFile.cpp
