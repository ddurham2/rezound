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
	   AAction.h \
	   AActionDialog.h \
	   AFrontendHooks.h \
	   ALFO.h \
	   ASoundClipboard.h \
	   ASoundFileManager.h \
	   ASoundPlayer.h \
	   ASoundRecorder.h \
	   ASoundTranslator.h \
	   AStatusComm.h \
	   CActionParameters.h \
	   CActionSound.h \
	   CEnvelope.h \
	   CGraphParamValueNode.h \
	   CLoadedSound.h \
	   CNativeSoundClipboard.h \
	   COSSSoundPlayer.h \
	   COSSSoundRecorder.h \
	   CRecordSoundClipboard.h \
	   CSound.h \
	   CSoundPlayerChannel.h \
	   CSound_defs.h \
	   CTrigger.h \
	   ClibaudiofileSoundTranslator.h \
	   Cold_rezSoundTranslator.h \
	   CrawSoundTranslator.h \
	   CrezSoundTranslator.h \
	   DSPBlocks.h \
	   TSoundStretcher.h \
	   initialize.h \
	   main_controls.h \
	   settings.h \
	   unit_conv.h 
	   

## see PoolFile Makefile.am for explanation here out
## 
noinst_LTLIBRARIES=libbackend.la
libbackend_la_SOURCES= \
			AAction.cpp  \
			CActionParameters.cpp  \
			AActionDialog.cpp  \
			CActionSound.cpp  \
			CSound.cpp  \
			ASoundFileManager.cpp \
			AFrontendHooks.cpp \
			ASoundPlayer.cpp  \
			ASoundRecorder.cpp \
			AStatusComm.cpp \
			ASoundClipboard.cpp \
			CNativeSoundClipboard.cpp \
			CRecordSoundClipboard.cpp \
			CEnvelope.cpp \
			CGraphParamValueNode.cpp \
			CLoadedSound.cpp  \
			COSSSoundPlayer.cpp \
			COSSSoundRecorder.cpp \
			CSoundPlayerChannel.cpp \
			CTrigger.cpp \
			main_controls.cpp \
		        settings.cpp \
			ASoundTranslator.cpp \
			CrezSoundTranslator.cpp \
			Cold_rezSoundTranslator.cpp \
			ClibaudiofileSoundTranslator.cpp \
			CrawSoundTranslator.cpp \
		        initialize.cpp
