# ReZound frontend_fox/Makefile.am Written by Anthony Ventimiglia
## Process this file with automake to create Makefile.in
##
##
##   Copyright (C) 2002 - Anthony Ventimiglia
##   
##   This file is part of ReZound, an audio editing application, but
##   could be used for other applications which could use the PoolFile
##   class's functionality.
##   
##   PoolFile is free software; you can redistribute it and/or modify
##   it under the terms of the GNU General Public License as published
##   by the Free Software Foundation; either version 2 of the License,
##   or (at your option) any later version.
##   
##   PoolFile is distributed in the hope that it will be useful, but
##   WITHOUT ANY WARRANTY; without even the implied warranty of
##   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##   GNU General Public License for more details.
##   
##   You should have received a copy of the GNU General Public License
##   along with this program; if not, write to the Free Software
##   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
################################################################################

include $(top_srcdir)/config/am_include.mk

noinst_LTLIBRARIES=libfrontend.la
libfrontend_la_SOURCES= CActionButton.cpp \
		 CChannelSelectDialog.cpp \
		 CActionParamDialog.cpp \
		 CCueDialog.cpp \
		 CCueListDialog.cpp \
		 CEditToolbar.cpp \
		 CMainWindow.cpp \
		 CNewSoundDialog.cpp \
		 CRecordDialog.cpp \
		 CPasteChannelsDialog.cpp \
		 CProgressDialog.cpp \
		 CSoundFileManager.cpp \
		 CSoundListWindow.cpp \
		 CSoundWindow.cpp \
		 CStatusComm.cpp \
		 CUserNotesDialog.cpp \
		 drawPortion.cpp \
		 EditActionDialogs.cpp \
		 EffectActionDialogs.cpp \
		 FXConstantParamValue.cpp \
		 FXGraphParamValue.cpp \
		 FXModalDialogBox.cpp \
		 FXRezWaveView.cpp \
		 initialize.cpp \
		 RemasterActionDialogs.cpp \
		 rememberShow.cpp \
		 settings.cpp 

## bin_PROGRAMS is the macro that tells automake the name of the programs to
## install in the bin directory (/usr/local/bin) by default. By setting
## --prefix= at configure time the user can change this (eg: ./configure
## --prefix=/usr will install ReZound under /usr/bin/ReZound )
bin_PROGRAMS=ReZound

## for every name listed under bin_PROGRAMS, you have a prog_SOURCES. This lists
## all the sources in the current directory that are used to build ReZound.
ReZound_SOURCES= main.cpp 

## ReZound_LDADD is a list of extras to pass at link time. All the objects
## created by the above ReZound_SOURCES are automatically linked in, so here I
## list object files from other directories as well as flags passed to the
## linker. By using convienience libraries, it makes this part much easier. 
## It's up to each subdirectory's Makefile to build it's .la library.
## notice we use $(top_builddir) for the modules. This allows building in a 
## separate directory
ReZound_LDADD= libfrontend.la\
	$(top_builddir)/src/misc/cc++/libmiscCC.la \
	$(top_builddir)/src/misc/CNestedDataFile/libmiscCN.la \
	$(top_builddir)/src/PoolFile/libPoolFile.la \
	$(top_builddir)/src/backend/Edits/libEdits.la \
	$(top_builddir)/src/backend/Effects/libEffects.la \
	$(top_builddir)/src/backend/Remaster/libRemaster.la \
	$(top_builddir)/src/backend/libbackend.la

EXTRA_DIST=\
	CActionButton.h \
	CChannelSelectDialog.h \
	CActionParamDialog.h \
	CCueDialog.h \
	CCueListDialog.h \
	CEditToolbar.h \
	CMainWindow.h \
	CNewSoundDialog.h \
	CRecordDialog.h \
	CPasteChannelsDialog.h \
	CProgressDialog.h \
	CSoundFileManager.h \
	CSoundListWindow.h \
	CSoundWindow.h \
	CStatusComm.h \
	CUserNotesDialog.h \
	EditActionDialogs.h \
	EffectActionDialogs.h \
	FXConstantParamValue.h \
	FXGraphParamValue.h \
	FXModalDialogBox.h \
	FXRezWaveView.h \
	RemasterActionDialogs.h \
	drawPortion.h \
	initialize.h \
	rememberShow.h \
	settings.h 
