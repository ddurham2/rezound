# ReZound backend/File/Makefile.am Written by David Durham
## Process this file with automake to create Makefile.in
##
##
##   Copyright (C) 2005 - David Durham
##   
##  This file is part of ReZound, an audio editing application.
##  
##  ReZound is free software; you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published
##  by the Free Software Foundation; either version 2 of the License,
##  or (at your option) any later version.
##  
##  ReZound is distributed in the hope that it will be useful, but
##  WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU General Public License for more details.
##  
##  You should have received a copy of the GNU General Public License
##  along with this program; if not, write to the Free Software
##  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
##  


include $(top_srcdir)/config/am_include.mk

## EXTRA_DIST is used to add files that aren't automatically included 
## in make dist
noinst_HEADERS= \
		CNewAudioFileAction.h \
		COpenAudioFileAction.h \
		CSaveAudioFileAction.h \
		CSaveAsAudioFileAction.h \
		CSaveSelectionAsAction.h \
		CSaveAsMultipleFilesAction.h \
		CBurnToCDAction.h \
		parse_segment_cues.h \
		FileActions.h

## see PoolFile Makefile.am for explanation here out
## 
noinst_LTLIBRARIES=libFile.la
libFile_la_SOURCES= \
		CNewAudioFileAction.cpp \
		COpenAudioFileAction.cpp \
		CSaveAudioFileAction.cpp \
		CSaveAsAudioFileAction.cpp \
		CSaveSelectionAsAction.cpp \
		CSaveAsMultipleFilesAction.cpp \
		CBurnToCDAction.cpp

