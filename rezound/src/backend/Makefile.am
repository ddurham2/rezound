# ReZound backend/Makefile.am Written by Anthony Ventimiglia
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

## first we'll dive into the subs to make everything in there
##
SUBDIRS=\
	Edits  \
	Effects \
	Remaster

## EXTRA_DIST these files don't make it into make dist by default but
## they are required for the build
noinst_HEADERS= \
	   file.h \
	   playcontrols.h \
	   initialize.h \
	   settings.h \
	   unit_conv.h \
	   AAction.h \
	   ALFO.h \
	   ASound.h \
	   ASoundPlayer.h \
	   ASoundRecorder.h \
	   ASoundFileManager.h \
	   AStatusComm.h \
	   CEnvelope.h \
	   CGraphParamValueNode.h \
	   CLoadedSound.h \
	   COSSSoundPlayer.h \
	   COSSSoundRecorder.h \
	   CSoundManager.h \
	   CSoundManagerClient.h \
	   CSoundPlayerChannel.h \
	   CTrigger.h \
	   ClibaudiofileSound.h \
	   Cold_rezSound.h \
	   CrezSound.h \
	   DSPBlocks.h 
	   

## see PoolFile Makefile.am for explanation here out
## 
noinst_LTLIBRARIES=libbackend.la
libbackend_la_SOURCES= \
			AAction.cpp  \
			ASound.cpp  \
			ASoundFileManager.cpp \
			ASoundPlayer.cpp  \
			ASoundRecorder.cpp \
			AStatusComm.cpp \
			CEnvelope.cpp \
			CGraphParamValueNode.cpp \
			CLoadedSound.cpp  \
			COSSSoundPlayer.cpp \
			COSSSoundRecorder.cpp \
			CSoundManager.cpp \
			CSoundManagerClient.cpp \
			CSoundPlayerChannel.cpp \
			CTrigger.cpp \
			ClibaudiofileSound.cpp \
			Cold_rezSound.cpp \
			CrezSound.cpp \
			file.cpp  \
			playcontrols.cpp \
		        settings.cpp \
		        initialize.cpp
