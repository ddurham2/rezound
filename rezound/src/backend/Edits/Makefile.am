# ReZound backend/Edits/Makefile.am Written by Anthony Ventimiglia
## Process this file with automake to create Makefile.in
##
##
##   Copyright (C) 2002 - Anthony Ventimiglia
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
		CCopyCutDeleteEdit.h \
		CCropEdit.h \
		CCueAction.h \
		CInsertSilenceEdit.h \
		CMuteEdit.h \
		CPasteEdit.h \
		CPasteAsNewEdit.h \
		CRotateEdit.h \
		CSwapChannelsEdit.h \
		CAddChannelsEdit.h \
		CRemoveChannelsEdit.h \
		CSelectionEdit.h \
		CSaveSelectionAsAction.h \
		CSaveAsMultipleFilesAction.h \
		EditActions.h

## see PoolFile Makefile.am for explanation here out
## 
noinst_LTLIBRARIES=libEdits.la
libEdits_la_SOURCES= \
		CCopyCutDeleteEdit.cpp \
		CCropEdit.cpp \
		CCueAction.cpp \
		CInsertSilenceEdit.cpp \
		CMuteEdit.cpp \
		CPasteEdit.cpp \
		CPasteAsNewEdit.cpp \
		CRotateEdit.cpp \
		CSwapChannelsEdit.cpp \
		CAddChannelsEdit.cpp \
		CRemoveChannelsEdit.cpp \
		CSaveSelectionAsAction.cpp \
		CSaveAsMultipleFilesAction.cpp \
		CSelectionEdit.cpp 
