# ReZound misc/Makefile.am Written by Anthony Ventimiglia
## Process this file with automake to create Makefile.in
##
##
##   Copyright (C) 2002 - Anthony Ventimiglia
##   
##   This file is part of ReZound, an audio editing application, 
##   
##   ReZound is free software; you can redistribute it and/or modify
##   it under the terms of the GNU General Public License as published
##   by the Free Software Foundation; either version 2 of the License,
##   or (at your option) any later version.
##   
##   ReZound is distributed in the hope that it will be useful, but
##   WITHOUT ANY WARRANTY; without even the implied warranty of
##   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##   GNU General Public License for more details.
##   
##   You should have received a copy of the GNU General Public License
##   along with this program; if not, write to the Free Software
##   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
##  

include $(top_srcdir)/config/am_include.mk

SUBDIRS= \
	CNestedDataFile

noinst_HEADERS=\
		AThread.h \
		CAtomicCounter.h \
		CMutex.h \
		CConditionVariable.h \
		CPath.h \
		CRWMutex.h \
		TAutoBuffer.h \
		TTempVarSetter.h \
		TMemoryPipe.h \
		TMemoryPipe.cpp \
		clocks.h \
		ScopedAutoReleasePool.h

EXTRA_DIST= \
	istring \
	endian_util.h \
	auto_array.h \
	TTempVarSetter.h \
	missing/sstream-missing \
	missing/stdint.h-missing

noinst_LTLIBRARIES=libmisc.la

libmisc_la_SOURCES= \
			CMutex.cpp  \
			CRWMutex.cpp \
			CConditionVariable.cpp  \
			AThread.cpp \
			clocks.cpp


