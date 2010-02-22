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
	DSP  \
	File  \
	Edits  \
	Effects \
	Filters \
	Looping \
	Generate \
	Remaster \
	@MAYBE_LADSPA_DIR@

# this makes makedist happy
EXTRA_DIST= \
	CStubAction.h \
	CStubAction.cpp

noinst_HEADERS= \
	   AActionDialog.h \
	   AAction.h \
	   AFrontendHooks.h \
	   ALFO.h \
	   ApipedSoundTranslator.h \
	   ASoundClipboard.h \
	   ASoundFileManager.h \
	   ASoundPlayer.h \
	   ASoundRecorder.h \
	   ASoundTranslator.h \
	   AStatusComm.h \
	   CActionParameters.h \
	   CActionSound.h \
	   ActionParamMappers.h \
	   AActionParamMapper.h \
	   CGraphParamValueNode.h \
	   ClameSoundTranslator.h \
	   ClibaudiofileSoundTranslator.h \
	   ClibvorbisSoundTranslator.h \
	   CFLACSoundTranslator.h \
	   CMIDISDSSoundTranslator.h \
	   CLoadedSound.h \
	   CNativeSoundClipboard.h \
	   Cold_rezSoundTranslator.h \
	   CNULLSoundPlayer.h \
	   COSSSoundPlayer.h \
	   COSSSoundRecorder.h \
	   CALSASoundPlayer.h \
	   CALSASoundRecorder.h \
	   CPortAudioSoundPlayer.h \
	   CPortAudioSoundRecorder.h \
	   CJACKSoundPlayer.h \
	   CJACKSoundRecorder.h \
	   CrawSoundTranslator.h \
	   CRecordSoundClipboard.h \
	   CrezSoundTranslator.h \
	   CSound_defs.h \
	   CSound.h \
	   CSoundPlayerChannel.h \
	   CTrigger.h \
	   CPluginMapping.h \
	   CMacroPlayer.h \
	   CMacroRecorder.h \
	   CRunMacroAction.h \
	   CvoxSoundTranslator.h \
	   initialize.h \
	   license.h \
	   main_controls.h \
	   mypopen.h \
	   settings.h \
	   unit_conv.h
	   

noinst_LTLIBRARIES=libbackend.la
libbackend_la_SOURCES= \
			AAction.cpp  \
			AActionDialog.cpp  \
			AFrontendHooks.cpp \
	   		ALFO.cpp \
			ApipedSoundTranslator.cpp \
			ASoundClipboard.cpp \
			ASoundFileManager.cpp \
			ASoundPlayer.cpp  \
			ASoundRecorder.cpp \
			ASoundTranslator.cpp \
			AStatusComm.cpp \
			CActionParameters.cpp  \
			CActionSound.cpp  \
			AActionParamMapper.cpp \
			CGraphParamValueNode.cpp \
			ClameSoundTranslator.cpp \
			ClibaudiofileSoundTranslator.cpp \
			ClibvorbisSoundTranslator.cpp \
			CFLACSoundTranslator.cpp \
			CMIDISDSSoundTranslator.cpp \
			CLoadedSound.cpp  \
			CNativeSoundClipboard.cpp \
			Cold_rezSoundTranslator.cpp \
			COSSSoundPlayer.cpp \
			COSSSoundRecorder.cpp \
			CALSASoundPlayer.cpp \
			CALSASoundRecorder.cpp \
	   		CPortAudioSoundPlayer.cpp \
	   		CPortAudioSoundRecorder.cpp \
	   		CJACKSoundPlayer.cpp \
			CJACKSoundRecorder.cpp \
			CrawSoundTranslator.cpp \
			CRecordSoundClipboard.cpp \
			CrezSoundTranslator.cpp \
			CSound.cpp  \
			CSoundPlayerChannel.cpp \
			CTrigger.cpp \
			CPluginMapping.cpp \
			CMacroPlayer.cpp \
			CMacroRecorder.cpp \
			CRunMacroAction.cpp \
			CvoxSoundTranslator.cpp \
		        initialize.cpp \
			main_controls.cpp \
		        mypopen.cpp \
		        settings.cpp

# soundtouch_CFLAGS is in Remaster/Makefile.am
AM_CXXFLAGS=@audiofile_CFLAGS@ @ALSA_CFLAGS@ @JACK_CFLAGS@ @ogg_CFLAGS@ @vorbis_CFLAGS@ @flacpp_CFLAGS@ @flac_CFLAGS@

# soundtouch_LIBS is in Remaster/Makefile.am
libbackend_la_LIBADD=@LTLIBINTL@ @audiofile_LIBS@ @ALSA_LIBS@ @JACK_LIBS@ @ogg_LIBS@ @vorbisfile_LIBS@ @vorbisenc_LIBS@ @vorbis_LIBS@ @flacpp_LIBS@ @flac_LIBS@

