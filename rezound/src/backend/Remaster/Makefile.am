# ReZound backend/Remaster/Makefile.am Written by Anthony Ventimiglia
## Process this file with automake to create Makefile.in
##
##
##   Copyright (C) 2002 - David W. Durham
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

## see PoolFile Makefile.am for explanation here out
## 
EXTRA_DIST=CNoiseGateAction.h \
	   CRemoveDCAction.h \
	   CUnclipAction.h \
	   RemasterActions.h

noinst_LTLIBRARIES=libRemaster.la
libRemaster_la_SOURCES= CNoiseGateAction.cpp \
			CRemoveDCAction.cpp \
			CUnclipAction.cpp 


CNoiseGateAction.o: CNoiseGateAction.cpp \
	CNoiseGateAction.h \
	$(top_srcdir)/src/backend/AAction.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/DSPBlocks.h \
	$(top_srcdir)/src/backend/unit_conv.h \
	$(top_srcdir)/src/backend/ALFO.h

CRemoveDCAction.o: CRemoveDCAction.cpp \
	CRemoveDCAction.h \
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

CUnclipAction.o: CUnclipAction.cpp \
	CUnclipAction.h \
	$(top_srcdir)/src/backend/AAction.h \
	$(top_srcdir)/src/backend/ASound.h \
	$(top_srcdir)/src/backend/ASoundPlayer.h \
	$(top_srcdir)/src/backend/AStatusComm.h \
	$(top_srcdir)/src/backend/CEnvelope.h \
	$(top_srcdir)/src/backend/CGraphParamValueNode.h \
	$(top_srcdir)/src/backend/CSoundPlayerChannel.h \
	$(top_srcdir)/src/backend/CTrigger.h \
	$(top_srcdir)/src/backend/unit_conv.h \
	$(top_srcdir)/src/misc/cc++/TAutoBuffer.h \
	$(top_srcdir)/src/misc/istring \
	$(top_srcdir)/src/PoolFile/CMultiFile.h \
	$(top_srcdir)/src/PoolFile/TPoolAccesser.h \
	$(top_srcdir)/src/PoolFile/TPoolFile.h \
	$(top_srcdir)/src/PoolFile/TStaticPoolAccesser.h
