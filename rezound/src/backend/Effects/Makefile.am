# ReZound backend/Effects/Makefile.am Written by Anthony Ventimiglia
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


include $(top_srcdir)/config/am_include.mk


## see PoolFile Makefile.am for explanation here out
## 
noinst_LTLIBRARIES=libEffects.la
libEffects_la_SOURCES=CChangeAmplitudeEffect.cpp \
		      CChangeRateEffect.cpp \
		      CDelayEffect.cpp \
		      CFlangeEffect.cpp \
		      CReverseEffect.cpp \
		      CStaticReverbEffect.cpp \
		      CVariedRepeatEffect.cpp \
		      CTestEffect.cpp 


EXTRA_DIST=CChangeAmplitudeEffect.h \
	   CChangeRateEffect.h \
	   CDelayEffect.h \
	   CFlangeEffect.h \
	   CReverseEffect.h \
	   CStaticReverbEffect.h \
	   CVariedRepeatEffect.h \
	   CTestEffect.h \
	   EffectActions.h

## Dependencies
CChangeAmplitudeEffect.o: CChangeAmplitudeEffect.cpp \
	CChangeAmplitudeEffect.h \
	$(top_srcdir)/src/backend/AAction.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h

CChangeRateEffect.o: CChangeRateEffect.cpp \
	CChangeRateEffect.h \
	$(top_srcdir)/src/backend/AAction.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.cpp \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h

CDelayEffect.o: CDelayEffect.cpp \
	CDelayEffect.h \
	$(top_srcdir)/src/backend/AAction.h \
	$(top_srcdir)/src/backend/ALFO.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/backend/DSPBlocks.h \
	$(top_srcdir)/src/backend/unit_conv.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h

CFlangeEffect.o: CFlangeEffect.cpp \
	CFlangeEffect.h \
	$(top_srcdir)/src/backend/AAction.h \
	$(top_srcdir)/src/backend/ALFO.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/backend/DSPBlocks.h \
	$(top_srcdir)/src/backend/unit_conv.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h

CReverseEffect.o: CReverseEffect.cpp \
	CReverseEffect.h \
	$(top_srcdir)/src/backend/AAction.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h

CStaticReverbEffect.o: CStaticReverbEffect.cpp \
	CStaticReverbEffect.h \
	$(top_srcdir)/src/backend/AAction.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h

CTestEffect.o: CTestEffect.cpp \
	CTestEffect.h \
	$(top_srcdir)/src/backend/AAction.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h
